/* Copyright (c) 2013-2015 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "main.h"
#include "scaler.h"
#include "scaler.h"

#include <mgba/core/core.h>
#include <mgba/core/thread.h>
#include <mgba/core/version.h>
#include <mgba-util/arm-algo.h>

//sunxu added for GKD350
#include <sys/time.h>
#include <stdint.h>
#include <sys/resource.h>
#include <sys/types.h>
#include "menu.h"

#define likely(x) __builtin_expect(!!(x), 1) //gcc内置函数, 帮助编译器分支优化
#define unlikely(x) __builtin_expect(!!(x), 0)
#define  GBA_SCREEN_HEIGHT 160
#define  GBA_SCREEN_WIDTH 240
//#define  SCREEN_WIDTH 320
//#define  SCREEN_HEIGHT 240
//#define  GBAScreenPitch 480
//u16 * GBAScreen =NULL;
#define likely(x) __builtin_expect(!!(x), 1) //gcc内置函数, 帮助编译器分支优化
#define unlikely(x) __builtin_expect(!!(x), 0)

/***************************************************************************
 *   Scaler copyright (C) 2013 by Paul Cercueil                            *
 *   paul@crapouillou.net                                                  *
 ***************************************************************************/
#define bgr555_to_rgb565(px) (((px & 0x7c007c00) >> 10) | ((px & 0x03e003e0) << 1)| ((px & 0x001f001f) << 11))
// static inline uint32_t bgr555_to_rgb565(uint32_t px)
// {
	// return ((px & 0x7c007c00) >> 10)
	  // | ((px & 0x03e003e0) << 1)
	  // | ((px & 0x001f001f) << 11);
// }

/***************************************************************************
 *   16-bit I/O version used by the sub-pixel and bilinear scalers         *
 *   (C) 2013 kuwanger                                                     *
 ***************************************************************************/
#define bgr555_to_rgb565_16(px) (((px & 0x7c00) >> 10) | ((px & 0x03e0) << 1) | ((px & 0x001f) << 11))
// static inline uint16_t bgr555_to_rgb565_16(uint16_t px)
// {
	// return ((px & 0x7c00) >> 10)
	  // | ((px & 0x03e0) << 1)
	  // | ((px & 0x001f) << 11);
// }
#define bgr555_to_native(px) (px)
#define bgr555_to_native_16(px) (px)

//#define bgr555_to_native bgr555_to_rgb565
//#define bgr555_to_native_16 bgr555_to_rgb565_16


#define MAGIC_VAL1 0xF7DEu
#define MAGIC_VAL2 0x0821u
#define MAGIC_VAL3 0xE79Cu
#define MAGIC_VAL4 0x1863u
#define MAGIC_DOUBLE(n) ((n << 16) | n)
#define MAGIC_VAL1D MAGIC_DOUBLE(MAGIC_VAL1)
#define MAGIC_VAL2D MAGIC_DOUBLE(MAGIC_VAL2)
#define Average(A, B) ((((A) & MAGIC_VAL1) >> 1) + (((B) & MAGIC_VAL1) >> 1) + ((A) & (B) & MAGIC_VAL2))

/* Calculates the average of two pairs of RGB565 pixels. The result is, in
 * the lower bits, the average of both lower pixels, and in the upper bits,
 * the average of both upper pixels. */
#define Average32(A, B) ((((A) & MAGIC_VAL1D) >> 1) + (((B) & MAGIC_VAL1D) >> 1) + ((A) & (B) & MAGIC_VAL2D))

/* Raises a pixel from the lower half to the upper half of a pair. */
#define Raise(N) ((N) << 16)

/* Extracts the upper pixel of a pair into the lower pixel of a pair. */
#define Hi(N) ((N) >> 16)

/* Extracts the lower pixel of a pair. */
#define Lo(N) ((N) & 0xFFFF)

/* Calculates the average of two RGB565 pixels. The source of the pixels is
 * the lower 16 bits of both parameters. The result is in the lower 16 bits.
 * The average is weighted so that the first pixel contributes 3/4 of its
 * color and the second pixel contributes 1/4. */
#define AverageQuarters3_1(A, B) ( (((A) & MAGIC_VAL1) >> 1) + (((A) & MAGIC_VAL3) >> 2) + (((B) & MAGIC_VAL3) >> 2) + ((( (( ((A) & MAGIC_VAL4) + ((A) & MAGIC_VAL2) ) << 1) + ((B) & MAGIC_VAL4) ) >> 2) & MAGIC_VAL4) )


#define RED_FROM_NATIVE(rgb565) (((rgb565) >> 11) & 0x1F)
#define RED_TO_NATIVE(r) (((r) & 0x1F) << 11)
#define GREEN_FROM_NATIVE(rgb565) (((rgb565) >> 5) & 0x3F)
#define GREEN_TO_NATIVE(g) (((g) & 0x3F) << 5)
#define BLUE_FROM_NATIVE(rgb565) ((rgb565) & 0x1F)
#define BLUE_TO_NATIVE(b) ((b) & 0x1F)


/*
 * Blends, with sub-pixel accuracy, 3/4 of the first argument with 1/4 of the
 * second argument. The pixel format of both arguments is RGB 565. The first
 * pixel is assumed to be to the left of the second pixel.
 */
static inline uint16_t SubpixelRGB3_1(uint16_t A, uint16_t B)
{
	return RED_TO_NATIVE(RED_FROM_NATIVE(A))
	     | GREEN_TO_NATIVE(GREEN_FROM_NATIVE(A) * 3 / 4 + GREEN_FROM_NATIVE(B) * 1 / 4)
	     | BLUE_TO_NATIVE(BLUE_FROM_NATIVE(A) * 1 / 4 + BLUE_FROM_NATIVE(B) * 3 / 4);
}

/*
 * Blends, with sub-pixel accuracy, 1/2 of the first argument with 1/2 of the
 * second argument. The pixel format of both arguments is RGB 565. The first
 * pixel is assumed to be to the left of the second pixel.
 */
static inline uint16_t SubpixelRGB1_1(uint16_t A, uint16_t B)
{
	return RED_TO_NATIVE(RED_FROM_NATIVE(A) * 3 / 4 + RED_FROM_NATIVE(B) * 1 / 4)
	     | GREEN_TO_NATIVE(GREEN_FROM_NATIVE(A) * 1 / 2 + GREEN_FROM_NATIVE(B) * 1 / 2)
	     | BLUE_TO_NATIVE(BLUE_FROM_NATIVE(A) * 1 / 4 + BLUE_FROM_NATIVE(B) * 3 / 4);
}

/*
 * Blends, with sub-pixel accuracy, 1/4 of the first argument with 3/4 of the
 * second argument. The pixel format of both arguments is RGB 565. The first
 * pixel is assumed to be to the left of the second pixel.
 */
static inline uint16_t SubpixelRGB1_3(uint16_t A, uint16_t B)
{
	return RED_TO_NATIVE(RED_FROM_NATIVE(B) * 1 / 4 + RED_FROM_NATIVE(A) * 3 / 4)
	     | GREEN_TO_NATIVE(GREEN_FROM_NATIVE(B) * 3 / 4 + GREEN_FROM_NATIVE(A) * 1 / 4)
	     | BLUE_TO_NATIVE(BLUE_FROM_NATIVE(B));
}

//static TTF::Font *ttf_font = NULL;


extern struct timeval last, tval;
extern int fullscreensetting;  
struct timeval tv_last_ru_utime, tv_last_ru_stime;
unsigned mFps=0, mLastTimer = 0;
unsigned newTimer=0;
unsigned mainwidth = 0, mainhigh = 0, vedioWidth=0, vedioHigh=0;  
SDL_Surface *mScreen = NULL;   
struct mSDLRenderer* mRenderer = NULL; 
s8 mFpsDisplay[16]={""};
int srcpitch, posOffset;

//uint8_t*  gbabuffer=NULL;

#define SAL_RGB(r,g,b) (u16)((r) << 11 | (g) << 6 | (b))
u32 mFont8x8[]= {
0x0,0x0,0xc3663c18,0x3c2424e7,0xe724243c,0x183c66c3,0xc16f3818,0x18386fc1,0x83f61c18,0x181cf683,0xe7c3993c,0x3c99c3,0x3f7fffff,0xe7cf9f,0x3c99c3e7,0xe7c399,0x3160c080,0x40e1b,0xcbcbc37e,0x7ec3c3db,0x3c3c3c18,0x81c087e,0x8683818,0x60f0e08,0x81422418,0x18244281,0xbd5a2418,0x18245abd,0x818181ff,0xff8181,0xa1c181ff,0xff8995,0x63633e,0x3e6363,0x606060,0x606060,0x3e60603e,0x3e0303,0x3e60603e,0x3e6060,0x3e636363,0x606060,0x3e03033e,0x3e6060,0x3e03033e,0x3e6363,
0x60603e,0x606060,0x3e63633e,0x3e6363,0x3e63633e,0x3e6060,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x18181818,0x180018,0x666666,0x0,0x367f3600,0x367f36,0x3c067c18,0x183e60,0x18366600,0x62660c,0xe1c361c,0x6e337b,0x181818,0x0,0x18183870,0x703818,0x18181c0e,0xe1c18,0xff3c6600,0x663c,0x7e181800,0x1818,0x0,0x60c0c00,0x7e000000,0x0,0x0,0x181800,0x18306040,0x2060c,0x6e76663c,0x3c6666,0x18181c18,0x7e1818,0x3060663c,0x7e0c18,0x3018307e,0x3c6660,
0x363c3830,0x30307e,0x603e067e,0x3c6660,0x3e06063c,0x3c6666,0x1830607e,0xc0c0c,0x3c66663c,0x3c6666,0x7c66663c,0x1c3060,0x181800,0x1818,0x181800,0xc1818,0xc183060,0x603018,0x7e0000,0x7e00,0x30180c06,0x60c18,0x3060663c,0x180018,0x5676663c,0x7c0676,0x66663c18,0x66667e,0x3e66663e,0x3e6666,0x606663c,0x3c6606,0x6666361e,0x1e3666,0x3e06067e,0x7e0606,0x3e06067e,0x60606,0x7606067c,0x7c6666,0x7e666666,0x666666,0x1818183c,0x3c1818,0x60606060,0x3c6660,0xe1e3666,
0x66361e,0x6060606,0x7e0606,0x6b7f7763,0x636363,0x7e7e6e66,0x666676,0x6666663c,0x3c6666,0x3e66663e,0x60606,0x6666663c,0x6c366e,0x3e66663e,0x666636,0x3c06663c,0x3c6660,0x1818187e,0x181818,0x66666666,0x7c6666,0x66666666,0x183c66,0x6b636363,0x63777f,0x183c6666,0x66663c,0x3c666666,0x181818,0x1830607e,0x7e060c,0x18181878,0x781818,0x180c0602,0x406030,0x1818181e,0x1e1818,0x63361c08,0x0,0x0,0x7f0000,0xc060300,0x0,0x603c0000,0x7c667c,0x663e0606,0x3e6666,0x63c0000,
0x3c0606,0x667c6060,0x7c6666,0x663c0000,0x3c067e,0xc3e0c38,0xc0c0c,0x667c0000,0x3e607c66,0x663e0606,0x666666,0x181c0018,0x3c1818,0x18180018,0xe181818,0x36660606,0x66361e,0x1818181c,0x3c1818,0x7f370000,0x63636b,0x663e0000,0x666666,0x663c0000,0x3c6666,0x663e0000,0x63e6666,0x667c0000,0x607c6666,0x663e0000,0x60606,0x67c0000,0x3e603c,0x187e1800,0x701818,0x66660000,0x7c6666,0x66660000,0x183c66,0x63630000,0x363e6b,0x3c660000,0x663c18,0x66660000,0x3e607c66,0x307e0000,0x7e0c18,0xc181870,0x701818,0x18181818,0x18181818,0x3018180e,0xe1818,0x794f0600,0x30};
//sunxu added end

static bool mSDLSWInit(struct mSDLRenderer* renderer);
static void mSDLSWRunloop(struct mSDLRenderer* renderer, void* user);
static void mSDLSWDeinit(struct mSDLRenderer* renderer);




static void sal_VideoDrawRect(s32 x, s32 y, s32 width, s32 height, u16 color)
{
	u16 *pixy = (u16*)mScreen->pixels;
	u16 *pixx;
	s32 h,w;
	pixy = ((u16*) ((u8*) pixy + y * mScreen->pitch)) + x;
	for(h=0;h<height;h++)
	{
		pixx=pixy;		
		for(w=0; w<width; w++)
		{
			*pixx++ = color;
		}
		pixy = (u16*) ((u8*) pixy + mScreen->pitch);
	}
}

static void sal_VideoPrint(s32 x, s32 y, const char *buffer, u16 color)
{
	s32 m,b;
	u16 *pix = (u16*)mScreen->pixels;
	s32 len=0;
	s32 maxLen=(GKD350_WIDTH>>3)-(x>>3);

	pix = ((u16*) ((u8*) pix + y * mScreen->pitch)) + x;
	while(1) 
	{
		s8 letter = *buffer++;
		u32 *offset;

		//Check for end of string
		if (letter == 0) break;

		//Get pointer to graphics for letter
		offset=mFont8x8+((letter)<<1);
		
		//read first 32bits of char pixel mask data
		for (m=0; m<2; m++)
		{
			u32 mask = *offset++;

			//process 32bits of data in 8bit chunks
			for (b=0; b<4; b++)
			{
				if(mask&(1<<0)) pix[0] = color;
				if(mask&(1<<1)) pix[1] = color;
				if(mask&(1<<2)) pix[2] = color;
				if(mask&(1<<3)) pix[3] = color;
				if(mask&(1<<4)) pix[4] = color;
				if(mask&(1<<5)) pix[5] = color;
				if(mask&(1<<6)) pix[6] = color;
				if(mask&(1<<7)) pix[7] = color;
				pix=(u16*) ((u8*) pix +  mScreen->pitch); //move to next line
				mask>>=8; //shift mask data ready for next loop
			}
		}
		//position pix pointer to start of next char
		pix = (u16*) ((u8*) pix - ( mScreen->pitch << 3)) + (1 << 3);
		len++;
		if (len>=maxLen-1) break;
	}
}

void VideoClear() {
    memset(mScreen->pixels, 0, mScreen->pitch * mScreen->h);
}

void VideoFlip() {
	SDL_UnlockSurface(mScreen);
    SDL_Flip(mScreen);
    SDL_LockSurface(mScreen);
}




/*
void VideoOutputStringPixel(int x, int y, const char *text, bool allowWrap, bool shadow) {
    static const uint8_t font8x8data[128][8] = {
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x3E, 0x41, 0x55, 0x41, 0x55, 0x49, 0x3E },
        { 0x00, 0x3E, 0x7F, 0x6B, 0x7F, 0x6B, 0x77, 0x3E },
        { 0x00, 0x22, 0x77, 0x7F, 0x7F, 0x3E, 0x1C, 0x08 },
        { 0x00, 0x08, 0x1C, 0x3E, 0x7F, 0x3E, 0x1C, 0x08 },
        { 0x00, 0x08, 0x1C, 0x2A, 0x7F, 0x2A, 0x08, 0x1C },
        { 0x00, 0x08, 0x1C, 0x3E, 0x7F, 0x3E, 0x08, 0x1C },
        { 0x00, 0x00, 0x1C, 0x3E, 0x3E, 0x3E, 0x1C, 0x00 },
        { 0xFF, 0xFF, 0xE3, 0xC1, 0xC1, 0xC1, 0xE3, 0xFF },
        { 0x00, 0x00, 0x1C, 0x22, 0x22, 0x22, 0x1C, 0x00 },
        { 0xFF, 0xFF, 0xE3, 0xDD, 0xDD, 0xDD, 0xE3, 0xFF },
        { 0x00, 0x0F, 0x03, 0x05, 0x39, 0x48, 0x48, 0x30 },
        { 0x00, 0x08, 0x3E, 0x08, 0x1C, 0x22, 0x22, 0x1C },
        { 0x00, 0x18, 0x14, 0x10, 0x10, 0x30, 0x70, 0x60 },
        { 0x00, 0x0F, 0x19, 0x11, 0x13, 0x37, 0x76, 0x60 },
        { 0x00, 0x08, 0x2A, 0x1C, 0x77, 0x1C, 0x2A, 0x08 },
        { 0x00, 0x60, 0x78, 0x7E, 0x7F, 0x7E, 0x78, 0x60 },
        { 0x00, 0x03, 0x0F, 0x3F, 0x7F, 0x3F, 0x0F, 0x03 },
        { 0x00, 0x08, 0x1C, 0x2A, 0x08, 0x2A, 0x1C, 0x08 },
        { 0x00, 0x66, 0x66, 0x66, 0x66, 0x00, 0x66, 0x66 },
        { 0x00, 0x3F, 0x65, 0x65, 0x3D, 0x05, 0x05, 0x05 },
        { 0x00, 0x0C, 0x32, 0x48, 0x24, 0x12, 0x4C, 0x30 },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x7F, 0x7F },
        { 0x00, 0x08, 0x1C, 0x2A, 0x08, 0x2A, 0x1C, 0x3E },
        { 0x00, 0x08, 0x1C, 0x3E, 0x7F, 0x1C, 0x1C, 0x1C },
        { 0x00, 0x1C, 0x1C, 0x1C, 0x7F, 0x3E, 0x1C, 0x08 },
        { 0x00, 0x08, 0x0C, 0x7E, 0x7F, 0x7E, 0x0C, 0x08 },
        { 0x00, 0x08, 0x18, 0x3F, 0x7F, 0x3F, 0x18, 0x08 },
        { 0x00, 0x00, 0x00, 0x70, 0x70, 0x70, 0x7F, 0x7F },
        { 0x00, 0x00, 0x14, 0x22, 0x7F, 0x22, 0x14, 0x00 },
        { 0x00, 0x08, 0x1C, 0x1C, 0x3E, 0x3E, 0x7F, 0x7F },
        { 0x00, 0x7F, 0x7F, 0x3E, 0x3E, 0x1C, 0x1C, 0x08 },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18 },
        { 0x00, 0x36, 0x36, 0x14, 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36 },
        { 0x00, 0x08, 0x1E, 0x20, 0x1C, 0x02, 0x3C, 0x08 },
        { 0x00, 0x60, 0x66, 0x0C, 0x18, 0x30, 0x66, 0x06 },
        { 0x00, 0x3C, 0x66, 0x3C, 0x28, 0x65, 0x66, 0x3F },
        { 0x00, 0x18, 0x18, 0x18, 0x30, 0x00, 0x00, 0x00 },
        { 0x00, 0x60, 0x30, 0x18, 0x18, 0x18, 0x30, 0x60 },
        { 0x00, 0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06 },
        { 0x00, 0x00, 0x36, 0x1C, 0x7F, 0x1C, 0x36, 0x00 },
        { 0x00, 0x00, 0x08, 0x08, 0x3E, 0x08, 0x08, 0x00 },
        { 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x30, 0x60 },
        { 0x00, 0x00, 0x00, 0x00, 0x3C, 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x60, 0x60 },
        { 0x00, 0x00, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x00 },
        { 0x00, 0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C },
        { 0x00, 0x18, 0x18, 0x38, 0x18, 0x18, 0x18, 0x7E },
        { 0x00, 0x3C, 0x66, 0x06, 0x0C, 0x30, 0x60, 0x7E },
        { 0x00, 0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C },
        { 0x00, 0x0C, 0x1C, 0x2C, 0x4C, 0x7E, 0x0C, 0x0C },
        { 0x00, 0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C },
        { 0x00, 0x3C, 0x66, 0x60, 0x7C, 0x66, 0x66, 0x3C },
        { 0x00, 0x7E, 0x66, 0x0C, 0x0C, 0x18, 0x18, 0x18 },
        { 0x00, 0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C },
        { 0x00, 0x3C, 0x66, 0x66, 0x3E, 0x06, 0x66, 0x3C },
        { 0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00 },
        { 0x00, 0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x30 },
        { 0x00, 0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06 },
        { 0x00, 0x00, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x00 },
        { 0x00, 0x60, 0x30, 0x18, 0x0C, 0x18, 0x30, 0x60 },
        { 0x00, 0x3C, 0x66, 0x06, 0x1C, 0x18, 0x00, 0x18 },
        { 0x00, 0x38, 0x44, 0x5C, 0x58, 0x42, 0x3C, 0x00 },
        { 0x00, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66 },
        { 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C },
        { 0x00, 0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C },
        { 0x00, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x7C },
        { 0x00, 0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x7E },
        { 0x00, 0x7E, 0x60, 0x60, 0x7C, 0x60, 0x60, 0x60 },
        { 0x00, 0x3C, 0x66, 0x60, 0x60, 0x6E, 0x66, 0x3C },
        { 0x00, 0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66 },
        { 0x00, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C },
        { 0x00, 0x1E, 0x0C, 0x0C, 0x0C, 0x6C, 0x6C, 0x38 },
        { 0x00, 0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66 },
        { 0x00, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E },
        { 0x00, 0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63 },
        { 0x00, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x63, 0x63 },
        { 0x00, 0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C },
        { 0x00, 0x7C, 0x66, 0x66, 0x66, 0x7C, 0x60, 0x60 },
        { 0x00, 0x3C, 0x66, 0x66, 0x66, 0x6E, 0x3C, 0x06 },
        { 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x78, 0x6C, 0x66 },
        { 0x00, 0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C },
        { 0x00, 0x7E, 0x5A, 0x18, 0x18, 0x18, 0x18, 0x18 },
        { 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3E },
        { 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18 },
        { 0x00, 0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63 },
        { 0x00, 0x63, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x63 },
        { 0x00, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18 },
        { 0x00, 0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E },
        { 0x00, 0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E },
        { 0x00, 0x00, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x00 },
        { 0x00, 0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78 },
        { 0x00, 0x08, 0x14, 0x22, 0x41, 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7F },
        { 0x00, 0x0C, 0x0C, 0x06, 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E },
        { 0x00, 0x60, 0x60, 0x60, 0x7C, 0x66, 0x66, 0x7C },
        { 0x00, 0x00, 0x00, 0x3C, 0x66, 0x60, 0x66, 0x3C },
        { 0x00, 0x06, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3E },
        { 0x00, 0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C },
        { 0x00, 0x1C, 0x36, 0x30, 0x30, 0x7C, 0x30, 0x30 },
        { 0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x3C },
        { 0x00, 0x60, 0x60, 0x60, 0x7C, 0x66, 0x66, 0x66 },
        { 0x00, 0x00, 0x18, 0x00, 0x18, 0x18, 0x18, 0x3C },
        { 0x00, 0x0C, 0x00, 0x0C, 0x0C, 0x6C, 0x6C, 0x38 },
        { 0x00, 0x60, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0x66 },
        { 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18 },
        { 0x00, 0x00, 0x00, 0x63, 0x77, 0x7F, 0x6B, 0x6B },
        { 0x00, 0x00, 0x00, 0x7C, 0x7E, 0x66, 0x66, 0x66 },
        { 0x00, 0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C },
        { 0x00, 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60 },
        { 0x00, 0x00, 0x3C, 0x6C, 0x6C, 0x3C, 0x0D, 0x0F },
        { 0x00, 0x00, 0x00, 0x7C, 0x66, 0x66, 0x60, 0x60 },
        { 0x00, 0x00, 0x00, 0x3E, 0x40, 0x3C, 0x02, 0x7C },
        { 0x00, 0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x18 },
        { 0x00, 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3E },
        { 0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x3C, 0x18 },
        { 0x00, 0x00, 0x00, 0x63, 0x6B, 0x6B, 0x6B, 0x3E },
        { 0x00, 0x00, 0x00, 0x66, 0x3C, 0x18, 0x3C, 0x66 },
        { 0x00, 0x00, 0x00, 0x66, 0x66, 0x3E, 0x06, 0x3C },
        { 0x00, 0x00, 0x00, 0x3C, 0x0C, 0x18, 0x30, 0x3C },
        { 0x00, 0x0E, 0x18, 0x18, 0x30, 0x18, 0x18, 0x0E },
        { 0x00, 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18 },
        { 0x00, 0x70, 0x18, 0x18, 0x0C, 0x18, 0x18, 0x70 },
        { 0x00, 0x00, 0x00, 0x3A, 0x6C, 0x00, 0x00, 0x00 },
        { 0x00, 0x08, 0x1C, 0x36, 0x63, 0x41, 0x41, 0x7F }
    };
    uint32_t wrapx = GKD350_WIDTH - 8;
    while (*text) {
        int pos = 0;
        uint8_t c = *text++;
        if (c > 0x7F) continue;
        uint16_t *ptr = (uint16_t *)mScreen->pixels + x + y*GKD350_WIDTH;
        const unsigned char *dataptr = font8x8data[c];
        for (int l = 0; l < 8; l++) {
            unsigned char data = *dataptr++;
            if (shadow) {
                if (data & 0x80u) {
                    ptr[pos + 0] = 0xFFFFu;
                    ptr[pos + 1 + GKD350_WIDTH] = 0;
                }
                if (data & 0x40u) {
                    ptr[pos + 1] = 0xFFFFu;
                    ptr[pos + 2 + GKD350_WIDTH] = 0;
                }
                if (data & 0x20u) {
                    ptr[pos + 2] = 0xFFFFu;
                    ptr[pos + 3 + GKD350_WIDTH] = 0;
                }
                if (data & 0x10u) {
                    ptr[pos + 3] = 0xFFFFu;
                    ptr[pos + 4 + GKD350_WIDTH] = 0;
                }
                if (data & 0x08u) {
                    ptr[pos + 4] = 0xFFFFu;
                    ptr[pos + 5 + GKD350_WIDTH] = 0;
                }
                if (data & 0x04u) {
                    ptr[pos + 5] = 0xFFFFu;
                    ptr[pos + 6 + GKD350_WIDTH] = 0;
                }
                if (data & 0x02u) {
                    ptr[pos + 6] = 0xFFFFu;
                    ptr[pos + 7 + GKD350_WIDTH] = 0;
                }
                if (data & 0x01u) {
                    ptr[pos + 7] = 0xFFFFu;
                    ptr[pos + 8 + GKD350_WIDTH] = 0;
                }
            } else {
                if (data & 0x80u) ptr[pos + 0] = 0xFFFFu;
                if (data & 0x40u) ptr[pos + 1] = 0xFFFFu;
                if (data & 0x20u) ptr[pos + 2] = 0xFFFFu;
                if (data & 0x10u) ptr[pos + 3] = 0xFFFFu;
                if (data & 0x08u) ptr[pos + 4] = 0xFFFFu;
                if (data & 0x04u) ptr[pos + 5] = 0xFFFFu;
                if (data & 0x02u) ptr[pos + 6] = 0xFFFFu;
                if (data & 0x01u) ptr[pos + 7] = 0xFFFFu;
            }
            pos += GKD350_WIDTH;
        }
        x += 8;
        if (x > wrapx) {
            if (!allowWrap) break;
            x = 0;
            y += 12;
        }
    }
}
*/

void VideoDrawImage(int x, int y, uint32_t*data ,int w, int h, int strchlen) {
    uint16_t *__restrict__ ptr = data;
    if (!ptr) return;
    int linebytes = w * 2;
    int pitch = mScreen->pitch / mScreen->format->BytesPerPixel;
    uint16_t *__restrict__ out = (uint16_t*)mScreen->pixels + y * pitch + x;
    for (int j = h; j; --j) {
        memmove(out, ptr, linebytes);
        out += pitch;
        ptr += strchlen;
    }
}


#include <SDL/SDL_ttf.h>
#include <mgba-util/string.h>
TTF_Font *font=NULL;
void  Fontrender(int x, int y, char *text, int allowWrap, int shadow)
{	
	
	uint32_t wrapx = (GKD350_WIDTH - 8)*2;
	char info[128];
	//初始化字体库
	if( TTF_Init() == -1 )
	{
		printf("fount init fail!\n");
		return ;
	}
	
	//打开字体
	if(!font)
	{
		font=TTF_OpenFont("./font.ttf", 12);// 高度12
	}	
	if(!font)
	{ 
		printf("TTF_OpenFont: Open font.ttf %s\n", TTF_GetError());
		return ; 
	}	
	SDL_Color color2={255,255,255};
	int len = 0, flag = 0, isNotUTF8=0;
	
	int nBytes = 0;////UTF8可用1-6个字节编码,ASCII用一个字节
    unsigned char ch = 0;
	do{
		
		if (text[len]!= '\0')
		{
			ch = text[len];
			if(nBytes == 0)
			{
				if((ch & 0x80) != 0)
				{
					while((ch & 0x80) != 0)
					{
						ch <<= 1;
						nBytes ++;
					}
					if((nBytes < 2) || (nBytes > 6))
					{
						isNotUTF8 =1;
					}
					nBytes --;
				}
			}
			else
			{
				if((ch & 0xc0) != 0x80)
				{
					isNotUTF8 =1;
				}
				nBytes --;
			}
			++len;
		}
		else
		{
			if(isNotUTF8 == 0) isNotUTF8 = !(nBytes == 0);
			break;
		}
	}while(1);
				
	SDL_Surface *text_surface = NULL;
	//printf("put menu iinfo %s  len is %d \n", text, len);
	
	if(isNotUTF8==0)
	{
		text_surface = TTF_RenderUTF8_Solid(font, text, color2);
		//printf("UTF8 menu info %s length %d \n", text, text_surface->w);
	}
	else
	{
		char* utf8string = gbkToUtf8(text, (len+len%2));
		text_surface = TTF_RenderUTF8_Solid(font, utf8string, color2);
		//printf("decode menu info %s length %d \n", utf8string, text_surface->w);
		free(utf8string);
	
	}	
	
	
	
	
	if (text_surface != NULL)
	{
		//text_surface info 1 ,8 --- 96 ,----W96 * H13 
		
		/*
		printf("text_surface info %d ,%d --- %d ,----W%d * H%d \n", 
		            text_surface->format->BytesPerPixel,
					text_surface->format->BitsPerPixel,
					text_surface->pitch,
					text_surface->w,
					text_surface->h);
		*/
		
		//sal_VideoDrawRect(x,y,GKD350_WIDTH, GKD350_HEIGHT,(u16)SAL_RGB(0,0,0));
		u16 pointcolor = SAL_RGB(31,31,31);
		u16 backgroundcolor = SAL_RGB(0,0,0);
		
		u8 *src = (u8*)text_surface->pixels;
		
		uint16_t  *desc = (uint16_t*)mScreen->pixels;
		
		desc = ((u16*) ((u8*) desc + y * mScreen->pitch)) + x;
		
		if (text_surface->w%4) flag=2;
		len = text_surface->w-flag;
		
		if (text_surface->w > (wrapx -x*2)) len = wrapx -x*2;
		
		for (int i=0;i< text_surface->h; ++i)
		{
			for (int j =0;j<(len); ++j)			
			{	

				if(src[(i)* (text_surface->w+flag) +j] >0) 
					desc[(i)* mScreen->w+j]=pointcolor;
				else
					desc[(i)* mScreen->w+j] = backgroundcolor;					

			}
			//printf("address %d info src%d , des%d \n",i,  &src[(i)* text_surface->w], &desc[(i)* mScreen->w] );
		}		

	} 
	else    
	{
		printf("oupput ttf error \n");
	}
	SDL_FreeSurface(text_surface);
	
}


static inline suseconds_t tvdiff_usec(const struct timeval tv, const struct timeval tv_old)
{
  return (tv.tv_sec - tv_old.tv_sec) * 1000000 + tv.tv_usec - tv_old.tv_usec;
}

static int pmonGetCpuUsage()
{
  struct rusage ru;
  suseconds_t diff = tvdiff_usec(tval, last);
  
  if (diff == 0 || getrusage(RUSAGE_SELF, &ru) < 0)
    return 0;
  suseconds_t usecs_used = tvdiff_usec(ru.ru_utime, tv_last_ru_utime) +
                           tvdiff_usec(ru.ru_stime, tv_last_ru_stime);
  tv_last_ru_utime = ru.ru_utime;
  tv_last_ru_stime = ru.ru_stime;
  return (int)(float)(100 * usecs_used) / (float)diff;
}

//sunxu added end


void mSDLSWCreate(struct mSDLRenderer* renderer) {
	renderer->init = mSDLSWInit;
	renderer->deinit = mSDLSWDeinit;
	renderer->runloop = mSDLSWRunloop;
}

bool mSDLSWInit(struct mSDLRenderer* renderer) {
#ifdef COLOR_16_BIT
	//sunxu mdoify
	//mScreen = SDL_SetVideoMode(GKD350_WIDTH, GKD350_HEIGHT, 16, SDL_HWSURFACE|SDL_DOUBLEBUF|SDL_FULLSCREEN);	
	mScreen = SDL_SetVideoMode(GKD350_WIDTH, GKD350_HEIGHT, 16, SDL_HWSURFACE | SDL_DOUBLEBUF);//SDL_TRIPLEBUF);
	if( mScreen == NULL )  	
		return false;    	
	//sunxu modify complete
	
#else
	SDL_SetVideoMode(renderer->viewportWidth, renderer->viewportHeight, 32, SDL_DOUBLEBUF | SDL_HWSURFACE | (SDL_FULLSCREEN * renderer->player.fullscreen));
#endif
	SDL_WM_SetCaption(projectName, ""); 

	
	
	//SDL_Surface* surface = SDL_GetVideoSurface();  
	SDL_LockSurface(mScreen);     
	mRenderer = renderer;
	
	renderer->core->desiredVideoDimensions(renderer->core, &mainwidth, &mainhigh);
	//if (renderer->ratio == 1) {
	//void *startpixels = surface->pixels+ (GKD350_WIDTH-mainwidth) + (GKD350_HEIGHT-mainhigh)* surface->pitch/2;
	//renderer->core->setVideoBuffer(renderer->core, startpixels, surface->pitch / BYTES_PER_PIXEL);
	renderer->outputBuffer = malloc(mainwidth * mainhigh * BYTES_PER_PIXEL);
	//gbabuffer = malloc(240*160*2);
	renderer->core->setVideoBuffer(renderer->core, renderer->outputBuffer, mainwidth);
	return true;
}

void mSDLSWRunloop(struct mSDLRenderer* renderer, void* user) {
	struct mCoreThread* context = user;
	SDL_Event event;
	//SDL_Surface* surface = SDL_GetVideoSurface();	
	context->impl->sync.videoFrameOn = true; //sunxu added for sync frame
	
	
	// thread remove 
	//mNoThreadBeforeGameRoutine(context);		
			
	while (mCoreThreadIsActive(context)) {

		while (SDL_PollEvent(&event)) {
			mSDLHandleEvent(context, &renderer->player, &event);
		}
				
		// thread remove  begin
		//mNoThreadGameRoutine(context);
		
				
		if(context->impl->state == THREAD_RUN_ON)
		{
			if(context->run) 
			{
				context->run(context);
				context->impl->state = context->impl->savedState;
			}
		}					
		//while (mainimpl->state <= THREAD_MAX_RUNNING) {
		context->core->runFrame(context->core);
		//maincore->runLoop(maincore);

		
		/*
		if (mainimpl->sync.audioWait) {
			mCoreSyncLockAudio(&mainimpl->sync);
			mCoreSyncProduceAudio(&mainimpl->sync, maincore->getAudioChannel(maincore, 0), maincore->getAudioBufferSize(maincore));
		}
		*/
			
		// thread remove  end 
		
		
		// thread remove  if (mCoreSyncWaitFrameStart(&context->impl->sync)) {
	
#ifdef USE_PIXMAN
			if(!vedioWidth && !vedioHigh)
			{ 
				renderer->core->desiredVideoDimensions(renderer->core, &vedioWidth, &vedioHigh);		
				srcpitch=vedioWidth*2;
				//posOffsetS = (GKD350_WIDTH-160) + (GKD350_HEIGHT-144)* mScreen->pitch/2;
				posOffset = (GKD350_WIDTH-vedioWidth) + (GKD350_HEIGHT-vedioHigh)* mScreen->pitch/2;				
				printf("vedioWidth %d vedioHigh %d\n", vedioWidth, vedioHigh);
			}
			
			refreshscreen();
			
#else
			switch (renderer->ratio) {
#if defined(__ARM_NEON) && COLOR_16_BIT
			case 2:
				_neon2x(mScreen->pixels, renderer->outputBuffer, width, height);
				break;
			case 4:
				_neon4x(mScreen->pixels, renderer->outputBuffer, width, height);
				break;
#endif
			case 1:
				break;
			default:
				abort();
			}
#endif				
				//sunxu  add FPS  show
				if (showFPSinformation) 
				{
					mFps++;
					if(mFps==1)
					{
						newTimer=sal_TimerRead(&tval, &last, 1);
						mLastTimer=newTimer;
					}
					else
					{
						if (mFps>10)
						{
							newTimer=sal_TimerRead(&tval,&last,0);
							if(newTimer-mLastTimer>60) //renderer->core->config FPS=60
							{
								mLastTimer=newTimer;
								int cpuusage = pmonGetCpuUsage();
								//sprintf(mFpsDisplay,"%2d/%2d/%6.1f", mFps,60,cpuusage); //FPS=60
								if(cpuusage >200) cpuusage=200;
								
								sprintf(mFpsDisplay,"%2d/%2d/%2d/%3d", mFps,minframe,soundframe,cpuusage);
								soundframe=0;
								minframe=0;								
								//sprintf(mFpsDisplay,"%2d/%2d/%2d", tempw,temph,temppitch);//,tempw,temph,Width,Height);  sunxu test
								mFps=0;								
								sal_VideoDrawRect(0,0,16*8,8,(u16)SAL_RGB(0,0,0));
								sal_VideoPrint(0,0,mFpsDisplay,(u16)SAL_RGB(31,31,31));
								
								/*
								if(renderer->core->platform(renderer->core) == PLATFORM_GB)
								{
									struct GB *checkinfo = (struct GB *)(renderer->core->board);
									int mm = checkinfo->model;
									printf("frame count %d \n", mm);
								}
								*/
								//int width,high;
								//renderer->core->desiredVideoDimensions(renderer->core, &width, &high);		
								//printf("mainwidth %d mainhigh %d\n", width, high);
							}					

						}
					}									
				}
				// FPS show complete
				
				SDL_UnlockSurface(mScreen); 
				SDL_Flip(mScreen);
				SDL_LockSurface(mScreen);

		// thread remove  }
		//mCoreSyncWaitFrameEnd(&context->impl->sync);
	}
	
	// thread remove  
	//mNoThreadClosephase(&context); 
	
}

void mSDLSWDeinit(struct mSDLRenderer* renderer) {
	if (renderer->outputBuffer) {
		free(renderer->outputBuffer);
		//free(gbabuffer);
		
#ifdef USE_PIXMAN
/*   //sunxu remove
		pixman_image_unref(renderer->pix);
		pixman_image_unref(renderer->screenpix);
*/
#endif
	}
	
	TTF_CloseFont(font);
    TTF_Quit();	
	
	SDL_Surface* surface = SDL_GetVideoSurface();
	SDL_UnlockSurface(surface);
}



static inline void  refreshscreen()
{
	if(!mRenderer)
		return;	
	void *des = mScreen->pixels+posOffset;			
	int contentlen = mainwidth<<1;
	int hight = vedioHigh, widthsrcpitch = srcpitch;
	void *src = NULL;
	if((!superGBMode || fullscreensetting)&& vedioWidth ==256)
		{
			src = mRenderer->outputBuffer + (256-160)/2 +(224-144)*256/2;
			hight = 144;
			widthsrcpitch = 160*2;
			des = mScreen->pixels+(GKD350_WIDTH-160) + (GKD350_HEIGHT-144)* 640/2;
		}
	else
		src = mRenderer->outputBuffer;
	
	
	switch (fullscreensetting){	
	case 0:
		for (int i=0; i<hight; i++)
		{
			memcpy(des,src,widthsrcpitch);
			des += mScreen->pitch;
			src += contentlen;
		}
		break;
	case 1:
		{
			if(vedioWidth ==240)
			{
				//memcpy(gbabuffer, src, 240*160*2);
				gba_upscale_aspect((uint16_t*) ((uint8_t*)mScreen->pixels +(13/*((240 - (160) * 4/3)/2)*/*640)),
			src, 240, 160, 480, 640);
			}
			else
			{
				scale166x_fast((uint32_t*) ((uint8_t*)mScreen->pixels+54), src);
			}
				
			break;
		}
	case 2:
		{
			if(vedioWidth ==240)
			{
				//memcpy(gbabuffer, src, 240*160*2);
				gba_upscale_aspect_subpixel((uint16_t*) ((uint8_t*)mScreen->pixels +(13/*((240 - (160) * 4/3)/2)*/*640)),
			src, 240, 160, 480, 640);
			}
			else
				scale166x_fast((uint32_t*) ((uint8_t*)mScreen->pixels+54), src);
			break;
		}
	case 3:
		{
			if(vedioWidth ==240)
			{
				//memcpy(gbabuffer, src, 240*160*2);
				gba_upscale_aspect_bilinear((uint16_t*) ((uint8_t*)mScreen->pixels +(13/*((240 - (160) * 4/3)/2)*/*640)),
			src, 240, 160, 480, 640);
			}
			else
				scale166x_pseudobilinear((uint32_t*) ((uint8_t*)mScreen->pixels+54), src);
			break;
		}
	}
						
		
}



/* Upscales an image by 33% in width and in height; also does color conversion
 * using the function above.
 * Input:
 *   from: A pointer to the pixels member of a src_x by src_y surface to be
 *     read by this function. The pixel format of this surface is XBGR 1555.
 *   src_x: The width of the source.
 *   src_y: The height of the source.
 *   src_pitch: The number of bytes making up a scanline in the source
 *     surface.
 *   dst_pitch: The number of bytes making up a scanline in the destination
 *     surface.
 * Output:
 *   to: A pointer to the pixels member of a (src_x * 4/3) by (src_y * 4/3)
 *     surface to be filled with the upscaled GBA image. The pixel format of
 *     this surface is RGB 565.
 */
static inline void gba_upscale_aspect(uint16_t *to, uint16_t *from,
	  uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch)
{
	/* Before:
	 *    a b c d e f
	 *    g h i j k l
	 *    m n o p q r
	 *
	 * After (multiple letters = average):
	 *    a    ab   bc   c    d    de   ef   f
	 *    ag   abgh bchi ci   dj   dejk efkl fl
	 *    gm   ghmn hino io   jp   jkpq klqr lr
	 *    m    mn   no   o    p    pq   qr   r
	 */

	const uint32_t dst_x = 320;//src_x * 4 / 3;
	const uint32_t src_skip = src_pitch - src_x * sizeof(uint16_t),
	               dst_skip = dst_pitch - dst_x * sizeof(uint16_t);

	uint32_t x, y;

	for (y = 0; y < src_y; y += 3) {
		for (x = 0; x < 40/*src_x / 6*/; x++) {
			// -- Row 1 --
			// Read RGB565 elements in the source grid.
			// The notation is high_low (little-endian).
			uint32_t b_a = bgr555_to_native(*(uint32_t*) (from    )),
			         d_c = bgr555_to_native(*(uint32_t*) (from + 2)),
			         f_e = bgr555_to_native(*(uint32_t*) (from + 4));

			// Generate ab_a from b_a.
			*(uint32_t*) (to) = likely(Hi(b_a) == Lo(b_a))
				? b_a
				: Lo(b_a) /* 'a' verbatim to low pixel */ |
				  Raise(Average(Hi(b_a), Lo(b_a))) /* ba to high pixel */;

			// Generate c_bc from b_a and d_c.
			*(uint32_t*) (to + 2) = likely(Hi(b_a) == Lo(d_c))
				? Lo(d_c) | Raise(Lo(d_c))
				: Raise(Lo(d_c)) /* 'c' verbatim to high pixel */ |
				  Average(Lo(d_c), Hi(b_a)) /* bc to low pixel */;

			// Generate de_d from d_c and f_e.
			*(uint32_t*) (to + 4) = likely(Hi(d_c) == Lo(f_e))
				? Lo(f_e) | Raise(Lo(f_e))
				: Hi(d_c) /* 'd' verbatim to low pixel */ |
				  Raise(Average(Lo(f_e), Hi(d_c))) /* de to high pixel */;

			// Generate f_ef from f_e.
			*(uint32_t*) (to + 6) = likely(Hi(f_e) == Lo(f_e))
				? f_e
				: Raise(Hi(f_e)) /* 'f' verbatim to high pixel */ |
				  Average(Hi(f_e), Lo(f_e)) /* ef to low pixel */;

			if (likely(y + 1 < src_y))  // Is there a source row 2?
			{
				// -- Row 2 --
				uint32_t h_g = bgr555_to_native(*(uint32_t*) ((uint8_t*) from + src_pitch    )),
				         j_i = bgr555_to_native(*(uint32_t*) ((uint8_t*) from + src_pitch + 4)),
				         l_k = bgr555_to_native(*(uint32_t*) ((uint8_t*) from + src_pitch + 8));

				// Generate abgh_ag from b_a and h_g.
				uint32_t bh_ag = Average32(b_a, h_g);
				*(uint32_t*) ((uint8_t*) to + dst_pitch) = likely(Hi(bh_ag) == Lo(bh_ag))
					? bh_ag
					: Lo(bh_ag) /* ag verbatim to low pixel */ |
					  Raise(Average(Hi(bh_ag), Lo(bh_ag))) /* abgh to high pixel */;

				// Generate ci_bchi from b_a, d_c, h_g and j_i.
				uint32_t ci_bh =
					Hi(bh_ag) /* bh verbatim to low pixel */ |
					Raise(Average(Lo(d_c), Lo(j_i))) /* ci to high pixel */;
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 4) = likely(Hi(ci_bh) == Lo(ci_bh))
					? ci_bh
					: Raise(Hi(ci_bh)) /* ci verbatim to high pixel */ |
					  Average(Hi(ci_bh), Lo(ci_bh)) /* bchi to low pixel */;

				// Generate fl_efkl from f_e and l_k.
				uint32_t fl_ek = Average32(f_e, l_k);
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 12) = likely(Hi(fl_ek) == Lo(fl_ek))
					? fl_ek
					: Raise(Hi(fl_ek)) /* fl verbatim to high pixel */ |
					  Average(Hi(fl_ek), Lo(fl_ek)) /* efkl to low pixel */;

				// Generate dejk_dj from d_c, f_e, j_i and l_k.
				uint32_t ek_dj =
					Raise(Lo(fl_ek)) /* ek verbatim to high pixel */ |
					Average(Hi(d_c), Hi(j_i)) /* dj to low pixel */;
				*(uint32_t*) ((uint8_t*) to + dst_pitch + 8) = likely(Hi(ek_dj) == Lo(ek_dj))
					? ek_dj
					: Lo(ek_dj) /* dj verbatim to low pixel */ |
					  Raise(Average(Hi(ek_dj), Lo(ek_dj))) /* dejk to high pixel */;

				if (likely(y + 2 < src_y))  // Is there a source row 3?
				{
					// -- Row 3 --
					uint32_t n_m = bgr555_to_native(*(uint32_t*) ((uint8_t*) from + src_pitch * 2    )),
					         p_o = bgr555_to_native(*(uint32_t*) ((uint8_t*) from + src_pitch * 2 + 4)),
					         r_q = bgr555_to_native(*(uint32_t*) ((uint8_t*) from + src_pitch * 2 + 8));

					// Generate ghmn_gm from h_g and n_m.
					uint32_t hn_gm = Average32(h_g, n_m);
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 2) = likely(Hi(hn_gm) == Lo(hn_gm))
						? hn_gm
						: Lo(hn_gm) /* gm verbatim to low pixel */ |
						  Raise(Average(Hi(hn_gm), Lo(hn_gm))) /* ghmn to high pixel */;

					// Generate io_hino from h_g, j_i, n_m and p_o.
					uint32_t io_hn =
						Hi(hn_gm) /* hn verbatim to low pixel */ |
						Raise(Average(Lo(j_i), Lo(p_o))) /* io to high pixel */;
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 4) = likely(Hi(io_hn) == Lo(io_hn))
						? io_hn
						: Raise(Hi(io_hn)) /* io verbatim to high pixel */ |
						  Average(Hi(io_hn), Lo(io_hn)) /* hino to low pixel */;

					// Generate lr_klqr from l_k and r_q.
					uint32_t lr_kq = Average32(l_k, r_q);
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 12) = likely(Hi(lr_kq) == Lo(lr_kq))
						? lr_kq
						: Raise(Hi(lr_kq)) /* lr verbatim to high pixel */ |
						  Average(Hi(lr_kq), Lo(lr_kq)) /* klqr to low pixel */;

					// Generate jkpq_jp from j_i, l_k, p_o and r_q.
					uint32_t kq_jp =
						Raise(Lo(lr_kq)) /* kq verbatim to high pixel */ |
						Average(Hi(j_i), Hi(p_o)) /* jp to low pixel */;
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 2 + 8) = likely(Hi(kq_jp) == Lo(kq_jp))
						? kq_jp
						: Lo(kq_jp) /* jp verbatim to low pixel */ |
						  Raise(Average(Hi(kq_jp), Lo(kq_jp))) /* jkpq to high pixel */;

					// -- Row 4 --
					// Generate mn_m from n_m.
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 3) = likely(Hi(n_m) == Lo(n_m))
						? n_m
						: Lo(n_m) /* 'm' verbatim to low pixel */ |
						  Raise(Average(Hi(n_m), Lo(n_m))) /* mn to high pixel */;

					// Generate o_no from n_m and p_o.
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 3 + 4) = likely(Hi(n_m) == Lo(p_o))
						? Lo(p_o) | Raise(Lo(p_o))
						: Raise(Lo(p_o)) /* 'o' verbatim to high pixel */ |
						  Average(Lo(p_o), Hi(n_m)) /* no to low pixel */;

					// Generate pq_p from p_o and r_q.
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 3 + 8) = likely(Hi(p_o) == Lo(r_q))
						? Lo(r_q) | Raise(Lo(r_q))
						: Hi(p_o) /* 'p' verbatim to low pixel */ |
						  Raise(Average(Hi(p_o), Lo(r_q))) /* pq to high pixel */;

					// Generate r_qr from r_q.
					*(uint32_t*) ((uint8_t*) to + dst_pitch * 3 + 12) = likely(Hi(r_q) == Lo(r_q))
						? r_q
						: Raise(Hi(r_q)) /* 'r' verbatim to high pixel */ |
						  Average(Hi(r_q), Lo(r_q)) /* qr to low pixel */;
				}
			}

			from += 6;
			to += 8;
		}

		// Skip past the waste at the end of the first line, if any,
		// then past 2 whole lines of source and 3 of destination.
		from = (uint16_t*) ((uint8_t*) from + src_skip + 2 * src_pitch);
		to   = (uint16_t*) ((uint8_t*) to   + dst_skip + 3 * dst_pitch);
	}
}

/* Upscales an image by 33% in width and in height, based on subpixel
 * rendering; also does color conversion using the function above.
 * Input:
 *   from: A pointer to the pixels member of a src_x by src_y surface to be
 *     read by this function. The pixel format of this surface is XBGR 1555.
 *   src_x: The width of the source.
 *   src_y: The height of the source.
 *   src_pitch: The number of bytes making up a scanline in the source
 *     surface.
 *   dst_pitch: The number of bytes making up a scanline in the destination
 *     surface.
 * Output:
 *   to: A pointer to the pixels member of a (src_x * 4/3) by (src_y * 4/3)
 *     surface to be filled with the upscaled GBA image. The pixel format of
 *     this surface is RGB 565.
 */
static inline void gba_upscale_aspect_subpixel(uint16_t *to, uint16_t *from,
	  uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch)
{
	const uint32_t dst_x = 320;//src_x * 4 / 3;
	const uint32_t src_skip = src_pitch - src_x * sizeof(uint16_t),
	               dst_skip = dst_pitch - dst_x * sizeof(uint16_t);

	uint_fast16_t sectY;
	for (sectY = 0; sectY < 53/*src_y / 3*/; sectY++)
	{
		uint_fast16_t sectX;
		for (sectX = 0; sectX < 80/*src_x / 3*/; sectX++)
		{
			uint_fast16_t rightCol = (sectX == 80/*src_x / 3*/ - 1) ? 4 : 6;
			/* Read RGB565 elements in the source grid.
			 * The last column blends with the first column of the next
			 * section. The last row does the same thing.
			 *
			 * a b c | d
			 * e f g | h
			 * i j k | l
			 * ---------
			 * m n o | p
			 */
			uint32_t a = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from    )),
			         b = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + 2)),
			         c = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + 4)),
			         d = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + rightCol));
			// The 4 output pixels in a row use 0.75, 1.5 then 2.25 as the X
			// coordinate for interpolation.

			// -- Row 1 --
			// All pixels in this row use 0 as the Y coordinate.

			// -- Row 1 pixel 1 (X = 0) --
			*to = a;

			// -- Row 1 pixel 2 (X = 0.75) --
			*(uint16_t*) ((uint8_t*) to + 2) = likely(a == b)
				? a
				: SubpixelRGB1_3(a, b);

			// -- Row 1 pixel 3 (X = 1.5) --
			*(uint16_t*) ((uint8_t*) to + 4) = likely(b == c)
				? b
				: SubpixelRGB1_1(b, c);

			// -- Row 1 pixel 4 (X = 2.25) --
			*(uint16_t*) ((uint8_t*) to + 6) = likely(c == d)
				? c
				: SubpixelRGB3_1(c, d);

			// -- Row 2 --
			// All pixels in this row use 0.75 as the Y coordinate.
			uint32_t e = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch    )),
			         f = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch + 2)),
			         g = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch + 4)),
			         h = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch + rightCol));

			// -- Row 2 pixel 1 (X = 0) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch) = likely(a == e)
				? a
				: AverageQuarters3_1(e, a);

			// -- Row 2 pixel 2 (X = 0.75) --
			uint16_t e1f3 = likely(e == f)
				? e
				: SubpixelRGB1_3(e, f);
			uint16_t a1b3 = likely(a == b)
				? a
				: SubpixelRGB1_3(a, b);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 2) = /* in Y */ likely(a1b3 == e1f3)
				? a1b3
				: AverageQuarters3_1(/* in X, bottom */ e1f3, /* in X, top */ a1b3);

			// -- Row 2 pixel 3 (X = 1.5) --
			uint16_t fg = likely(f == g)
				? f
				: SubpixelRGB1_1(f, g);
			uint16_t bc = likely(b == c)
				? b
				: SubpixelRGB1_1(b, c);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 4) = /* in Y */ likely(bc == fg)
				? bc
				: AverageQuarters3_1(/* in X, bottom */ fg, /* in X, top */ bc);

			// -- Row 2 pixel 4 (X = 2.25) --
			uint16_t g3h1 = likely(g == h)
				? g
				: SubpixelRGB3_1(g, h);
			uint16_t c3d1 = likely(c == d)
				? c
				: SubpixelRGB3_1(c, d);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 6) = /* in Y */ likely(g3h1 == c3d1)
				? c3d1
				: AverageQuarters3_1(/* in X, bottom */ g3h1, /* in X, top */ c3d1);

			// -- Row 3 --
			// All pixels in this row use 1.5 as the Y coordinate.
			uint32_t i = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 2    )),
			         j = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 2 + 2)),
			         k = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 2 + 4)),
			         l = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 2 + rightCol));

			// -- Row 3 pixel 1 (X = 0) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2) = likely(e == i)
				? e
				: Average(e, i);

			// -- Row 3 pixel 2 (X = 0.75) --
			uint16_t i1j3 = likely(i == j)
				? i
				: SubpixelRGB1_3(i, j);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 2) = /* in Y */ likely(e1f3 == i1j3)
				? e1f3
				: Average(e1f3, i1j3);

			// -- Row 3 pixel 3 (X = 1.5) --
			uint16_t jk = likely(j == k)
				? j
				: SubpixelRGB1_1(j, k);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 4) = /* in Y */ likely(fg == jk)
				? fg
				: Average(fg, jk);

			// -- Row 3 pixel 4 (X = 2.25) --
			uint16_t k3l1 = likely(k == l)
				? k
				: SubpixelRGB3_1(k, l);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 6) = /* in Y */ likely(g3h1 == k3l1)
				? g3h1
				: Average(g3h1, k3l1);

			// -- Row 4 --
			// All pixels in this row use 2.25 as the Y coordinate.
			uint32_t m = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 3    )),
			         n = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 3 + 2)),
			         o = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 3 + 4)),
			         p = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 3 + rightCol));

			// -- Row 4 pixel 1 (X = 0) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 3) = likely(i == m)
				? i
				: AverageQuarters3_1(i, m);

			// -- Row 4 pixel 2 (X = 0.75) --
			uint16_t m1n3 = likely(m == n)
				? m
				: SubpixelRGB1_3(m, n);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 3 + 2) = /* in Y */ likely(i1j3 == m1n3)
				? i1j3
				: AverageQuarters3_1(/* in X, top */ i1j3, /* in X, bottom */ m1n3);

			// -- Row 4 pixel 3 (X = 1.5) --
			uint16_t no = likely(n == o)
				? n
				: SubpixelRGB1_1(n, o);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 3 + 4) = /* in Y */ likely(jk == no)
				? jk
				: AverageQuarters3_1(/* in X, top */ jk, /* in X, bottom */ no);

			// -- Row 4 pixel 4 (X = 2.25) --
			uint16_t o3p1 = likely(o == p)
				? o
				: SubpixelRGB3_1(o, p);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 3 + 6) = /* in Y */ likely(k3l1 == o3p1)
				? k3l1
				: AverageQuarters3_1(/* in X, top */ k3l1, /* in X, bottom */ o3p1);

			from += 3;
			to   += 4;
		}

		// Skip past the waste at the end of the first line, if any,
		// then past 2 whole lines of source and 3 of destination.
		from = (uint16_t*) ((uint8_t*) from + src_skip + 2 * src_pitch);
		to   = (uint16_t*) ((uint8_t*) to   + dst_skip + 3 * dst_pitch);
	}

	if (src_y % 3 == 1)
	{
		uint_fast16_t sectX;
		for (sectX = 0; sectX < 80/*src_x / 3*/; sectX++)
		{
			uint_fast16_t rightCol = (sectX == 80/*src_x / 3*/ - 1) ? 4 : 6;
			/* Read RGB565 elements in the source grid.
			 * The last column blends with the first column of the next
			 * section. The last row does the same thing.
			 *
			 * a b c | d
			 */
			uint32_t a = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from    )),
			         b = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + 2)),
			         c = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + 4)),
			         d = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + rightCol));
			// The 4 output pixels in a row use 0.75, 1.5 then 2.25 as the X
			// coordinate for interpolation.

			// -- Row 1 pixel 1 (X = 0) --
			*to = a;

			// -- Row 1 pixel 2 (X = 0.75) --
			*(uint16_t*) ((uint8_t*) to + 2) = likely(a == b)
				? a
				: AverageQuarters3_1(b, a);

			// -- Row 1 pixel 3 (X = 1.5) --
			*(uint16_t*) ((uint8_t*) to + 4) = likely(b == c)
				? b
				: Average(b, c);

			// -- Row 1 pixel 4 (X = 2.25) --
			*(uint16_t*) ((uint8_t*) to + 6) = likely(c == d)
				? c
				: AverageQuarters3_1(c, d);

			from += 3;
			to   += 4;
		}
	}
}

/* Upscales an image by 33% in width and in height with bilinear filtering;
 * also does color conversion using the function above.
 * Input:
 *   from: A pointer to the pixels member of a src_x by src_y surface to be
 *     read by this function. The pixel format of this surface is XBGR 1555.
 *   src_x: The width of the source.
 *   src_y: The height of the source.
 *   src_pitch: The number of bytes making up a scanline in the source
 *     surface.
 *   dst_pitch: The number of bytes making up a scanline in the destination
 *     surface.
 * Output:
 *   to: A pointer to the pixels member of a (src_x * 4/3) by (src_y * 4/3)
 *     surface to be filled with the upscaled GBA image. The pixel format of
 *     this surface is RGB 565.
 */
static inline void gba_upscale_aspect_bilinear(uint16_t *to, uint16_t *from,
	  uint32_t src_x, uint32_t src_y, uint32_t src_pitch, uint32_t dst_pitch)
{
	const uint32_t dst_x = 320;//src_x * 4 / 3;
	const uint32_t src_skip = src_pitch - src_x * sizeof(uint16_t),
	               dst_skip = dst_pitch - dst_x * sizeof(uint16_t);

	uint_fast16_t sectY;
	for (sectY = 0; sectY < 53/*src_y / 3*/; sectY++)
	{
		uint_fast16_t sectX;
		for (sectX = 0; sectX < 80/*src_x / 3*/; sectX++)
		{
			uint_fast16_t rightCol = (sectX == 80/*src_x / 3*/- 1) ? 4 : 6;
			/* Read RGB565 elements in the source grid.
			 * The last column blends with the first column of the next
			 * section. The last row does the same thing.
			 *
			 * a b c | d
			 * e f g | h
			 * i j k | l
			 * ---------
			 * m n o | p
			 */
			uint32_t a = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from    )),
			         b = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + 2)),
			         c = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + 4)),
			         d = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + rightCol));
			// The 4 output pixels in a row use 0.75, 1.5 then 2.25 as the X
			// coordinate for interpolation.

			// -- Row 1 --
			// All pixels in this row use 0 as the Y coordinate.

			// -- Row 1 pixel 1 (X = 0) --
			*to = a;

			// -- Row 1 pixel 2 (X = 0.75) --
			*(uint16_t*) ((uint8_t*) to + 2) = likely(a == b)
				? a
				: AverageQuarters3_1(b, a);

			// -- Row 1 pixel 3 (X = 1.5) --
			*(uint16_t*) ((uint8_t*) to + 4) = likely(b == c)
				? b
				: Average(b, c);

			// -- Row 1 pixel 4 (X = 2.25) --
			*(uint16_t*) ((uint8_t*) to + 6) = likely(c == d)
				? c
				: AverageQuarters3_1(c, d);

			// -- Row 2 --
			// All pixels in this row use 0.75 as the Y coordinate.
			uint32_t e = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch    )),
			         f = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch + 2)),
			         g = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch + 4)),
			         h = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch + rightCol));

			// -- Row 2 pixel 1 (X = 0) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch) = likely(a == e)
				? a
				: AverageQuarters3_1(e, a);

			// -- Row 2 pixel 2 (X = 0.75) --
			uint16_t e1f3 = likely(e == f)
				? e
				: AverageQuarters3_1(f, e);
			uint16_t a1b3 = likely(a == b)
				? a
				: AverageQuarters3_1(b, a);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 2) = /* in Y */ likely(a1b3 == e1f3)
				? a1b3
				: AverageQuarters3_1(/* in X, bottom */ e1f3, /* in X, top */ a1b3);

			// -- Row 2 pixel 3 (X = 1.5) --
			uint16_t fg = likely(f == g)
				? f
				: Average(f, g);
			uint16_t bc = likely(b == c)
				? b
				: Average(b, c);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 4) = /* in Y */ likely(bc == fg)
				? bc
				: AverageQuarters3_1(/* in X, bottom */ fg, /* in X, top */ bc);

			// -- Row 2 pixel 4 (X = 2.25) --
			uint16_t g3h1 = likely(g == h)
				? g
				: AverageQuarters3_1(g, h);
			uint16_t c3d1 = likely(c == d)
				? c
				: AverageQuarters3_1(c, d);
			*(uint16_t*) ((uint8_t*) to + dst_pitch + 6) = /* in Y */ likely(g3h1 == c3d1)
				? c3d1
				: AverageQuarters3_1(/* in X, bottom */ g3h1, /* in X, top */ c3d1);

			// -- Row 3 --
			// All pixels in this row use 1.5 as the Y coordinate.
			uint32_t i = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 2    )),
			         j = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 2 + 2)),
			         k = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 2 + 4)),
			         l = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 2 + rightCol));

			// -- Row 3 pixel 1 (X = 0) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2) = likely(e == i)
				? e
				: Average(e, i);

			// -- Row 3 pixel 2 (X = 0.75) --
			uint16_t i1j3 = likely(i == j)
				? i
				: AverageQuarters3_1(j, i);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 2) = /* in Y */ likely(e1f3 == i1j3)
				? e1f3
				: Average(e1f3, i1j3);

			// -- Row 3 pixel 3 (X = 1.5) --
			uint16_t jk = likely(j == k)
				? j
				: Average(j, k);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 4) = /* in Y */ likely(fg == jk)
				? fg
				: Average(fg, jk);

			// -- Row 3 pixel 4 (X = 2.25) --
			uint16_t k3l1 = likely(k == l)
				? k
				: AverageQuarters3_1(k, l);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 2 + 6) = /* in Y */ likely(g3h1 == k3l1)
				? g3h1
				: Average(g3h1, k3l1);

			// -- Row 4 --
			// All pixels in this row use 2.25 as the Y coordinate.
			uint32_t m = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 3    )),
			         n = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 3 + 2)),
			         o = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 3 + 4)),
			         p = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + src_pitch * 3 + rightCol));

			// -- Row 4 pixel 1 (X = 0) --
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 3) = likely(i == m)
				? i
				: AverageQuarters3_1(i, m);

			// -- Row 4 pixel 2 (X = 0.75) --
			uint16_t m1n3 = likely(m == n)
				? m
				: AverageQuarters3_1(n, m);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 3 + 2) = /* in Y */ likely(i1j3 == m1n3)
				? i1j3
				: AverageQuarters3_1(/* in X, top */ i1j3, /* in X, bottom */ m1n3);

			// -- Row 4 pixel 3 (X = 1.5) --
			uint16_t no = likely(n == o)
				? n
				: Average(n, o);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 3 + 4) = /* in Y */ likely(jk == no)
				? jk
				: AverageQuarters3_1(/* in X, top */ jk, /* in X, bottom */ no);

			// -- Row 4 pixel 4 (X = 2.25) --
			uint16_t o3p1 = likely(o == p)
				? o
				: AverageQuarters3_1(o, p);
			*(uint16_t*) ((uint8_t*) to + dst_pitch * 3 + 6) = /* in Y */ likely(k3l1 == o3p1)
				? k3l1
				: AverageQuarters3_1(/* in X, top */ k3l1, /* in X, bottom */ o3p1);

			from += 3;
			to   += 4;
		}

		// Skip past the waste at the end of the first line, if any,
		// then past 2 whole lines of source and 3 of destination.
		from = (uint16_t*) ((uint8_t*) from + src_skip + 2 * src_pitch);
		to   = (uint16_t*) ((uint8_t*) to   + dst_skip + 3 * dst_pitch);
	}

	if (src_y % 3 == 1)
	{
		uint_fast16_t sectX;
		for (sectX = 0; sectX < 80/*src_x / 3*/; sectX++)
		{
			uint_fast16_t rightCol = (sectX == 80/*src_x / 3*/ - 1) ? 4 : 6;
			/* Read RGB565 elements in the source grid.
			 * The last column blends with the first column of the next
			 * section. The last row does the same thing.
			 *
			 * a b c | d
			 */
			uint32_t a = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from    )),
			         b = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + 2)),
			         c = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + 4)),
			         d = bgr555_to_native_16(*(uint16_t*) ((uint8_t*) from + rightCol));
			// The 4 output pixels in a row use 0.75, 1.5 then 2.25 as the X
			// coordinate for interpolation.

			// -- Row 1 pixel 1 (X = 0) --
			*to = a;

			// -- Row 1 pixel 2 (X = 0.75) --
			*(uint16_t*) ((uint8_t*) to + 2) = likely(a == b)
				? a
				: AverageQuarters3_1(b, a);

			// -- Row 1 pixel 3 (X = 1.5) --
			*(uint16_t*) ((uint8_t*) to + 4) = likely(b == c)
				? b
				: Average(b, c);

			// -- Row 1 pixel 4 (X = 2.25) --
			*(uint16_t*) ((uint8_t*) to + 6) = likely(c == d)
				? c
				: AverageQuarters3_1(c, d);

			from += 3;
			to   += 4;
		}
	}
}




/* Downscales an image by half in width and in height; also does color
 * conversion using the function above.
 * Input:
 *   Src: A pointer to the pixels member of a 240x160 surface to be read by
 *     this function. The pixel format of this surface is XBGR 1555.
 *   DestX: The column to start the thumbnail at in the destination surface.
 *   DestY: The row to start the thumbnail at in the destination surface.
 *   SrcPitch: The number of bytes making up a scanline in the source
 *     surface.
 *   DstPitch: The number of bytes making up a scanline in the destination
 *     surface.
 * Output:
 *   Dest: A pointer to the pixels member of a surface to be filled with the
 *     downscaled GBA image. The pixel format of this surface is RGB 565.
 */
void gba_render_half(uint16_t* Src, uint32_t DestX, uint32_t DestY,
	uint32_t SrcPitch, uint32_t DestPitch)
{
	uint16_t* Dest = (u16*)mScreen->pixels;
	Dest = (uint16_t*) ((uint8_t*) Dest
		+ (DestY * DestPitch)
		+ (DestX * sizeof(uint16_t))
	);
	uint32_t SrcSkip = SrcPitch - GBA_SCREEN_WIDTH * sizeof(uint16_t);
	uint32_t DestSkip = DestPitch - (GBA_SCREEN_WIDTH / 2) * sizeof(uint16_t);

	uint32_t X, Y;
	for (Y = 0; Y < GBA_SCREEN_HEIGHT / 2; Y++)
	{
		for (X = 0; X < GBA_SCREEN_WIDTH * sizeof(uint16_t) / (sizeof(uint32_t) * 2); X++)
		{
			/* Before:
			*    a b c d
			*    e f g h
			*
			* After (multiple letters = average):
			*    abef cdgh
			*/
			uint32_t b_a = bgr555_to_native(*(uint32_t*) ((uint8_t*) Src    )),
			         d_c = bgr555_to_native(*(uint32_t*) ((uint8_t*) Src + 4)),
			         f_e = bgr555_to_native(*(uint32_t*) ((uint8_t*) Src + SrcPitch    )),
			         h_g = bgr555_to_native(*(uint32_t*) ((uint8_t*) Src + SrcPitch + 4));
			uint32_t bf_ae = likely(b_a == f_e)
				? b_a
				: Average32(b_a, f_e);
			uint32_t dh_cg = likely(d_c == h_g)
				? d_c
				: Average32(d_c, h_g);
			*(uint32_t*) Dest = likely(bf_ae == dh_cg)
				? bf_ae
				: Average(Hi(bf_ae), Lo(bf_ae)) |
				  Raise(Average(Hi(dh_cg), Lo(dh_cg)));
			Dest += 2;
			Src += 4;
		}
		Src = (uint16_t*) ((uint8_t*) Src + SrcSkip + SrcPitch);
		Dest = (uint16_t*) ((uint8_t*) Dest + DestSkip);
	}
}

#define cR(A) (((A) & 0xf800) >> 8)
#define cG(A) (((A) & 0x7e0) >> 3)
#define cB(A) (((A) & 0x1f) << 3)
#define Weight1_1(A, B) ((((cR(A) + cR(B)) / 2) & 0xf8) << 8 | (((cG(A) + cG(B)) / 2) & 0xfc) << 3 | (((cB(A) + cB(B)) / 2) & 0xf8) >> 3)
#define Weight1_2(A, B) ((((cR(A) + cR(B) + cR(B)) / 3) & 0xf8) << 8 | (((cG(A) + cG(B) + cG(B)) / 3) & 0xfc) << 3 | (((cB(A) + cB(B) + cB(B)) / 3) & 0xf8) >> 3)
#define Weight2_1(A, B) ((((cR(A) + cR(A) + cR(B)) / 3) & 0xf8) << 8 | (((cG(A) + cG(A) + cG(B)) / 3) & 0xfc) << 3 | (((cB(A) + cB(A) + cB(B)) / 3) & 0xf8) >> 3)
#define Weight1_4(A, B) ((((cR(A) + cR(B) + cR(B) + cR(B)+ cR(B)) / 5) & 0xf8) << 8 | (((cG(A) + cG(B) + cG(B) + cG(B) + cG(B)) / 5) & 0xfc) << 3 | (((cB(A) + cB(B) + cB(B) + cB(B) + cB(B)) / 5) & 0xf8) >> 3)
#define Weight4_1(A, B) ((((cR(A) + cR(A) + cR(A) + cR(A)+ cR(B)) / 5) & 0xf8) << 8 | (((cG(A) + cG(A) + cG(A) + cG(A) + cG(B)) / 5) & 0xfc) << 3 | (((cB(A) + cB(A) + cB(A) + cB(A) + cB(B)) / 5) & 0xf8) >> 3)
#define Weight2_3(A, B) ((((cR(A) + cR(A) + cR(B) + cR(B)+ cR(B)) / 5) & 0xf8) << 8 | (((cG(A) + cG(A) + cG(B) + cG(B) + cG(B)) / 5) & 0xfc) << 3 | (((cB(A) + cB(A) + cB(B) + cB(B) + cB(B)) / 5) & 0xf8) >> 3)
#define Weight3_2(A, B) ((((cR(A) + cR(A) + cR(A) + cR(B)+ cR(B)) / 5) & 0xf8) << 8 | (((cG(A) + cG(A) + cG(A) + cG(B) + cG(B)) / 5) & 0xfc) << 3 | (((cB(A) + cB(A) + cB(A) + cB(B) + cB(B)) / 5) & 0xf8) >> 3)
#define Weight1_1_1_1(A, B, C, D) ((((cR(A) + cR(B) + cR(C) + cR(D)) / 4) & 0xf8) << 8 | (((cG(A) + cG(B) + cG(C) + cG(D)) / 4) & 0xfc) << 3 | (((cB(A) + cB(B) + cB(C) + cB(D)) / 4) & 0xf8) >> 3)



/* Upscales a 160x144 image to 266x240 using a faster resampling algorithm.
*
* Input:
* src: A packed 160x144 pixel image. The pixel format of this image is RGB 565.
* Output:
* dst: A packed 266x240 pixel image. The pixel format of this image is RGB 565.
*/
static inline  void scale166x_fast(uint32_t* dst, uint32_t* src)
{
	uint16_t* Src16 = (uint16_t*) src;
	uint16_t* Dst16 = (uint16_t*) dst;
	// There are 53(+1) blocks of 3 pixels horizontally, and 48 of 3 vertically.
	// Each block of 3x3 becomes 5x5. There is a last column of blocks of 1x3 which becomes 1x5.
	uint32_t BlockX, BlockY;
	uint16_t* BlockSrc;
	uint16_t* BlockDst;
	for (BlockY = 0; BlockY < 48; BlockY++)
	{
		BlockSrc = Src16 + BlockY * 256 * 3;
		BlockDst = Dst16 + BlockY * 320 * 5;
		for (BlockX = 0; BlockX < 54; BlockX++)
		{
			// HORIZONTAL:
			// Before: After:
			// (a)(b)(c) (a)(aab)(b)(bcc)(c)
			//
			// VERTICAL:
			// Before: After:
			// (a) (a)
			// (b) (aab)
			// (c) (b)
			// (bcc)
			// (c)
			if(BlockX == 53){
			// -- Row 1 --
			uint16_t _1 = *(BlockSrc );
			*(BlockDst ) = _1;
			// -- Row 2 --
			uint16_t _2 = *(BlockSrc + 256 * 1 );
			*(BlockDst + 320 * 1 ) = Weight2_1( _1, _2);
			// -- Row 3 --
			*(BlockDst + 320 * 2 ) = _2;
			// -- Row 4 --
			uint16_t _3 = *(BlockSrc + 256 * 2 );
			*(BlockDst + 320 * 3 ) = Weight1_2( _2, _3);
			// -- Row 5 --
			*(BlockDst + 320 * 4 ) = _3;
			} else {
			// -- Row 1 --
			uint16_t _1 = *(BlockSrc );
			*(BlockDst ) = _1;
			uint16_t _2 = *(BlockSrc + 1);
			*(BlockDst + 1) = Weight2_1( _1, _2);
			*(BlockDst + 2) = _2;
			uint16_t _3 = *(BlockSrc + 2);
			*(BlockDst + 3) = Weight1_2( _2, _3);
			*(BlockDst + 4) = _3;
			// -- Row 2 --
			uint16_t _4 = *(BlockSrc + 256 * 1 );
			*(BlockDst + 320 * 1 ) = Weight2_1( _1, _4);
			uint16_t _5 = *(BlockSrc + 256 * 1 + 1);
			*(BlockDst + 320 * 1 + 1) = Weight2_1(Weight2_1( _1, _2), Weight2_1( _4, _5));
			*(BlockDst + 320 * 1 + 2) = Weight2_1( _2, _5);
			uint16_t _6 = *(BlockSrc + 256 * 1 + 2);
			*(BlockDst + 320 * 1 + 3) = Weight2_1(Weight1_2( _2, _3), Weight1_2( _5, _6));
			*(BlockDst + 320 * 1 + 4) = Weight2_1( _3, _6);
			// -- Row 3 --
			*(BlockDst + 320 * 2 ) = _4;
			*(BlockDst + 320 * 2 + 1) = Weight2_1( _4, _5);
			*(BlockDst + 320 * 2 + 2) = _5;
			*(BlockDst + 320 * 2 + 3) = Weight1_2( _5, _6);
			*(BlockDst + 320 * 2 + 4) = _6;
			// -- Row 4 --
			uint16_t _7 = *(BlockSrc + 256 * 2 );
			*(BlockDst + 320 * 3 ) = Weight1_2( _4, _7);
			uint16_t _8 = *(BlockSrc + 256 * 2 + 1);
			*(BlockDst + 320 * 3 + 1) = Weight1_2(Weight2_1( _4, _5), Weight2_1( _7, _8));
			*(BlockDst + 320 * 3 + 2) = Weight1_2( _5, _8);
			uint16_t _9 = *(BlockSrc + 256 * 2 + 2);
			*(BlockDst + 320 * 3 + 3) = Weight1_2(Weight1_2( _5, _6), Weight1_2( _8, _9));
			*(BlockDst + 320 * 3 + 4) = Weight1_2( _6, _9);
			// -- Row 5 --
			*(BlockDst + 320 * 4 ) = _7;
			*(BlockDst + 320 * 4 + 1) = Weight2_1( _7, _8);
			*(BlockDst + 320 * 4 + 2) = _8;
			*(BlockDst + 320 * 4 + 3) = Weight1_2( _8, _9);
			*(BlockDst + 320 * 4 + 4) = _9;
			}
			BlockSrc += 3;
			BlockDst += 5;
		}
	}
}
/* Upscales a 160x144 image to 266x240 using a pseudo-bilinear resampling algorithm.
*
* Input:
* src: A packed 160x144 pixel image. The pixel format of this image is RGB 565.
* Output:
* dst: A packed 266x240 pixel image. The pixel format of this image is RGB 565.
*/
static inline void scale166x_pseudobilinear(uint32_t* dst, uint32_t* src)
{
	uint16_t* Src16 = (uint16_t*) src;
	uint16_t* Dst16 = (uint16_t*) dst;
	// There are 53(+1) blocks of 3 pixels horizontally, and 48 of 3 vertically.
	// Each block of 3x3 becomes 5x5. There is a last column of blocks of 1x3 which becomes 1x5.
	uint32_t BlockX, BlockY;
	uint16_t* BlockSrc;
	uint16_t* BlockDst;
	for (BlockY = 0; BlockY < 48; BlockY++)
	{
		BlockSrc = Src16 + BlockY * 256 * 3;
		BlockDst = Dst16 + BlockY * 320 * 5;
		for (BlockX = 0; BlockX < 54; BlockX++)
		{
			// HORIZONTAL:
			// Before: After:
			// (a)(b)(c)--->(d) (a)(aabbb)(bbbbc)(bcccc)(cccdd)
			//
			//
			// VERTICAL:
			// Before: After:
			// (a) (a)
			// (b) (aabbb)
			// (c) (bbbbc)
			// | (bcccc)
			// V (cccdd)
			// (d)
			if((BlockX == 53) && (BlockY == 47)){
			// -- Row 1 --
			uint16_t _1 = *(BlockSrc );
			*(BlockDst ) = _1;
			// -- Row 2 --
			uint16_t _2 = *(BlockSrc + 256 * 1 );
			*(BlockDst + 320 * 1 ) = Weight2_3( _1, _2);
			// -- Row 3 --
			uint16_t _3 = *(BlockSrc + 256 * 2 );
			*(BlockDst + 320 * 2 ) = Weight4_1( _2, _3);
			// -- Row 4 --
			*(BlockDst + 320 * 3 ) = Weight1_4( _2, _3);
			// -- Row 5 --
			*(BlockDst + 320 * 4 ) = _3;
			} else if(BlockX == 53){
			// -- Row 1 --
			uint16_t _1 = *(BlockSrc );
			*(BlockDst ) = _1;
			// -- Row 2 --
			uint16_t _2 = *(BlockSrc + 256 * 1 );
			*(BlockDst + 320 * 1 ) = Weight2_3( _1, _2);
			// -- Row 3 --
			uint16_t _3 = *(BlockSrc + 256 * 2 );
			*(BlockDst + 320 * 2 ) = Weight4_1( _2, _3);
			// -- Row 4 --
			*(BlockDst + 320 * 3 ) = Weight1_4( _2, _3);
			// -- Row 5 --
			uint16_t _4 = *(BlockSrc + 256 * 3 );
			*(BlockDst + 320 * 4 ) = Weight3_2( _3, _4);
			} else if(BlockY == 47){
			// -- Row 1 --;
			uint16_t _1 = *(BlockSrc );
			*(BlockDst ) = _1;
			uint16_t _2 = *(BlockSrc + 1);
			*(BlockDst + 1) = Weight2_3( _1, _2);
			uint16_t _3 = *(BlockSrc + 2);
			*(BlockDst + 2) = Weight4_1( _2, _3);
			*(BlockDst + 3) = Weight1_4( _2, _3);
			uint16_t _4 = *(BlockSrc + 3);
			*(BlockDst + 4) = Weight3_2( _3, _4);
			// -- Row 2 --
			uint16_t _5 = *(BlockSrc + 256 * 1 );
			*(BlockDst + 320 * 1 ) = Weight2_3( _1, _5);
			uint16_t _6 = *(BlockSrc + 256 * 1 + 1);
			*(BlockDst + 320 * 1 + 1) = Weight2_3(Weight2_3( _1, _2), Weight2_3( _5, _6));
			uint16_t _7 = *(BlockSrc + 256 * 1 + 2);
			*(BlockDst + 320 * 1 + 2) = Weight2_3(Weight4_1( _2, _3), Weight4_1( _6, _7));
			*(BlockDst + 320 * 1 + 3) = Weight2_3(Weight1_4( _2, _3), Weight1_4( _6, _7));
			uint16_t _8 = *(BlockSrc + 256 * 1 + 3);
			*(BlockDst + 320 * 1 + 4) = Weight2_3(Weight3_2( _3, _4), Weight3_2( _7, _8));
			// -- Row 3 --
			uint16_t _9 = *(BlockSrc + 256 * 2 );
			*(BlockDst + 320 * 2 ) = Weight4_1( _5, _9);
			uint16_t _10 = *(BlockSrc + 256 * 2 + 1);
			*(BlockDst + 320 * 2 + 1) = Weight4_1(Weight2_3( _5, _6), Weight2_3( _9, _10));
			uint16_t _11 = *(BlockSrc + 256 * 2 + 2);
			*(BlockDst + 320 * 2 + 2) = Weight4_1(Weight4_1( _6, _7), Weight4_1(_10, _11));
			*(BlockDst + 320 * 2 + 3) = Weight4_1(Weight1_4( _6, _7), Weight1_4(_10, _11));
			uint16_t _12 = *(BlockSrc + 256 * 2 + 3);
			*(BlockDst + 320 * 2 + 4) = Weight4_1(Weight3_2( _7, _8), Weight3_2(_11, _12));
			// -- Row 4 --
			*(BlockDst + 320 * 3 ) = Weight1_4( _5, _9);
			*(BlockDst + 320 * 3 + 1) = Weight1_4(Weight2_3( _5, _6), Weight2_3( _9, _10));
			*(BlockDst + 320 * 3 + 2) = Weight1_4(Weight4_1( _6, _7), Weight4_1(_10, _11));
			*(BlockDst + 320 * 3 + 3) = Weight1_4(Weight1_4( _6, _7), Weight1_4(_10, _11));
			*(BlockDst + 320 * 3 + 4) = Weight1_4(Weight3_2( _7, _8), Weight3_2(_11, _12));
			// -- Row 5 --
			*(BlockDst + 320 * 4 ) = _9;
			*(BlockDst + 320 * 4 + 1) = Weight2_3( _9, _10);
			*(BlockDst + 320 * 4 + 2) = Weight4_1(_10, _11);
			*(BlockDst + 320 * 4 + 3) = Weight1_4(_10, _11);
			*(BlockDst + 320 * 4 + 4) = Weight3_2(_11, _12);
			} else {
			// -- Row 1 --;
			uint16_t _1 = *(BlockSrc );
			*(BlockDst ) = _1;
			uint16_t _2 = *(BlockSrc + 1);
			*(BlockDst + 1) = Weight2_3( _1, _2);
			uint16_t _3 = *(BlockSrc + 2);
			*(BlockDst + 2) = Weight4_1( _2, _3);
			*(BlockDst + 3) = Weight1_4( _2, _3);
			uint16_t _4 = *(BlockSrc + 3);
			*(BlockDst + 4) = Weight3_2( _3, _4);
			// -- Row 2 --
			uint16_t _5 = *(BlockSrc + 256 * 1 );
			*(BlockDst + 320 * 1 ) = Weight2_3( _1, _5);
			uint16_t _6 = *(BlockSrc + 256 * 1 + 1);
			*(BlockDst + 320 * 1 + 1) = Weight2_3(Weight2_3( _1, _2), Weight2_3( _5, _6));
			uint16_t _7 = *(BlockSrc + 256 * 1 + 2);
			*(BlockDst + 320 * 1 + 2) = Weight2_3(Weight4_1( _2, _3), Weight4_1( _6, _7));
			*(BlockDst + 320 * 1 + 3) = Weight2_3(Weight1_4( _2, _3), Weight1_4( _6, _7));
			uint16_t _8 = *(BlockSrc + 256 * 1 + 3);
			*(BlockDst + 320 * 1 + 4) = Weight2_3(Weight3_2( _3, _4), Weight3_2( _7, _8));
			// -- Row 3 --
			uint16_t _9 = *(BlockSrc + 256 * 2 );
			*(BlockDst + 320 * 2 ) = Weight4_1( _5, _9);
			uint16_t _10 = *(BlockSrc + 256 * 2 + 1);
			*(BlockDst + 320 * 2 + 1) = Weight4_1(Weight2_3( _5, _6), Weight2_3( _9, _10));
			uint16_t _11 = *(BlockSrc + 256 * 2 + 2);
			*(BlockDst + 320 * 2 + 2) = Weight4_1(Weight4_1( _6, _7), Weight4_1(_10, _11));
			*(BlockDst + 320 * 2 + 3) = Weight4_1(Weight1_4( _6, _7), Weight1_4(_10, _11));
			uint16_t _12 = *(BlockSrc + 256 * 2 + 3);
			*(BlockDst + 320 * 2 + 4) = Weight4_1(Weight3_2( _7, _8), Weight3_2(_11, _12));
			// -- Row 4 --
			*(BlockDst + 320 * 3 ) = Weight1_4( _5, _9);
			*(BlockDst + 320 * 3 + 1) = Weight1_4(Weight2_3( _5, _6), Weight2_3( _9, _10));
			*(BlockDst + 320 * 3 + 2) = Weight1_4(Weight4_1( _6, _7), Weight4_1(_10, _11));
			*(BlockDst + 320 * 3 + 3) = Weight1_4(Weight1_4( _6, _7), Weight1_4(_10, _11));
			*(BlockDst + 320 * 3 + 4) = Weight1_4(Weight3_2( _7, _8), Weight3_2(_11, _12));
			// -- Row 5 --
			uint16_t _13 = *(BlockSrc + 256 * 3 );
			*(BlockDst + 320 * 4 ) = Weight3_2( _9, _13);
			uint16_t _14 = *(BlockSrc + 256 * 3 + 1);
			*(BlockDst + 320 * 4 + 1) = Weight3_2(Weight2_3( _9, _10), Weight2_3(_13, _14));
			uint16_t _15 = *(BlockSrc + 256 * 3 + 2);
			*(BlockDst + 320 * 4 + 2) = Weight3_2(Weight4_1(_10, _11), Weight4_1(_14, _15));
			*(BlockDst + 320 * 4 + 3) = Weight3_2(Weight1_4(_10, _11), Weight1_4(_14, _15));
			uint16_t _16 = *(BlockSrc + 256 * 3 + 3);
			*(BlockDst + 320 * 4 + 4) = Weight3_2(Weight3_2(_11, _12), Weight3_2(_15, _16));
			}
			BlockSrc += 3;
			BlockDst += 5;
		}
	}
}
