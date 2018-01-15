# HAAR cascades bot

This bot solves object detection problem using Haar cascades. For information on Haar cascades training read [here](https://github.com/mrnugget/opencv-haar-classifier-training).

OpenCV comes with a bunch of trained Haar cascade models https://github.com/opencv/opencv/tree/master/data/haarcascades.

## Configuration
Bot configuration is a map `classifier file -> tag` pairs. Example:
```json
{
  "frontalface_default.xml": "a face",
  "smile.xml": "a smile"
}
```
It could be passed via file or via command line argument.

## Building and running locally
```bash
# Building
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8

# Running (processing video file)
./bin/haar-cascades-bot --input-video-file my_video_file.mp4 --config-file config.json
```

## Building and running in docker
```bash
# Building docker image
make

# Running docker image (processing Satori video stream)
docker run --rm -ti haar-cascades-bot --id sample-haar-cascades-bot --endpoint <satori-endpoint> --appkey <satori-appkey> --input-channel <satori-channel> --config "{\"frontalface_default.xml\": \"a face\", \"smile.xml\": \"a smile\"}"
```

## Sample bot output
```json
{
  "objects": [
    {
      "id":1,
      "rect":[0.25,0.383333,0.509375,0.3375],
      "tag":"some smile"
    },
    {
      "id":1,
      "rect":[0.090625,0.341667,0.2625,0.175],
      "tag":"some smile"
    },
    {
      "id":1,
      "rect":[0.621875,0.325,0.28125,0.1875],
      "tag":"some smile"
    }
  ],
  "i":[51900,51900]
}
```