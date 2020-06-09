/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "main.h"

#include <mgba/core/core.h>
#include <mgba/core/thread.h>
#include <mgba/core/version.h>
#include <mgba-util/arm-algo.h>

static SDL_Surface *mScreen = NULL;   //sunxu added


int screenhight =240, screenwidth = 320  , testflag = 0;  //sunxu added for gkd 350

static bool mSDLSWInit(struct mSDLRenderer* renderer);
static void mSDLSWRunloop(struct mSDLRenderer* renderer, void* user);
static void mSDLSWDeinit(struct mSDLRenderer* renderer);

void mSDLSWCreate(struct mSDLRenderer* renderer) {
	renderer->init = mSDLSWInit;
	renderer->deinit = mSDLSWDeinit;
	renderer->runloop = mSDLSWRunloop;
}

bool mSDLSWInit(struct mSDLRenderer* renderer) {
	unsigned width, height;
	renderer->core->desiredVideoDimensions(renderer->core, &width, &height);
	
	//sunxu  added
	printf("desiredVideoDimensions resut is  %d * %d\n",width, height);  
	int totalFrameBytes = width * height * BYTES_PER_PIXEL;
	//renderer->outputBuffer = malloc(totalFrameBytes);
	FILE *logfp; 
	/*
	FILE *logfp = fopen("./mGbatest.txt", "a+");	
	int num = SDL_GetNumVideoDrivers();
	printf("SDL_GetNumVideoDrivers resut is  %d\n", num);
	logfp = fopen("./mGbatest.txt", "a+");
	fprintf(logfp, "SDL_GetNumVideoDrivers resut is  %d\n", num);
	fclose(logfp);
	
	char* name = SDL_GetCurrentVideoDriver();	
	printf("SDL_GetCurrentVideoDriver resut is  %s\n", name);
	printf("BEFORE ERROR : %s\n", SDL_GetError());	
	logfp = fopen("./mGbatest.txt", "a+");
	fprintf(logfp, "SDL_GetCurrentVideoDriver resut is  %s\n", name);
	fprintf(logfp, "BEFORE ERROR : %s\n", SDL_GetError());	
	fclose(logfp);		
	*/
	//sunxu modify
	//renderer->window = SDL_CreateWindow(projectName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, renderer->viewportWidth, //renderer->viewportHeight, SDL_WINDOW_OPENGL | (SDL_WINDOW_FULLSCREEN_DESKTOP * renderer->player.fullscreen));	
	//renderer->window = SDL_CreateWindow(projectName, 0, 0, screenwidth, screenhight, SDL_WINDOW_FULLSCREEN_DESKTOP);
	testflag = SDL_CreateWindowAndRenderer(screenwidth, screenhight, SDL_WINDOW_FULLSCREEN_DESKTOP, &renderer->window, &renderer->sdlRenderer);
		
	printf("Could not SDL_CreateWindowAndRenderer : %s\n", SDL_GetError());	
	printf("SDL_CreateWindowAndRenderer result is : %d\n", testflag);	
	logfp = fopen("./mGbatest.txt", "a+");
	fprintf(logfp,"Could not SDL_CreateWindowAndRenderer : %s\n", SDL_GetError());	
	fprintf(logfp,"SDL_CreateWindowAndRenderer result is : %d\n", testflag);	
	fclose(logfp);
	
	
	mScreen = SDL_CreateRGBSurface(0, screenwidth, screenhight, 16,
                                        0x0000F800,
                                        0x000007E0,
                                        0x0000001F,
                                        0x00000000);
			
	printf("Could not SDL_CreateRGBSurface : %s\n", SDL_GetError());
	logfp = fopen("./mGbatest.txt", "a+");
	fprintf(logfp,"Could not SDL_CreateRGBSurface : %s\n", SDL_GetError());
	fclose(logfp);
	if(!mScreen->pixels)  //this is screen buffer
	{
		printf("surface->pixels need allocate\n"); 
		mScreen->pixels = malloc(totalFrameBytes);
	}
	else
		printf("surface->pixels No need allocate\n"); 
	
	//SDL_GetWindowSize(renderer->window, &renderer->viewportWidth, &renderer->viewportHeight);	
	
	printf("renderer size resut is  %d * %d\n",renderer->viewportWidth, renderer->viewportHeight); //sunxu
	
	
	renderer->player.window = renderer->window;	
	
	//sunxu modify 
	//renderer->sdlRenderer = SDL_CreateRenderer(renderer->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	//renderer->sdlRenderer = SDL_CreateRenderer(renderer->window, -1, SDL_RENDERER_SOFTWARE | SDL_RENDERER_PRESENTVSYNC);
	
	/*
	testflag = SDL_LockSurface(mScreen);  
	
	printf("SDL_LockSurface result is %d\n", testflag); //sunxu 
	printf("Could not SDL_LockSurface : %s\n", SDL_GetError());
	logfp = fopen("./mGbatest.txt", "a+");
	fprintf(logfp, "SDL_LockSurface result is %d\n", testflag);
	fprintf(logfp,"Could not SDL_LockSurface : %s\n", SDL_GetError());
	fclose(logfp);
	*/
	
		
#ifdef COLOR_16_BIT
#ifdef COLOR_5_6_5
	//renderer->sdlTex = SDL_CreateTexture(renderer->sdlRenderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, width, height);
	
	/*
	renderer->sdlTex = SDL_CreateTexture(renderer->sdlRenderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, screenwidth, screenhight);
												
	printf("SDL_CreateTexture complete %d\n", renderer->sdlTex); //sunxu 
	printf("Could not SDL_CreateTexture : %s\n", SDL_GetError());
	logfp = fopen("./mGbatest.txt", "a+");
	fprintf(logfp, "SDL_CreateTexture complete %d\n", renderer->sdlTex);
	fprintf(logfp,"Could not SDL_CreateTexture : %s\n", SDL_GetError());
	fclose(logfp);
	*/
	
#else
	renderer->sdlTex = SDL_CreateTexture(renderer->sdlRenderer, SDL_PIXELFORMAT_ABGR1555, SDL_TEXTUREACCESS_STREAMING, width, height);
#endif
#else
	renderer->sdlTex = SDL_CreateTexture(renderer->sdlRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, width, height);
#endif

	/*
	int stride;
	testflag = SDL_LockTexture(renderer->sdlTex, 0, (void**) &renderer->outputBuffer, &stride);
	
	printf("SDL_LockTexture resut is %d  stride is %d\n",testflag, stride); //sunxu 
	printf("Could not SDL_LockTexture : %s\n", SDL_GetError());	
	logfp = fopen("./mGbatest.txt", "a+");
	fprintf(logfp, "SDL_LockTexture complete %d\n", testflag);
	fprintf(logfp,"Could not SDL_LockTexture : %s\n", SDL_GetError());	
	fclose(logfp);
	*/
		
	
	//renderer->core->setVideoBuffer(renderer->core, renderer->outputBuffer, stride / BYTES_PER_PIXEL);
	renderer->outputBuffer = mScreen->pixels;//malloc(totalFrameBytes);  //this is next frame buffer
	renderer->core->setVideoBuffer(renderer->core, renderer->outputBuffer, width);
	
	printf("SDL_INIT complete\n");
	
	return true;
}

void mSDLSWRunloop(struct mSDLRenderer* renderer, void* user) {
	struct mCoreThread* context = user;
	SDL_Event event;

	while (mCoreThreadIsActive(context)) {
		while (SDL_PollEvent(&event)) {
			mSDLHandleEvent(context, &renderer->player, &event);
		}

		if (mCoreSyncWaitFrameStart(&context->impl->sync)) {
			
			/* //sunxu modify
			SDL_UnlockTexture(renderer->sdlTex);
			SDL_RenderCopy(renderer->sdlRenderer, renderer->sdlTex, 0, 0);
			SDL_RenderPresent(renderer->sdlRenderer);
			int stride;
			SDL_LockTexture(renderer->sdlTex, 0, (void**) &renderer->outputBuffer, &stride);
			renderer->core->setVideoBuffer(renderer->core, renderer->outputBuffer, stride / BYTES_PER_PIXEL);
			*/
			printf("start cp frame %d - %d -%d\n", renderer->width, renderer->height, mScreen->pitch); 
			//color_t * dst = mScreen->pixels, *src = renderer->outputBuffer;
					 
			//for (int y = 0; y <(renderer->height); y++)
			//	{
					//SDL_LockSurface(mScreen);
					//memcpy(dst, src, renderer->width*renderer->height*BYTES_PER_PIXEL);
					//SDL_UnlockSurface(mScreen); 
					//src += SNES_WIDTH * sizeof(u16);
			//		src += renderer->width*BYTES_PER_PIXEL;
			//		dst += mScreen->pitch;
			//	}
			SDL_UpdateTexture(renderer->sdlTex, NULL, mScreen->pixels, mScreen->pitch);
			SDL_RenderClear(renderer->sdlRenderer);
			SDL_RenderCopy(renderer->sdlRenderer, renderer->sdlTex, NULL, NULL);
			SDL_RenderPresent(renderer->sdlRenderer);			
		    
		}
		
		mCoreSyncWaitFrameEnd(&context->impl->sync);
	}
}

void mSDLSWDeinit(struct mSDLRenderer* renderer) {
	if (renderer->ratio > 1) {
		free(renderer->outputBuffer);
	}
}
