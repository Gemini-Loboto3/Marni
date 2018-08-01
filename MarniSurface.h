#pragma once
#include <stdafx.h>
#include "MarniUtil.h"

class CMarniSurface;
class CMarniSurfaceEx;

#define BLTMODE_MIRROR	0x10
#define BLTMODE_FLIP	0x20

// software surfaces
class CMarniSurface2
{
public:
	CMarniSurface2();
	~CMarniSurface2();

	CMarniSurface2* CMarniSurface2::constructor();

	virtual int ClearBg(int *adjust, int rgb, int use_image);
	virtual int Blt(RECT *dstrect, RECT *srcrect, CMarniSurface2 *src, int a5, int a6);
	virtual int Null();
	virtual int PalBlt(CMarniSurface2 *pSurfaceSrc, int reserved0, int colcnt, int reserved1);
	virtual int Lock(u8 **ppBitmap, u8 **ppPalette);
	virtual int Unlock();
	virtual int Release();

	u8 *pData;
	u8 *pPalette;
	int field_C;	// lock status
	MARNI_SURFACE_DESC sdesc;
	int dwWidth;
	int dwHeight;
	int lPitch;
	int field_38;
	int field_3C;
	int Is_open;
	int field_44;		// Is_copy
	int Has_palette;
	int field_4C;
	int field_50;

	int   WriteBitmap(LPCSTR lpFilename);
	int   copy(CMarniSurface2 &srf);
	void* CalcAddress(int x, int y);
	int   SetAddress(u8* pData, u8 *pPalette);
	int   SetIndexColor(int x, int y, u32 rgb, u32 flag);
	int   SetPaletteColor(int index, u32 rgb, int a3);
	int   SetColor(int x, int y, u32 rgb, int mode) { return 1; }
	int   SetCurrentColor(int x, int y, u32 rgb, u32 flag);
	int   GetColor(int x, int y, u32 *rgb) { return 1; }
	int   GetCurrentColor(int x, int y, u32 *pixel);
	int   GetPaletteColor(int index, u32 *rgb);
	int   GetIndexColor(int x, int y, u32 *rgb) { return 1; }
	
	int   CreateWork(int width, int height, int bmp_depth, int srf_depth);

	// operator stuff
	//__inline int operator = (CMarniSurface2 &srf)
	//{
	//	return this->copy(srf);
	//}
};

// hardware surfaces
class CMarniSurface : public CMarniSurface2
{
public:
	CMarniSurface();
	~CMarniSurface();

	virtual int ClearBg(int *adjust, int rgb, int use_image);
	virtual int Blt(RECT *dstrect, RECT *srcrect, CMarniSurface2 *src, int a5, int a6);
	virtual int Null();
	//virtual int PalBlt(DWORD *a2, int a3, int a4, int a5);
	virtual int Lock(u8 **ppBitmap, u8 **ppPalette);
	virtual int Unlock();
	virtual int Release();

	LPDIRECTDRAWSURFACE DDsurface;
	LPDIRECTDRAWPALETTE DDpalette;
	int Is_vmem;

	//CMarniSurface* __thiscall constructor();

	int clearBG(int *adjust, u32 rgb, int use_image);
	int Test(IDirectDraw *pDirectDraw, DDSURFACEDESC *pDesc);

	int CreateCompatibleSurface(IDirectDraw *pDD, DDSURFACEDESC *desc);
	int CreateOffscreenSurface(IDirectDraw *pDD, DDSURFACEDESC *desc);

	void BltSurface(RECT *dstrect, RECT *srcrect, CMarniSurface *pSrcSurface, int a5, int a6){}
	int  CreateOffscreenSurface(IDirectDraw *pDD, int dwWidth, int dwHeight);
	int  DirectDrawSurface(IDirectDraw *pDD, DDSURFACEDESC *pDesc);

	//int operator =(CMarniSurface2 &srf)
	//{
	//	return this->copy(srf);
	//}
};

// extended hardware surfaces with texture
class CMarniSurfaceEx : public CMarniSurface
{
public:
	CMarniSurfaceEx();
	~CMarniSurfaceEx();

	virtual int Release();

	D3DTEXTUREHANDLE TextureHandle;
	IDirect3DTexture* DDtexture;

	//CMarniSurfaceEx* __thiscall constructor();

	int CreateTextureObject();
	int GetTextureHandle(IDirect3DDevice *pD3DDevice);
};

///////////////////////////////////////////
class CMarni216
{
public:
	CMarni216()
	{
		pSurfEx = NULL;
		memset(&desc, 0, sizeof(desc));
		field_C8 = 0;
		field_CC = 0;
		field_D0 = 0;
		field_D4 = 0;
	}

	//int field_0;
	CMarniSurfaceEx *pSurfEx;
	CMarniSurface2 Surface;
	DDSURFACEDESC desc;
	int field_C8;
	int field_CC;
	int field_D0;
	int field_D4;
};

