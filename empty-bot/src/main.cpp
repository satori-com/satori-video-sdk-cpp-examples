#include <iostream>
#include <librtmvideo/video_bot.h>

namespace sv = satori::video;

namespace empty_bot {

void process_image(sv::bot_context &context, const sv::image_frame &frame) {
  std::cout << "got frame " << context.frame_metadata->width << "x"
            << context.frame_metadata->height << "\n";
}

cbor_item_t *process_command(sv::bot_context &, cbor_item_t *) {
  std::cout << "bot is initializing, libraries are ok" << '\n';
  return nullptr;
}

} // namespace empty_bot

int main(int argc, char *argv[]) {
  sv::bot_register(sv::bot_descriptor{sv::image_pixel_format::BGR,
                                      &empty_bot::process_image,
                                      &empty_bot::process_command});
  return sv::bot_main(argc, argv);
}