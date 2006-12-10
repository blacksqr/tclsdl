/* tclsdl.c - Copyright (C) 2006 Pat Thoyts <patthoyts@users.sourceforge.net>
 *
 */

#include "tclsdl.h"
#include <SDL/SDL.h>
#include <SDL/SDL_version.h>

#define TCLSDLEVENT (SDL_NUMEVENTS - 2)

typedef struct Tclsdl_Event {
    struct Tcl_Event ev;
    Tcl_Interp *interp;
} Tclsdl_Event;

static void
BgEvalObjv(Tcl_Interp *interp, int objc, Tcl_Obj *const *objv)
{
    int n = 0;
    for (n = 0; n < objc; n++)
	Tcl_IncrRefCount(objv[n]);
    Tclsdl_BackgroundEvalObjv(interp, objc, objv, 0);
    for (n = 0; n < objc; n++)
	Tcl_DecrRefCount(objv[n]);
}

static int
EventProc(Tcl_Event *eventPtr, int flags)
{
    Tclsdl_Event *evPtr = (Tclsdl_Event *)eventPtr;
    Tcl_Interp *interp = evPtr->interp;
    SDL_Event sdl_event;

    if (!(flags & TCL_WINDOW_EVENTS)) {
        return 0;
    }
    while (SDL_PollEvent(&sdl_event)) {
	/* call registered function */
        switch (sdl_event.type) {
	    case SDL_QUIT: {
		Tcl_Obj *objv[2];
		objv[0] = Tcl_NewStringObj("sdl::onEvent", -1);
		objv[1] = Tcl_NewStringObj("Quit", -1);
		BgEvalObjv(interp, ARRAYSIZEOF(objv), objv);
                break;
	    }
	    case SDL_ACTIVEEVENT: {
		SDL_ActiveEvent *e = (SDL_ActiveEvent *)&sdl_event;
		Tcl_Obj *objv[2];
		if (e->state & SDL_APPACTIVE) {
		    objv[0] = Tcl_NewStringObj("sdl::onEvent", -1);
		    objv[1] = Tcl_NewStringObj(e->gain 
			? "Activate" : "Deactivate", -1);
		    BgEvalObjv(interp, ARRAYSIZEOF(objv), objv);
		}
		if (e->state & SDL_APPMOUSEFOCUS) {
		    objv[0] = Tcl_NewStringObj("sdl::onEvent", -1);
		    objv[1] = Tcl_NewStringObj(e->gain 
			? "Enter" : "Leave", -1);
		    BgEvalObjv(interp, ARRAYSIZEOF(objv), objv);
		}
		if (e->state & SDL_APPINPUTFOCUS) {
		    objv[0] = Tcl_NewStringObj("sdl::onEvent", -1);
		    objv[1] = Tcl_NewStringObj(e->gain 
			? "FocusIn" : "FocusOut", -1);
		    BgEvalObjv(interp, ARRAYSIZEOF(objv), objv);
		}
		break;
	    }
	    case SDL_VIDEORESIZE: {
		SDL_ResizeEvent *e = (SDL_ResizeEvent *)&sdl_event;
		Tcl_Obj *objv[4];
		objv[0] = Tcl_NewStringObj("sdl::onEvent", -1);
		objv[1] = Tcl_NewStringObj("Configure", -1);
		objv[2] = Tcl_NewIntObj(e->w);
		objv[3] = Tcl_NewIntObj(e->h);
		BgEvalObjv(interp, ARRAYSIZEOF(objv), objv);
		break;
	    }
	    case SDL_MOUSEBUTTONUP:
	    case SDL_MOUSEBUTTONDOWN: {
		Tcl_Obj *objv[5];
		SDL_MouseButtonEvent *e = (SDL_MouseButtonEvent *)&sdl_event;
		objv[0] = Tcl_NewStringObj("sdl::onEvent", -1);
		objv[1] = Tcl_NewStringObj(e->state == SDL_PRESSED 
		    ? "ButtonPress" : "ButtonRelease", -1);
		objv[2] = Tcl_NewIntObj(e->button);
		objv[3] = Tcl_NewIntObj(e->x);
		objv[4] = Tcl_NewIntObj(e->y);
		BgEvalObjv(interp, ARRAYSIZEOF(objv), objv);
                break;
	    }
		
	    case SDL_MOUSEMOTION: {
		Tcl_Obj *objv[7];
		SDL_MouseMotionEvent *e = (SDL_MouseMotionEvent *)&sdl_event;
		objv[0] = Tcl_NewStringObj("sdl::onEvent", -1);
		objv[1] = Tcl_NewStringObj("Motion", -1);
		objv[2] = Tcl_NewIntObj(e->state);
		objv[3] = Tcl_NewIntObj(e->x);
		objv[4] = Tcl_NewIntObj(e->y);
		objv[5] = Tcl_NewIntObj(e->xrel);
		objv[6] = Tcl_NewIntObj(e->yrel);
		BgEvalObjv(interp, ARRAYSIZEOF(objv), objv);
                break;
	    }
	    case TCLSDLEVENT: {
		SDL_UserEvent *e = (SDL_UserEvent *)&sdl_event;
		Tcl_Obj *objv[4];
		objv[0] = Tcl_NewStringObj("sdl::onEvent", -1);
		objv[1] = Tcl_NewStringObj("User", -1);
		objv[2] = (Tcl_Obj *)e->data1;
		objv[3] = (Tcl_Obj *)e->data2;
		BgEvalObjv(interp, ARRAYSIZEOF(objv), objv);
		Tcl_DecrRefCount((Tcl_Obj *)e->data1);
		Tcl_DecrRefCount((Tcl_Obj *)e->data2);
                break;
	    }
        }
    }
    return 1;
}

static void
SetupProc(ClientData clientData, int flags) {
    Tcl_Time block_time = {0, 0};
    if (!(flags & TCL_WINDOW_EVENTS)) {
        return;
    }
    /* If there are no events to process then set a wait */
    if (!SDL_PollEvent(NULL)) {
        block_time.usec = 10000;
    }
    Tcl_SetMaxBlockTime(&block_time);
    return;
}

static void
CheckProc(ClientData clientData, int flags) {
    if (!(flags & TCL_WINDOW_EVENTS)) {
        return;
    }
    /* if there are SDL events, fire a Tk event to get them processed */
    if (SDL_PollEvent(NULL)) {
        Tclsdl_Event *event = (Tclsdl_Event *)ckalloc(sizeof(Tclsdl_Event));
        event->ev.proc = EventProc;
	event->interp = (Tcl_Interp *)clientData;
        Tcl_QueueEvent((Tcl_Event *)event, TCL_QUEUE_TAIL);
    }
    return;
}

static int
WarpObjCmd(ClientData clientData, Tcl_Interp *interp, 
                  int objc, Tcl_Obj *const objv[])
{
    int x, y;
    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "x y");
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[1], &x) != TCL_OK
	|| Tcl_GetIntFromObj(interp, objv[2], &y) != TCL_OK) {
	return TCL_ERROR;
    }
    SDL_WarpMouse((Uint16)x, (Uint16)y);
    return TCL_OK;
}

static int
VersionObjCmd(ClientData clientData, Tcl_Interp *interp, 
                  int objc, Tcl_Obj *const objv[])
{
    const SDL_version *vPtr;
    char buffer[TCL_INTEGER_SPACE * 3 + 2];
    if (objc != 1) {
        Tcl_WrongNumArgs(interp, 1, objv, "");
        return TCL_ERROR;
    }
    vPtr = SDL_Linked_Version();
    sprintf(buffer, "%d.%d.%d", vPtr->major, vPtr->minor, vPtr->patch);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(buffer, -1));
    return TCL_OK;
}

static int
InfoObjCmd(ClientData clientData, Tcl_Interp *interp, 
                  int objc, Tcl_Obj *const objv[])
{
    const SDL_VideoInfo *infoPtr;
    Tcl_Obj *objPtr;
    infoPtr = SDL_GetVideoInfo();
    objPtr = Tcl_NewListObj(0, NULL);

    Tcl_ListObjAppendElement(interp, objPtr, 
	Tcl_NewStringObj("hw_available", -1));
    Tcl_ListObjAppendElement(interp, objPtr,
	Tcl_NewBooleanObj(infoPtr->hw_available));

    Tcl_ListObjAppendElement(interp, objPtr, 
	Tcl_NewStringObj("wm_available", -1));
    Tcl_ListObjAppendElement(interp, objPtr,
	Tcl_NewBooleanObj(infoPtr->wm_available));

    Tcl_ListObjAppendElement(interp, objPtr, 
	Tcl_NewStringObj("blit_hw", -1));
    Tcl_ListObjAppendElement(interp, objPtr,
	Tcl_NewBooleanObj(infoPtr->blit_hw));

    Tcl_ListObjAppendElement(interp, objPtr, 
	Tcl_NewStringObj("blit_sw", -1));
    Tcl_ListObjAppendElement(interp, objPtr,
	Tcl_NewBooleanObj(infoPtr->blit_sw));

    Tcl_ListObjAppendElement(interp, objPtr, 
	Tcl_NewStringObj("video_mem", -1));
    Tcl_ListObjAppendElement(interp, objPtr,
	Tcl_NewWideIntObj((Tcl_WideInt)infoPtr->video_mem));
    
    Tcl_SetObjResult(interp, objPtr);
    return TCL_OK;
}

static int
EventObjCmd(ClientData clientData, Tcl_Interp *interp, 
                  int objc, Tcl_Obj *const objv[])
{
    SDL_UserEvent event;
    Tcl_Obj *tmpObj = NULL;

    if (objc < 2 || objc > 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "eventname ?value?");
	return TCL_ERROR;
    }

    event.type = TCLSDLEVENT;
    event.code = (int)0;
    event.data2 = NULL;

    /* copy objv[1] into the event */
    tmpObj = objv[1];
    if (Tcl_IsShared(tmpObj)) {
	tmpObj = Tcl_DuplicateObj(tmpObj);
    }
    Tcl_IncrRefCount(tmpObj);
    event.data1 = (void *)tmpObj;
    
    tmpObj = (objc == 3) ? objv[2] : Tcl_NewStringObj("", -1);
    if (Tcl_IsShared(tmpObj)) {
	tmpObj = Tcl_DuplicateObj(tmpObj);
    }
    Tcl_IncrRefCount(tmpObj);
    event.data2 = (void *)tmpObj;
    
    SDL_PushEvent((SDL_Event *)&event);
    return TCL_OK;
}

static void
InterpDeleteProc(ClientData clientData, Tcl_Interp *interp)
{
    SDL_Quit();
}
        
#define TCL_VERSION_WRONG "8.0" /* see tktable bug #1091431 */

static char initScript[] = 
  "namespace eval ::sdl { if {[llength [info commands ::sdl::onEvent]] == 0}" 
  " { proc onEvent {type args} {} } }";

int DLLEXPORT
Tclsdl_Init(Tcl_Interp *interp)
{
#ifdef USE_TCL_STUBS
    if (Tcl_InitStubs(interp, TCL_VERSION_WRONG, 0) == NULL) {
        return TCL_ERROR;
    }
#endif

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        Tcl_SetResult(interp, "failed to init SDL library", TCL_STATIC);
        return TCL_ERROR;
    }

    /* register cleanup to call SDL_Quit */
    Tcl_CallWhenDeleted(interp, InterpDeleteProc, NULL);

    /* Register our eventloop integration */
    Tcl_CreateEventSource(SetupProc, CheckProc, interp);

    Tcl_CreateObjCommand(interp, "sdl::surface", SurfaceObjCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "sdl::mixer", MixerObjCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "sdl::warp", WarpObjCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "sdl::version", VersionObjCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "sdl::videoinfo", InfoObjCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "sdl::event", EventObjCmd, NULL, NULL);

    if (Tcl_Eval(interp, initScript) != TCL_OK)
	return TCL_ERROR;

    Tcl_PkgProvideEx(interp, PACKAGE_NAME, PACKAGE_VERSION, NULL);
    return TCL_OK;
}

/*
 * Local variables:
 *   indent-tabs-mode: t
 *   tab-width: 8
 * End:
 */
