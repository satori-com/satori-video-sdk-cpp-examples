BOT_NAME=empty-bot
DOCKER_TAG = gcr.io/kubernetes-live/video/${BOT_NAME}

.RECIPEPREFIX = >
.PHONY: image push build-tidy

all: build

image: 
> docker build --no-cache=true -t ${DOCKER_TAG} .

push: image
> docker push ${DOCKER_TAG}

build-tidy:
> docker build --build-arg CMAKE_TIDY="/usr/bin/clang-tidy-5.0" .