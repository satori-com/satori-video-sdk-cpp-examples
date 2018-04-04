# Motion Detector Bot

- Building: `mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ../ && make -j8`

This bot uses OpenCV background extraction to detect motion in camera view.

It is also an example of dynamic reconfiguration.
When running, bot accepts messages with "params" object, it sets "featureSize" used for thresholding moving objects. For more details, please check the source code.
