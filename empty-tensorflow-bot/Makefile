DOCKER_BUILD_OPTIONS?=--pull
DOCKER_TAG?=latest

DOCKER_IMAGE=gcr.io/kubernetes-live/video/empty-tensorflow-bot
.RECIPEPREFIX = >
.PHONY: all image test push

all: image

image:
> docker build $(DOCKER_BUILD_OPTIONS) \
	--build-arg CMAKE_TIDY="/usr/bin/clang-tidy-5.0" -t $(DOCKER_IMAGE) .

test:
> docker run -v `cd ../smoke-test-video/;pwd`:/test/ -i $(DOCKER_IMAGE) \
	--input-video-file=/test/test.mp4

push: image
> docker tag $(DOCKER_IMAGE) $(DOCKER_IMAGE):$(DOCKER_TAG)
> docker push $(DOCKER_IMAGE)
> docker push $(DOCKER_IMAGE):$(DOCKER_TAG)
