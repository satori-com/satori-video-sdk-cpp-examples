#include <iostream>
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

void process_image(sv::bot_context &context, const cv::Mat &frame) {
  tf::Tensor image_tensor(
      tf::DT_FLOAT, tf::TensorShape({1, frame.cols, frame.rows, 3}));
  float *new_buffer = image_tensor.flat<float>().data();
  cv::Mat new_img(frame.rows, frame.cols, CV_32FC3, new_buffer);
  frame.convertTo(new_img, CV_32FC3);
  // use image_tensor

  std::cout << "image_tensor size = " << image_tensor.TotalBytes() << "\n";
}

cbor_item_t *process_command(sv::bot_context &, cbor_item_t *) {
  std::cout << "bot is initializing, libraries are ok" << '\n';
  return nullptr;
}

} // namespace empty_tensorflow_bot

int main(int argc, char *argv[]) {
  sv::opencv_bot_register({&empty_tensorflow_bot::process_image,
                           &empty_tensorflow_bot::process_command});
  return sv::opencv_bot_main(argc, argv);
}
