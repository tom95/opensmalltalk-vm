PLUGIN_NAME=SkiaPlugin
SOURCES=skia.c
PATH_TO_SQUEAK=../../../../
BASE=../../../../../opensmalltalk-vm

CFLAGS=-Wall \
	   -g \
	   -I$(BASE)/platforms/Cross/vm \
	   -I$(BASE)/platforms/Cross/plugins/$(PLUGIN_NAME) \
	   -I../Cross \
	   -I. \
	   -I$(BASE)/platforms/unix/vm \
	   -I$(BASE)/src/vm \
	   -I$(BASE)/build.linux64x64/squeak.cog.spur/build.debug \
	   -DLSB_FIRST=1 \
	   -I/home/tom/tmp/skia/include/c \
	   -L/home/tom/tmp/skia/out/Shared
LDFLAGS=-shared \
	   -fPIC
LDLIBS=-lskia
SOURCES += $(BASE)/src/plugins/$(PLUGIN_NAME)/$(PLUGIN_NAME).c
OBJECTS = $(SOURCES:.c=.o)

EXECUTABLE=$(BASE)/build.linux64x64/squeak.cog.spur/build.debug

install: $(EXECUTABLE)

$(EXECUTABLE): $(PLUGIN_NAME)
	cp $(PLUGIN_NAME) $(EXECUTABLE)

$(PLUGIN_NAME): $(OBJECTS)

clean:
	ls $(EXECUTABLE) $(PLUGIN_NAME) $(OBJECTS)

LINK_PLATFORM=$(BASE)/platforms/unix/plugins/$(PLUGIN_NAME)
LINK_CROSS=$(BASE)/platforms/Cross/plugins/$(PLUGIN_NAME)
symlink:
	ln -s $(shell pwd) $(LINK_PLATFORM)
	ln -s $(shell pwd)/../Cross $(LINK_CROSS)
cleansymlink:
	rm -f $(LINK_PLATFORM) $(LINK_CROSS)

.PHONY: clean install
