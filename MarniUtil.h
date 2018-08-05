#pragma once
#include "dxgl\include\ddraw.h"
#include "dxgl\include\d3d.h"

// Fill memory block with an integer value
__inline static void memset32(void *ptr, u32 value, size_t count)
{
	u32 *p = (u32 *)ptr;
	for (size_t i = 0; i < count; i++)
		*p++ = value;
}

__inline static void memcpy32(void *dst, void *src, size_t count)
{
	u32 *pdst = (u32 *)dst,
		*psrc = (u32 *)src;
	for (size_t i = 0; i < count; i++)
		*pdst++ = *psrc++;
}

#define ClassReset(ths,first)		memset(&ths->first, 0, (size_t)&ths[1] - (size_t)&ths->first)

typedef struct tagCtxTexture
{
	int count,
		max;
	DDSURFACEDESC desc[20];
} CTX_TEXTURE;

struct MARNI_MATRIX
{
	float m[16];
};

struct MARNI_RES
{
	int W;
	int H;
	int Depth;
	int Fullscreen;
};

struct MARNI_SOFTLIGHT
{
	float x;
	float y;
	float z;
	float r;
	float g;
	float b;
};

struct MARNI_SURFACE_DESC
{
	u16 dwRBitMask_setcnt;
	u16 field_2;
	u16 field_4;
	u16 dwRBitMask_cnt;
	u16 field_8;
	u16 field_A;
	u16 dwBBitMask_setcnt;
	u16 field_E;
	u16 field_10;
	u16 dwRGBAlphaBitMask_setcnt;
	u16 field_14;
	u16 field_16;
	u16 Pal_index;
	u8 dwRGBBitCount;		// texture depth
	u8 bpp;				// palette size
};

enum
{
	GFX_MONSTER,	// 3dfx
	GFX_MYSTIQUE,	// [????] mystique
	GFX_TOTAL,		// [good] rendition
	GFX_BLASTER,	// [good] rendition
	GFX_RIGHTEOUS,	// 3dfx
	GFX_NOCARD1,	// [good] software (vram)
	GFX_INTENSE,	// [good] rendition
	GFX_NOCARD2,	// [good] software (ram)
	GFX_UNKNOWN,
	// for convenience
	GFX_SOFTWARE = GFX_NOCARD1
};

typedef struct _D3DDeviceDesc1 {
	DWORD           dwSize;
	DWORD           dwFlags;
	D3DCOLORMODEL   dcmColorModel;
	DWORD           dwDevCaps;
	D3DTRANSFORMCAPS dtcTransformCaps;
	BOOL            bClipping;
	D3DLIGHTINGCAPS dlcLightingCaps;
	D3DPRIMCAPS     dpcLineCaps;
	D3DPRIMCAPS     dpcTriCaps;
	DWORD           dwDeviceRenderBitDepth;
	DWORD           dwDeviceZBufferBitDepth;
	DWORD           dwMaxBufferSize;
	DWORD           dwMaxVertexCount;
} D3DDEVICEDESC1, *LPD3DDEVICEDESC1;

int CalcPixelFormat(DDSURFACEDESC *ddesc, MARNI_SURFACE_DESC *sdesc);
HRESULT CALLBACK d3dEnumTextureProc(DDSURFACEDESC *surface_desc, void *ctx);
int D3DEnumTextureFormats(LPDIRECT3DDEVICE g_pDDdev, int max, DDSURFACEDESC *pdesc);

typedef struct tagRes
{
	int width,
		height,
		depth,
		fullscreen;
} res;

struct CBED_RES
{
	int width;
	int height;
	int depth;
	int pad;
};

struct CBED_CTX
{
	int max;
	CBED_RES *data;
	int id;
};

int cbEnumDisplayModes(DDSURFACEDESC *a1, CBED_CTX *ctx);
int CreatePrimarySurface(HWND hWnd, int Width, int Height, int Is_fullscreen, IDirectDraw *pDD, LPDIRECTDRAWSURFACE *dst, LPDIRECTDRAWSURFACE *main, LPDIRECTDRAWCLIPPER *clipper, int video_memory);
int CreateDevice(int enumerate, LPDIRECTDRAW *pDirectDraw, DWORD *a3);
int QueryInterface(IDirectDraw *pDD, LPDIRECTDRAW *lpDD);
int InvalidateWindow(HWND hWnd, int Width, int Height, int Is_fullscreen, RECT *lprc);
//int CMarni::SearchMatchTextureFormat(CMarniSurface2 *pSurface, DDSURFACEDESC *pDesc);
//CMarniSurface* CMarniSurface::constructor();
int CalcPixelFormat(DDSURFACEDESC *ddesc, MARNI_SURFACE_DESC *sdesc);
int GetSurfaceDesc(DDSURFACEDESC *dsdesc, LPDIRECTDRAWSURFACE a2);
void LoadIdentity(MARNI_MATRIX *dst);
int SetCooperativeLevel(HWND hWnd, int is_fullscreen, IDirectDraw *g_DD);
u32 D3DIBPPToDDBD(int bpp);
void Marni1Out(char *fmt, ...);
void Marni2Out(char *text, char *caption);
int Adjust_rect(RECT *lpRect, int *adjust, RECT *lpRect1);
DWORD upper_power_of_two(DWORD w, DWORD h);

void SetDisplayRect();
int SwitchResolution(int index);
int IsGpuActive();

extern int Max_resolutions,
	Display_mode;

class CRect : public RECT
{
public:
	__inline void Set(int left, int top, int right, int bottom)
	{
		this->left = left;
		this->right = right;
		this->top = top;
		this->bottom = bottom;
	}
	__inline void SetXYWH(int x, int y, int w, int h)
	{
		SetRect(this, x, y, x + w, y + h);
	}

	__inline LONG Width() { return right - left; }
	__inline LONG Height() { return bottom - top; }

	void operator = (RECT *r) { memcpy(&this->left, r, sizeof(*r)); }
};

void LoadFile(LPCSTR filename, u8* buffer);
