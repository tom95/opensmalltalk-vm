# -*- sh -*-

AC_MSG_CHECKING([for sdl2 display support])

AC_ARG_WITH(sdl2,
[  --with-sdl2   enable sdl2 window support [default=disabled]],
  [have_dpy_sdl2="$withval"],
  [have_dpy_sdl2="no"])

if test "$have_dpy_sdl2" = "yes"; then
  # check for libraries, headers, etc., here...
  AC_MSG_RESULT([yes])
  AC_CHECK_LIB(SDL2, SDL_Init)

  AC_TRY_COMPILE([#include <SDL2/SDL.h>],[(void)SDL_Init;],[
      AC_MSG_RESULT(yes)
  ],[
    AC_MSG_RESULT(no)
    AC_PLUGIN_DISABLE
  ])

else
  AC_MSG_RESULT([no])
  AC_PLUGIN_DISABLE
fi

