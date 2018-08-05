#include "Marni.h"

int CalcPixelFormat(DDSURFACEDESC *ddesc, MARNI_SURFACE_DESC *sdesc)
{
	if (ddesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8)
	{
		sdesc->Pal_index = 1;
		return 1;
	}
	if (ddesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED4)
	{
		sdesc->Pal_index = 0;
		return 1;
	}

	switch (ddesc->ddpfPixelFormat.dwRGBBitCount)
	{
	case 8:
		sdesc->Pal_index = 1;
		break;
	case 16:
		sdesc->Pal_index = 2;
		break;
	case 32:
		sdesc->Pal_index = 4;
		break;
	default:
		return 0;
	}

	u32 mask = ddesc->ddpfPixelFormat.dwRBitMask;
	sdesc->dwRBitMask_setcnt = 0;
	if (mask)
	{
		for (; sdesc->dwRBitMask_setcnt < 32;)
		{
			if (mask & 1) break;
			mask >>= 1;
			sdesc->dwRBitMask_setcnt++;
		}
		sdesc->field_4 = 0;
		for (; sdesc->field_4 < 32;)
		{
			if (!(mask & 1)) break;
			mask >>= 1;
			sdesc->field_4++;
		}
	}
	else sdesc->field_4 = 0;
	sdesc->field_2 = (1 << sdesc->field_4) - 1;

	mask = ddesc->ddpfPixelFormat.dwGBitMask;
	sdesc->dwRBitMask_cnt = 0;
	if (mask)
	{
		for(; sdesc->dwRBitMask_cnt < 32;)
		{
			if (mask & 1)
				break;
			mask >>= 1;
			sdesc->dwRBitMask_cnt++;
		}

		sdesc->field_A = 0;
		for (; sdesc->field_A < 32;)
		{
			if (!(mask & 1))
				break;
			mask >>= 1;
			sdesc->field_A++;
		}
	}
	else sdesc->field_A = 0;
	sdesc->field_8 = (1 << sdesc->field_A) - 1;

	mask = ddesc->ddpfPixelFormat.dwBBitMask;
	sdesc->dwBBitMask_setcnt = 0;
	if (mask)
	{
		for (; sdesc->dwBBitMask_setcnt < 32;)
		{
			if (mask & 1)
				break;
			mask >>= 1;
			sdesc->dwBBitMask_setcnt++;
		}

		sdesc->field_10 = 0;
		for (; sdesc->field_10 < 32;)
		{
			if (!(mask & 1))
				break;
			mask >>= 1;
			sdesc->field_10++;
		}
	}
	else sdesc->field_10 = 0;
	sdesc->field_E = (1 << sdesc->field_10) - 1;

	mask = ddesc->ddpfPixelFormat.dwRGBAlphaBitMask;
	sdesc->dwRGBAlphaBitMask_setcnt = 0;
	if (mask)
	{
		for (; sdesc->dwRGBAlphaBitMask_setcnt < 32;)
		{
			if (mask & 1)
				break;
			mask >>= 1;
			sdesc->dwRGBAlphaBitMask_setcnt++;
		}
		sdesc->field_16 = 0;
		for (; sdesc->field_16 < 32;)
		{
			if (!(mask & 1))
				break;
			mask >>= 1;
			sdesc->field_16++;
		}
	}
	else sdesc->field_16 = 0;
	sdesc->field_14 = (1 << sdesc->field_16) - 1;

	return 1;
}

HRESULT CALLBACK d3dEnumTextureProc(DDSURFACEDESC *surface_desc, void *ctx)
{
	CTX_TEXTURE *tex = (CTX_TEXTURE*)ctx;

	if (tex->max > tex->count)
	{
		memcpy(&tex->desc[tex->count], surface_desc, sizeof(DDSURFACEDESC));
		tex->count++;
		return 1;
	}

	return 0;
}

int D3DEnumTextureFormats(LPDIRECT3DDEVICE g_pDDdev, int max, DDSURFACEDESC *pdesc)
{
	CTX_TEXTURE ctx;

	ctx.count = 0;
	ctx.max = max;
	if (g_pDDdev->EnumTextureFormats(d3dEnumTextureProc, &ctx))
		return 0;

	if (max > ctx.count)
	{
		memcpy(pdesc, ctx.desc, sizeof(DDSURFACEDESC) * ctx.count);
		return ctx.count;
	}

	return 0;
}

#include <stdio.h>

#ifdef _DEBUG
static int ix = 0;
#endif
int cbEnumDisplayModes(DDSURFACEDESC *a1, CBED_CTX *ctx)
{
	int i = ctx->id;

#ifdef _DEBUG
	printf("%02d: %dx%d @%d\n", ix++, a1->dwWidth, a1->dwHeight, a1->ddpfPixelFormat.dwRGBBitCount);
#endif

	if (i < ctx->max)
	{
		if (a1->ddpfPixelFormat.dwRGBBitCount == 32 && a1->dwWidth >= 640 && a1->dwHeight >= 480)
		{
			ctx->data[i].width = a1->dwWidth;
			ctx->data[i].height = a1->dwHeight;
			ctx->data[i].depth = a1->ddpfPixelFormat.dwRGBBitCount;
			ctx->id++;
		}
		return 1;
	}

	return 0;
}

int CreatePrimarySurface(HWND hWnd, int Width, int Height, int Is_fullscreen, IDirectDraw *pDD, LPDIRECTDRAWSURFACE *dst, LPDIRECTDRAWSURFACE *main,
	LPDIRECTDRAWCLIPPER *clipper, int video_memory)
{
	try
	{
		if (Width < 50) Width = 50;
		if (Height < 50) Height = 50;

		*main = NULL;
		*dst = NULL;
		*clipper = NULL;

		DDSURFACEDESC dsdesc;
		memset(&dsdesc, 0, sizeof(dsdesc));
		dsdesc.dwSize = sizeof(dsdesc);
		if (!Is_fullscreen)
		{
			// primary surface
			dsdesc.dwFlags = DDSD_CAPS;
			dsdesc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
			if (pDD->CreateSurface(&dsdesc, dst, NULL))
				throw 0;
			// back surface
			if (!video_memory) dsdesc.ddsCaps.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
			else dsdesc.ddsCaps.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_OFFSCREENPLAIN;
			dsdesc.dwWidth = Width;
			dsdesc.dwHeight = Height;
			dsdesc.dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_CAPS;
			if (pDD->CreateSurface(&dsdesc, main, NULL))
				throw 0;
			if (pDD->CreateClipper(NULL, clipper, NULL))
				throw 0;
			if ((*clipper)->SetHWnd(NULL, hWnd))
				throw 0;
			if ((*dst)->SetClipper(*clipper))
				throw 0;
		}
		else
		{
			// primary surface
			dsdesc.dwFlags = DDSD_BACKBUFFERCOUNT | DDSD_CAPS;
			dsdesc.ddsCaps.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX;
			dsdesc.dwBackBufferCount = 1;
			if (pDD->CreateSurface(&dsdesc, dst, NULL))
				throw 0;

			DDSCAPS caps;
			caps.dwCaps = sizeof(caps);
			if ((*dst)->GetAttachedSurface(&caps, main))
				throw 0;

			RECT rc;
			DDBLTFX fx;
			GetSurfaceDesc(&dsdesc, *dst);
			SetRect(&rc, 0, 0, dsdesc.dwWidth, dsdesc.dwHeight);
			memset(&fx, 0, sizeof(fx));
			fx.dwSize = sizeof(fx);
			fx.dwFillColor = 0;
			(*dst)->Blt(&rc, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
		}
	}
	catch (int)
	{
		if (*main) { (*main)->Release(); *main = NULL; }
		if (*dst) { (*dst)->Release(); *dst = NULL; }
		if (*clipper) { (*clipper)->Release(); *clipper = NULL; }

		return 1;
	}

	return 0;
}

BOOL CALLBACK cbCreateDevice(GUID *guid, char *driver_description, char *driver_name, void *ctx)
{
	LPDIRECTDRAW lpDD;
	DDCAPS hal_caps,
		hel_caps;

	if (guid)
	{
		if (DirectDrawCreate(guid, &lpDD, 0))
			return 1;
		memset(&hal_caps, 0, sizeof(hal_caps));
		hal_caps.dwSize = sizeof(hal_caps);
		memset(&hel_caps, 0, sizeof(hel_caps));
		hel_caps.dwSize = sizeof(hel_caps);
		if (lpDD->GetCaps(&hal_caps, &hel_caps))
		{
			lpDD->Release();
			return 1;
		}
		if (hal_caps.dwCaps & DDCAPS_3D)
		{
			ctx = lpDD;
			return 0;
		}
		lpDD->Release();
	}
	return 1;
}

int CreateDevice(int enumerate, LPDIRECTDRAW *pDirectDraw, DWORD *a3)
{
	LPDIRECTDRAW lpDirectDraw;

	lpDirectDraw = NULL;
	if (enumerate)
	{
		if (DirectDrawEnumerateA(cbCreateDevice, &lpDirectDraw))
		{
			Marni2Out("", "");
			return 1;
		}

		if (lpDirectDraw) *a3 = 0;
	}

	if (!lpDirectDraw)
	{
		if (DirectDrawCreate(0, &lpDirectDraw, 0))
		{
			Marni2Out("", "");
			return 1;
		}
		*a3 = 1;
	}
	*pDirectDraw = lpDirectDraw;
	return 0;
}

int QueryInterface(IDirectDraw *pDD, LPDIRECTDRAW *lpDD)
{
	return pDD->QueryInterface(IID_IDirectDraw2, (LPVOID *)lpDD);
}

int InvalidateWindow(HWND hWnd, int Width, int Height, int Is_fullscreen, RECT *lprc)
{
	RECT rc;

	// basic resolution rect
	SetRect(&rc, 0, 0, Width, Height);
	// change size and position depending on current window style
	AdjustWindowRectEx(&rc, GetWindowLongA(hWnd, GWL_STYLE), GetMenu(hWnd) ? 1 : 0, GetWindowLongA(hWnd, GWL_EXSTYLE));
	// move the window
	SetWindowPos(hWnd, HWND_TOP,       0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
	SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0,                  0,                  SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	
	if (lprc)
	{
		POINT p0, p1;
		p0.y = 0;
		p0.x = 0;
		ClientToScreen(hWnd, &p0);
		p1.x = Width;
		p1.y = Height;
		ClientToScreen(hWnd, &p1);
		SetRect(lprc, p0.x, p0.y, p1.x, p1.y);
	}
	// time to repaint it
	InvalidateRect(hWnd, NULL, TRUE);

	return 1;
}

int GetSurfaceDesc(DDSURFACEDESC *dsdesc, LPDIRECTDRAWSURFACE a2)
{
	memset(dsdesc, 0, sizeof(DDSURFACEDESC));
	dsdesc->dwSize = 108;
	return a2->GetSurfaceDesc(dsdesc);
}

void LoadIdentity(MARNI_MATRIX *dst)
{
	static MARNI_MATRIX id =
	{
		1.f, 0, 0, 0,
		0, 1.f, 0, 0,
		0, 0, 1.f, 0,
		0, 0, 0, 1.f
	};

	memcpy(dst, &id, sizeof(id));
}

int SetCooperativeLevel(HWND hWnd, int is_fullscreen, IDirectDraw *g_DD)
{
	if (is_fullscreen)
	{
		if (g_DD->SetCooperativeLevel(hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN))
		{
			Marni2Out("SetCooperativeLevel", "MarniSystem DDSetCoopLevel");
			return 1;
		}
	}
	else
	{
		if (g_DD->SetCooperativeLevel(hWnd, DDSCL_NORMAL))
		{
			Marni2Out("SetCooperativeLevel", "MarniSystem DDSetCoopLevel");
			return 1;
		}
		if (g_DD->RestoreDisplayMode())
			return 0;
	}
	return 0;
}

u32 D3DIBPPToDDBD(int bpp)
{
	switch (bpp)
	{
	case 1:  return DDBD_1;
	case 2:  return DDBD_2;
	case 4:  return DDBD_4;
	case 8:  return DDBD_8;
	case 16: return DDBD_16;
	case 24: return DDBD_24;
	case 32: return DDBD_32;
	}

	Marni1Out("shit failed with conversion from %d bpp [D3DIBPPToDDBD]", bpp);
	return 0;
}

void Marni1Out(char *fmt, ...)
{
	va_list myargs;
	char buffer[2048];

	va_start(myargs, fmt);
	vsprintf_s(buffer, sizeof(buffer), fmt, myargs);
	strcat_s(buffer, sizeof(buffer), "\n");
	OutputDebugString(buffer);
	va_end(myargs);
}

void Marni2Out(char *text, char *caption)
{
	char buffer[2048];

	sprintf_s(buffer, sizeof(buffer), "[%s] %s\n", caption, text);
	OutputDebugString(buffer);
}

int Adjust_rect(RECT *lpRect, int *adjust, RECT *lpRect1)
{
	int right; // esi
	int bottom; // edx
	int left; // edi
	LONG top; // ebx

	right = lpRect->right;
	if (right < *adjust)
		return 0;
	bottom = lpRect->bottom;
	if (adjust[1] > bottom)
		return 0;
	left = lpRect->left;
	if (adjust[2] < lpRect->left)
		return 0;
	top = lpRect->top;
	if (top > adjust[3])
		return 0;
	if (adjust[2] - *adjust <= 0 || adjust[3] - adjust[1] <= 0)
		return 0;
	if (right - left <= 0 || bottom - top <= 0)
		return 0;
	if (*adjust >= left)
		left = *adjust;
	if (adjust[1] >= top)
		top = adjust[1];
	if (adjust[2] <= right)
		right = adjust[2];
	if (adjust[3] <= bottom)
		bottom = adjust[3];
	lpRect1->left = left;
	lpRect1->top = top;
	lpRect1->right = right;
	lpRect1->bottom = bottom;
	return 1;
}

DWORD upper_power_of_two(DWORD w, DWORD h)
{
	DWORD v = w > h ? w : h;

	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

extern CMarni *pMarni;
extern HWND hWnd;
static int Is_fullscreen,
	Is_now_fullscreen,
	Display_cursor,
	Res_switched;
int Max_resolutions,
	Display_mode;

typedef struct tagResolution
{
	MARNI_RES res;
	int mode;
} RES;

static RES resolutions[64];

void SetDisplayRect()
{
	if (!pMarni)
		return;

	Max_resolutions = pMarni->RequestDisplayModeCount();
	for (int i = 0, si = Max_resolutions; i < si; i++)
	{

		MARNI_RES res;
		pMarni->RequestDisplayRect(i, &res);
		memcpy(&resolutions[i].res, &res, sizeof(MARNI_RES));
		resolutions[i].mode = i;	// kinda redundant
	}
}

int SwitchResolution(int index)
{
	if (index >= Max_resolutions)
		return 0;

	if (!pMarni->ChangeMode(resolutions[index].mode))
		return 0;

	int Is_fullscreen = resolutions[index].res.Fullscreen;
	Display_mode = index;
	Is_now_fullscreen = Is_fullscreen;
	if (Is_fullscreen)
	{
		if (!Display_cursor)
			ShowCursor(FALSE);
		Display_cursor = 0;
	}
	else
	{
		if (!Display_cursor)
			ShowCursor(TRUE);
		Display_cursor = 1;
	}
	// there should be a call to Move_movie_window() here

	Res_switched = 1;
	return 1;
}

int IsGpuActive()
{
	if (pMarni)
	{
		if (!pMarni->Is_gpu_init)
		{
			Marni2Out("win is sleeping...", __FILE__);
			return 0;
		}
	}
	else if (hWnd)
	{
		// marni is not created but window is
		Marni2Out("win whatever", __FILE__);
		return 0;
	}

	return 1;
}

#include <vector>
#include "lodepng.h"

void encodeOneStep(const char* filename, std::vector<u8>& image, u32 width, u32 height)
{
	lodepng::encode(filename, image, width, height);
}

//Example 2
//Encode from raw pixels to an in-memory PNG file first, then write it to disk
//The image argument has width * height RGBA pixels or width * height * 4 bytes
void encodeTwoSteps(const char* filename, std::vector<u8>& image, u32 width, u32 height)
{
	std::vector<unsigned char> png;

	lodepng::encode(png, image, width, height);
	lodepng::save_file(png, filename);
}

//Example 3
//Save a PNG file to disk using a State, normally needed for more advanced usage.
//The image argument has width * height RGBA pixels or width * height * 4 bytes
void encodeWithState(const char* filename, std::vector<u8>& image, u32 width, u32 height)
{
	std::vector<unsigned char> png;
	lodepng::State state; //optionally customize this one

	lodepng::encode(png, image, width, height, state);
	lodepng::save_file(png, filename);
}

void LoadFile(LPCSTR filename, u8* buffer)
{
	FILE *f;
	if (fopen_s(&f, filename, "rb+"))
		return;

	size_t size;
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);

	fread(buffer, size, 1, f);
	fclose(f);
}
