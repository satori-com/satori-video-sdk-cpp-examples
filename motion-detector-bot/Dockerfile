FROM gcr.io/kubernetes-live/video/cmake-bot-builder as builder
FROM ubuntu:rolling
RUN mkdir -p /app
WORKDIR /app
COPY --from=builder /build/bin/* /app/
ENTRYPOINT ["/app/motion-detector-bot"]