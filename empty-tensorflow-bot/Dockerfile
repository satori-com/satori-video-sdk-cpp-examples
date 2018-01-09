FROM gcr.io/kubernetes-live/video/cmake-bot-builder as builder
FROM ubuntu:rolling

ARG MODEL_ARCHIVE_NAME=inception_v3_2016_08_28_frozen.pb.tar.gz

RUN mkdir -p /app/
COPY --from=builder /build/bin/* /app/
ENV LD_LIBRARY_PATH=/app/
COPY --from=builder /build/models/${MODEL_ARCHIVE_NAME} /app/
RUN cd /app/ && tar -xf ${MODEL_ARCHIVE_NAME} && rm ${MODEL_ARCHIVE_NAME}
WORKDIR /app/

ENTRYPOINT ["/app/empty-tensorflow-bot"]
