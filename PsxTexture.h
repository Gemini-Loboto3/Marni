#pragma once
#include "MarniSurface.h"

#define MAX_TEXTURES	8

class CPsxTexture
{
public:
	CPsxTexture();
	~CPsxTexture();

	CMarniSurface surf;
	CMarniSurface srfx[MAX_TEXTURES - 1];

	int Store(u8 *pTim);
	int StorePng(LPCSTR filename);

	DWORD dwWidth,
		dwHeight;
};

