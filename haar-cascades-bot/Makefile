.RECIPEPREFIX = >
.PHONY: image/in/haar-cascades-bot.tgz

BUILDING_IMAGE_NAME = haar-cascades-bot-building
IMAGE_NAME = haar-cascades-bot

all: image

building-image:
> docker build -t ${BUILDING_IMAGE_NAME} .

image/in/haar-cascades-bot.tgz: building-image
> rm -rf image/in
> mkdir image/in
> docker run --rm ${BUILDING_IMAGE_NAME} bash -c "cd /build/bin/ && tar -czhf - haar-cascades-bot" > $@

image: image/in/haar-cascades-bot.tgz
> docker build --no-cache=true -t ${IMAGE_NAME} image/