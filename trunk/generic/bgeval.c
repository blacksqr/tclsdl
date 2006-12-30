/* bgeval.c - Copyright (C) 2006 Pat Thoyts <patthoyts@users.sourceforge.net>
 *
 * $Id: bgeval.c,v 1.2 2006/10/25 08:26:21 pat Exp $
 */

#include "tclsdl.h"

/*
 * ----------------------------------------------------------------------
 *
 * BackgroundEvalObjEx --
 *
 *	Evaluate a command while ensuring that we do not affect the 
 *	interpreters state. This is important when evaluating script
 *	during background tasks.
 *
 * Results:
 *	A standard Tcl result code.
 *
 * Side Effects:
 *	The interpreters variables and code may be modified by the script
 *	but the result will not be modified.
 *
 * ----------------------------------------------------------------------
 */

PKGAPI int
Tclsdl_BackgroundEvalObjv(Tcl_Interp *interp, 
    int objc, Tcl_Obj *const *objv, int flags)
{
    Tcl_DString errorInfo, errorCode;
    Tcl_SavedResult state;
    int r = TCL_OK;
    
    Tcl_DStringInit(&errorInfo);
    Tcl_DStringInit(&errorCode);

    /*
     * Record the state of the interpreter
     */

    Tcl_SaveResult(interp, &state);
    Tcl_DStringAppend(&errorInfo, 
	Tcl_GetVar(interp, "errorInfo", TCL_GLOBAL_ONLY), -1);
    Tcl_DStringAppend(&errorCode, 
	Tcl_GetVar(interp, "errorCode", TCL_GLOBAL_ONLY), -1);
    
    /*
     * Evaluate the command and handle any error.
     */
    
    r = Tcl_EvalObjv(interp, objc, objv, flags);
    if (r == TCL_ERROR) {
        Tcl_AddErrorInfo(interp, "\n    (background event handler)");
        Tcl_BackgroundError(interp);
    }
    
    /*
     * Restore the state of the interpreter
     */
    
    Tcl_SetVar(interp, "errorInfo",
	Tcl_DStringValue(&errorInfo), TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "errorCode",
	Tcl_DStringValue(&errorCode), TCL_GLOBAL_ONLY);
    Tcl_RestoreResult(interp, &state);
    
    /*
     * Clean up references.
     */
    
    Tcl_DStringFree(&errorInfo);
    Tcl_DStringFree(&errorCode);
    
    return r;
}

/*
 * Local variables:
 *   indent-tabs-mode: t
 *   tab-width: 8
 * End:
 */
