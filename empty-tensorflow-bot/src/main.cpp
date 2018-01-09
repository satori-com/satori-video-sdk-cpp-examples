#include <satorivideo/opencv/opencv_bot.h>
#include <tensorflow/cc/ops/const_op.h>
#include <tensorflow/cc/ops/image_ops.h>
#include <tensorflow/cc/ops/standard_ops.h>
#include <tensorflow/core/framework/graph.pb.h>
#include <tensorflow/core/framework/tensor.h>
#include <tensorflow/core/platform/init_main.h>
#include <tensorflow/core/platform/logging.h>
#include <tensorflow/core/platform/types.h>
#include <tensorflow/core/public/session.h>
#include <iostream>

#define LOGURU_WITH_STREAMS 1
#include <loguru/loguru.hpp>

namespace sv = satori::video;
namespace tf = tensorflow;

namespace empty_tensorflow_bot {
namespace {
struct state {
  std::unique_ptr<tf::Session> session;
};
}  // namespace

tf::Tensor mat_to_tensor(const cv::Mat &frame) {
  tf::Tensor image_tensor(tf::DT_FLOAT, tf::TensorShape({1, frame.cols, frame.rows, 3}));
  float *new_buffer = image_tensor.flat<float>().data();
  cv::Mat new_img(frame.rows, frame.cols, CV_32FC3, new_buffer);
  frame.convertTo(new_img, CV_32FC3);
  return image_tensor;
}

void process_image(sv::bot_context &context, const cv::Mat &frame) {
  auto *s = (state *)context.instance_data;
  tf::Tensor image_tensor = mat_to_tensor(frame);
  // use image_tensor
  std::vector<tf::Tensor> outputs;
  tf::Status run_status = s->session->Run(
      {{"input", image_tensor}}, {"InceptionV3/Predictions/Reshape_1"}, {}, &outputs);
  if (!run_status.ok()) {
    std::cerr << "Running model failed: " << run_status << "\n";
    return;
  }
  std::cout << "output tensor size = " << outputs[0].TotalBytes() << "\n";
}

nlohmann::json process_command(sv::bot_context &ctx, const nlohmann::json &config) {
  CHECK_S(config.is_object()) << "config is not an object: " << config;
  CHECK_S(config.find("action") != config.end()) << "no action in config: " << config;

  auto &action = config["action"];
  CHECK_S(action.is_string()) << "action is not a string: " << config;

  if (action == "configure") {
    CHECK_S(config.find("body") != config.end()) << "no body in config: " << config;
    auto &body = config["body"];
    CHECK_S(body.is_object()) << "body is not an object: " << body;

    state *s = new state;
    tf::GraphDef graph_def;
    s->session.reset(tf::NewSession(tf::SessionOptions()));

    const std::string graph_path = (body.find("graph_path") != body.end())
                                       ? body["graph_path"]
                                       : "inception_v3_2016_08_28_frozen.pb";
    tf::Status load_graph_status =
        tf::ReadBinaryProto(tf::Env::Default(), graph_path, &graph_def);
    if (!load_graph_status.ok()) {
      std::cerr << "Failed to load graph at '" << graph_path << "'" << load_graph_status
                << "\n";
      exit(1);
    }
    tf::Status create_status = s->session->Create(graph_def);
    if (!create_status.ok()) {
      std::cerr << "Failed to create graph " << create_status << "\n";
      exit(1);
    }
    ctx.instance_data = s;
  }
  return nullptr;
}

}  // namespace empty_tensorflow_bot

int main(int argc, char *argv[]) {
  tf::port::InitMain(argv[0], &argc, &argv);
  sv::opencv_bot_register(
      {&empty_tensorflow_bot::process_image, &empty_tensorflow_bot::process_command});
  return sv::opencv_bot_main(argc, argv);
}
