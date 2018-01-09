# Eventually we won't depend on our internal docker registry

FROM gcr.io/kubernetes-live/video/cmake-bot-builder as builder

FROM ubuntu:rolling

RUN mkdir -p /app
COPY --from=builder /build/bin/* /app/
COPY --from=builder /build/models/* /app/models/
WORKDIR /app

ENTRYPOINT ["/app/haar-cascades-bot"]
