XCPPFLAGS=	-I$(topdir)/platforms/unix/plugins/B3DAcceleratorPlugin
CXXFLAGS=	-std=c++11\
	-I/home/tom/Code/c/skia_wrapper/third_party/skia/include/core/\
	-I/home/tom/Code/c/skia_wrapper/third_party/skia/include/config/\
	-I/home/tom/Code/c/skia_wrapper/third_party/skia/include/gpu/\
	-I/home/tom/Code/c/skia_wrapper/third_party/skia/include/utils/\
	-I/home/tom/Code/c/skia_wrapper/third_party/skia/src/gpu/\
	-I/home/tom/Code/c/skia_wrapper/third_party/skia/include/private/\
	-I/home/tom/Code/c/skia_wrapper/third_party/skia/src/gpu/gl/\
	-I/home/tom/Code/c/skia_wrapper/third_party/skia/include/gpu/gl/\
	-D_REENTRANT\
	-I/usr/include/SDL2\
	-lSDL2\
	-L/home/tom/Code/c/skia_wrapper/third_party/skia/out/Shared\
	-DGR_GL_CHECK_ERROR=0\
	-DGR_GL_LOG_CALLS=0

XCFLAGS=	$(WFLAGS) $(X_INCLUDES) -L/home/tom/Code/c/skia_wrapper/third_party/skia/out/Shared 
PLIBS=-lSDL2 -lGL -lskia
