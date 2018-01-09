DOCKER_BUILD_OPTIONS?=--pull
DOCKER_TAG?=latest
TEST_VIDEO_PATH?=../../satori-video-sdk-cpp/test_data

DOCKER_IMAGE=gcr.io/kubernetes-live/video/empty-opencv-bot
.RECIPEPREFIX = >
.PHONY: all image test push

all: image

image:
> docker build $(DOCKER_BUILD_OPTIONS) \
	--build-arg CMAKE_TIDY="/usr/bin/clang-tidy-5.0" -t $(DOCKER_IMAGE) .

test:
> docker run -v `cd $(TEST_VIDEO_PATH);pwd`:/test/ -i $(DOCKER_IMAGE) \
	--input-video-file=/test/test.mp4

push: image
> docker tag $(DOCKER_IMAGE) $(DOCKER_IMAGE):$(DOCKER_TAG)
> docker push $(DOCKER_IMAGE)
> docker push $(DOCKER_IMAGE):$(DOCKER_TAG)
