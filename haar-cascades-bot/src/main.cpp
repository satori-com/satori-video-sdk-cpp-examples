#include <satorivideo/opencv/opencv_bot.h>
#include <satorivideo/opencv/opencv_utils.h>
#include <satorivideo/video_bot.h>
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

cbor_item_t *build_object(const cv::Rect &detection, uint32_t id,
                          const cv::Size &image_size, const std::string &tag) {
  cbor_item_t *object = cbor_new_definite_map(3);
  CHECK_NOTNULL_S(object);

  cbor_map_add(object,
               {cbor_move(cbor_build_string("id")), cbor_move(cbor_build_uint32(id))});
  cbor_map_add(object, {cbor_move(cbor_build_string("rect")),
                        cbor_move(sv::opencv::to_cbor(
                            sv::opencv::to_fractional(detection, image_size)))});
  cbor_map_add(object, {cbor_move(cbor_build_string("tag")),
                        cbor_move(cbor_build_string(tag.c_str()))});

  return object;
}

cbor_item_t *build_analysis_message(cbor_item_t *objects) {
  cbor_item_t *analysis_message = cbor_new_indefinite_map();
  CHECK_NOTNULL_S(analysis_message);

  cbor_map_add(analysis_message,
               {cbor_move(cbor_build_string("objects")), cbor_move(objects)});

  return analysis_message;
}

std::string cbor_to_string(const cbor_item_t *item) {
  CHECK_S(cbor_isa_string(item));

  return std::string{reinterpret_cast<char *>(cbor_string_handle(item)),
                     cbor_string_length(item)};
}

void process_image(sv::bot_context &context, const cv::Mat &image) {
  auto state = static_cast<struct state *>(context.instance_data);

  const cv::Size image_size{image.cols, image.rows};

  cbor_item_t *objects = cbor_new_indefinite_array();
  CHECK_NOTNULL_S(objects);

  size_t objects_counter = 0;

  for (auto &cascade : state->cascades) {
    std::vector<cv::Rect> detections;
    cascade.classifier.detectMultiScale(image, detections);

    for (const auto &detection : detections) {
      cbor_array_set(objects, objects_counter++,
                     cbor_move(build_object(detection, state->detection_id++, image_size,
                                            cascade.tag)));
    }
  }

  if (objects_counter == 0) {
    cbor_decref(&objects);
    return;
  }

  bot_message(context, sv::bot_message_kind::ANALYSIS,
              cbor_move(build_analysis_message(objects)));
}

/*
 * Configuration is a map of `classifier file -> tag` pairs. Example:
 * {
 *   "frontalface_default.xml": "some face",
 *   "smile.xml": "some smile"
 * }
 */
cbor_item_t *process_command(sv::bot_context &context, cbor_item_t *config) {
  CHECK_S(cbor_isa_map(config));

  cbor_item_t *action{nullptr};
  cbor_item_t *body{nullptr};

  for (size_t i = 0; i < cbor_map_size(config); i++) {
    cbor_item_t *key = cbor_map_handle(config)[i].key;
    cbor_item_t *value = cbor_map_handle(config)[i].value;

    const std::string key_str = cbor_to_string(key);

    if (key_str == "action")
      action = value;
    else if (key_str == "body")
      body = value;
  }

  CHECK_NOTNULL_S(action);
  CHECK_NOTNULL_S(body);
  CHECK_S(cbor_isa_string(action));

  if (cbor_to_string(action) == "configure") {
    CHECK_S(context.instance_data == nullptr);

    const size_t body_size = cbor_map_size(body);
    CHECK_GT_S(body_size, 0) << "Configuration was not provided";

    std::unique_ptr<struct state> state = std::make_unique<struct state>();

    for (size_t i = 0; i < body_size; i++) {
      const cbor_item_t *key = cbor_map_handle(body)[i].key;
      const cbor_item_t *value = cbor_map_handle(body)[i].value;

      CHECK_S(cbor_isa_string(value));

      std::string cascade_file = cbor_to_string(key);
      std::string cascade_tag = cbor_to_string(value);

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
