# Satori Video C++ SDK Examples

This repository contains examples for [Satori Video C++ SDK](https://github.com/satori-com/satori-video-sdk-cpp).

Templates:

* [empty-bot](empty-bot) - minimal video bot template, processing video frames as byte buffers.
* [empty-opencv-bot](empty-opencv-bot) - minimal video bot template, processing video frames as OpenCV `cv::Mat` object.
* [empty-tensorflow-bot](empty-tensorflow-bot) - minimal tensorflow bot template, processing video frames as
  `tensorflow::Tensor` objects.

More Complicated Examples:

* [haar-cascades-bot](haar-cascades-bot) - OpenCV-based bot doing object recognition using Haar cascade.
  Demonstrates configuration loading and producing analysis messages.

Deployment:

* [empty-bot](deployment/empty-bot.yaml) - Example configuration for Kubernetes.
