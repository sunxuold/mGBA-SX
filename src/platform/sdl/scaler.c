#include <SDL.h>
#include <SDL_ttf.h>
#define FONT_RGB(r,g,b) (u16)((r) << 11 | (g) << 6 | (b))
/*
Fontrender(SDL_Surface *surface, int x, int y, const char *text, bool allowWrap, bool shadow)
{	
	int stride = surface->pitch / surface->format->BytesPerPixel;
    int surface_w = surface->w;
	
	//初始化字体库
	if( TTF_Init() == -1 )
		return -1;
	
	//打开字体
	TTF_Font *font;
	font=TTF_OpenFont("./sfont.ttf", 12);// 高度12
	if(!font)
	{ 
		printf("TTF_OpenFont: Open simsun.ttf %s\n", TTF_GetError());
		return -1; 
	}
	
	SDL_Color color2={255,255,255};
	surface = TTF_RenderText_Blended(font,"中文测试",color2);	
    
	
	TTF_CloseFont(font);
    TTF_Quit();		
}




Fontrender(SDL_Surface *surface, int x, int y, const char *text, bool allowWrap, bool shadow)
{	
	int stride = surface->pitch / surface->format->BytesPerPixel;
    int surface_w = surface->w;
	
		
	
	
	//SDL_Window* pSDLWindow = NULL;  //sdl窗口
	//SDL_Renderer* pWndRenderer = NULL;//窗口渲染器
	//SDL_Surface * pTextSurface = NULL;//文本表面
	//SDL_Texture* pTextTexture = NULL;//文本纹理
	//文本显示区域
	
	SDL_Rect rcText;
	rcText.x = 100;
	rcText.y = 100;
	rcText.w = 100;
	rcText.h = 20;
	
	
	//初始化SDL  
	//if (SDL_Init(SDL_INIT_EVERYTHING) < 0) return false;  
	//创建窗口  
	//pSDLWindow = SDL_CreateWindow("SDLWnd", 200, 200, 640, 480, 0);  
	//if (NULL == pSDLWindow) return false; 
	//创建窗口渲染器
	 //pWndRenderer = SDL_CreateRenderer(pSDLWindow, -1, 0); 
 
 
	//初始化字体库
	if( TTF_Init() == -1 )
		return -1;
	//打开字体
	TTF_Font *font;
	font=TTF_OpenFont("./sfont.ttf", 12);// 高度12
	if(!font)
	{ 
		printf("TTF_OpenFont: Open simsun.ttf %s\n", TTF_GetError());
		return -1; 
	}
	//创建文本表面
	//FONT_RGB(0,0,0);
	SDL_Color color2={255,255,255};
	surface = TTF_RenderText_Blended(font,"中文测试",color2);
	//创建文本纹理
	//pTextTexture = SDL_CreateTextureFromSurface(pWndRenderer, pTextSurface);
 
	//清理窗口渲染器
	//SDL_RenderClear(pWndRenderer);
	//将文本纹理复制到窗口渲染器
	//SDL_RenderCopy(pWndRenderer, pTextTexture, NULL, &rcText);  // NULL means that use all texture( and renderer)  
	//刷新窗口渲染器的显示
	//SDL_RenderPresent(pWndRenderer);
	//清理  
    TTF_CloseFont(font);
    TTF_Quit();
	 
	//SDL_DestroyWindow(pSDLWindow); 
	//SDL_DestroyRenderer(pWndRenderer);
	//SDL_DestroyTexture(pTextTexture);
	//SDL_FreeSurface(pTextSurface);
	//SDL_Quit(); 
}
*/