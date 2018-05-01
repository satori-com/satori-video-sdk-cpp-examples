#include <prometheus/counter.h>
#include <prometheus/counter_builder.h>
#include <prometheus/family.h>
#include <prometheus/histogram.h>
#include <prometheus/histogram_builder.h>
#include <satorivideo/opencv/opencv_bot.h>
#include <satorivideo/opencv/opencv_utils.h>
#include <satorivideo/video_bot.h>
#include <cstdlib>
// Boost timer library
#include <boost/timer/timer.hpp>
// GNU scientific library
#include <gsl/gsl>
// JSON for Modern C++
#include <json.hpp>
#include <opencv2/opencv.hpp>

#define LOGURU_WITH_STREAMS 1
// Loguru for Modern C++
#include <loguru/loguru.hpp>

using namespace std;
namespace sv = satori::video;
namespace motion_detector_bot {
namespace {
    /*
    * Publishes the results of analyzing a frame to the analysis channel
    * Adds meta-data fields to each message
    */
    void publish_contours_analysis(sv::bot_context &context, const cv::Size &original_size,
                                   const std::vector<std::vector<cv::Point>> &contours) {
      // Instantiates a JSON array
      nlohmann::json rects = nlohmann::json::array();
      // Iterates over the input vector
      for (const auto &contour : contours) {
        auto rect = boundingRect(contour);
        // Instantiates a JSON object to hold the input vector
        nlohmann::json obj = nlohmann::json::object();
        // Sets the JSON object meta-data
        obj["id"] = 1;
        obj["color"] = "green";
        // Scales the input vector
        obj["rect"] = sv::opencv::to_json(sv::opencv::to_fractional(rect, original_size));
        // Adds the resulting JSON object to an array of objects
        rects.emplace_back(std::move(obj));
      }
      // Instantiates a JSON object for the message
      nlohmann::json analysis_message = nlohmann::json::object();
      // Sets the key for the message to "detected_objects" and the value to the array of objects
      analysis_message["detected_objects"] = rects;
      /*
      * Publishes the message to the analysis channel
      */
      sv::bot_message(context, sv::bot_message_kind::ANALYSIS, std::move(analysis_message));
    }
    /*
    * Defines a constant for initializing the latency buckets for a Prometheus histogram. For example, the first
    * bucket is 0 to 0.1 seconds, the second bucket is .1 to .2 seconds, and so forth. The last bucket is 900 seconds
    * and above.
    */
    constexpr std::initializer_list<double> latency_buckets = {
        0,  0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1,  2,  3,
        4,  5,   6,   7,   8,   9,   10,  20,  30,  40,  50, 60, 70,
        80, 90,  100, 200, 300, 400, 500, 600, 700, 800, 900};
    /*
    * Uses Prometheus to time the duration of an image processing operation.
    * To reduce memory usage, latency_reporter() does its work during its destructor.
    */
    class latency_reporter {
        // Constructor. Initializes the histogram object
    public:
        explicit latency_reporter(prometheus::Histogram &hist) : _hist(hist) {}
        /*
        * Destructor. Just before cleanup, records the elapsed time since initialization.
        * Timer returns milliseconds, so divide by 1 million to get seconds
        */
        ~latency_reporter() { _hist.Observe(_timer.elapsed().wall / 1e6); }
    
    private:
        prometheus::Histogram &_hist;
        boost::timer::cpu_timer _timer;
    };
    } // end namespace
    /*
    *  Moves configuration parameters from a configuration message to the bot context.
    *
    *  This is an example. The configuration parameters can be any JSON type you want.
    *
    *  merge_json() sets parameters.feature_size_value to the value of the featureSize property in the message.
    *  to_json() returns a JSON property with key "featureSize" and value parameters.feature_size_value.
    */
    struct parameters {
        uint32_t feature_size_value{5};
        /*
        * Copies the feature size from the parameters object to feature_size_value.
        */
        void merge_json(const nlohmann::json &params) {
          if (!params.is_object()) {
            LOG_S(ERROR) << "merge_json: Ignoring bad params: " << params;
            return;
          }
          /*
          * Copies the feature size from the message to the local variable.
          */
          if (params.find("featureSize") != params.end()) {
            auto &featureSize_value = params["featureSize"];
            if (featureSize_value.is_number_integer()) {
              this->feature_size_value = featureSize_value;
            }
          }
        }
        
        /*
        * Returns the field "featureSize" with value set to the new feature size.
        * The API publishes this message to the control channel.
        */
        nlohmann::json to_json() const { return {{"featureSize", feature_size_value}}; }
    };
    /*
    * Sets up storage in the bot context as members of the struct
    * The video SDK API defines the members "metrics" and "registry"; see video_bot.h
    *
    * This struct defines counters, buckets, and timers for Prometheus, sets up a member that
    * stores the featureSize configuration (params), and adds a BackgroundSubtractor instance.
    *
    */
    struct state {
        /*
        * Constructor
        */
        explicit state(sv::bot_context &context)
        /*
        * Initializes Prometheus counters and histograms in the bot context metrics registry
        */
            :
            frames_counter(prometheus::BuildCounter()
                               .Name("frames")
                               .Register(context.metrics.registry)
                               .Add({})),
            contours_counter(prometheus::BuildCounter()
                                 .Name("contours")
                                 .Register(context.metrics.registry)
                                 .Add({})),
            blur_time(prometheus::BuildHistogram()
                          .Name("motion_detector_blur_times_millis")
                          .Register(context.metrics.registry)
                          .Add({}, std::vector<double>(latency_buckets))),
            extract_time(prometheus::BuildHistogram()
                             .Name("motion_detector_extract_times_millis")
                             .Register(context.metrics.registry)
                             .Add({}, std::vector<double>(latency_buckets))),
            morph_time(prometheus::BuildHistogram()
                           .Name("motion_detector_morph_times_millis")
                           .Register(context.metrics.registry)
                           .Add({}, std::vector<double>(latency_buckets))),
            contours_time(prometheus::BuildHistogram()
                              .Name("motion_detector_contours_times_millis")
                              .Register(context.metrics.registry)
                              .Add({}, std::vector<double>(latency_buckets)))
        {}
        
        /*
        * Stores parameters received in process_command
        */
        parameters params;
        
        cv::Ptr<cv::BackgroundSubtractorKNN> background_subtractor{
            cv::createBackgroundSubtractorKNN(500, 500.0, true)
        };
        
        /*
        * Intermediate members that store Prometheus metrics
        */
        prometheus::Counter &frames_counter;
        prometheus::Counter &contours_counter;
        prometheus::Histogram &blur_time;
        prometheus::Histogram &extract_time;
        prometheus::Histogram &morph_time;
        prometheus::Histogram &contours_time;
    };
    /*
    * Invoked each time the API decodes a frame. The API passes in the bot context and an OpenCV Mat object.
    * **Note:** Log statements in process_image() can cause performance degradation.
    */
    void process_image(sv::bot_context &context, const cv::Mat &original_image) {
      /*
      * Points to the instance data area of the bot context. The default value is nullptr.
      */
      auto *s = (state *)context.instance_data;
      s->frames_counter.Increment();
      
      /*
      * Sets or declares control variables used by the OpenCV contour detection algorithm
      */
      cv::Size original_image_size{original_image.cols, original_image.rows};
      cv::Mat gaussian_blurred_image;
      cv::Mat morphed_image;
      {
        latency_reporter reporter(s->blur_time);
        cv::GaussianBlur(original_image, gaussian_blurred_image, cv::Size(5, 5), 0);
      }
      {
        latency_reporter reporter(s->extract_time);
        s->background_subtractor->apply(gaussian_blurred_image, gaussian_blurred_image);
      }
      /*
      * Note: getStructuredElement() uses the feature_size_value variable stored in the instance_data member of
      * the bot context. You can change this variable dynamically by publishing a new value to the control channel.
      * To learn more, see the code for process_command())
      */
      {
        latency_reporter reporter(s->morph_time);
        cv::Mat element = cv::getStructuringElement(
            cv::MORPH_RECT, cv::Size(s->params.feature_size_value, s->params.feature_size_value));
        cv::morphologyEx(gaussian_blurred_image, morphed_image, cv::MORPH_OPEN, element);
      }
      std::vector<std::vector<cv::Point>> contours;
      std::vector<cv::Vec4i> contours_topology_hierarchy;
      {
        latency_reporter reporter(s->contours_time);
        cv::findContours(morphed_image, contours, contours_topology_hierarchy, CV_RETR_EXTERNAL,
                         CV_CHAIN_APPROX_SIMPLE);
      }
      if (contours.empty()) {
        return;
      }
      
      s->contours_counter.Increment(contours.size());
      /*
      * Publishes the results to the analysis channel.
      */
      publish_contours_analysis(context, original_image_size, contours);
      
    } // end process_image
  /*
  * Receives update configurations and stores them in the bot context.
  *
  * This example bot expects a message that has the form
  * {
  *   "params": { "featureSize": &lt;size&gt; }
  * }
  */
  nlohmann::json process_command(sv::bot_context &context, const nlohmann::json &command_message) {
    /*
    * The bot context has pre-defined members, such as the metrics registry. Use the instance_data member to store
    * your own data. The tutorial uses instance_data to store members of the `state` struct.
    */
    auto *s = (state *)context.instance_data;
    /*
    * The API invokes process_command() during initialization, before it starts ingesting video. Use this first invocation
    * to initialize instance_data.
    */
    if (nullptr == s) {
      context.instance_data = new state(context);
      LOG_S(INFO) << "process_command: Bot configuration initialized";
      return nullptr;
    }
    // The received message should be a JSON object
    if (!command_message.is_object()) {
      LOG_S(ERROR) << "process_command: Command message isn't a JSON object, message= " << command_message;
      return nullptr;
    }

    if (command_message.find("ack") != command_message.end()) {
        /*
        * process_command() returns an ack message if it successfully processes a command message
        * The SDK publishes the message back to the control channel. Because the SDK is also *subscribed* to the
        * control channel, the ack arrives in back in process_command().
        * In general, you can do whatever you want with the ack. Note that you can subscribe to the control channel
        * from another bot and process the ack separately.
        * In this tutorial, process_command() logs a message
        */
        LOG_S(INFO) << "process_command: Received ack message= " << command_message;
        // This  message is fully handled, so process_command() returns null to the SDK
        return nullptr;
    }
    
    /*
    * Command message isn't an ack, so it must contain new or updated configuration parameters.
    * Sets the parameters in the bot context
    * **Note:** This is a sample. The field can use any key and value. To avoid confusion, use a single key and a
    * JSON child object for the configuration parameters, rather than making each parameter a child of the command
    * message itself.
    */
    if (command_message.find("params") != command_message.end()) {
      // Gets the configuration parameters from the message
      auto &params = command_message["params"];
      LOG_S(INFO) << "process_command: Received config parameters: " << command_message;
      // Moves the parameters to the context
      s->params.merge_json(params);
      // Gets the bot id from the command_message
      std::string bot_id = command_message["to"];
      /*
      * Returns an acknowledgement ("ack") message to the SDK, which publishes it back to the control channel.
      * The ack is JSON that contains:
      * {"ack": true, "to": "<bot_id>", "<params_key>": <params_value>}
      * - <bot_id>: Value of --id parameter on bot command line
      * - <params_key>: Top-level key for the configuration parameters.
      * - <params_value>: Configuration parameters object
      *
      * **Note:** Always include the "to" field in the ack. If you don't, the SDK issues
      * "unsupported kind of message:" errors because it expects all control messages to have a "to" field.
      */
      // Creates the ack field
      nlohmann::json return_object = {{"ack", true}};
      // Inserts the bot id field
      nlohmann::json to_object = {{"to", bot_id}};
      return_object.insert(to_object.begin(), to_object.end());

      // Inserts the received configuration parameters object field
      nlohmann::json config_object = s->params.to_json();
      return_object.insert(config_object.begin(), config_object.end());

      LOG_S(INFO) << "Return ack message: " << return_object.dump();
      // Returns the ack
      return return_object;
    
    } else {
        // Control reaches here if the "params" key isn't found and the message isn't an "ack"
        LOG_S(ERROR) << "Control message doesn't contain params key." << command_message;
        return nullptr;
    }
  }
} // end motion_detector_bot namespace
/*
* Motion detector bot program
*/
int main(int argc, char *argv[]) {
  // Disables OpenCV thread optimization
  cv::setNumThreads(0);
  // Turns off escape sequences in log output
  loguru::g_colorlogtostderr = false;
  // Registers the image processing and configuration callbacks
  sv::opencv_bot_register(
      {&motion_detector_bot::process_image, &motion_detector_bot::process_command});
  // Starts the main processing loop
  return sv::opencv_bot_main(argc, argv);
}