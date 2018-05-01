# Motion Detector Bot

- Building: `mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ../ && make -j8`

This bot uses OpenCV background extraction to detect motion in camera view.

It is also an example of dynamic reconfiguration.
When it runs, the bot subscribes to the `control` channel and receives messages that can the "featureSize" field used for 
thresholding moving objects. To use this feature, publish a message to the control channel with the format:
{
    "to": "<bot_id>",
    "params": {
        "featureSize": <real_number>
    }
}

<bot_id> is the value you provide for the `--id` parameter on the bot command line.

For more details, see the source code.
