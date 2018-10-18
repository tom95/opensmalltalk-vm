# -*- sh -*-

AC_MSG_CHECKING([for Skia support])

LIB_SKIA="-lskia -L$(top_builddir)/../../../third_party/skia/out/Shared"
AC_SUBST(LIB_SKIA)
