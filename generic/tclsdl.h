/* tclsdl.h - Copyright (C) 2006 Pat Thoyts <patthoyts@users.sourceforge.net>
 *
 */

#ifndef TCLSDL_H_INCLUDE
#define TCLSDL_H_INCLUDE 1

#ifdef __cplusplus
extern "C" {
#endif

#include <tcl.h>

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "Tclsdl"
#endif
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "1.0"
#endif

#if defined(BUILD_tclsdl)
#  define PKGAPI DLLEXPORT
#  undef USE_TCLSDL_STUBS
#else
#  define PKGAPI DLLIMPORT
#endif

#ifndef ARRAYSIZEOF
#define ARRAYSIZEOF(x) (sizeof((x))/sizeof((x)[0]))
#endif

/* Package scope */
Tcl_ObjCmdProc SurfaceObjCmd;
Tcl_ObjCmdProc MixerObjCmd;

/* API Functions */
PKGAPI int  Tclsdl_BackgroundEvalObjv(Tcl_Interp *interp, 
    int objc, Tcl_Obj *const *objv, int flags);


#ifdef __cplusplus
}
#endif

#endif /* TCLSDL_H_INCLUDE */

/*
 * Local variables:
 *   indent-tabs-mode: t
 *   tab-width: 8
 * End:
 */
