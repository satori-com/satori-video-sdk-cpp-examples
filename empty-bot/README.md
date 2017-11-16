# Empty Video Bot

This is a skeleton-bot.

It could be used as an example of Docker image build. The idea is:
all binaries are built with cpp-builder image and put into clean ubuntu container.

To build cpp-builder please run:

```bash
make -C ../../ cpp-builder
```

After the image is successfully built, you should be able to run bot with:

```bash
docker run -it gcr.io/kubernetes-live/video/empty-bot
```

For more details please refer to a Makefile.

To build this bot locally:

```bash
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release ../ && make -j8
```
