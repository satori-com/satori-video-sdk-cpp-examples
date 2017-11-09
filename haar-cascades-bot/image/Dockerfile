FROM ubuntu:rolling

RUN mkdir -p /app/models
ADD in/haar-cascades-bot.tgz /app/
ADD https://raw.githubusercontent.com/opencv/opencv/master/data/haarcascades/haarcascade_frontalface_default.xml /app/models/frontalface_default.xml
ADD https://raw.githubusercontent.com/opencv/opencv/master/data/haarcascades/haarcascade_smile.xml /app/models/smile.xml

WORKDIR /app
ENTRYPOINT ["/app/haar-cascades-bot"]