#
# Include the TEA standard macro set
#

builtin(include,tclconfig/tcl.m4)

#
# Add here whatever m4 macros you want to define for your package
#

#-------------------------------------------------------------------------
# TCLSDL_SDL_CONFIG
#
#	Do we have sdl-config or can we find SDL headers
#
#-------------------------------------------------------------------------

AC_DEFUN(TCLSDL_SDL_CONFIG, [
    AC_DEFINE(SDL_CFLAGS)
    AC_DEFINE(SDL_LIBS)
    AC_PATH_TOOL([sdlconfig],[sdl-config],[:])
    if test "$sdlconfig" = ":"
    then
        AC_MSG_ERROR([Failed to find sdl-config])
    else
        SDL_CFLAGS=`$sdlconfig --cflags`
        SDL_LIBS="`$sdlconfig --libs` -lSDL_mixer"
    fi

    AC_SUBST(SDL_CFLAGS)
    AC_SUBST(SDL_LIBS)
])
