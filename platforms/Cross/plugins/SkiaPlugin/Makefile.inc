XCPPFLAGS=	-I/home/tom/tmp/skia/include/c
CXXFLAGS=	-I/home/tom/tmp/skia/include/c -I/home/tom/tmp/skia/include/core -I/home/tom/tmp/skia/include/config -std=c++11
XCFLAGS=	$(WFLAGS) $(X_INCLUDES)
PLIBS=-lskia -L/home/tom/tmp/skia/out/Shared
