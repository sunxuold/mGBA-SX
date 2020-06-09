/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "main.h"

#include <mgba/internal/debugger/cli-debugger.h>

#ifdef USE_GDB_STUB
#include <mgba/internal/debugger/gdb-stub.h>
#endif
#ifdef USE_EDITLINE
#include "feature/editline/cli-el-backend.h"
#endif
#ifdef ENABLE_SCRIPTING
#include <mgba/core/scripting.h>

#ifdef ENABLE_PYTHON
#include "platform/python/engine.h"
#endif
#endif

#include <mgba/core/cheats.h>
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/core/input.h>
#include <mgba/core/serialize.h>
#include <mgba/core/thread.h>
#include <mgba/internal/gba/input.h>

#include <mgba/feature/commandline.h>
#include <mgba-util/vfs.h>

#include <SDL.h>

#include <errno.h>
#include <signal.h>

#define PORT "sdl"

//sunxu added for GKD350
#define SAL_MAX_PATH			128
#define SAL_DIR_SEP				"/"

int debugtest = 0; 

char * home = ".mgba_sx",
	 * savegame= ".mgba_sx/savegame",
	 * savestate= ".mgba_sx/savestate",
	 * screenshot= ".mgba_sx/screenshot",
	 * patch= ".mgba_sx/patch",
	 * cheats= ".mgba_sx/cheats";
	 
char homepath [SAL_MAX_PATH],
      savegamePath[SAL_MAX_PATH],
	  savestatePath[SAL_MAX_PATH],
	  screenshotPath[SAL_MAX_PATH],
	  patchPath[SAL_MAX_PATH],
	  cheatsPath[SAL_MAX_PATH];	

char* testfile = "./3.gba";   //sunxu test


int fullscreensetting = 0;
int showFPSinformation = 0;
int superGBMode = 1;
int frameWritedelay = 2000;
int frameReaddelay = 10000; 

int* cheatconf[128];
char* cheatName[128];
int  cheatoldconf[128];
int cheatsize = 0;
int cheatrefresh = 0;

//#include <mgba/internal/gba/cheats.h>
//#include <mgba/internal/gba/cheats.h>

//Solar logic
#include <mgba/gba/interface.h>
int SolarLevelSetting = 5;
static struct GBALuminanceSource lux;	 

extern MGBA_EXPORT const int GBA_LUX_LEVELS[10];

static void _updateLux(struct GBALuminanceSource* lux) {
	UNUSED(lux);
}

static uint8_t _readLux(struct GBALuminanceSource* lux) {
	UNUSED(lux);
	int value = 0x16;
	if (SolarLevelSetting > 0) {
		value += GBA_LUX_LEVELS[SolarLevelSetting - 1];
	}	
	return 0xFF - value;
}	
//sunxu added complete

	  
static bool mSDLInit(struct mSDLRenderer* renderer);
static void mSDLDeinit(struct mSDLRenderer* renderer);

static int mSDLRun(struct mSDLRenderer* renderer, struct mArguments* args);

static struct VFile* _state = NULL;

static void _loadState(struct mCoreThread* thread) {
	mCoreLoadStateNamed(thread->core, _state, SAVESTATE_RTC);
}



int main(int argc, char** argv) {
	struct mSDLRenderer renderer = {0};

	//getInfo(); //test
	
	//sunxu added		
	for(int i=0; i<128; i++)	
	{
		cheatconf[i]=NULL;
		cheatoldconf[i] = 0;
		cheatName[i] = NULL;
	}
	
	sal_DirectoryGet(homepath, home);
	sal_DirectoryGet(savegamePath, savegame);
	sal_DirectoryGet(savestatePath, savestate);
	sal_DirectoryGet(screenshotPath, screenshot);
	sal_DirectoryGet(patchPath, patch);
	sal_DirectoryGet(cheatsPath, cheats);
	
	
	

	struct mCoreOptions opts = {
		.useBios = true,
		.rewindEnable = true,
		.rewindBufferCapacity = 600,
		.audioBuffers = 1024,//1024,//735 used
		.videoSync = true,
		.audioSync = true,
		.volume = 0x100,
		.interframeBlending=false,   //sunxu added 
		.savegamePath = savegamePath,
		.savestatePath = savestatePath,
		.screenshotPath = screenshotPath,
		.patchPath =  patchPath,
		.cheatsPath = cheatsPath,
	/*	
	char* bios;
	bool skipBios;
	int logLevel;
	int frameskip;
	float fpsTarget;
	int fullscreen;
	bool lockAspectRatio;
	bool resampleVideo;
	bool suspendScreensaver;
	char* shader;
	bool mute;
	*/				
		
	};


	
	
	struct mArguments args;
	struct mGraphicsOpts graphicsOpts;

	struct mSubParser subparser;

	initParserForGraphics(&subparser, &graphicsOpts);
	
	
	bool parsed = parseArguments(&args, argc, argv, &subparser);
	
	FILE *fp = NULL;//sunxu test
	if (debugtest)
	{
		fp = fopen("./test.txt", "a+");  //sunxu  test
		args.fname = testfile; //sunxu test code
	}
	
	if (!args.fname && !args.showVersion) {
		parsed = false;
	}
	if (!parsed || args.showHelp) {
		usage(argv[0], subparser.usage);
		freeArguments(&args);
			
		return !parsed;
	}
	if (args.showVersion) {
		version(argv[0]);
		freeArguments(&args);		
		
		return 0;
	}
		
		if (debugtest)
		{
		//sunxu test code
		fp = fopen("./test.txt", "a+");
		fprintf(fp,"00\n"); 
		fclose(fp);	
		}		

	renderer.core = mCoreFind(args.fname);
		
		if (debugtest)
		{
		//sunxu test code
		fp = fopen("./test.txt", "a+");
		fprintf(fp,"01\n"); 
		fclose(fp);
		}		

	
	if (!renderer.core) {
		printf("Could not run game. Are you sure the file exists and is a compatible game?\n");
		
		if (debugtest)
		{
		//sunxu test code
		fp = fopen("./test.txt", "a+");
		fprintf(fp,"11\n"); 
		fclose(fp);
		}
		
		freeArguments(&args);
		

		
		return 1;
	}

	if (!renderer.core->init(renderer.core)) {
		
		if (debugtest)
		{
		//sunxu test code
		fp = fopen("./test.txt", "a+");
		fprintf(fp,"12\n"); 
		fclose(fp);	
		}		
		
		freeArguments(&args);
				
		return 1;
	}

	//renderer.core->desiredVideoDimensions(renderer.core, &renderer.width, &renderer.height);
	
	//printf("reader width %d  High %d\n", renderer.width, renderer.height); //Sunxu add test
	
	
#ifdef BUILD_GL
	mSDLGLCreate(&renderer);
#elif defined(BUILD_GLES2) || defined(USE_EPOXY)
	mSDLGLES2Create(&renderer);
#else
	mSDLSWCreate(&renderer);
#endif
	renderer.ratio = graphicsOpts.multiplier;
	if (renderer.ratio == 0) {
		renderer.ratio = 1;
	}
	opts.width = renderer.width * renderer.ratio;
	opts.height = renderer.height * renderer.ratio;
	
	struct mCheatDevice* device = renderer.core->cheatDevice(renderer.core);
	if(device) mCheatDeviceClear(device);		
	if (args.cheatsFile) {				
		struct VFile* vf = VFileOpen(args.cheatsFile, O_RDONLY);
		if (vf) {			
			mCheatParseFile(device, vf);
			vf->close(vf);
		}
	}	

	mInputMapInit(&renderer.core->inputMap, &GBAInputInfo);
	
	mCoreInitConfig(renderer.core, PORT);
	applyArguments(&args, &subparser, &renderer.core->config);

	mCoreConfigLoadDefaults(&renderer.core->config, &opts);
	mCoreLoadConfig(renderer.core);	
	
	mDirectorySetMapOptions(&renderer.core->dirs, &opts); //sunxu added for GKD350

	renderer.viewportWidth = renderer.core->opts.width;
	renderer.viewportHeight = renderer.core->opts.height;
	renderer.player.fullscreen = renderer.core->opts.fullscreen;
	renderer.player.windowUpdated = 0;

	renderer.lockAspectRatio = renderer.core->opts.lockAspectRatio;
	renderer.lockIntegerScaling = renderer.core->opts.lockIntegerScaling;
	renderer.interframeBlending = renderer.core->opts.interframeBlending;
	renderer.filter = renderer.core->opts.resampleVideo;
	
	
	
	
	if (!mSDLInit(&renderer)) {
		
		if (debugtest)
		{
		//sunxu test code
		fp = fopen("./test.txt", "a+");
		fprintf(fp,"13\n"); 
		fclose(fp);	
		}
		
		freeArguments(&args);
		mCoreConfigDeinit(&renderer.core->config);
		renderer.core->deinit(renderer.core);
		

		
		return 1;
	}

		if (debugtest)
		{
		//sunxu test code
		fp = fopen("./test.txt", "a+");
		fprintf(fp,"21\n"); 
		fclose(fp);	
		}

	renderer.player.bindings = &renderer.core->inputMap;
	mSDLInitBindingsGBA(&renderer.core->inputMap);

		if (debugtest)
		{
		//sunxu test code
		fp = fopen("./test.txt", "a+");
		fprintf(fp,"22\n"); 
		fclose(fp);	
		}
	
	mSDLInitEvents(&renderer.events);
	
		if (debugtest)
		{
		//sunxu test code
		fp = fopen("./test.txt", "a+");
		fprintf(fp,"23\n"); 
		fclose(fp);		
		}
		
	mSDLEventsLoadConfig(&renderer.events, mCoreConfigGetInput(&renderer.core->config));

		if (debugtest)
		{
		//sunxu test code
		fp = fopen("./test.txt", "a+");
		fprintf(fp,"24\n"); 
		fclose(fp);	
		}
		
		
	mSDLAttachPlayer(&renderer.events, &renderer.player);

		if (debugtest)
		{
		//sunxu test code
		fp = fopen("./test.txt", "a+");
		fprintf(fp,"25\n"); 
		fclose(fp);	
		}
		
	mSDLPlayerLoadConfig(&renderer.player, mCoreConfigGetInput(&renderer.core->config));

		if (debugtest)
		{
		//sunxu test code
		fp = fopen("./test.txt", "a+");
		fprintf(fp,"26\n"); 
		fclose(fp);		
		}
	

#if SDL_VERSION_ATLEAST(2, 0, 0)
	renderer.core->setPeripheral(renderer.core, mPERIPH_RUMBLE, &renderer.player.rumble.d);
#endif

	int ret;

	// TODO: Use opts and config
	
	//char* title[128], code[128];
	//renderer.core->getGameTitle(renderer.core, title);
	//renderer.core->getGameCode(renderer.core, code);	
	//printf("title %s code %s\n", title, code);
	
	//sunxu added Solar config
	if (renderer.core->platform(renderer.core) == PLATFORM_GBA) 
	{
		renderer.core->setPeripheral(renderer.core, mPERIPH_GBA_LUMINANCE, &lux);
		lux.readLuminance = _readLux;
		lux.sample = _updateLux;
		_updateLux(&lux);
	} 
	//Solar config add complete
	
	
	
	ret = mSDLRun(&renderer, &args);   //sunxu Main Loop
		
	
	mSDLDetachPlayer(&renderer.events, &renderer.player);
	mInputMapDeinit(&renderer.core->inputMap);

	if (device) {
		mCheatDeviceDestroy(device);
	}

	mSDLDeinit(&renderer);

	freeArguments(&args);
	mCoreConfigFreeOpts(&opts);
	mCoreConfigDeinit(&renderer.core->config);
	renderer.core->deinit(renderer.core);

	return ret;
}

#if defined(_WIN32) && !defined(_UNICODE)
#include <mgba-util/string.h>

int wmain(int argc, wchar_t** argv) {
	char** argv8 = malloc(sizeof(char*) * argc);
	int i;
	for (i = 0; i < argc; ++i) {
		argv8[i] = utf16to8((uint16_t*) argv[i], wcslen(argv[i]) * 2);
	}
	int ret = main(argc, argv8);
	for (i = 0; i < argc; ++i) {
		free(argv8[i]);
	}
	free(argv8);
	return ret;
}
#endif

int mSDLRun(struct mSDLRenderer* renderer, struct mArguments* args) {
	struct mCoreThread thread = {
		.core = renderer->core
	};
	if (!mCoreLoadFile(renderer->core, args->fname)) {
		return 1;
	}		
	
	mCoreAutoloadSave(renderer->core);
	
	//printf("before auto load cheatsize %d!\n",device->cheats.size);
	mCoreAutoloadCheats(renderer->core);
		
	//sunxu added  for cheat
	struct mCheatDevice* device = renderer->core->cheatDevice(renderer->core);
		if(device)
		{
			cheatsize = device->cheats.size;
			printf("get cheatsize %d!\n", cheatsize);
			struct mCheatSet*  maincheats = NULL;
			//printf("true is %d!\n", true);
			//printf("false is %d!\n", false);
			for(int i = 0; i< cheatsize ;i++)	
			{	
				maincheats = device->cheats.vector[i];//(struct mCheatSet*)mCheatSetsGetPointer(&(device->cheats),i);							
					if(i<128)
					{	
						if(maincheats)
						{	
							cheatconf[i] = &(maincheats->enabled);
							cheatoldconf[i]	= maincheats->enabled;
							cheatName[i]= maincheats->name ;//, 64-1);
							//printf("cheat id is %d enabled: %c Name %s!\n", i, *cheatconf[i], cheatName[i]);
						}
						else
						{
							printf("not enough space total is%d \n", cheatsize);
							cheatsize = i;
							break;							
						}
					}
			}
		}
	//sunxu added complete		
	
#ifdef ENABLE_SCRIPTING
	struct mScriptBridge* bridge = mScriptBridgeCreate();
#ifdef ENABLE_PYTHON
	mPythonSetup(bridge);
#endif
#endif

#ifdef USE_DEBUGGERS
	struct mDebugger* debugger = mDebuggerCreate(args->debuggerType, renderer->core);
	if (debugger) {
#ifdef USE_EDITLINE
		if (args->debuggerType == DEBUGGER_CLI) {
			struct CLIDebugger* cliDebugger = (struct CLIDebugger*) debugger;
			CLIDebuggerAttachBackend(cliDebugger, CLIDebuggerEditLineBackendCreate());
		}
#endif
		mDebuggerAttach(debugger, renderer->core);
		mDebuggerEnter(debugger, DEBUGGER_ENTER_MANUAL, NULL);
 #ifdef ENABLE_SCRIPTING
		mScriptBridgeSetDebugger(bridge, debugger);
#endif
	}
#endif

	if (args->patch) {
		struct VFile* patch = VFileOpen(args->patch, O_RDONLY);
		if (patch) {
			renderer->core->loadPatch(renderer->core, patch);
		}
	} else {
		mCoreAutoloadPatch(renderer->core);
	}

	renderer->audio.samples = renderer->core->opts.audioBuffers;
	renderer->audio.sampleRate = 44100;//44100;//44100;

	// thread remove     bool didFail = !mCoreThreadStart(&thread);
	bool didFail = !mCoreThreadBeginNoStart(&thread);
	
	if (!didFail) {
#if SDL_VERSION_ATLEAST(2, 0, 0)
		renderer->core->desiredVideoDimensions(renderer->core, &renderer->width, &renderer->height);
		unsigned width = renderer->width * renderer->ratio;
		unsigned height = renderer->height * renderer->ratio;
		if (width != (unsigned) renderer->viewportWidth && height != (unsigned) renderer->viewportHeight) {
			SDL_SetWindowSize(renderer->window, width, height);
			renderer->player.windowUpdated = 1;
		}
		mSDLSetScreensaverSuspendable(&renderer->events, renderer->core->opts.suspendScreensaver);
		mSDLSuspendScreensaver(&renderer->events);
#endif
		if (mSDLInitAudio(&renderer->audio, &thread)) {
			if (args->savestate) {
				struct VFile* state = VFileOpen(args->savestate, O_RDONLY);
				if (state) {								
					_state = state;
					mCoreThreadRunFunction(&thread, _loadState);
					_state = NULL;
					state->close(state);
				}
			}			
	
	
				
			
			renderer->runloop(renderer, &thread);
			mSDLPauseAudio(&renderer->audio);
			if (mCoreThreadHasCrashed(&thread)) {
				didFail = true;
				printf("The game crashed!\n");
			}
		} else {
			didFail = true;
			printf("Could not initialize audio.\n");
		}
#if SDL_VERSION_ATLEAST(2, 0, 0)
		mSDLResumeScreensaver(&renderer->events);
		mSDLSetScreensaverSuspendable(&renderer->events, false);
#endif

		mCoreThreadJoin(&thread);
	} else {
		printf("Could not run game. Are you sure the file exists and is a compatible game?\n");
	}
	renderer->core->unloadROM(renderer->core);

#ifdef ENABLE_SCRIPTING
	mScriptBridgeDestroy(bridge);
#endif

	return didFail;
}

static bool mSDLInit(struct mSDLRenderer* renderer) {
	if (SDL_Init( SDL_INIT_VIDEO /*SDL_INIT_EVERYTHING sunxu modify*/) < 0) {
		printf("Could not initialize video: %s\n", SDL_GetError());
		return false;
	}

	return renderer->init(renderer);
}

static void mSDLDeinit(struct mSDLRenderer* renderer) {
	mSDLDeinitEvents(&renderer->events);
	mSDLDeinitAudio(&renderer->audio);
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_DestroyWindow(renderer->window);
#endif

	renderer->deinit(renderer);
	SDL_Quit();
}

//sunxu added for FPs show
unsigned sal_TimerRead(struct timeval *outtval, struct timeval * lasttval, int getformat)
{
  	gettimeofday(outtval, 0);
  	//tval.tv_usec
  	//tval.tv_sec
	if(getformat==1)
	{
		lasttval->tv_sec = outtval->tv_sec;
		lasttval->tv_usec = outtval->tv_usec;
	}
	if(getformat ==2)
		return ((outtval->tv_sec * 1000) + outtval->tv_usec/1000);
	else
		return ((outtval->tv_sec * 1000000) + outtval->tv_usec) / 16667;// =1000*1000*1/60(us)
}

void sal_DirectoryCombine(s8 *path, const char *name)
{
	s32 len = strlen(path);
	s32 i=0;

	if (len>0)
	{
		if((path[len-1] != SAL_DIR_SEP[0]))
		{
			path[len]=SAL_DIR_SEP[0];
			len++;
		}
	}
	while(1)
	{
		path[len]=name[i];
		if (name[i]==0) break;
		len++;
		i++;
	}
}


void sal_DirectoryGet(char* base, char* path)
{
	if (!base[0]) {
		char *env_home = getenv("HOME");
		strcpy(base, env_home);
		sal_DirectoryCombine(base, path);

		/* Create the directory if it didn't already exist */
		mkdir(base, 0755);
		//printf("Path is :   %s\n", base );  //sunxu test
	}
}


void getInfo()
{
	FILE *fp = fopen("/proc/cpuinfo", "r");
	size_t len = 0;
	int nread = 0;
	char* buffer = NULL;
	unsigned char result[256] = {'\0'};
	while(nread=getline(&buffer, &len,fp)!=-1)
	{
		if(strstr(buffer,"vendor_id")!=NULL)
        {
            buffer[strlen(buffer)-1]=0;
            sscanf(buffer,"%s%s%s",result,result,result);
			printf("new is %s \n", result); 
			break;
        }
		else
		{
			printf("old  %s \n", buffer);
		}

	} 
	fclose(fp);
	//printf("uptime: %f", uptime);
}



