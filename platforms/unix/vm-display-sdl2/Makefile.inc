XCPPFLAGS=	-I$(topdir)/platforms/unix/plugins/B3DAcceleratorPlugin
CXXFLAGS=	-std=c++11 -I/home/tom/tmp/skia/include/core -I/home/tom/tmp/skia/include/gpu -I/home/tom/tmp/skia/include/config -I/home/tom/tmp/skia/include/utils -I/home/tom/tmp/skia/src/gpu -D_REENTRANT -I/usr/include/SDL2 -lSDL2 -L/home/tom/tmp/skia/out/Shared -DGR_GL_CHECK_ERROR=0 -DGR_GL_LOG_CALLS=0
XCFLAGS=	$(WFLAGS) $(X_INCLUDES) -L/home/tom/tmp/skia/out/Shared 
PLIBS=-lSDL2 -lGL -lskia
