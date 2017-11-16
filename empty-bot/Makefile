BOT_NAME=empty-bot

BUILD_IMAGE_NAME = gcr.io/kubernetes-live/video/${BOT_NAME}-build
PROD_IMAGE_NAME = gcr.io/kubernetes-live/video/${BOT_NAME}

.RECIPEPREFIX = >
.PHONY: image push build

all: build

build:
> docker build -t ${BUILD_IMAGE_NAME} .

build-tidy:
> docker build --build-arg CMAKE_TIDY="/usr/bin/clang-tidy-5.0" -t ${BUILD_IMAGE_NAME}-tidy .

prod-image/in/${BOT_NAME}.tgz: build
> mkdir -p prod-image/in/
> docker run --rm ${BUILD_IMAGE_NAME} bash -c "cd /build/bin/ && tar -czhf - ${BOT_NAME}" > $@

image: prod-image/in/${BOT_NAME}.tgz
> @echo "Building bot image"
> docker build --no-cache=true -t ${PROD_IMAGE_NAME} prod-image/

push: image
> @echo "Pushing video bot image"
> docker push ${PROD_IMAGE_NAME}
