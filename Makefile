SUBDIRS := empty-bot empty-opencv-bot empty-tensorflow-bot haar-cascades-bot motion-detector-bot

.RECIPEPREFIX = >

.PHONY: all $(SUBDIRS)

DOCKER_TAG=satori-video-sdk-cpp-examples

all: $(SUBDIRS)

$(SUBDIRS):
> make -C $@
