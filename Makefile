SUBDIRS := empty-bot

.RECIPEPREFIX = >

all: $(SUBDIRS)
$(SUBDIRS):
> mkdir -p $@/cmake-build-debug && cd $@/cmake-build-debug && cmake .. && cmake --build .

.PHONY: all $(SUBDIRS)
