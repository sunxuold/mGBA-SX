#pragma once

typedef unsigned char		bool8;

#include <unistd.h>

enum MenuItemType {
    MIT_END,    /* mark end of menu items list */
    MIT_STATIC, /* static text, nothing is triggered */
    MIT_CLICK,  /* click to trigger */
    MIT_BOOL8,  /* use left/right or ok button to toggle on/off, for type `bool8` */
    MIT_BOOL,   /* for type `bool` */
    MIT_INT8,   /* use left/right to change value, for type `(u)int8` */
    MIT_INT16,  /* for type `(u)int16` */
    MIT_INT32,  /* for type `(u)int32` */
};

enum MenuResult {
    MR_NONE = 0,    /* do nothing */
    MR_OK = 1,      /* return with selected index */
    MR_CANCEL = -1, /* return with cancelled */
    MR_LEAVE = -2,  /* close whole menu */
};

struct MenuItemValue {
    /* value as text, use original data if NULL */
    const char *text;

    /* comment text if not NULL */
    const char *comment;
};

struct MenuItem {
    enum MenuItemType type;

    /* pointer to value */
    void *value;

    /* display text */
    const char *text;

    /* min/max value */
    int valMin, valMax;

    /* called on click or value change if not NULL
     * See enum MenuResult for return value */
    
	
	//MenuResult (*triggerFunc)(const MenuItem*);
	int (*triggerFunc)(const struct MenuItem*);
	
    /* called when drawing item, used to draw extra things */
    void (*drawFunc)(const struct MenuItem*, int y, bool8 selected);

    /* translate value to text/comment if not NULL */
    struct MenuItemValue (*valueFunc)(int);

    void *customData;
};

/* Set pre-draw function */
void MenuSetPreDrawFunc(void (*func)());

/* items:   list of menu items, end with a item set type to MIT_END
 * x:         x offset of menu
 * valx:      x offset of displayed value
 * y:         y offset of menu
 * pageCount: count of max displayed items
 * index:     first selected index
 * returns:   MR_CANCEL/MR_LEAVE as comments in MenuResult, otherwise the index selected for OK */
int MenuRun(const struct MenuItem items[], int x, int valx, int y, int pageCount, int index);

extern int enterMainMenu(int index, void* info);
int enterSaveStateMenu();
int enterLoadStateMenu();
int enterSettingsMenu();
int enterCheatSettingMenu();

int savestatus(const struct MenuItem* iteminfo);
int loadstatus(const struct MenuItem* iteminfo);
int refreshcheat (const struct MenuItem* iteminfo);
int updatedelay (const struct MenuItem* iteminfo);
int updaterepeatCount (const struct MenuItem* iteminfo);

void drawsnapshot(const struct MenuItem* iteminfo, int y, bool8 selected);
bool8 checkslot(int i);
struct MenuItemValue SolarValue(int value);
struct MenuItemValue FullscreenMode(int value);