#include <satorivideo/opencv/opencv_bot.h>
#include <satorivideo/opencv/opencv_utils.h>
#include <satorivideo/video_bot.h>
#include <json.hpp>
#include <opencv2/opencv.hpp>

#define LOGURU_WITH_STREAMS 1
#include <loguru/loguru.hpp>

namespace sv = satori::video;

namespace haar_cascades_bot {

struct cascade {
  cv::CascadeClassifier classifier;
  std::string tag;
};

struct state {
  std::list<cascade> cascades;
  uint32_t detection_id{0};
};

nlohmann::json build_object(const cv::Rect &detection, uint32_t id,
                            const cv::Size &image_size, const std::string &tag) {
  nlohmann::json object = nlohmann::json::object();
  object["id"] = id;
  object["rect"] = sv::opencv::to_json(sv::opencv::to_fractional(detection, image_size));
  object["tag"] = tag;

  return object;
}

nlohmann::json build_analysis_message(nlohmann::json &&objects) {
  nlohmann::json analysis_message = nlohmann::json::object();
  analysis_message.emplace("detected_objects", std::move(objects));

  return analysis_message;
}

void process_image(sv::bot_context &context, const cv::Mat &image) {
  auto state = static_cast<struct state *>(context.instance_data);

  const cv::Size image_size{image.cols, image.rows};

  nlohmann::json objects = nlohmann::json::array();

  for (auto &cascade : state->cascades) {
    std::vector<cv::Rect> detections;
    cascade.classifier.detectMultiScale(image, detections);

    for (const auto &detection : detections) {
      objects.emplace_back(
          build_object(detection, state->detection_id++, image_size, cascade.tag));
    }
  }

  if (objects.empty()) {
    return;
  }

  bot_message(context, sv::bot_message_kind::ANALYSIS,
              build_analysis_message(std::move(objects)));
}

/*
 * Configuration is a map of `classifier file -> tag` pairs. Example:
 * {
 *   "frontalface_default.xml": "some face",
 *   "smile.xml": "some smile"
 * }
 */
nlohmann::json process_command(sv::bot_context &context, const nlohmann::json &config) {
  CHECK_S(config.is_object()) << "config is not an object: " << config;
  CHECK_S(config.find("action") != config.end()) << "no action in config: " << config;

  auto &action = config["action"];
  CHECK_S(action.is_string()) << "action is not a string: " << config;

  if (action == "configure") {
    CHECK_S(config.find("body") != config.end()) << "no body in config: " << config;
    auto &body = config["body"];
    CHECK_S(body.is_object()) << "body is not an object: " << body;
    CHECK_GT_S(body.size(), 0) << "Configuration was not provided: " << config;

    CHECK_S(context.instance_data == nullptr) << "state should be null";

    std::unique_ptr<struct state> state = std::make_unique<struct state>();

    for (auto it = body.begin(); it != body.end(); it++) {
      CHECK_S(it.value().is_string()) << "value is not a string: " << config;

      std::string cascade_file = it.key();
      std::string cascade_tag = it.value();

      struct cascade cascade;

      if (!cascade.classifier.load("models/" + cascade_file)) {
        ABORT_S() << "Can't load classifier " << cascade_file;
      }

      cascade.tag = cascade_tag;
      state->cascades.push_back(std::move(cascade));
    }

    context.instance_data = state.release();
    LOG_S(INFO) << "Bot is initialized";
  }

  return nullptr;
}

}  // namespace haar_cascades_bot

int main(int argc, char *argv[]) {
  sv::opencv_bot_register(
      {&haar_cascades_bot::process_image, &haar_cascades_bot::process_command});
  return sv::opencv_bot_main(argc, argv);
}
