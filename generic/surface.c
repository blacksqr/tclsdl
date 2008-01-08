/*
 * set surface [sdl::surface create -width 800 -height 600 -bpp 32 -hardware 1]
 * $surface delete               ;# call SDL_FreeSurface
 * $surface flip $surface        ;# swap two surfaces
 * $surface blit dest x y
 * $surface loadbmp filename     ;# load a bitmap from file to surface.
 *
 */

#include "tclsdl.h"
#include <SDL/SDL.h>
#include <SDL/SDL_version.h>
#include <SDL/SDL_getenv.h>

typedef struct SurfaceData {
    SDL_Surface  *surface;
    Tcl_Command   token;
    unsigned long windowid;
} SurfaceData;

struct Ensemble {
    const char *name;          /* subcommand name */
    Tcl_ObjCmdProc *command;   /* subcommand implementation OR */
    struct Ensemble *ensemble; /* subcommand ensemble */
};

Tcl_ObjCmdProc SurfaceObjCmd;

/* ----------------------------------------------------------------------
 * SDL Color object wrapper
 */

static void SDLColor_FreeIntRep(Tcl_Obj *objPtr);
static void SDLColor_DupIntRep(Tcl_Obj *srcPtr, Tcl_Obj *dstPtr);
static int  SDLColor_SetFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr);

static Tcl_ObjType sdlColorType = {
    "sdlcolor",
    SDLColor_FreeIntRep,        /* freeIntRepProc */
    SDLColor_DupIntRep,         /* dupIntRepProc*/
    NULL,                       /* updateStringProc */
    SDLColor_SetFromAny,        /* setFromAnyProc */
};

#define SDLCOLOR_INTREP(objPtr) ((objPtr)->internalRep.otherValuePtr)

void
SDLColor_FreeIntRep(Tcl_Obj *objPtr)
{
    ckfree((char *)SDLCOLOR_INTREP(objPtr));
    SDLCOLOR_INTREP(objPtr) = NULL;
    objPtr->typePtr = NULL;
}
void
SDLColor_DupIntRep(Tcl_Obj *srcPtr, Tcl_Obj *dstPtr)
{
    dstPtr->typePtr = srcPtr->typePtr;
    SDLCOLOR_INTREP(dstPtr) = ckalloc(sizeof(SDL_Color));
    memcpy(SDLCOLOR_INTREP(dstPtr), SDLCOLOR_INTREP(srcPtr),
           sizeof(SDL_Color));
}
int
SDLColor_SetFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    SDL_Color *colorPtr = NULL;
    Tcl_Obj **objv = NULL;
    int r = 0, g = 0, b = 0, objc = 0;
    long value = 0;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc == 1) {
        if (Tcl_GetLongFromObj(interp, objPtr, &value) != TCL_OK) {
            return TCL_ERROR;
        }
        r = (int)((value >> 16) & 0xff);
        g = (int)((value >> 8) & 0xff);
        b = (int)((value     ) & 0xff);
    } else if (objc == 3) {
        if (Tcl_GetIntFromObj(interp, objv[0], &r) != TCL_OK
            || Tcl_GetIntFromObj(interp, objv[1], &g) != TCL_OK
            || Tcl_GetIntFromObj(interp, objv[2], &b) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    colorPtr = (SDL_Color *)ckalloc(sizeof(SDL_Color));
    colorPtr->r = (Uint8)r;
    colorPtr->g = (Uint8)g;
    colorPtr->b = (Uint8)b;

    objPtr->typePtr = &sdlColorType;
    SDLCOLOR_INTREP(objPtr) = colorPtr;
    return TCL_OK;
}
static int
GetSDLColorFromObj(Tcl_Interp *interp, SDL_Surface *surface, 
                   Tcl_Obj *objPtr, Uint32 *colorPtr)
{
    SDL_Color *p = NULL;

    if (objPtr->typePtr != &sdlColorType) {
        if (SDLColor_SetFromAny(interp, objPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    p = (SDL_Color *)SDLCOLOR_INTREP(objPtr);
    *colorPtr = SDL_MapRGB(surface->format, p->r, p->g, p->b);
    return TCL_OK;
}

/* ----------------------------------------------------------------------
 * SDL_Rect object wrapper
 */

static void SDLRect_FreeIntRep(Tcl_Obj *objPtr);
static void SDLRect_DupIntRep(Tcl_Obj *srcPtr, Tcl_Obj *dstPtr);
static int  SDLRect_SetFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr);

static Tcl_ObjType sdlRectType = {
    "sdlrect",
    SDLRect_FreeIntRep,         /* freeIntRepProc */
    SDLRect_DupIntRep,          /* dupIntRepProc*/
    NULL,                       /* updateStringProc */
    SDLRect_SetFromAny,         /* setFromAnyProc */
};

#define SDLRECT_INTREP(objPtr) ((objPtr)->internalRep.otherValuePtr)

void
SDLRect_FreeIntRep(Tcl_Obj *objPtr)
{
    ckfree((char *) SDLRECT_INTREP(objPtr));
    SDLRECT_INTREP(objPtr) = NULL;
    objPtr->typePtr = NULL;
}
void
SDLRect_DupIntRep(Tcl_Obj *srcPtr, Tcl_Obj *dstPtr)
{
    dstPtr->typePtr = srcPtr->typePtr;
    SDLRECT_INTREP(dstPtr) = (void *)ckalloc(sizeof(SDL_Rect));
    memcpy(SDLRECT_INTREP(dstPtr), SDLRECT_INTREP(srcPtr), sizeof(SDL_Rect));
}
int
SDLRect_SetFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    int objc;
    Tcl_Obj **objv;
    int x = 0, y = 0, w = 0, h = 0;
    SDL_Rect *rectPtr = NULL;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }

    if (objc < 2 || objc > 4 || objc == 3) {
        Tcl_AppendResult(interp, "invalid rect specification", NULL);
        return TCL_ERROR;
    }
    
    if (Tcl_GetIntFromObj(interp, objv[0], &x) != TCL_OK
        || Tcl_GetIntFromObj(interp, objv[1], &y) != TCL_OK) {
        return TCL_ERROR;
    }

    if (objc == 4) {
        if (Tcl_GetIntFromObj(interp, objv[2], &w) != TCL_OK
            || Tcl_GetIntFromObj(interp, objv[3], &h) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    
    if (objPtr->typePtr != NULL && objPtr->typePtr->freeIntRepProc != NULL) {
        (*objPtr->typePtr->freeIntRepProc)(objPtr);
    }
    
    objPtr->typePtr = &sdlRectType;
    rectPtr = (SDL_Rect *)ckalloc(sizeof(SDL_Rect));
    rectPtr->x = x;
    rectPtr->y = y;
    rectPtr->w = w;
    rectPtr->h = h;
    SDLRECT_INTREP(objPtr) = rectPtr;
    return TCL_OK;
}
static int
GetSDLRectFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, SDL_Rect *rectPtr)
{
    int r = TCL_OK;

    if (objPtr->typePtr != &sdlRectType) {
        r = SDLRect_SetFromAny(interp, objPtr);
        if (r != TCL_OK) {
            return r;
        }
    }
    memcpy(rectPtr, SDLRECT_INTREP(objPtr), sizeof(SDL_Rect));
    return TCL_OK;
}

/* ----------------------------------------------------------------------
 * Direct pixel access functions
 */

static int SetPixel8(Tcl_Interp * interp, SDL_Surface * surface, int x, int y, Tcl_Obj * colorObj) {
    Uint8 * pixels = (Uint8 *)surface->pixels;
    int color = 0;
    if (Tcl_GetIntFromObj(interp,colorObj,&color)!=TCL_OK) {
        return TCL_ERROR;
    }
    pixels[y*surface->pitch+x] =  (Uint8)color ;
    return TCL_OK;
}

static int GetPixel8(Tcl_Interp * interp, SDL_Surface * surface, int x, int y, Tcl_Obj ** colorObj) {
    Uint8 * pixels = (Uint8 *)surface->pixels;
    int color = 0;
    color = (int)pixels[y*surface->pitch+x] ;
    *colorObj = Tcl_NewIntObj(color);
    return TCL_OK;
}

static int SetPixel32(Tcl_Interp * interp, SDL_Surface * surface, int x, int y, Tcl_Obj * colorObj) {
    Uint32 * pixels = (Uint8 *)surface->pixels;
    int objc, r, g, b;
    Tcl_Obj ** objv;
    unsigned long color;
    if (Tcl_ListObjGetElements(interp, colorObj, &objc, &objv) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc != 3) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("32 bit colors require a {r g b} list",-1));
        return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[0], &r) != TCL_OK
            || Tcl_GetIntFromObj(interp, objv[1], &g) != TCL_OK
            || Tcl_GetIntFromObj(interp, objv[2], &b) != TCL_OK) {
        return TCL_ERROR;
    }
    color = (b << 16) + (g << 8) + r;
    pixels[y*surface->pitch/4+x] =  (Uint32)color ;

    return TCL_OK;
}

static int GetPixel32(Tcl_Interp * interp, SDL_Surface * surface, int x, int y, Tcl_Obj ** colorObj) {
    Uint32 * pixels = (Uint32 *)surface->pixels;
    Uint32 color = 0;
    Tcl_Obj * rgb;
    unsigned int r, g, b ;
    color = pixels[y*surface->pitch/4+x] ;

    r = ( color >> 0 ) && 0xFF;
    g = ( color >> 8 ) && 0xFF;
    b = ( color >> 16 ) && 0xFF;
    rgb = Tcl_NewListObj(0,NULL);
    Tcl_ListObjAppendElement(interp,rgb, Tcl_NewIntObj(r));
    Tcl_ListObjAppendElement(interp,rgb, Tcl_NewIntObj(g));
    Tcl_ListObjAppendElement(interp,rgb, Tcl_NewIntObj(b));
    *colorObj = rgb;
    return TCL_OK;
}

/* ---------------------------------------------------------------------- */

static int
SurfaceDeleteCmd(ClientData clientData, Tcl_Interp *interp, 
                  int objc, Tcl_Obj *const objv[])
{
    SurfaceData *dataPtr = clientData;
    Tcl_DeleteCommandFromToken(interp, dataPtr->token);
    return TCL_OK;
}

static int
SurfaceFlipCmd(ClientData clientData, Tcl_Interp *interp, 
                  int objc, Tcl_Obj *const objv[])
{
    SurfaceData *dataPtr = clientData;
    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "");
        return TCL_ERROR;
    }
    if (SDL_Flip(dataPtr->surface) < 0) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(SDL_GetError(), -1));
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
SurfacePixelCmd(ClientData clientData, Tcl_Interp *interp, 
                  int objc, Tcl_Obj *const objv[])
{
    int (*getPixelProcPtr)(Tcl_Interp *, SDL_Surface *, int, int, Tcl_Obj **) = NULL ; 
    int (*setPixelProcPtr)(Tcl_Interp *, SDL_Surface *, int, int, Tcl_Obj *) = NULL ; 
    SurfaceData *dataPtr = clientData;
    int x, y, rc ;
    Tcl_Obj * res ; 

    if (objc < 4 || objc > 5 ) {
        Tcl_WrongNumArgs(interp, 2, objv, "x y ?color?");
        return TCL_ERROR;
    }

    switch (dataPtr->surface->format->BitsPerPixel) {
        case 8:
            setPixelProcPtr = SetPixel8;
            getPixelProcPtr = GetPixel8;
            break;
        case 32:
            setPixelProcPtr = SetPixel32;
            getPixelProcPtr = GetPixel32;
            break;
        default:
            Tcl_AppendResult(interp,"operation not supported for this surface format",NULL);
            return TCL_ERROR;
    }
    
    if (Tcl_GetIntFromObj(interp,objv[2],&x)!=TCL_OK || Tcl_GetIntFromObj(interp,objv[3],&y)!=TCL_OK) {
        return TCL_ERROR;
    }

    if (objc == 5) {
        rc = setPixelProcPtr(interp, dataPtr->surface, x, y, objv[4]);
    } else {
        rc = getPixelProcPtr(interp, dataPtr->surface, x, y, &res);
        if (rc==TCL_OK) {
            Tcl_SetObjResult(interp,res);
        }
    }
    return rc;
}

static int
SurfaceSetColorsCmd(ClientData clientData, Tcl_Interp *interp, 
                  int objc, Tcl_Obj *const objv[])
{
    SurfaceData *dataPtr = clientData;
    unsigned long firstcolor = 0;
    int i;
    int listObjc, colorObjc;
    int col;
    int res = TCL_OK;
    SDL_Color * colors;
    Tcl_Obj ** listObjv;
    Tcl_Obj ** colorObjv ;

    if (objc != 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "colors firstcolor");
        return TCL_ERROR;
    }

    if (dataPtr->surface->format->BitsPerPixel!=8) {
        Tcl_AppendResult(interp,"surface is not paletted (8 bit)",NULL);
        return TCL_ERROR;
    }
    
    if (Tcl_GetLongFromObj(interp, objv[3], (long *)&firstcolor) != TCL_OK || 
            Tcl_ListObjGetElements(interp,objv[2],&listObjc,&listObjv)!=TCL_OK) {
        return TCL_ERROR;
    }
    colors = (SDL_Color *)ckalloc(listObjc*sizeof(SDL_Color));
    for (i = 0 ; i < listObjc; i++) {
        if (Tcl_ListObjGetElements(interp,listObjv[i],&colorObjc,&colorObjv)!=TCL_OK || colorObjc != 3) {
            Tcl_AppendResult(interp,"invalid color",NULL);
            return TCL_ERROR;
        }
        Tcl_GetIntFromObj(interp,colorObjv[0],&col);
        colors[i].r = (Uint8)col;
        Tcl_GetIntFromObj(interp,colorObjv[1],&col);
        colors[i].g = (Uint8)col;
        Tcl_GetIntFromObj(interp,colorObjv[2],&col);
        colors[i].b = (Uint8)col;
    }
    if (SDL_SetColors(dataPtr->surface,colors,firstcolor,listObjc)!=1) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(SDL_GetError(), -1));
        res = TCL_ERROR;
    }
    ckfree((char *)colors);
    return res;
}

static int
SurfaceSetColorKeyCmd(ClientData clientData, Tcl_Interp *interp, 
                  int objc, Tcl_Obj *const objv[])
{
    SurfaceData *dataPtr = clientData;
    unsigned long clr = 0;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "rgb");
        return TCL_ERROR;
    }
    
    if (Tcl_GetLongFromObj(interp, objv[2], (long *)&clr) != TCL_OK)
        return TCL_ERROR;

    if (SDL_SetColorKey(dataPtr->surface, SDL_SRCCOLORKEY, 
                        SDL_MapRGB(dataPtr->surface->format, 
                                   (Uint8)((clr >> 16) & 0xFF),
                                   (Uint8)((clr >>  8) & 0xFF),
                                   (Uint8)(clr        & 0xFF))) < 0) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(SDL_GetError(), -1));
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
SurfaceMustLockCmd(ClientData clientData, Tcl_Interp *interp, 
                  int objc, Tcl_Obj *const objv[])
{
    int res ;
    SurfaceData *dataPtr = clientData;
    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
    }
    
    res = SDL_MUSTLOCK(dataPtr->surface);
    if (res!=0) {
        res = 1;
    }
    Tcl_SetObjResult(interp,Tcl_NewIntObj(res));
    return TCL_OK;
}

static int
SurfaceGetBufferCmd(ClientData clientData, Tcl_Interp *interp, 
                  int objc, Tcl_Obj *const objv[])
{
    SurfaceData *dataPtr = clientData;    
    Uint32* pixels ;
    int color ;
    int x , y;
    Tcl_Obj * row;
    Tcl_Obj * buffer;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
    }
    
    pixels = (Uint32*)dataPtr->surface->pixels;

    buffer = Tcl_NewListObj(0,NULL);
    
    for (y = 0 ; y < dataPtr->surface->h; y++) {
        row = Tcl_NewListObj(0,NULL);
        for (x = 0 ; x < dataPtr->surface->w; x++) {
            color = pixels[y*dataPtr->surface->w+x];
            Tcl_ListObjAppendElement(interp,row,Tcl_NewIntObj(color));
        }
        Tcl_ListObjAppendElement(interp,buffer,row);
    }
    Tcl_SetObjResult(interp,buffer);
    return TCL_OK;
}

static int
SurfaceSetBufferCmd(ClientData clientData, Tcl_Interp *interp, 
                  int objc, Tcl_Obj *const objv[])
{
    SurfaceData *dataPtr = clientData;
    int (*pixelProcPtr)(Tcl_Interp *, SDL_Surface *, int, int, Tcl_Obj *) = NULL ; 
    int x , y;
    int res = TCL_OK;
    Tcl_Obj ** xObjv ;
    Tcl_Obj ** yObjv ;
    int nr_x, nr_y ;


    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "buffer");
        return TCL_ERROR;
    }
    
    if (Tcl_ListObjGetElements(interp,objv[2],&nr_y,&yObjv)!=TCL_OK) {
        Tcl_AppendResult(interp,"buffer is not a valid list",NULL);
        return TCL_ERROR;
    }

    switch (dataPtr->surface->format->BitsPerPixel) {
        case 8:
            pixelProcPtr = SetPixel8;
            break;
        default:
            Tcl_AppendResult(interp,"operation not supported for this surface format",NULL);
            return TCL_ERROR;
    }

    for (y = 0 ; y < nr_y && y < dataPtr->surface->h; y++) {
        if (Tcl_ListObjGetElements(interp,yObjv[y],&nr_x,&xObjv)!=TCL_OK) {
            Tcl_AppendResult(interp,"invalid row list",NULL);
            return TCL_ERROR;
        }
        for (x = 0 ; x < nr_x && x < dataPtr->surface->w; x++) {
            if (pixelProcPtr(interp, dataPtr->surface, x, y, xObjv[x])!=TCL_OK) {
                return TCL_ERROR;
            }
        }
    }
    return TCL_OK;
}

static int
SurfaceGetRawBufferCmd(ClientData clientData, Tcl_Interp *interp, 
                  int objc, Tcl_Obj *const objv[])
{
    SurfaceData *dataPtr = clientData;
    void * pixels ;
    int w;
    int h;
    Tcl_Obj * buffer;

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 2, objv, NULL);
        return TCL_ERROR;
    }

    pixels = dataPtr->surface->pixels;
    w = dataPtr->surface->w;
    h = dataPtr->surface->h;
    buffer = Tcl_NewByteArrayObj(pixels,h*dataPtr->surface->pitch);
    
    Tcl_SetObjResult(interp,buffer);
    return TCL_OK;
}

static int
SurfaceSetRawBufferCmd(ClientData clientData, Tcl_Interp *interp, 
                  int objc, Tcl_Obj *const objv[])
{
    SurfaceData *dataPtr = clientData;
    Uint32 * pixels ;
    char * bytes;
    int w;
    int h;
    int length;

    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "rawbuffer");
        return TCL_ERROR;
    }

    pixels = (Uint32 *)dataPtr->surface->pixels;
    w = dataPtr->surface->w;
    h = dataPtr->surface->h;
    bytes = Tcl_GetByteArrayFromObj(objv[2],&length);
    if (length < h*w) {
        Tcl_AppendResult(interp, "rawbuffer not big enough for this surface",NULL);
        return TCL_ERROR;
    }
    memcpy((char *)pixels,bytes,h*w);
    return TCL_OK;
}



/*
 * $surface blit $surface x y ?sourcerect?
 */
static int
SurfaceBlitCmd(ClientData clientData, Tcl_Interp *interp, 
                  int objc, Tcl_Obj *const objv[])
{
    SurfaceData *dataPtr = clientData;
    SurfaceData *dstPtr = NULL;
    SDL_Rect rc = {0, 0, 0, 0}, srcRect = {0, 0, 0, 0}, *srcRectPtr = NULL;
    Tcl_CmdInfo info;
    int r = TCL_OK;

    if (objc < 5 || objc > 6) {
        Tcl_WrongNumArgs(interp, 2, objv, "surface x y ?source_rect?");
        return TCL_ERROR;
    }

    if (!Tcl_GetCommandInfo(interp, Tcl_GetString(objv[2]), &info)) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("argh!", -1));
        r = TCL_ERROR;
    }
    if (TCL_OK == r && info.isNativeObjectProc != 1 
        && info.objProc == SurfaceObjCmd) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("not a surface", -1));
        return TCL_ERROR;
    }
    if (TCL_OK == r)
        r = Tcl_GetIntFromObj(interp, objv[3], (int *)&rc.x);
    if (TCL_OK == r)
        r = Tcl_GetIntFromObj(interp, objv[4], (int *)&rc.y);
    if (TCL_OK == r && objc == 6) {
        srcRectPtr = &srcRect;
        r = GetSDLRectFromObj(interp, objv[5], &srcRect);
    }
    if (TCL_OK == r) {
        dstPtr = (SurfaceData *)info.objClientData;
        if (SDL_BlitSurface(dataPtr->surface, srcRectPtr, dstPtr->surface, &rc) < 0) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj(SDL_GetError(), -1));
            r = TCL_ERROR;
        }
    }

    return r;
}

/*
 * $surface fill rgb ?{0 0 100 100}?
 */

static int
SurfaceFillCmd(ClientData clientData, Tcl_Interp *interp, 
               int objc, Tcl_Obj *const objv[])
{
    SurfaceData *dataPtr = clientData;
    SDL_Rect rect = {0, 0, 0, 0}, *rectPtr = NULL;
    Uint32 color;

    if (objc < 3 || objc > 4) {
        Tcl_WrongNumArgs(interp, 2, objv, "color ?rectangle?");
    }

    if (GetSDLColorFromObj(interp, dataPtr->surface, objv[2], &color)
        != TCL_OK) {
        return TCL_ERROR;
    }

    if (objc == 4) {
        rectPtr = &rect;
        if (GetSDLRectFromObj(interp, objv[3], rectPtr) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    
    if (SDL_FillRect(dataPtr->surface, rectPtr, color) < 0) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(SDL_GetError(), -1));

    }
    return TCL_OK;
}

/*
 * Look for a collision between two images. What we are looking
 * for is non-transparent pixels in both regions at the same point
 * 
 * $surface collide 
 */
static int
SurfaceCollisionCmd(ClientData clientData, Tcl_Interp *interp, 
                    int objc, Tcl_Obj *const objv[])
{
    /* SurfaceData *dataPtr = clientData; */

    Tcl_SetResult(interp, "e-notimpl", TCL_STATIC);
    return TCL_ERROR;
}

static int
SurfaceConfigureCmd(ClientData clientData, Tcl_Interp *interp, 
    int objc, Tcl_Obj *const objv[])
{
    SurfaceData *dataPtr = clientData;
    int opt, index, cget = 0, r = TCL_OK;
    int width = 0, height = 0, bpp = 0, flags = 0, pitch = 0;
    Tcl_Obj *resObj = NULL;
    enum { OPT_HEIGHT, OPT_WIDTH, OPT_BPP, OPT_FULLSCREEN, OPT_PITCH, OPT_RESIZE, 
	   OPT_WINDOWID };
    const char *options[] = {
	"-height", "-width", "-bpp", "-fullscreen",  "-pitch", "-resizable", 
	"-windowid", NULL
    };
    
    height = dataPtr->surface->h;
    width = dataPtr->surface->w;
    bpp = dataPtr->surface->format->BitsPerPixel;
    flags = dataPtr->surface->flags;
    pitch = dataPtr->surface->pitch;

    if (objc == 2) {
	Tcl_Obj *listObj = Tcl_NewListObj(-1, NULL);
	Tcl_ListObjAppendElement(interp, listObj,
	    Tcl_NewStringObj(options[OPT_WIDTH], -1));
	Tcl_ListObjAppendElement(interp, listObj, Tcl_NewIntObj(width));
	Tcl_ListObjAppendElement(interp, listObj,
	    Tcl_NewStringObj(options[OPT_HEIGHT], -1));
	Tcl_ListObjAppendElement(interp, listObj, Tcl_NewIntObj(height));
	Tcl_ListObjAppendElement(interp, listObj,
	    Tcl_NewStringObj(options[OPT_BPP], -1));
	Tcl_ListObjAppendElement(interp, listObj, Tcl_NewIntObj(bpp));
	Tcl_ListObjAppendElement(interp, listObj,
	    Tcl_NewStringObj(options[OPT_FULLSCREEN], -1));
	Tcl_ListObjAppendElement(interp, listObj, 
	    Tcl_NewBooleanObj(flags & SDL_FULLSCREEN));
	Tcl_ListObjAppendElement(interp, listObj,
	    Tcl_NewStringObj(options[OPT_RESIZE], -1));
	Tcl_ListObjAppendElement(interp, listObj, 
	    Tcl_NewBooleanObj(flags & SDL_RESIZABLE));
	Tcl_ListObjAppendElement(interp, listObj,
	    Tcl_NewStringObj(options[OPT_WINDOWID], -1));
	Tcl_ListObjAppendElement(interp, listObj, 
	    Tcl_NewLongObj((long)dataPtr->windowid));
	Tcl_SetObjResult(interp, listObj);
	return TCL_OK;
    }

    if (objc == 3) { 
	cget = 1; 
    } else if (objc & 1) {
	Tcl_WrongNumArgs(interp, 2, objv, 
	    "?-option value? ?-option value ...?");
	return TCL_ERROR;
    }

    for (opt = 2; opt < objc; ++opt) {
        if (Tcl_GetIndexFromObj(interp, objv[opt], options,
		"option", 0, &index) != TCL_OK) {
            return TCL_ERROR;
        }
	switch (index) {
	    case OPT_HEIGHT:
		if (cget) {
		    resObj = Tcl_NewIntObj(height);
		} else {
		    r = Tcl_GetIntFromObj(interp, objv[++opt], &height);
		}
		break;
	    case OPT_WIDTH:
		if (cget) {
		    resObj = Tcl_NewIntObj(width);
		} else {
		    r = Tcl_GetIntFromObj(interp, objv[++opt], &width);
		}
        break;
	    case OPT_PITCH:
		if (cget) {
		    resObj = Tcl_NewIntObj(pitch);
		} else {
		    r = Tcl_GetIntFromObj(interp, objv[++opt], &width);
		}
        break;
	    case OPT_BPP:
		if (cget) {
		    resObj = Tcl_NewIntObj(bpp);
		} else {
		    r = Tcl_GetIntFromObj(interp, objv[++opt], &bpp);
		}
		break;
	    case OPT_WINDOWID: {
		char sz[32];
		if (cget) {
		    sprintf(sz, "0x%08lx", dataPtr->windowid);
		    resObj = Tcl_NewStringObj(sz, 10);
		} else {
		    long id;
		    r = Tcl_GetLongFromObj(interp, objv[++opt], &id);
		    if (r == TCL_OK) {
			dataPtr->windowid = (unsigned long)id;
			sprintf(sz, "SDL_WINDOWID=0x%08lx", dataPtr->windowid);
			putenv(sz);
		    }
		}
		break;
	    }
	    case OPT_FULLSCREEN:
		if (cget) {
		    resObj = Tcl_NewBooleanObj(flags & SDL_FULLSCREEN);
		} else {
		    int b = 0;
		    r = Tcl_GetBooleanFromObj(interp, objv[++opt], &b);
		    if (b)
			flags |= SDL_FULLSCREEN;
		    else
			flags &= ~SDL_FULLSCREEN;
		}
	    case OPT_RESIZE:
		if (cget) {
		    resObj = Tcl_NewBooleanObj(flags & SDL_RESIZABLE);
		} else {
		    int b = 0;
		    r = Tcl_GetBooleanFromObj(interp, objv[++opt], &b);
		    if (b)
			flags |= SDL_RESIZABLE;
		    else
			flags &= ~SDL_RESIZABLE;
		}
	}
    }
    if (cget) {
	Tcl_SetObjResult(interp, resObj);
    } else if (r == TCL_OK) {
	SDL_Surface *surface = SDL_SetVideoMode(width, height, bpp, flags);
	if (surface) {
	    dataPtr->surface = surface;
	} else {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(SDL_GetError(), -1));
	    r = TCL_ERROR;
	}
    }
    return r;
}

struct Ensemble surfaceEnsemble[] = {
    { "getbuffer", SurfaceGetBufferCmd, NULL },
    { "getrawbuffer", SurfaceGetRawBufferCmd, NULL },
    { "setbuffer", SurfaceSetBufferCmd, NULL },
    { "setrawbuffer", SurfaceSetRawBufferCmd, NULL },
    { "delete", SurfaceDeleteCmd, NULL },
    { "flip",   SurfaceFlipCmd, NULL },
    { "blit",   SurfaceBlitCmd, NULL },
    { "pixel",   SurfacePixelCmd, NULL },
    { "fill",   SurfaceFillCmd, NULL },
    { "configure", SurfaceConfigureCmd, NULL },
    { "mustlock", SurfaceMustLockCmd, NULL },
    { "setcolors", SurfaceSetColorsCmd, NULL},
    { "setcolorkey", SurfaceSetColorKeyCmd, NULL},
    { NULL, NULL, NULL },
};

static int
SurfaceEnsemble(ClientData clientData, Tcl_Interp *interp,
                int objc, Tcl_Obj *const objv[])
{
    struct Ensemble *ensemble = surfaceEnsemble;
    int option = 1, index;

    while (option < objc) {
        if (Tcl_GetIndexFromObjStruct(interp, objv[option], 
		ensemble, sizeof(ensemble[0]), "command", 0, &index) != TCL_OK)
        {
            return TCL_ERROR;
        }

        if (ensemble[index].command) {
            return ensemble[index].command(clientData, interp, objc, objv);
        }
        ensemble = ensemble[index].ensemble;
        ++option;
    }
    Tcl_WrongNumArgs(interp, option, objv, "command ?arg arg...?");
    return TCL_ERROR;
}

static void 
SurfaceCleanup(ClientData clientData)
{
    SurfaceData *dataPtr = clientData;
    printf("cleanup - deleting surface\n");
    if (dataPtr->surface)
        SDL_FreeSurface(dataPtr->surface);
    ckfree((char *)dataPtr);
}

/*export*/ int
SurfaceObjCmd(ClientData clientData, Tcl_Interp *interp, 
                  int objc, Tcl_Obj *const objv[])
{
    SurfaceData *dataPtr;
    int width = 800, height = 600, bpp = 32;
    int flags = SDL_HWSURFACE | SDL_ANYFORMAT | SDL_DOUBLEBUF | SDL_HWPALETTE;
    int index, option = 1, r = TCL_OK;
    unsigned long windowid = 0;
    const char *bmpfile = NULL;
    static int uid = 0;
    char name[4 + TCL_INTEGER_SPACE];

    enum {SURF_WIDTH, SURF_HEIGHT, SURF_BPP, SURF_BITMAP, SURF_FULLSCREEN, 
    SURF_RESIZE, SURF_WINDOWID};
    static const char * cmds[] = {
        "-width", "-height", "-bpp", "-bitmap", "-fullscreen", "-resizable", 
        "-windowid", NULL
    };

    for (option = 1; option < objc; ++option) {
        if (Tcl_GetIndexFromObj(interp, objv[option], cmds, 
                                "option", 0, &index) != TCL_OK) {
            return TCL_ERROR;
        }
        switch (index) {
	    case SURF_WIDTH:
		if (++option >= objc) goto WrongNumArgs;
		if (Tcl_GetIntFromObj(interp, objv[option], &width) != TCL_OK)
		    return TCL_ERROR;
		break;
	    case SURF_HEIGHT:
		if (++option >= objc) goto WrongNumArgs;
		if (Tcl_GetIntFromObj(interp, objv[option], &height) != TCL_OK)
		    return TCL_ERROR;
		break;
	    case SURF_BPP:
		if (++option >= objc) goto WrongNumArgs;
		if (Tcl_GetIntFromObj(interp, objv[option], &bpp) != TCL_OK)
		    return TCL_ERROR;
		break;
	    case SURF_BITMAP:
		if (++option >= objc) goto WrongNumArgs;
		bmpfile = Tcl_GetString(objv[option]);
		break;
	    case SURF_FULLSCREEN:
		flags |= SDL_FULLSCREEN;
		break;
            case SURF_RESIZE:
                flags |= SDL_RESIZABLE;
		break;
	    case SURF_WINDOWID: {
		char sz[28];
		if (++option >= objc) goto WrongNumArgs;
		if (Tcl_GetLongFromObj(interp, objv[option], 
			(long *)&windowid) != TCL_OK)
		    return TCL_OK;
		sprintf(sz, "SDL_WINDOWID=0x%08lx", windowid);
		putenv(sz);
		break;
	    }
        }
    }
            
    if (r == TCL_OK) {
        if (bmpfile == NULL) {
            SDL_Surface *surface = NULL;
            if (SDL_GetVideoSurface()) {
                surface = SDL_CreateRGBSurface(flags, width, height, bpp,
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		    0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff
#else
		    0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000
#endif
					       );
		if (surface) {
		    surface = SDL_DisplayFormat(surface);
		}
		if (!surface) {
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(SDL_GetError(), -1));
		    return TCL_ERROR;
		}
		dataPtr = (SurfaceData *)ckalloc(sizeof(SurfaceData));
		dataPtr->surface = surface;
		dataPtr->windowid = windowid;
		sprintf(name, "sdl%u", uid++);
		dataPtr->token  = Tcl_CreateObjCommand(interp, 
		    name, SurfaceEnsemble, dataPtr, SurfaceCleanup);
		Tcl_SetObjResult(interp, Tcl_NewStringObj(name, -1));
	    } else {
		dataPtr = (SurfaceData *)ckalloc(sizeof(SurfaceData));
		dataPtr->windowid = windowid;
		dataPtr->surface = SDL_SetVideoMode(width, height, bpp, flags);
		sprintf(name, "sdl%u", uid++);
		dataPtr->token  = Tcl_CreateObjCommand(interp, 
		    name, SurfaceEnsemble, dataPtr, SurfaceCleanup);
		Tcl_SetObjResult(interp, Tcl_NewStringObj(name, -1));
	    }
        } else {
            SDL_Surface *tmp = NULL;
            dataPtr = (SurfaceData *)ckalloc(sizeof(SurfaceData));
            dataPtr->surface = NULL;
	    dataPtr->windowid = windowid;
            tmp = SDL_LoadBMP(bmpfile);
            if (tmp) {
                dataPtr->surface = SDL_DisplayFormat(tmp);
                SDL_FreeSurface(tmp);
            }
            if (tmp && dataPtr->surface) {
                sprintf(name, "sdl%u", uid++);
                dataPtr->token  = Tcl_CreateObjCommand(interp, 
                                                       name, SurfaceEnsemble,
                                                       dataPtr, SurfaceCleanup);
                Tcl_SetObjResult(interp, Tcl_NewStringObj(name, -1));
            } else {
                Tcl_SetObjResult(interp, Tcl_NewStringObj(SDL_GetError(), -1));
                r = TCL_ERROR;
            }
        }
    }
    return r;
    
 WrongNumArgs:
    Tcl_WrongNumArgs(interp, 1, objv, "?-option value? ?-option value ...?");
    return TCL_OK;
}
