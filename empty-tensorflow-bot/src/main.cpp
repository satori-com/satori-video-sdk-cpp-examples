#include <iostream>
#include <satorivideo/impl/cbor_tools.h>
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

namespace sv = satori::video;
namespace tf = tensorflow;

namespace empty_tensorflow_bot {
namespace {
struct state {
  std::unique_ptr<tf::Session> session;
};
} // namespace

tf::Tensor mat_to_tensor(const cv::Mat &frame) {
  tf::Tensor image_tensor(tf::DT_FLOAT,
                          tf::TensorShape({1, frame.cols, frame.rows, 3}));
  float *new_buffer = image_tensor.flat<float>().data();
  cv::Mat new_img(frame.rows, frame.cols, CV_32FC3, new_buffer);
  frame.convertTo(new_img, CV_32FC3);
  return image_tensor;
}

void process_image(sv::bot_context &context, const cv::Mat &frame) {
  state *s = (state *)context.instance_data;
  tf::Tensor image_tensor = mat_to_tensor(frame);
  // use image_tensor
  std::vector<tf::Tensor> outputs;
  tf::Status run_status =
      s->session->Run({{"input", image_tensor}},
                      {"InceptionV3/Predictions/Reshape_1"}, {}, &outputs);
  if (!run_status.ok()) {
    std::cerr << "Running model failed: " << run_status << "\n";
    return;
  }
  std::cout << "output tensor size = " << outputs[0].TotalBytes() << "\n";
}

cbor_item_t *process_command(sv::bot_context &ctx, cbor_item_t *config) {
  if (cbor::map_has_str_value(config, "action", "configure")) {
    state *s = new state;
    tf::GraphDef graph_def;
    s->session.reset(tf::NewSession(tf::SessionOptions()));

    std::string graph_path(
        cbor::map(cbor::map(config).get("body"))
            .get_str("graph_path", "inception_v3_2016_08_28_frozen.pb"));
    tf::Status load_graph_status =
        tf::ReadBinaryProto(tf::Env::Default(), graph_path, &graph_def);
    if (!load_graph_status.ok()) {
      std::cerr << "Failed to load graph at '" << graph_path << "'"
                << load_graph_status;
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

} // namespace empty_tensorflow_bot

int main(int argc, char *argv[]) {
  tf::port::InitMain(argv[0], &argc, &argv);
  sv::opencv_bot_register({&empty_tensorflow_bot::process_image,
                           &empty_tensorflow_bot::process_command});
  return sv::opencv_bot_main(argc, argv);
}
