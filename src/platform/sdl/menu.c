#include "menu.h"

//#include "video.h"

//#include "port.h"
//#include "snes9x.h"

#include <SDL.h>

#include <libintl.h>
#include <sys/stat.h>

extern void VideoClear();
extern void VideoFlip();
//extern void VideoOutputStringPixel(int x, int y, const char *text, bool8 allowWrap, bool8 shadow);
extern void  Fontrender(int x, int y, char *text, int allowWrap, int shadow);
extern void drawsnapshot4menu(void* context, int slot, int y);
extern int savestatus4menu(void* context, int slot);
extern int loadstatus4menu(void* context, int slot);
extern int checkslot4menu(void* context, int slot);


extern int fullscreensetting;
extern int  showFPSinformation;
extern int  superGBMode;
extern int  SolarLevelSetting;
extern int frameWritedelay;
extern int frameReaddelay;
extern int usejoysticks;

int repeatrate = 3;
int framesyncflag = 1;

extern int repeatCount;

extern int* cheatconf[256];
extern char* cheatName[256];
extern int cheatsize;
extern int cheatrefresh;

int writedelay, readdelay;

void* mCoreThread_p = NULL;

enum MenuAction {
    MA_NONE     = 0,
    MA_OK       = 1 << 0,
    MA_CANCEL   = 1 << 1,
    MA_LEAVE    = 1 << 2,
    MA_UP       = 1 << 3,
    MA_DOWN     = 1 << 4,
    MA_LEFT     = 1 << 5,
    MA_RIGHT    = 1 << 6,
    MA_FIRST    = 1 << 7,
    MA_LAST     = 1 << 8,
    MA_PAGEUP   = 1 << 9,
    MA_PAGEDOWN = 1 << 10,
};

static void (*preDrawFunc)() = NULL;

//static MenuAction mapAction(SDLKey); //sunxu modify for C 
static uint32_t mapAction(SDLKey);

static void drawMenu(const struct MenuItem items[], int x, int valx, int y, int index, int topIndex, int count);

void MenuSetPreDrawFunc(void (*func)()) {
    preDrawFunc = func;
}

#define FIX_INDEX if (topIndex + pageCount <= index) topIndex = index - pageCount + 1; \
    else if (topIndex > index) topIndex = index
#define CALL_TRIGGER changed = 1; if (cur->triggerFunc && (mr = (*cur->triggerFunc)(cur)) != 0) { \
    if (mr < 0) return mr; if (mr == MR_OK) return index; }
int MenuRun(const struct MenuItem items[], int x, int valx, int y, int pageCount, int index) {
    int count = 0, topIndex = 0;
    const struct MenuItem *cur = items;
    uint32_t actions = 0;
    
	//MenuResult mr; //sunxu modify for C 
	enum MenuResult mr;
	
    bool8 changed = 1;
    while (cur->type != MIT_END) {
        ++cur; ++count;
    }
    if (pageCount == 0) pageCount = count;
    if (index >= count) index = count - 1;
    FIX_INDEX;
    cur = items + index;
    SDL_Event event;
    for (;;) {
        if (changed) {
            changed = 0;
            drawMenu(items, x, valx, y, index, topIndex, pageCount);
            usleep(150000);
        } else {
            usleep(50000);
        }
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    //s9xTerm = 1;
					VideoClear();
					VideoFlip();
                    return MR_LEAVE;
                case SDL_KEYDOWN:
                    actions |= mapAction(event.key.keysym.sym);
                    break;
                case SDL_KEYUP:
                    actions &= ~mapAction(event.key.keysym.sym);
                    break;
                default: break;
            }
        }
        if (!actions) continue;
        if (actions & MA_OK) {
            actions &= ~MA_OK;
            switch (cur->type) {
                case MIT_BOOL:
                    if (!cur->value) break;
                    *(bool8 *)cur->value = !*(bool8 *)cur->value;
                    changed = 1;
                    CALL_TRIGGER;
                    break;
                case MIT_BOOL8:
                    if (!cur->value) break;
                    *(bool8 *)cur->value = !*(bool8 *)cur->value;
                    changed = 1;
                    CALL_TRIGGER;
                    break;
                case MIT_CLICK:
                    CALL_TRIGGER;
                    break;
                default: break;
            }
            continue;
        }
        if (actions & MA_CANCEL) {
            actions &= ~MA_CANCEL;
			VideoClear();
			VideoFlip();
            return MR_CANCEL;
        }
        if (actions & MA_LEAVE) {
            actions &= ~MA_LEAVE;

			VideoClear();
			VideoFlip();
			
            return MR_LEAVE;
        }
        if (actions & MA_UP) {
            if (--index < 0) index = count - 1;
            cur = items + index;
            changed = 1;
            FIX_INDEX;
            continue;
        }
        if (actions & MA_DOWN) {
            if (++index >= count) index = 0;
            cur = items + index;
            changed = 1;
            FIX_INDEX;
            continue;
        }
        if (actions & MA_PAGEUP) {
            if (index == 0) continue;
            index -= pageCount;
            if (index < 0) index = 0;
            cur = items + index;
            changed = 1;
            FIX_INDEX;
            continue;
        }
        if (actions & MA_PAGEDOWN) {
            if (index == count - 1) continue;
            index += pageCount;
            if (index >= count) index = count - 1;
            cur = items + index;
            changed = 1;
            FIX_INDEX;
            continue;
        }
        if (actions & MA_FIRST) {
            if (index == 0) continue;
            index = 0;
            cur = items + index;
            changed = 1;
            FIX_INDEX;
            continue;
        }
        if (actions & MA_LAST) {
            if (index == count - 1) continue;
            index = count - 1;
            cur = items + index;
            changed = 1;
            FIX_INDEX;
            continue;
        }
        if (actions & MA_LEFT) {
            actions &= ~MA_LEFT;
            int32_t val;
            switch (cur->type) {
                case MIT_BOOL:
                    if (!cur->value) break;
                    val = *(bool8*)cur->value;
                    *(bool8*)cur->value = !val;
                    changed = 1;
                    CALL_TRIGGER;
                    break;
                case MIT_BOOL8:
                    if (!cur->value) break;
                    val = *(bool8*)cur->value;
                    *(bool8*)cur->value = !val;
                    changed = 1;
                    CALL_TRIGGER;
                    break;
                case MIT_INT8:
                    if (!cur->value) break;
                    val = *(int8_t *)cur->value;
                    if (--val < cur->valMin) val = cur->valMax;
                    *(int8_t *)cur->value = val;
                    changed = 1;
                    CALL_TRIGGER;
                    break;
                case MIT_INT16:
                    if (!cur->value) break;
                    val = *(int16_t *)cur->value;
                    if (--val < cur->valMin) val = cur->valMax;
                    *(int16_t *)cur->value = val;
                    changed = 1;
                    CALL_TRIGGER;
                    break;
                case MIT_INT32:
                    if (!cur->value) break;
                    val = *(int32_t *)cur->value;
                    if (--val < cur->valMin) val = cur->valMax;
                    *(int32_t *)cur->value = val;
                    changed = 1;
                    CALL_TRIGGER;
                    break;
                default: break;
            }
            continue;
        }
        if (actions & MA_RIGHT) {
            actions &= ~MA_RIGHT;
            int32_t val;
            switch (cur->type) {
                case MIT_BOOL:
                    if (!cur->value) break;
                    val = *(bool8*)cur->value;
                    *(bool8*)cur->value = !val;
                    changed = 1;
                    CALL_TRIGGER;
                    break;
                case MIT_BOOL8:
                    if (!cur->value) break;
                    val = *(bool8*)cur->value;
                    *(bool8*)cur->value = !val;
                    changed = 1;
                    CALL_TRIGGER;
                    break;
                case MIT_INT8:
                    if (!cur->value) break;
                    val = *(int8_t *)cur->value;
                    if (++val > cur->valMax) val = cur->valMin;
                    *(int8_t *)cur->value = val;
                    changed = 1;
                    CALL_TRIGGER;
                    break;
                case MIT_INT16:
                    if (!cur->value) break;
                    val = *(int16_t *)cur->value;
                    if (++val > cur->valMax) val = cur->valMin;
                    *(int16_t *)cur->value = val;
                    changed = 1;
                    CALL_TRIGGER;
                    break;
                case MIT_INT32:
                    if (!cur->value) break;
                    val = *(int32_t *)cur->value;
                    if (++val > cur->valMax) val = cur->valMin;
                    *(int32_t *)cur->value = val;
                    changed = 1;
                    CALL_TRIGGER;
                    break;
                default: break;
            }
            continue;
        }
    }
}

//static MenuAction mapAction(SDLKey key) {

static uint32_t mapAction(SDLKey key) {	
    switch (key) {

    case SDLK_UP: return MA_UP;
    case SDLK_DOWN: return MA_DOWN;
    case SDLK_LEFT: return MA_LEFT;
    case SDLK_RIGHT: return MA_RIGHT;
    case SDLK_ESCAPE: case SDLK_HOME: return MA_LEAVE;
    case SDLK_TAB: return MA_PAGEUP;
    case SDLK_BACKSPACE: return MA_PAGEDOWN;
    case SDLK_PAGEUP: return MA_FIRST;
    case SDLK_PAGEDOWN: return MA_LAST;
    case SDLK_LCTRL: SDLK_RETURN: return MA_OK;
    case SDLK_LALT: return MA_CANCEL;
    default: return MA_NONE;
    }
}

static void drawMenu(const struct MenuItem items[], int x, int valx, int y, int index, int topIndex, int count) {
    const struct MenuItem *cur = items + topIndex;
    int curIndex = topIndex;
    int32_t val = 0;
    struct MenuItemValue miv = {};
    VideoClear();
	//VideoOutputStringPixel(0, 0, "mGBA_SX for GKD350h v0.7", 0, 0);
	//test code
	//Fontrender(20, 20, "掌上abcd+-。，掌上明珠掌", 0, 1);
	//VideoFlip();
	//return;
	//test 
	
	
    if (preDrawFunc) (*preDrawFunc)();
    while (cur->type != MIT_END && count > 0) {
		
		Fontrender(80, 0, "mGBA_SX for GKD350h v0.91", 0, 1);
        Fontrender(x, y, cur->text, 0, 1);
        bool8 hasvalue = 0;
        switch (cur->type) {
            case MIT_BOOL8:
                val = *(bool8*)cur->value ? 1 : 0;
                hasvalue = 1;
                break;
            case MIT_BOOL:
                val = *(bool8*)cur->value ? 1 : 0;
                hasvalue = 1;
                break;
            case MIT_INT8:
                val = *(int8_t*)cur->value;
                hasvalue = 1;
                break;
            case MIT_INT16:
                val = *(int16_t*)cur->value;
                hasvalue = 1;
                break;
            case MIT_INT32:
                val = *(int32_t*)cur->value;
                hasvalue = 1;
                break;
            default: break;
        }
        if (hasvalue) {
            if (cur->valueFunc && (miv = (*cur->valueFunc)(val), miv.text != NULL)) {
                Fontrender(valx, y, miv.text, 0, 1);
            } else if (cur->type == MIT_BOOL8 || cur->type == MIT_BOOL) {
                Fontrender(valx, y, val ? "已打开" : "已关闭 ", 0, 1);
            } else {
                char valstr[16];
                snprintf(valstr, 16, "%d", val);
                Fontrender(valx, y, valstr, 0, 1);
            }
        }
        if (index == curIndex) {
            Fontrender(x - 12, y, "> ", 0, 1);
        }
        if (cur->drawFunc) {
            (*cur->drawFunc)(cur, y, index == curIndex);
        }
        y += 14;
        --count;
        ++cur;
        ++curIndex;
    }
    VideoFlip();
}
int enterMainMenu(int index,void* info) {
	writedelay = frameWritedelay/1000;
	readdelay = frameReaddelay/1000;
    struct MenuItem items[] = {
        { MIT_CLICK, NULL, "保存游戏",      0, 0, enterSaveStateMenu,},
        { MIT_CLICK, NULL, "读取游戏",      0, 0, enterLoadStateMenu,},
        //{ MIT_CLICK, NULL, "系统设置",        0, 0, enterSettingsMenu,},
        { MIT_CLICK, NULL, "金手指设置",   0, 0, enterCheatSettingMenu,},		
		//{ MIT_INT32, &writedelay, "write delay:", 0, 15, updatedelay, NULL,SolarValue},
		//{ MIT_INT32, &readdelay, "read delay:", 0, 15, updatedelay, NULL,SolarValue},
		{ MIT_INT32, &fullscreensetting, "全屏模式：", 0, 3, NULL, NULL,FullscreenMode},
		{ MIT_BOOL8, &usejoysticks, "摇杆开关" },
		{ MIT_BOOL8, &superGBMode, "Super GB 开关" },		
        { MIT_BOOL8, &showFPSinformation,"FPS开关" },		
		{ MIT_BOOL8, &framesyncflag, "同步开关："},
		{ MIT_INT32, &repeatrate, "每秒重按次数：", 0, 9, updaterepeatCount, NULL,SolarValue},		
		{ MIT_INT32, &SolarLevelSetting, "光感强度：", 1, 10, NULL, NULL,SolarValue},
		
        { MIT_CLICK, NULL, "退出",            0, 0, MR_LEAVE,},
        { MIT_END, }
    };
	mCoreThread_p = info;
	
    return MenuRun(items, 80, 200, 40, 0, index);
}

int enterCheatSettingMenu() {
	//extern int cheatconf[64];
	//extern char* cheatName[64];
	//extern int cheatsize;
	if(cheatsize == 0) return MR_NONE;
	
    struct MenuItem items[128+1] = {};
	if(cheatsize>128) cheatsize = 128;
    char text[128][128];
    for (int i = 0; i < cheatsize; ++i) {
        items[i].type = MIT_BOOL8;		
        snprintf(text[i], 127, cheatName[i]);
		text[i][127]= '\0';
        items[i].text = text[i];
		items[i].value = cheatconf[i];
		//printf("cheat id is %d enabled: %u \n", i, *cheatconf[i]);
		items[i].customData = (void*)(uintptr_t)i;
		items[i].triggerFunc =refreshcheat;
    }
    items[cheatsize].type = MIT_END;
    if (MenuRun(items, 20, 270, 20, 14, 0) == MR_LEAVE) return MR_LEAVE;
    return MR_NONE;
}


int enterSaveStateMenu() {
    struct MenuItem items[10] = {};
    char text[9][64];	
    for (int i = 0; i <9; ++i) {
        items[i].type = MIT_CLICK;
		if (!checkslot(i)) {
			snprintf(text[i], 64, "保存游戏：%d ", i);
			items[i].drawFunc = NULL; 
		}
		else
		{
			snprintf(text[i], 64, "覆盖游戏：%d ", i);
			items[i].drawFunc = drawsnapshot; 
		}
        items[i].text = text[i];
        items[i].customData = (void*)(uintptr_t)i;
        items[i].triggerFunc = savestatus;				
    }
    items[9].type = MIT_END;
    if (MenuRun(items,30, 90, 60, 0, 0) == MR_LEAVE) return MR_LEAVE;
    return MR_NONE;
}

int enterLoadStateMenu() {    
	struct MenuItem items[10] = {};
    char text[9][64];
    for (int i = 0; i <9; ++i) {
        items[i].type = MIT_CLICK;
        if (!checkslot(i)) {
            snprintf(text[i], 64, "空白存档：%d ", i);
            items[i].triggerFunc = NULL;
            items[i].drawFunc = NULL;
        } else {
            snprintf(text[i], 64, "读取存档：%d ", i);            
            items[i].triggerFunc =loadstatus;
            items[i].drawFunc = drawsnapshot; 
        }
		items[i].customData = (void*)(uintptr_t)i;
        items[i].text = text[i];
    }
    items[9].type = MIT_END;
    if (MenuRun(items, 30, 90, 60, 0, 0) == MR_LEAVE) return MR_LEAVE;
    return MR_NONE;
}

int enterSettingsMenu() {
    const struct MenuItem items[] = {
        /*{ MIT_BOOL, &Settings.Mute, "Mute Audio" },*/		
        //{ MIT_BOOL, &fullscreensetting,  "Full Screen" },
		{ MIT_INT32, &fullscreensetting, "全屏模式：", 0, 3, NULL, NULL,FullscreenMode},
        { MIT_BOOL8, &showFPSinformation,"FPS开关" },
		{ MIT_BOOL8, &superGBMode, "Super GB 开关" },
        { MIT_END }
    };
    if (MenuRun(items, 70, 180, 80, 0, 0) == MR_LEAVE) return MR_LEAVE;
    return MR_NONE;
}
int refreshcheat (const struct MenuItem* iteminfo)
{
	cheatrefresh = 1;
	return MR_NONE;
}

int savestatus(const struct MenuItem* iteminfo)
{	
	int slot = (int)(uintptr_t)iteminfo->customData;
	return savestatus4menu(mCoreThread_p, slot);
}
int loadstatus(const struct MenuItem* iteminfo)
{	
	int slot = (int)(uintptr_t)iteminfo->customData;
	return loadstatus4menu (mCoreThread_p, slot);
}


void drawsnapshot(const struct MenuItem* iteminfo, int y, bool8 selected)
{
	if (!selected) 
	{
		return;			
	}
	int slot = (int)(uintptr_t)iteminfo->customData;
	
	return drawsnapshot4menu(mCoreThread_p, slot, y);
}
bool8 checkslot(int i)
{		
	return checkslot4menu(mCoreThread_p, i);
}

struct MenuItemValue SolarValue(int value)
{
	const char *values[16] = {"已关闭", "1 ", "2 ", "3 ", "4 ", "5 ", "6 ", "7 ", "8 ", "9 ", "10","11", "12","13", "14", "15"};
	struct MenuItemValue ret={values[value], NULL};
    return ret;
}

struct MenuItemValue FullscreenMode(int value)
{
	const char *values[4] = {"已关闭", "锐利 ", "像素", "平滑"};
	struct MenuItemValue ret={values[value], NULL};
    return ret;
}
int updatedelay (const struct MenuItem* iteminfo)
{
	frameWritedelay = writedelay * 1000;
	frameReaddelay = readdelay * 1000;
	return MR_NONE;
}

int updaterepeatCount (const struct MenuItem* iteminfo)
{
	//int repeatrate = 0;
	//extern int repeatCount = 0;
	if(repeatrate >0) repeatCount = 60/repeatrate -2;
	else repeatCount = 0;	
	return MR_NONE;
}