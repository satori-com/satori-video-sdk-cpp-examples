#include <prometheus/counter.h>
#include <prometheus/counter_builder.h>
#include <prometheus/family.h>
#include <prometheus/histogram.h>
#include <prometheus/histogram_builder.h>
#include <satorivideo/opencv/opencv_bot.h>
#include <satorivideo/opencv/opencv_utils.h>
#include <satorivideo/video_bot.h>
#include <boost/timer/timer.hpp>
#include <gsl/gsl>
#include <json.hpp>
#include <opencv2/opencv.hpp>

#define LOGURU_WITH_STREAMS 1
#include <loguru/loguru.hpp>

namespace sv = satori::video;

namespace motion_detector_bot {

namespace {

void send_contours_analysis(sv::bot_context &context, const cv::Size &original_size,
                            const std::vector<std::vector<cv::Point>> &contours) {
  nlohmann::json rects = nlohmann::json::array();
  for (const auto &contour : contours) {
    auto rect = boundingRect(contour);

    nlohmann::json obj = nlohmann::json::object();
    obj["id"] = 1;
    obj["color"] = "green";
    obj["rect"] = sv::opencv::to_json(sv::opencv::to_fractional(rect, original_size));

    rects.emplace_back(std::move(obj));
  }

  nlohmann::json analysis_message = nlohmann::json::object();
  analysis_message["detected_objects"] = rects;
  sv::bot_message(context, sv::bot_message_kind::ANALYSIS, std::move(analysis_message));
}

constexpr std::initializer_list<double> latency_buckets = {
    0,  0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1,  2,  3,
    4,  5,   6,   7,   8,   9,   10,  20,  30,  40,  50, 60, 70,
    80, 90,  100, 200, 300, 400, 500, 600, 700, 800, 900};

class latency_reporter {
 public:
  explicit latency_reporter(prometheus::Histogram &hist) : _hist(hist) {}

  ~latency_reporter() { _hist.Observe(_timer.elapsed().wall / 1e6); }

 private:
  prometheus::Histogram &_hist;
  boost::timer::cpu_timer _timer;
};

}  // namespace

struct parameters {
  uint32_t feature_size{5};

  void merge_json(const nlohmann::json &params) {
    if (!params.is_object()) {
      LOG_S(ERROR) << "ignoring bad params: " << params;
      return;
    }

    if (params.find("featureSize") != params.end()) {
      auto &feature_size = params["featureSize"];
      if (feature_size.is_number_integer()) {
        this->feature_size = feature_size;
      }
    }
  }

  nlohmann::json to_json() const { return {{"featureSize", feature_size}}; }
};

struct state {
  explicit state(sv::bot_context &context)
      : frames_counter(prometheus::BuildCounter()
                           .Name("frames")
                           .Register(context.metrics.registry)
                           .Add({})),
        countours_counter(prometheus::BuildCounter()
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
                          .Add({}, std::vector<double>(latency_buckets))) {}

  parameters params;

  cv::Ptr<cv::BackgroundSubtractorKNN> extractor{
      cv::createBackgroundSubtractorKNN(500, 500.0, true)};

  prometheus::Counter &frames_counter;
  prometheus::Counter &countours_counter;
  prometheus::Histogram &blur_time;
  prometheus::Histogram &extract_time;
  prometheus::Histogram &morph_time;
  prometheus::Histogram &contours_time;
};

void process_image(sv::bot_context &context, const cv::Mat &original) {
  auto *s = (state *)context.instance_data;

  s->frames_counter.Increment();
  cv::Size original_size{original.cols, original.rows};

  cv::Mat blur;
  cv::Mat morph;

  {
    latency_reporter reporter(s->blur_time);
    cv::GaussianBlur(original, blur, cv::Size(5, 5), 0);
  }

  {
    latency_reporter reporter(s->extract_time);
    s->extractor->apply(blur, blur);
  }

  {
    latency_reporter reporter(s->morph_time);
    cv::Mat element = cv::getStructuringElement(
        cv::MORPH_RECT, cv::Size(s->params.feature_size, s->params.feature_size));
    cv::morphologyEx(blur, morph, cv::MORPH_OPEN, element);
  }

  std::vector<std::vector<cv::Point>> contours;
  std::vector<cv::Vec4i> hierarchy;
  {
    latency_reporter reporter(s->contours_time);
    cv::findContours(morph, contours, hierarchy, CV_RETR_EXTERNAL,
                     CV_CHAIN_APPROX_SIMPLE);
  }

  if (contours.empty()) {
    return;
  }

  s->countours_counter.Increment(contours.size());
  send_contours_analysis(context, original_size, contours);
}

nlohmann::json process_command(sv::bot_context &context, const nlohmann::json &command) {
  auto *s = (state *)context.instance_data;
  if (s == nullptr) {
    context.instance_data = s = new state(context);
    LOG_S(INFO) << "bot initialized";
  }

  if (!command.is_object()) {
    LOG_S(ERROR) << "unsupported command: " << command;
    return nullptr;
  }

  if (command.find("params") != command.end()) {
    auto &params = command["params"];
    LOG_S(INFO) << "received params command: " << command;

    s->params.merge_json(params);

    return {{"params", s->params.to_json()}};
  }

  LOG_S(ERROR) << "unsupported command";

  return nullptr;
}
}  // namespace motion_detector_bot

int main(int argc, char *argv[]) {
  cv::setNumThreads(0);
  sv::opencv_bot_register(
      {&motion_detector_bot::process_image, &motion_detector_bot::process_command});
  return sv::opencv_bot_main(argc, argv);
}
