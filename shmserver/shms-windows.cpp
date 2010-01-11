/*
www.sourceforge.net/projects/dfhack
Copyright (c) 2009 Petr Mrázek (peterix)

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any
damages arising from the use of this software.

Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.

2. Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source
distribution.
*/

/**
 * This is the source for the DF <-> dfhack shm bridge,
 * to be used with SDL 1.2 and DF 40d16. Windows sucks
 * using hacks like this sucks even more
 */

#define _WIN32_WINNT 0x0501 // needed for INPUT struct
#define WINVER 0x0501                   // OpenThread(), PSAPI, Toolhelp32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdarg.h>

#include "../library/integers.h"
#include "shms.h"
#include <stdio.h>
int errorstate = 0;
char *shm = 0;
int shmid = 0;
bool inited = 0;
HANDLE shmHandle = 0;
HANDLE DFSVMutex = 0;
HANDLE DFCLMutex = 0;
void SHM_Init ( void )
{
    // check that we do this only once per process
    if(inited)
    {
        MessageBox(0,"SDL_Init was called twice or more!","FUN", MB_OK);
        return;
    }
    inited = true;
    
    // create or open mutexes
    DFSVMutex = CreateMutex( 0, 1, "DFSVMutex");
    if(DFSVMutex == 0)
    {
        DFSVMutex = OpenMutex(SYNCHRONIZE,false, "DFSVMutex");
        if(DFSVMutex == 0)
        {
            errorstate = 1;
            return;
        }
    }
    DFCLMutex = CreateMutex( 0, 0, "DFCLMutex");
    if(DFCLMutex == 0)
    {
        DFCLMutex = OpenMutex(SYNCHRONIZE,false, "DFCLMutex");
        if(DFCLMutex == 0)
        {
            CloseHandle(DFSVMutex);
            errorstate = 1;
            return;
        }
    }
    
    // try locking server mutex
    uint32_t result;
    result = WaitForSingleObject(DFSVMutex,0);
    switch (result)
    {
        case WAIT_ABANDONED:
        {
            // picked up after a crashed DF process
            // do some sanity checks on client attached
            // otherwise the same thing as WAIT_OBJECT_0
            break;
        }
        case WAIT_OBJECT_0:
        {
            // all right, we have the mutex and are the one and only DF server
            break;
        }
        case WAIT_TIMEOUT:
        case WAIT_FAILED:
        default:
        {
            // error, bail
            errorstate = 1;
            MessageBox(0,"Could not aquire mutex","FUN", MB_OK);
            CloseHandle(DFSVMutex);
            CloseHandle(DFCLMutex);
            return;
        }
    }
    
    // create virtual memory mapping
    shmHandle = CreateFileMapping(INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,SHM_SIZE,"DFShm");
    // if can't create or already exists -> nothing happens
    if(!shmHandle || GetLastError() == ERROR_ALREADY_EXISTS)
    {
        MessageBox(0,"Couldn't create SHM mapping","FUN", MB_OK);
        errorstate = 1;
        ReleaseMutex(DFSVMutex);
        CloseHandle(DFSVMutex);
        CloseHandle(DFCLMutex);
        return;
    }
    // attempt to attach the created mapping
    shm = (char *) MapViewOfFile(shmHandle,FILE_MAP_ALL_ACCESS, 0,0, SHM_SIZE);
    if(shm)
    {
        ((shm_cmd *)shm)->pingpong = DFPP_RUNNING;
        MessageBox(0,"Sucessfully mapped SHM","FUN", MB_OK);
    }
    else
    {
        MessageBox(0,"Couldn't attach SHM mapping","FUN", MB_OK);
        errorstate = 1;
        ReleaseMutex(DFSVMutex);
        CloseHandle(DFSVMutex);
        CloseHandle(DFCLMutex);
    }
}

void SHM_Destroy ( void )
{
    if(errorstate)
        return;
    ReleaseMutex(DFSVMutex);
    CloseHandle(DFSVMutex);
    CloseHandle(DFCLMutex);
}

uint32_t getPID()
{
    return GetCurrentProcessId();
}

// is the other side still there?
bool isValidSHM()
{
    // try if CL mutex is free
    uint32_t result = WaitForSingleObject(DFCLMutex,0);
    
    switch (result)
    {
        case WAIT_ABANDONED:
        case WAIT_OBJECT_0:
        {
            ReleaseMutex(DFCLMutex);
            return false;
        }
        case WAIT_TIMEOUT:
        {
            // mutex is held by a process already
            return true;
        }   
        default:
        case WAIT_FAILED:
        {
            // TODO: now how do I respond to this?
            return false;
        }
    }
}

/*************************************************************************/
// boring wrappers beyond this point. Only fix when broken

// function and variable pointer... we don't try to understand what SDL does here
typedef void * fPtr;
typedef void * vPtr;

/// wrappers for SDL 1.2 functions used in 40d16
/***** Condition variables

SDL_CreateCond
    SDL_cond * SDLCALL SDL_CreateCond(void);
SDL_CondSignal
    int SDLCALL SDL_CondSignal(SDL_cond *cond);
SDL_CondWait
    int SDLCALL SDL_CondWait(SDL_cond *cond, SDL_mutex *mut);
SDL_DestroyCond
    void SDLCALL SDL_DestroyCond(SDL_cond *cond);
*/
static vPtr (*_SDL_CreateCond)() = 0;
extern "C" vPtr SDL_CreateCond()
{
    return _SDL_CreateCond();
}

static int (*_SDL_CondSignal)(vPtr cond) = 0;
extern "C" int SDL_CondSignal(vPtr cond)
{
    return _SDL_CondSignal(cond);
}

static int (*_SDL_CondWait)(vPtr cond, vPtr mutex) = 0;
extern "C" int SDL_CondWait(vPtr cond, vPtr mutex)
{
    return _SDL_CondWait(cond, mutex);
}

static void (*_SDL_DestroyCond)(vPtr cond) = 0;
extern "C" void SDL_DestroyCond(vPtr cond)
{
    _SDL_DestroyCond(cond);
}

/***** mutexes

SDL_CreateMutex
    SDL_mutex * SDLCALL SDL_CreateMutex(void);
SDL_mutexP
    int SDLCALL SDL_mutexP(SDL_mutex *mutex);
SDL_DestroyMutex
    void SDLCALL SDL_DestroyMutex(SDL_mutex *mutex);
*/
static vPtr (*_SDL_CreateMutex)(void) = 0;
extern "C" vPtr SDL_CreateMutex(void)
{
    return _SDL_CreateMutex();
}

static int (*_SDL_mutexP)(vPtr mutex) = 0;
extern "C" int SDL_mutexP(vPtr mutex)
{
    return _SDL_mutexP(mutex);
}

static void (*_SDL_DestroyMutex)(vPtr mutex) = 0;
extern "C" void SDL_DestroyMutex(vPtr mutex)
{
    _SDL_DestroyMutex(mutex);
}


/***** timers

SDL_AddTimer
    SDL_TimerID SDLCALL SDL_AddTimer(Uint32 interval, SDL_NewTimerCallback callback, void *param);
SDL_RemoveTimer
    SDL_bool SDLCALL SDL_RemoveTimer(SDL_TimerID t);
SDL_GetTicks
    Uint32 SDLCALL SDL_GetTicks(void);
*/
static vPtr (*_SDL_AddTimer)(uint32_t interval, fPtr callback, vPtr param) = 0;
extern "C" vPtr SDL_AddTimer(uint32_t interval, fPtr callback, vPtr param)
{
    return _SDL_AddTimer(interval, callback, param);
}

static bool (*_SDL_RemoveTimer)(vPtr timer) = 0;
extern "C" bool SDL_RemoveTimer(vPtr timer)
{
    return _SDL_RemoveTimer(timer);
}

static uint32_t (*_SDL_GetTicks)(void) = 0;
extern "C" uint32_t SDL_GetTicks(void)
{
    return _SDL_GetTicks();
}

/***** Surfaces
SDL_CreateRGBSurface
    SDL_Surface * SDLCALL SDL_CreateRGBSurface
        (Uint32 flags, int width, int height, int depth, 
        Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);

SDL_CreateRGBSurfaceFrom
    SDL_Surface * SDLCALL SDL_CreateRGBSurfaceFrom
        (void *pixels, int width, int height, int depth, int pitch,
        Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);

SDL_FreeSurface
    void SDLCALL SDL_FreeSurface(SDL_Surface *surface);

SDL_ConvertSurface
    SDL_Surface * SDLCALL SDL_ConvertSurface
        (SDL_Surface *src, SDL_PixelFormat *fmt, Uint32 flags);

SDL_LockSurface
    int SDLCALL SDL_LockSurface(SDL_Surface *surface);

SDL_UnlockSurface
    void SDLCALL SDL_UnlockSurface(SDL_Surface *surface);
*/

static vPtr (*_SDL_CreateRGBSurface)(uint32_t flags, int width, int height, int depth,
                                     uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) = 0;
extern "C" vPtr SDL_CreateRGBSurface(uint32_t flags, int width, int height, int depth,
                                     uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask)
{
    return _SDL_CreateRGBSurface(flags, width, height, depth, Rmask, Gmask, Bmask, Amask);
}

static vPtr (*_SDL_CreateRGBSurfaceFrom)(vPtr pixels, int width, int height, int depth, int pitch,
                                         uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask) = 0;
extern "C" vPtr SDL_CreateRGBSurfaceFrom(vPtr pixels, int width, int height, int depth, int pitch,
                                         uint32_t Rmask, uint32_t Gmask, uint32_t Bmask, uint32_t Amask)
{
    return _SDL_CreateRGBSurfaceFrom(pixels, width, height, depth, pitch, Rmask, Gmask, Bmask, Amask);
}

static void (*_SDL_FreeSurface)(vPtr surface) = 0;
extern "C" void SDL_FreeSurface(vPtr surface)
{
    _SDL_FreeSurface(surface);
}

static vPtr (*_SDL_ConvertSurface)(vPtr surface, vPtr format, uint32_t flags) = 0;
extern "C" vPtr SDL_ConvertSurface(vPtr surface, vPtr format, uint32_t flags)
{
    return _SDL_ConvertSurface(surface, format, flags);
}

static int (*_SDL_LockSurface)(vPtr surface) = 0;
extern "C" int SDL_LockSurface(vPtr surface)
{
    return _SDL_LockSurface(surface);
}

static void (*_SDL_UnlockSurface)(vPtr surface) = 0;
extern "C" void SDL_UnlockSurface(vPtr surface)
{
    _SDL_UnlockSurface(surface);
}

/***** More surface stuff
SDL_MapRGB
     Uint32 SDLCALL SDL_MapRGB
        (const SDL_PixelFormat * const format, const Uint8 r, const Uint8 g, const Uint8 b);

SDL_SaveBMP_RW
    int SDLCALL SDL_SaveBMP_RW
        (SDL_Surface *surface, SDL_RWops *dst, int freedst);

SDL_SetAlpha
    int SDLCALL SDL_SetAlpha(SDL_Surface *surface, Uint32 flag, Uint8 alpha);
    
SDL_SetColorKey
    int SDLCALL SDL_SetColorKey(SDL_Surface *surface, Uint32 flag, Uint32 key);

SDL_GetVideoInfo
    const SDL_VideoInfo * SDLCALL SDL_GetVideoInfo(void);

SDL_SetVideoMode
    SDL_Surface * SDLCALL SDL_SetVideoMode
        (int width, int height, int bpp, Uint32 flags);

SDL_UpperBlit
    int SDLCALL SDL_UpperBlit
        (SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
*/

static uint32_t (*_SDL_MapRGB)(vPtr pixelformat, uint8_t r, uint8_t g, uint8_t b) = 0;
extern "C" uint32_t SDL_MapRGB(vPtr pixelformat, uint8_t r, uint8_t g, uint8_t b)
{
    return _SDL_MapRGB(pixelformat,r,g,b);
}

static int (*_SDL_SaveBMP_RW)(vPtr surface, vPtr dst, int freedst) = 0;
extern "C" int SDL_SaveBMP_RW(vPtr surface, vPtr dst, int freedst)
{
    return _SDL_SaveBMP_RW(surface,dst,freedst);
}

static int (*_SDL_SetAlpha)(vPtr surface, uint32_t flag, uint8_t alpha) = 0;
extern "C" int SDL_SetAlpha(vPtr surface, uint32_t flag, uint8_t alpha)
{
    return _SDL_SetAlpha(surface,flag,alpha);
}

static int (*_SDL_SetColorKey)(vPtr surface, uint32_t flag, uint32_t key) = 0;
extern "C" int SDL_SetColorKey(vPtr surface, uint32_t flag, uint32_t key)
{
    return _SDL_SetColorKey(surface,flag,key);
}

static vPtr (*_SDL_GetVideoInfo)(void) = 0;
extern "C" vPtr SDL_GetVideoInfo(void)
{
    return _SDL_GetVideoInfo();
}

static vPtr (*_SDL_SetVideoMode)(int width, int height, int bpp, uint32_t flags) = 0;
extern "C" vPtr SDL_SetVideoMode(int width, int height, int bpp, uint32_t flags)
{
    return _SDL_SetVideoMode(width, height, bpp, flags);
}
static int (*_SDL_UpperBlit)(vPtr src, vPtr srcrect, vPtr dst, vPtr dstrect) = 0;
extern "C" int SDL_UpperBlit(vPtr src, vPtr srcrect, vPtr dst, vPtr dstrect)
{
    return _SDL_UpperBlit(src, srcrect, dst, dstrect);
}

/***** Even more surface
SDL_GL_GetAttribute
    int SDLCALL SDL_GL_GetAttribute(SDL_GLattr attr, int* value);
    
SDL_GL_SetAttribute
    int SDLCALL SDL_GL_SetAttribute(SDL_GLattr attr, int value);
    
SDL_WM_SetCaption
    void SDLCALL SDL_WM_SetCaption(const char *title, const char *icon);
    
SDL_WM_SetIcon
    void SDLCALL SDL_WM_SetIcon(SDL_Surface *icon, Uint8 *mask);
    
SDL_FillRect
    int SDLCALL SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, Uint32 color);
*/

static int (*_SDL_GL_GetAttribute)(int attr, int * value) = 0;
extern "C" int SDL_GL_GetAttribute(int attr, int * value)
{
    return _SDL_GL_GetAttribute(attr,value);
}

static int (*_SDL_GL_SetAttribute)(int attr, int value) = 0;
extern "C" int SDL_GL_SetAttribute(int attr, int value)
{
    return _SDL_GL_SetAttribute(attr,value);
}

static void (*_SDL_WM_SetCaption)(const char *title, const char *icon) = 0;
extern "C" void SDL_WM_SetCaption(const char *title, const char *icon)
{
    _SDL_WM_SetCaption("DwarfHacked the Fortress of Hacks",icon);
}

static void (*_SDL_WM_SetIcon)(vPtr icon, uint8_t *mask) = 0;
extern "C" void SDL_WM_SetIcon(vPtr icon, uint8_t *mask)
{
    _SDL_WM_SetIcon(icon, mask);
}

static int (*_SDL_FillRect)(vPtr dst, vPtr dstrect, uint32_t color) = 0;
extern "C" int SDL_FillRect(vPtr dst, vPtr dstrect, uint32_t color)
{
    return _SDL_FillRect(dst,dstrect,color);
}

/***** Events and input
SDL_EnableKeyRepeat
    int SDLCALL SDL_EnableKeyRepeat(int delay, int interval);
SDL_EnableUNICODE
    int SDLCALL SDL_EnableUNICODE(int enable);
SDL_GetKeyState
    Uint8 * SDLCALL SDL_GetKeyState(int *numkeys);
SDL_PollEvent
    int SDLCALL SDL_PollEvent(SDL_Event *event);
*/

static int (*_SDL_EnableKeyRepeat)(int delay, int interval) = 0;
extern "C" int SDL_EnableKeyRepeat(int delay, int interval)
{
    return _SDL_EnableKeyRepeat(delay, interval);
}

static int (*_SDL_EnableUNICODE)(int enable) = 0;
extern "C" int SDL_EnableUNICODE(int enable)
{
    return _SDL_EnableUNICODE(enable);
}

static uint8_t * (*_SDL_GetKeyState)(int* numkeys) = 0;
extern "C" uint8_t * SDL_GetKeyState(int* numkeys)
{
    return _SDL_GetKeyState(numkeys);
}

static int (*_SDL_PollEvent)(vPtr event) = 0;
extern "C" int SDL_PollEvent(vPtr event)
{
    return _SDL_PollEvent(event);
}

/***** error handling
SDL_GetError
    char * SDLCALL SDL_GetError(void);
SDL_SetError
    extern DECLSPEC void SDLCALL SDL_SetError(const char *fmt, ...);
SDL_ClearError
    extern DECLSPEC void SDLCALL SDL_ClearError(void);
SDL_Error
    extern DECLSPEC void SDLCALL SDL_Error(SDL_errorcode code);
*/

static char * (*_SDL_GetError)(void) = 0;
extern "C" char * SDL_GetError(void)
{
    return _SDL_GetError();
}

static void (*_SDL_SetError)(const char *fmt, ...) = 0;
extern "C" void SDL_SetError(const char *fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args,fmt);
    vsnprintf(buf, sizeof(buf) - 1 ,fmt,args);
    va_end(args);
    _SDL_SetError(buf);
}

static void (*_SDL_ClearError)(void) = 0;
extern "C" void SDL_ClearError(void)
{
    _SDL_ClearError();
}

static void (*_SDL_Error)(int code) = 0;
extern "C" void SDL_Error(int code)
{
    _SDL_Error(code);
}

/***** symbol resolution
SDL_LoadFunction
    extern DECLSPEC void * SDLCALL SDL_LoadFunction(void *handle, const char *name);
SDL_LoadObject
    extern DECLSPEC void * SDLCALL SDL_LoadObject(const char *sofile);
SDL_UnloadObject
    extern DECLSPEC void SDLCALL SDL_UnloadObject(void *handle);
*/

static vPtr (*_SDL_LoadFunction)(void *handle, const char *name) = 0;
extern "C" vPtr SDL_LoadFunction(void *handle, const char *name)
{
    return _SDL_LoadFunction(handle, name);
}

static vPtr (*_SDL_LoadObject)(const char *sofile) = 0;
extern "C" vPtr SDL_LoadObject(const char *sofile)
{
    return _SDL_LoadObject(sofile);
}

static void (*_SDL_UnloadObject)(vPtr handle) = 0;
extern "C" void SDL_UnloadObject(vPtr handle)
{
    _SDL_UnloadObject(handle);
}

/***** r/w
SDL_ReadBE32
    extern DECLSPEC Uint32 SDLCALL SDL_ReadBE32(SDL_RWops *src);
SDL_ReadLE16
    extern DECLSPEC Uint16 SDLCALL SDL_ReadLE16(SDL_RWops *src);
SDL_ReadLE32
    extern DECLSPEC Uint32 SDLCALL SDL_ReadLE32(SDL_RWops *src);
*/

static uint32_t (*_SDL_ReadBE32)(vPtr src) = 0;
extern "C" uint32_t SDL_ReadBE32(vPtr src)
{
    return _SDL_ReadBE32(src);
}

static uint16_t (*_SDL_ReadLE16)(vPtr src) = 0;
extern "C" uint16_t SDL_ReadLE16(vPtr src)
{
    return _SDL_ReadLE16(src);
}

static uint32_t (*_SDL_ReadLE32)(vPtr src) = 0;
extern "C" uint32_t SDL_ReadLE32(vPtr src)
{
    return _SDL_ReadLE32(src);
}

/***** Misc
SDL_RWFromFile
    SDL_RWops * SDLCALL SDL_RWFromFile(const char *file, const char *mode);
SDL_SetModuleHandle
    void SDLCALL SDL_SetModuleHandle(void *hInst);
SDL_ShowCursor
    int SDLCALL SDL_ShowCursor(int toggle);
SDL_strlcpy
    size_t SDLCALL SDL_strlcpy(char *dst, const char *src, size_t maxlen);
*/

static vPtr (*_SDL_RWFromFile)(const char* file, const char *mode) = 0;
extern "C" vPtr SDL_RWFromFile(const char* file, const char *mode)
{
    return _SDL_RWFromFile(file, mode);
}

static void (*_SDL_SetModuleHandle)(vPtr hInst) = 0;
extern "C" void SDL_SetModuleHandle(vPtr hInst)
{
    _SDL_SetModuleHandle(hInst);
}

static int (*_SDL_ShowCursor)(int toggle) = 0;
extern "C" int SDL_ShowCursor(int toggle)
{
    return _SDL_ShowCursor(toggle);
}

static size_t (*_SDL_strlcpy)(char *dst, const char *src, size_t maxlen) = 0;
extern "C" size_t SDL_strlcpy(char *dst, const char *src, size_t maxlen)
{
    if(!_SDL_strlcpy)
    {
        HMODULE realSDLlib =  LoadLibrary("SDLreal.dll");
        _SDL_strlcpy = (size_t (*)(char*, const char*, size_t))GetProcAddress(realSDLlib,"SDL_strlcpy");
    }
    return _SDL_strlcpy(dst,src,maxlen);
}

/***** The real meat of this
SDL_Init
SDL_Quit
SDL_GL_SwapBuffers
    void SDLCALL SDL_GL_SwapBuffers(void);
*/

static void (*_SDL_Quit)(void) = 0;
extern "C" void SDL_Quit(void)
{
    fprintf(stderr,"Quitting!\n");
    SHM_Destroy();
    _SDL_Quit();
}

static void (*_SDL_GL_SwapBuffers)(void) = 0;
extern "C" void SDL_GL_SwapBuffers(void)
{
    if(!errorstate && ((shm_cmd *)shm)->pingpong != DFPP_RUNNING)
    {
        SHM_Act();
    }
    _SDL_GL_SwapBuffers();
}

static int (*_SDL_Init)(uint32_t flags) = 0;
extern "C" int SDL_Init(uint32_t flags)
{
    char zlo[2560];
    //backtrace (&zlo, 2559);
    sprintf(zlo, "SDL_Init called from thread %d", GetCurrentThreadId());
    MessageBox(0,zlo,"FUN", MB_OK);
    
    HMODULE realSDLlib =  LoadLibrary("SDLreal.dll");
    if(!realSDLlib)
    {
        fprintf(stderr, "Can't load SDLreal.dll\n");
        return -1;
    }
    // stuff for DF
    _SDL_AddTimer = (void*(*)(uint32_t, void*, void*)) GetProcAddress(realSDLlib,"SDL_AddTimer");
    _SDL_CondSignal = (int (*)(void*))GetProcAddress(realSDLlib,"SDL_CondSignal");
    _SDL_CondWait = (int (*)(void*, void*))GetProcAddress(realSDLlib,"SDL_CondWait");
    _SDL_ConvertSurface = (void*(*)(void*, void*, uint32_t))GetProcAddress(realSDLlib,"SDL_ConvertSurface");
    _SDL_CreateCond = (void*(*)())GetProcAddress(realSDLlib,"SDL_CreateCond");
    _SDL_CreateMutex = (void*(*)())GetProcAddress(realSDLlib,"SDL_CreateMutex");
    _SDL_CreateRGBSurface = (void*(*)(uint32_t, int, int, int, uint32_t, uint32_t, uint32_t, uint32_t))GetProcAddress(realSDLlib,"SDL_CreateRGBSurface");
    _SDL_CreateRGBSurfaceFrom = (void*(*)(void*, int, int, int, int, uint32_t, uint32_t, uint32_t, uint32_t))GetProcAddress(realSDLlib,"SDL_CreateRGBSurfaceFrom");
    _SDL_DestroyCond = (void (*)(void*))GetProcAddress(realSDLlib,"SDL_DestroyCond");
    _SDL_DestroyMutex = (void (*)(void*))GetProcAddress(realSDLlib,"SDL_DestroyMutex");
    _SDL_EnableKeyRepeat = (int (*)(int, int))GetProcAddress(realSDLlib,"SDL_EnableKeyRepeat");
    _SDL_EnableUNICODE = (int (*)(int))GetProcAddress(realSDLlib,"SDL_EnableUNICODE");
    _SDL_FreeSurface = (void (*)(void*))GetProcAddress(realSDLlib,"SDL_FreeSurface");
    _SDL_GL_GetAttribute = (int (*)(int, int*))GetProcAddress(realSDLlib,"SDL_GL_GetAttribute");
    _SDL_GL_SetAttribute = (int (*)(int, int))GetProcAddress(realSDLlib,"SDL_GL_SetAttribute");
    _SDL_GL_SwapBuffers = (void (*)())GetProcAddress(realSDLlib,"SDL_GL_SwapBuffers");
    _SDL_GetError = (char*(*)())GetProcAddress(realSDLlib,"SDL_GetError");
    _SDL_GetKeyState = (uint8_t*(*)(int*))GetProcAddress(realSDLlib,"SDL_GetKeyState");
    _SDL_GetTicks = (uint32_t (*)())GetProcAddress(realSDLlib,"SDL_GetTicks");
    _SDL_GetVideoInfo = (void*(*)())GetProcAddress(realSDLlib,"SDL_GetVideoInfo");
    _SDL_Init = (int (*)(uint32_t))GetProcAddress(realSDLlib,"SDL_Init");
    _SDL_LockSurface = (int (*)(void*))GetProcAddress(realSDLlib,"SDL_LockSurface");
    _SDL_MapRGB = (uint32_t (*)(void*, uint8_t, uint8_t, uint8_t))GetProcAddress(realSDLlib,"SDL_MapRGB");
    _SDL_PollEvent = (int (*)(void*))GetProcAddress(realSDLlib,"SDL_PollEvent");
    _SDL_Quit = (void (*)())GetProcAddress(realSDLlib,"SDL_Quit");
    _SDL_RWFromFile = (void*(*)(const char*, const char*))GetProcAddress(realSDLlib,"SDL_RWFromFile");
    _SDL_RemoveTimer = (bool (*)(void*))GetProcAddress(realSDLlib,"SDL_RemoveTimer");
    _SDL_SaveBMP_RW = (int (*)(void*, void*, int))GetProcAddress(realSDLlib,"SDL_SaveBMP_RW");
    _SDL_SetAlpha = (int (*)(void*, uint32_t, uint8_t))GetProcAddress(realSDLlib,"SDL_SetAlpha");
    _SDL_SetColorKey = (int (*)(void*, uint32_t, uint32_t))GetProcAddress(realSDLlib,"SDL_SetColorKey");
    _SDL_SetModuleHandle = (void (*)(void*))GetProcAddress(realSDLlib,"SDL_SetModuleHandle");
    _SDL_SetVideoMode = (void*(*)(int, int, int, uint32_t))GetProcAddress(realSDLlib,"SDL_SetVideoMode");
    _SDL_ShowCursor = (int (*)(int))GetProcAddress(realSDLlib,"SDL_ShowCursor");
    _SDL_UnlockSurface = (void (*)(void*))GetProcAddress(realSDLlib,"SDL_UnlockSurface");
    _SDL_UpperBlit = (int (*)(void*, void*, void*, void*))GetProcAddress(realSDLlib,"SDL_UpperBlit");
    _SDL_WM_SetCaption = (void (*)(const char*, const char*))GetProcAddress(realSDLlib,"SDL_WM_SetCaption");
    _SDL_WM_SetIcon = (void (*)(void*, uint8_t*))GetProcAddress(realSDLlib,"SDL_WM_SetIcon");
    _SDL_mutexP = (int (*)(void*))GetProcAddress(realSDLlib,"SDL_mutexP");
    _SDL_strlcpy = (size_t (*)(char*, const char*, size_t))GetProcAddress(realSDLlib,"SDL_strlcpy");
    
    // stuff for SDL_Image
    _SDL_ClearError = (void (*)())GetProcAddress(realSDLlib,"SDL_ClearError");
    _SDL_Error = (void (*)(int))GetProcAddress(realSDLlib,"SDL_Error");
    _SDL_LoadFunction = (void*(*)(void*, const char*))GetProcAddress(realSDLlib,"SDL_LoadFunction");
    _SDL_LoadObject = (void*(*)(const char*))GetProcAddress(realSDLlib,"SDL_LoadObject");
    _SDL_ReadBE32 = (uint32_t (*)(void*))GetProcAddress(realSDLlib,"SDL_ReadBE32");
    _SDL_ReadLE16 = (uint16_t (*)(void*))GetProcAddress(realSDLlib,"SDL_ReadLE16");
    _SDL_ReadLE32 = (uint32_t (*)(void*))GetProcAddress(realSDLlib,"SDL_ReadLE32");
    _SDL_SetError = (void (*)(const char*, ...))GetProcAddress(realSDLlib,"SDL_SetError");
    _SDL_UnloadObject = (void (*)(void*))GetProcAddress(realSDLlib,"SDL_UnloadObject");
    _SDL_FillRect = (int (*)(void*,void*,uint32_t))GetProcAddress(realSDLlib,"SDL_FillRect");
    
    fprintf(stderr,"Initized HOOKS!\n");
    SHM_Init();
    return _SDL_Init(flags);
}
