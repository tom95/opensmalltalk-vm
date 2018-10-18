# XCPPFLAGS=	-Ithird_party/skia/include/c
CXXFLAGS= \
	-I$(top_builddir)/../../../third_party/skia/include/core\
	-I$(top_builddir)/../../../third_party/skia/include/config \
	-I$(top_builddir)/../../../third_party/skia/include/core/ \
	-I$(top_builddir)/../../../third_party/skia/include/config/ \
	-I$(top_builddir)/../../../third_party/skia/include/gpu/ \
	-I$(top_builddir)/../../../third_party/skia/include/utils/ \
	-I$(top_builddir)/../../../third_party/skia/src/gpu/ \
	-I$(top_builddir)/../../../third_party/skia/include/private/ \
	-I$(top_builddir)/../../../third_party/skia/src/gpu/gl/ \
	-I$(top_builddir)/../../../third_party/skia/include/gpu/gl/
	-std=c++11
# XCFLAGS=	$(WFLAGS) $(X_INCLUDES)
PLIBS=-lskia -L$(top_builddir)/../../../third_party/skia/out/Shared
XLDFLAGS=-lskia -L$(top_builddir)/../../../third_party/skia/out/Shared
EXTRALIBS=-lskia -L$(top_builddir)/../../../third_party/skia/out/Shared

