SAVED_CFLAGS="$CFLAGS"
PKG_CHECK_MODULES(PANGOCAIRO,pangocairo,
  PKG_CHECK_MODULES(FREETYPE,freetype2,
    AC_MSG_CHECKING([for FreeType support in Cairo])
    CFLAGS="$CFLAGS $FREETYPE_CFLAGS $PANGOCAIRO_CFLAGS"
    AC_TRY_COMPILE([
      #include <cairo-ft.h>
    ],[;],[
      AC_MSG_RESULT(yes)
      AC_PLUGIN_DEFINE_UNQUOTED(FREETYPE_CFLAGS,$FREETYPE_CFLAGS)
      AC_PLUGIN_DEFINE_UNQUOTED(FREETYPE_LIBS,$FREETYPE_LIBS)
      AC_PLUGIN_DEFINE_UNQUOTED(PANGOCAIRO_CFLAGS,$PANGOCAIRO_CFLAGS)
      AC_PLUGIN_DEFINE_UNQUOTED(PANGOCAIRO_LIBS,$PANGOCAIRO_LIBS)
    ],[
      AC_MSG_RESULT(no)
      AC_PLUGIN_DISABLE
    ])
  ,AC_PLUGIN_DISABLE)
,AC_PLUGIN_DISABLE)
CFLAGS="$SAVED_CFLAGS"