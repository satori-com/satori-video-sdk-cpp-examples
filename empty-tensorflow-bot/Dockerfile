FROM gcr.io/kubernetes-live/video/cmake-bot-builder as builder
FROM ubuntu:rolling

RUN mkdir -p /app/
COPY --from=builder /build/bin/* /app/
ENV LD_LIBRARY_PATH=/app/
WORKDIR /app/

ENTRYPOINT ["/app/empty-tensorflow-bot"]
