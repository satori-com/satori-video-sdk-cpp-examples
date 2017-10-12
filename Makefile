SUBDIRS := empty-bot

.RECIPEPREFIX = >

.PHONY: all $(SUBDIRS)

DOCKER_TAG=satori-video-sdk-cpp-examples

all: $(SUBDIRS)

$(SUBDIRS):
> docker build -t ${DOCKER_TAG}-$@ $@