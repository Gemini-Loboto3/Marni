#include "stdafx.h"
#include "Marni.h"
#include "PsxTexture.h"
#include "lodepng.h"

CPsxTexture::CPsxTexture()
{
}

CPsxTexture::~CPsxTexture()
{
}

typedef struct tagTimHead
{
	DWORD magic;
	DWORD type;
} TIM_HEAD;

typedef struct tagTimChunk
{
	DWORD size;
	u16 x, y,
		w, h;
} TIM_CHUNK;

typedef union tagMP
{
	u8 byte;
	u16 word;
	u32 dword;
} MPTR;

extern CMarni *pMarni;

int CPsxTexture::Store(u8 *pTim)
{
	TIM_HEAD *h = (TIM_HEAD*)pTim;

	if (h->magic != 0x10)
		return 0;

	pTim = (u8*)&h[1];
	int clut_bit = h->type & 7;
	int clut_sec = h->type >> 3;

	u16 *clut = NULL;
	if (clut_sec)
	{
		TIM_CHUNK *c = (TIM_CHUNK*)pTim;

		clut = (u16*)&c[1];

		pTim += c->size;
	}

	TIM_CHUNK *p = (TIM_CHUNK*)pTim;
	int width, height = p->h;
	switch (clut_bit)
	{
	case 0:	// 4 bits
		width = p->w * 4; break;
	case 1:	// 8 bits
		width = p->w * 2; break;
	case 2:	// 15 bits
		width = p->w; break;
	case 3:	// 24 bits
		width = p->w * 3 / 2; break;
	}

	dwWidth = width;
	dwHeight = height;

	DDSURFACEDESC desc;
	desc.dwWidth = width;
	desc.dwHeight = height;
	surf.CreateOffscreenSurface(pMarni->pDirectDraw, &desc);
	surf.Lock(NULL, NULL);

	u8 *pp = (u8*)&p[1];

	for (int y = 0; y < height; y++)
	{
		switch (clut_bit)
		{
		case 0:	// 4 bits
			for (int x = 0; x < width; x++)
			{
				u8 n, r, g, b, a;
				if (x & 1) n = *pp++ >> 4;
				else n = *pp & 0xf;
				r = (clut[n] & 0x1f   )        * 8;
				g = ((clut[n] & 0x3e0 ) >> 5)  * 8;
				b = ((clut[n] & 0x7c00) >> 10) * 8;
				a = clut[n] & 0x8000 ? 255 : 0;
				surf.SetCurrentColor(x, y, b | (g << 8) | (r << 16) | (a << 24), 0);
			}
			break;
		case 1:	// 8 bits
			for (int x = 0; x < width; x++, pp++)
			{
				u8 n, r, g, b, a;
				n = *pp;
				r = (clut[n] & 0x1f   )        * 8;
				g = ((clut[n] & 0x3e0 ) >> 5)  * 8;
				b = ((clut[n] & 0x7c00) >> 10) * 8;
				a = clut[n] & 0x8000 ? 255 : 0;
				surf.SetCurrentColor(x, y, b | (g << 8) | (r << 16) | (a << 24), 0);
			}
			break;
		case 2:	// 15 bits
			for (int x = 0; x < width; x++, pp+=2)
			{
				u16 n;
				u8 r, g, b, a;
				n = *(WORD*)pp;
				r = ( n & 0x1f  )        * 8;
				g = ((n & 0x3e0 ) >> 5)  * 8;
				b = ((n & 0x7c00) >> 10) * 8;
				a = n & 0x8000 ? 255 : 0;
				surf.SetCurrentColor(x, y, b | (g << 8) | (r << 16) | (a << 24), 0);
			}
			break;
		case 3:	// 24 bits
			for (int x = 0; x < width; x++)
			{
				surf.SetCurrentColor(x, y, pp[2] | (pp[1] << 8) | (pp[0] << 16) | (255 << 24), 0);
				pp += 3;
			}
			break;
		}
	}

	surf.Unlock();

	return 1;
}

int CPsxTexture::StorePng(LPCSTR filename)
{
	unsigned width, height;
	std::vector<unsigned char> out;
	lodepng::decode(out, width, height, filename, LCT_RGBA);

	dwWidth = width;
	dwHeight = height;

	DDSURFACEDESC desc;
	desc.dwWidth = width;
	desc.dwHeight = height;
	surf.CreateOffscreenSurface(pMarni->pDirectDraw, &desc);
	surf.Lock(NULL, NULL);

	u8 *pp = &out[0];
	for (unsigned y = 0; y < height; y++)
	{
		for (unsigned x = 0; x < width; x++)
		{
			surf.SetCurrentColor(x, y, pp[2] | (pp[1] << 8) | (pp[0] << 16) | (pp[3] << 24), 0);
			pp += 4;
		}
	}

	surf.Unlock();

	return 1;
}
