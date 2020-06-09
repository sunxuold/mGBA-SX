/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef SDL_MAIN_H
#define SDL_MAIN_H

#include <mgba-util/common.h>

CXX_GUARD_START

#include "sdl-audio.h"
#include "sdl-events.h"

#ifdef BUILD_GL
#include "platform/opengl/gl.h"
#endif

#ifdef BUILD_RASPI
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#include <SDL/SDL.h>
#include <EGL/egl.h>

#include <bcm_host.h>
#pragma GCC diagnostic pop
#endif

#if defined(BUILD_GLES2) || defined(USE_EPOXY)
#include "platform/opengl/gles2.h"
#endif

#ifdef USE_PIXMAN
//#include <pixman.h> //sunxu remove for gkd350
#define GKD350_HEIGHT 240
#define GKD350_WIDTH  320 

#endif

//sunxu add for gkd350h
 

extern unsigned soundframe, minframe;
extern int fullscreensetting, showFPSinformation, superGBMode ,SolarLevelSetting;
//extern int cheatconf[64];
//char* cheatName[64];
//extern int cheatsize;

typedef uint32_t u32;
typedef int32_t  s32;
typedef uint16_t u16;
typedef int16_t  s16;
typedef uint8_t  u8;
typedef char     s8;

//sunxu added end

struct mCore;
struct mSDLRenderer {
	struct mCore* core;
	color_t* outputBuffer;

	struct mSDLAudio audio;
	struct mSDLEvents events;
	struct mSDLPlayer player;

	bool (*init)(struct mSDLRenderer* renderer);
	void (*runloop)(struct mSDLRenderer* renderer, void* user);
	void (*deinit)(struct mSDLRenderer* renderer);

#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_Window* window;
	SDL_Texture* sdlTex;
	SDL_Renderer* sdlRenderer;
	SDL_GLContext* glCtx;
#endif

	unsigned width;
	unsigned height;
	int viewportWidth;
	int viewportHeight;
	int ratio;

	bool lockAspectRatio;
	bool lockIntegerScaling;
	bool interframeBlending;
	bool filter;

#ifdef BUILD_GL
	struct mGLContext gl;
#endif
#if defined(BUILD_GLES2) || defined(USE_EPOXY)
	struct mGLES2Context gl2;
#endif

#ifdef USE_PIXMAN
	//pixman_image_t* pix; //sunxu remove
	//pixman_image_t* screenpix; //suxu remove
#endif

#ifdef BUILD_RASPI
	EGLDisplay eglDisplay;
	EGLSurface eglSurface;
	EGLContext eglContext;
	EGL_DISPMANX_WINDOW_T eglWindow;
#endif

#ifdef BUILD_PANDORA
	int fb;
	int odd;
	void* base[2];
#endif
};

void mSDLSWCreate(struct mSDLRenderer* renderer);

#ifdef BUILD_GL
void mSDLGLCreate(struct mSDLRenderer* renderer);
#endif

#if defined(BUILD_GLES2) || defined(USE_EPOXY)
void mSDLGLES2Create(struct mSDLRenderer* renderer);
#endif

CXX_GUARD_END

#endif

//sunxu added 
//getformat 1 update last time   2--output ms value  other----normale way
unsigned sal_TimerRead(struct timeval * outtval, struct timeval * lasttval, int getformat);
void sal_DirectoryCombine(s8 *path, const char *name);
void sal_DirectoryGet(char* base, char* path);

//void VideoClear();
//void VideoFlip();
//void VideoOutputStringPixel(int x, int y, const char *text, bool allowWrap, bool shadow);
//void VideoDrawImage(int x, int y, uint32_t*data ,int w, int h, int strchlen);
//void refreshscreen();