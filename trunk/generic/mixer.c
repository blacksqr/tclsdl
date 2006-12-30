/*
 * sdl::mixer init -frequency N -format N -channels N -chunksize N
 * sdl::mixer volume ?-channel chan? vol
 *
 * set music [sdl::mixer load ?-type wav|ogg|etc? filename]
 * $music play ?-loop 0?
 * $music halt ?-channel chan?
 * $music pause ?-channel chan?
 * $music resume ?-channel chan?
 * $music delete
 */

#include "tclsdl.h"
#include <SDL/SDL.h>
#include <SDL/SDL_mixer.h>

typedef struct MusicData {
    Tcl_Command token;
    Mix_Music *musicPtr;
    Mix_Chunk *samplePtr;
    int channel;
} MusicData;

typedef struct MixerData {
    Tcl_Command token;
} MixerData;

struct Ensemble {
    const char *name;          /* subcommand name */
    Tcl_ObjCmdProc *command;   /* subcommand implementation OR */
    struct Ensemble *ensemble; /* subcommand ensemble */
};


static int
MusicPlayCmd(ClientData clientData, Tcl_Interp *interp, 
             int objc, Tcl_Obj *const objv[])
{
    MusicData *dataPtr = clientData;
    int channel = -1, loops = 0, opt = 2, index = 0;
    enum {mixChannel, mixLoops};
    const char *opts[] = { "-channel", "-loops", NULL };

    for (opt = 2; opt < objc; ++opt) {
        if (Tcl_GetIndexFromObj(interp, objv[opt], opts, "option", 0, &index)
            != TCL_OK) {
            return TCL_ERROR;
        }
        switch (index) {
            case mixChannel:
                ++opt;
                if (opt >= objc) {
                    Tcl_WrongNumArgs(interp, 2, objv, "");
                    return TCL_ERROR;
                }
                if (Tcl_GetIntFromObj(interp, objv[opt], &channel) != TCL_OK)
                    return TCL_ERROR;
                break;
            case mixLoops:
                ++opt;
                if (opt >= objc) {
                    Tcl_WrongNumArgs(interp, 2, objv, "");
                    return TCL_ERROR;
                }
                if (Tcl_GetIntFromObj(interp, objv[opt+1], &loops) != TCL_OK)
                    return TCL_ERROR;
                break;
        }
    }

    if (dataPtr->samplePtr) {
        dataPtr->channel = channel;
        index = Mix_PlayChannel(channel, dataPtr->samplePtr, loops);
    } else {
        index = Mix_PlayMusic(dataPtr->musicPtr, loops);
    }
    if (index < 0) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(Mix_GetError(), -1));
        return TCL_ERROR;
    }
    return TCL_OK;
}

static int
MusicHaltCmd(ClientData clientData, Tcl_Interp *interp, 
             int objc, Tcl_Obj *const objv[])
{
    MusicData *dataPtr = clientData;
    if (dataPtr->samplePtr) {
        if (dataPtr->channel != -1) {
            Mix_HaltChannel(dataPtr->channel);
        }
    } else {
        Mix_HaltMusic();
    }
    return TCL_OK;
}

static int
MusicPauseCmd(ClientData clientData, Tcl_Interp *interp, 
              int objc, Tcl_Obj *const objv[])
{
    MusicData *dataPtr = clientData;
    if (dataPtr->samplePtr) {
        if (dataPtr->channel != -1) {
            Mix_Pause(dataPtr->channel);
        }
    } else {
        Mix_PauseMusic();
    }
    return TCL_OK;
}

static int
MusicResumeCmd(ClientData clientData, Tcl_Interp *interp, 
               int objc, Tcl_Obj *const objv[])
{
    MusicData *dataPtr = clientData;
    if (dataPtr->samplePtr) {
        if (dataPtr->channel != -1) {
            Mix_Resume(dataPtr->channel);
        }
    } else {
        Mix_ResumeMusic();
    }
    return TCL_OK;
}

static int
MusicPlayingCmd(ClientData clientData, Tcl_Interp *interp, 
                int objc, Tcl_Obj *const objv[])
{
    MusicData *dataPtr = clientData;
    int r = 0;
    if (dataPtr->samplePtr) {
        if (dataPtr->channel != -1) {
            r = Mix_Playing(dataPtr->channel);
        }
    } else {
        r = Mix_PlayingMusic();
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(r));
    return TCL_OK;
}

static int
MusicPausedCmd(ClientData clientData, Tcl_Interp *interp, 
               int objc, Tcl_Obj *const objv[])
{
    MusicData *dataPtr = clientData;
    int r = 0;
    if (dataPtr->samplePtr) {
        if (dataPtr->channel != -1) {
            r = Mix_Paused(dataPtr->channel);
        }
    } else {
        r = Mix_PausedMusic();
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(r));
    return TCL_OK;
}

static int
MusicVolumeCmd(ClientData clientData, Tcl_Interp *interp, 
               int objc, Tcl_Obj *const objv[])
{
    MusicData *dataPtr = clientData;
    int volume = -1;
    if (objc < 2 || objc > 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "?level?");
        return TCL_ERROR;
    }
    if (objc == 3) {
        if (Tcl_GetIntFromObj(interp, objv[2], &volume) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (dataPtr->samplePtr) {
        volume = Mix_VolumeChunk(dataPtr->samplePtr, volume);
    } else {
        volume = Mix_VolumeMusic(volume);
    }
    Tcl_SetObjResult(interp, Tcl_NewIntObj(volume));
    return TCL_OK;
}

static int
MusicDeleteCmd(ClientData clientData, Tcl_Interp *interp, 
               int objc, Tcl_Obj *const objv[])
{
    /* MusicData *dataPtr = clientData; */

    Tcl_SetResult(interp, "e-notimpl", TCL_STATIC);
    return TCL_ERROR;
}

struct Ensemble musicEnsemble[] = {
    { "play", MusicPlayCmd, NULL },
    { "halt", MusicHaltCmd, NULL },
    { "pause", MusicPauseCmd, NULL },
    { "resume", MusicResumeCmd, NULL },
    { "playing", MusicPlayingCmd, NULL },
    { "paused", MusicPausedCmd, NULL },
    { "volume", MusicVolumeCmd, NULL },
    { "delete", MusicDeleteCmd, NULL },
    { NULL, NULL, NULL },
};

static int
MusicEnsemble(ClientData clientData, Tcl_Interp *interp,
              int objc, Tcl_Obj *const objv[])
{
    struct Ensemble *ensemble = musicEnsemble;
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
MusicCleanup(ClientData clientData)
{
    MusicData *dataPtr = clientData;
    if (dataPtr->musicPtr)
        Mix_FreeMusic(dataPtr->musicPtr);
    if (dataPtr->samplePtr)
        Mix_FreeChunk(dataPtr->samplePtr);
    ckfree((char *)dataPtr);
}

static int
MixerLoadCmd(ClientData clientData, Tcl_Interp *interp, 
            int objc, Tcl_Obj *const objv[])
{
    MusicData *dataPtr = NULL;
    Mix_Music *music = NULL;
    Mix_Chunk *sample = NULL;
    enum {mixSample, mixMusic};
    const char *types[] = {"sample", "music", NULL};
    int type = mixSample;
    char name[7 + TCL_INTEGER_SPACE];
    static int uid = 0;

    if (objc < 2 || objc > 5 || objc == 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "?-type type? filename");
        return TCL_ERROR;
    }

    if (objc == 5) {
        if (Tcl_GetIndexFromObj(interp, objv[3], types, "type", 0, &type)
            != TCL_OK) {
            return TCL_ERROR;
        }
    }
    if (type == mixSample) {
        sample = Mix_LoadWAV(Tcl_GetString(objv[objc-1]));
    } else {
        music = Mix_LoadMUS(Tcl_GetString(objv[objc-1]));
    }
    if (music == NULL && sample == NULL) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(Mix_GetError(), -1));
        return TCL_ERROR;
    }

    dataPtr = (MusicData *)ckalloc(sizeof(MusicData));
    dataPtr->musicPtr = music;
    dataPtr->samplePtr = sample;
    dataPtr->channel = -1;
    sprintf(name, "sdlmix%u", uid++);
    dataPtr->token  = Tcl_CreateObjCommand(interp, name, MusicEnsemble,
                                           dataPtr, MusicCleanup);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(name, -1));
    return TCL_OK;
}

static void
InterpDeleteProc(ClientData clientData, Tcl_Interp *interp)
{
    Mix_CloseAudio();
}

static int
MixerInitCmd(ClientData clientData, Tcl_Interp *interp, 
            int objc, Tcl_Obj *const objv[])
{
    Uint16 format = AUDIO_S8;// MIX_DEFAULT_FORMAT; //AUDIO_S16; /* 16bit stereo */
    int freq = 11025;// 44100; //22050;
    int channels = 2;
    int chunksize = 4096;
    int option = 2, index;
    enum {OPT_FREQ, OPT_CHANNELS, OPT_CHUNK, OPT_FORMAT};
    const char *opts[] = {"-frequency", "-channels", "-chunk", 
                          "-format", NULL};

    if (SDL_WasInit(0) && SDL_INIT_AUDIO == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj(SDL_GetError(), -1));
            return TCL_ERROR;
        } else {
            Tcl_CallWhenDeleted(interp, InterpDeleteProc, NULL);
        }
    }

    /* process options */
    for (option = 2; option < objc; ++option) {
        if (Tcl_GetIndexFromObj(interp, objv[option], opts,
                                "option", 0, &index) != TCL_OK) {
            return TCL_ERROR;
        }
        switch (index) {
            case OPT_FREQ:
                ++option;
                if (option >= objc) {
                    Tcl_WrongNumArgs(interp, 2, objv, "-frequency Hz");
                    return TCL_ERROR;
                }
                if (Tcl_GetIntFromObj(interp, objv[option], &freq) != TCL_OK)
                    return TCL_OK;
                break;
            case OPT_CHANNELS:
                ++option;
                if (option >= objc) {
                    Tcl_WrongNumArgs(interp, 2, objv, "-channels num");
                    return TCL_ERROR;
                }
                if (Tcl_GetIntFromObj(interp, objv[option], &channels) != TCL_OK)
                    return TCL_OK;
                break;
        }
    }

    if (Mix_OpenAudio(freq, format, channels, chunksize) < 0) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(Mix_GetError(), -1));
        return TCL_ERROR;
    }
    
    return TCL_OK;
}

static int
MixerVolumeCmd(ClientData clientData, Tcl_Interp *interp, 
               int objc, Tcl_Obj *const objv[])
{
    int volume = -1, oldvolume = -1;
    if (objc < 2 || objc > 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "level");
        return TCL_ERROR;
    }
    if (objc == 3) {
        if (Tcl_GetIntFromObj(interp, objv[2], &volume) != TCL_OK) {
            return TCL_ERROR;
        }
    }
    oldvolume = Mix_VolumeMusic(volume);
    if (volume == -1) {
        Tcl_SetObjResult(interp, Tcl_NewIntObj(oldvolume));
    }
    return TCL_OK;
}

struct Ensemble mixerEnsemble[] = {
    { "init", MixerInitCmd, NULL },
    { "load", MixerLoadCmd, NULL },
    { "volume", MixerVolumeCmd, NULL },
    { NULL, NULL, NULL },
};

/*export*/ int
MixerObjCmd(ClientData clientData, Tcl_Interp *interp,
            int objc, Tcl_Obj *const objv[])
{
    struct Ensemble *ensemble = mixerEnsemble;
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

