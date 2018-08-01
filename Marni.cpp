#include "stdafx.h"
#include "Marni.h"
#include "debug_new.h"
#include <assert.h>

typedef struct tagMarniDevice
{
	char name[30];
	char description[50];
	D3DDEVICEDESC1 device_desc;
	GUID guid;
	DWORD Is_hal,
		Is_mono,
		has_zbuffer,
		Bpp_compatible;
} MARNI_DEVICE;

DWORD DeviceId;
MARNI_DEVICE Marni_device[20];
HRESULT error;

CMarni::CMarni()
{
}


CMarni::~CMarni()
{
	this->ClearBuffers();
	delete[] this->pPtr_2KBuffer;
	this->pPtr_2KBuffer = NULL;
	if (this->Is_fullscreen && this->pDirectDraw)
	{
		this->Is_busy_ = 1;
		this->pDirectDraw->RestoreDisplayMode();
		this->Is_busy_ = 0;
	}

	if (this->pZBuffer_)
	{
		this->pZBuffer_->Release();
		this->pZBuffer_ = NULL;
	}
	if (this->pDirectDraw)
	{
		this->pDirectDraw->Release();
		this->pDirectDraw = NULL;
	}
	delete[] this->pTexture_buffer;
	this->pTexture_buffer = NULL;
}

////////////////////////////////////////
// VIRTUAL METHODS
int CMarni::ClearBg()
{
	if (!this->Is_active || !this->Is_gpu_init || this->Is_minimized)
		return 0;

	// needs device and viewport for hardware mode
	if (this->card != GFX_SOFTWARE && (!this->pD3DDevice || !this->pViewport))
		return 0;

	int flags;
	D3DRECT rect;

	rect.x1 = 0;
	rect.y1 = 0;
	rect.x2 = this->XSize;
	rect.y2 = this->YSize;

	// don't need bg cleared
	if (this->ClearBG_)
	{
		// do nothing in software mode
		if (this->card == GFX_SOFTWARE)
			return 1;
		flags = 2;
	}
	// clear bg
	else
	{
		if (this->card == GFX_SOFTWARE)
		{
			this->MarniBitsMain.ClearBg(NULL, this->ClearRGB, 0);
			return 1;
		}
		D3DMATERIAL mat;
		D3DMATERIALHANDLE handle;
		this->Material->SetMaterial(&mat);
		this->Material->GetHandle(this->pD3DDevice, &handle);
		this->pViewport->SetBackground(handle);
		flags = 3;
	}

	if (this->pViewport->Clear(1, &rect, flags))
		return 0;

	return 1;
}

int CMarni::RequestVideoMemory()
{
	if (!this->Is_gpu_init)
		return 0;

	// software mode returns a random valid value
	if (this->card == GFX_SOFTWARE)
		return 123456789;

	// hardware mode tries to detect the actual memory available
	if (this->pDirectDraw)
	{
		DDCAPS ddesc;
		memset(&ddesc, 0, sizeof(ddesc));
		ddesc.dwSize = sizeof(ddesc);
		if (this->pDirectDraw->GetCaps(&ddesc, NULL))
			return 0;
		this->dwVidMemFree = ddesc.dwVidMemFree;
		return this->dwVidMemFree;
	}

	return 0;
}

int CMarni::ChangeMode(int display_mode)
{
	if (display_mode >= this->Resolution_count)
		return 0;

	int old = this->display_mode;
	MARNI_RES *pRes = &this->Resolutions[display_mode];
	this->Is_fullscreen = pRes->Fullscreen;
	this->display_mode = display_mode;
	if (!this->ChangeResolution(pRes->W, pRes->H, pRes->Depth))
	{
		// failed, revert back
		this->display_mode = old;
		return 0;
	}

	this->field_308 = 0;
	return 1;
}

int CMarni::ChangeResolution(int display_mode)
{
	this->DeviceID = display_mode & DeviceId;
	this->ChangeResolution(this->XSize, this->YSize, this->BitDepth);

	return 1;
}

//int CMarni::Clear(DWORD *a2, int a3, int a4, int a5) { return 1; }
int CMarni::Render()
{
	if (this->Is_minimized)
		return 1;
	if (!this->Is_active)
		return 1;

	this->RestoreLostSurface();
	//this->doRender(&this->OTag0);

	if (this->card == GFX_SOFTWARE && this->Is_fullscreen && this->field_308++ < 4)
		this->MarniBitsDst.ClearBg(NULL, 0, 0);

	if (!this->Is_fullscreen)
	{
		RECT rc;
		SetRect(&rc, 0, 0, (int)(this->Render_w * this->Aspect_x - 1.f), (int)(this->Render_h * this->Aspect_y - 1.f));
		this->MarniBitsDst.BltSurface(&this->Resolution_rect, &rc, &this->MarniBitsMain, 0, 0);
		return 1;
	}
	// software mode
	if (this->card == GFX_SOFTWARE)
	{
		RECT rc;
		SetRect(&rc, 0, 0, (int)(this->Render_w * this->Aspect_x - 1.f), (int)(this->Render_h * this->Aspect_y - 1.f));
		if (this->Softfull_rescnt + 2 > this->display_mode)
			this->MarniBitsDst.BltSurface(&this->Resolution_rect, &rc, &this->MarniBitsMain, 0, 0);
		else
		{
			RECT r1;
			r1.left = (int)(this->Render_w * this->Aspect_x * -0.5 + (this->Resolution_rect.right / 2));
			r1.top = (int)(this->Render_h * this->Aspect_y * -0.5 + (this->Resolution_rect.bottom / 2));
			r1.right = r1.left + rc.right;
			r1.bottom = r1.top + rc.bottom;
			this->MarniBitsDst.BltSurface(&this->Resolution_rect, &rc, &this->MarniBitsMain, 0, 0);
		}
	}
	// hardware mode
	else this->MarniBitsDst.DDsurface->Flip(this->MarniBitsMain.DDsurface, 1);

	return 1;
}

int CMarni::Message(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_DESTROY:
		this->Destroy();
		break;
	case WM_MOVE:
		this->ClientToScreen();
		break;
	case WM_SIZE:
		if (!this->Is_busy_)
			this->Resize(hWnd, Msg, wParam, lParam);
		break;
	case WM_ACTIVATE:
		this->Is_active = LOWORD(wParam) ? 1 : 0;
		break;
	case WM_ACTIVATEAPP:
		this->Is_active = wParam ? 1 : 0;
		break;
	}

	return 1;
}

int CMarni::CreateTexture(CMarniSurface *pSrf, DWORD *a3, DWORD *a4)
{
	if (!this->Is_gpu_init)
		return 0;

	if (pSrf->dwWidth > 256 || pSrf->dwHeight > 256)
		return 0;

	int index = 1;
	CMarni216 **pSearch = &this->pSurfaces[index];
	for (;; index++, pSearch++)
	{
		if (index >= 512) return 0;
		if (!*pSearch) break;
	}

	CMarni216 *pObj = new CMarni216;
	this->pSurfaces[index] = pObj;
	//pObj->field_0 = 0;
	pObj->pSurfEx = NULL;
	pObj->field_D0 = (int)a3;
	pObj->field_D4 = 0;
	memset(&pObj->desc, 0, sizeof(pObj->desc));

	// only in hardware mode
	if (this->card != GFX_SOFTWARE)
	{
		if (pSrf->Has_palette)
		{
			pObj->Surface.CreateWork(pSrf->dwWidth, pSrf->dwHeight, pSrf->sdesc.dwRGBBitCount, pSrf->sdesc.bpp);
			pObj->Surface.Blt(NULL, NULL, pSrf, 0, 0);
			pObj->Surface.Lock(NULL, NULL);
			pSrf->Lock(NULL, NULL);
			for (int i = 0, si = 1 << pSrf->sdesc.dwRGBBitCount; i < si; i++)
			{
				u32 p;
				pSrf->GetPaletteColor(i, &p);
				pObj->Surface.SetPaletteColor(i, p, 0);
			}
			pObj->Surface.Unlock();
			pSrf->Unlock();
		}
		else
		{
			pObj->Surface.CreateWork(pSrf->dwWidth, pSrf->dwHeight, pSrf->sdesc.dwRGBBitCount, 0);
			memcpy(&pObj->Surface.sdesc, &pSrf->sdesc, sizeof(pSrf->sdesc));
			pObj->Surface = *pSrf;
		}

		if (a4) *a4 = 1;

		if (!this->ReloadTexture(index))
		{
			if (this->pSurfaces[index])
				delete this->pSurfaces[index];
			return 0;
		}

		this->ClearBg();
		this->Vmem_stats();
	}
	// software mode
	else
	{
		if (pSrf->Has_palette)
		{
		}
		else
		{
		}

		if (pSrf->sdesc.dwRGBBitCount == this->MarniBitsDst.sdesc.dwRGBBitCount && *a3 & 2 && this->Use_VRAM)
		{
			CMarniSurface *pS = new CMarniSurface;
			pObj->pSurfEx = (CMarniSurfaceEx*)pS;

			if (pObj->pSurfEx->CreateOffscreenSurface(this->pDirectDraw, pSrf->dwWidth, pSrf->dwHeight))
			{
				RECT rc;
				SetRect(&rc, 0, 0, pSrf->dwWidth - 1, pSrf->dwHeight - 1);
				pObj->pSurfEx->Blt(&rc, &rc, &pObj->Surface, 0, 0);
			}
			else
			{
				delete pS;
				pObj->pSurfEx = NULL;
				return 0;
			}
		}
	}

	return index;
}

int CMarni::CreateObject(DWORD *a2, DWORD a3) { return 1; }
int CMarni::Release(int type)
{
	try
	{
		if (!type)
			return 0;
		if (!this->pSurfaces[type])
			return 0;
		if (this->card == GFX_SOFTWARE)
		{

		}
	}
	catch (int a)
	{
		switch (a)
		{
		case 0:
			break;
		case 1:
			delete this->pSurfaces[type];
			break;
		}
		return 0;
	}

	return 1;
}
int CMarni::ClearBuffer(int type)
{
	if (this->pPtr_2KBuffer[type])
		delete this->pPtr_2KBuffer[type];
	this->pPtr_2KBuffer[type] = NULL;

	return 1;
}
int CMarni::LinkPoly(DWORD *pPoly, int otz) { return 1; }
int CMarni::InitPrims()
{
	this->OTag0.InitOTag();
	this->OTag1.InitOTag();

	//pPrim = &unk_7E13C0;

	return 1;
}

////////////////////////////////////////
// REGULAR METHODS
int CMarni::ChangeResolution(int Width, int Height, int BitsPerPixel)
{
	if (!this->Is_active)
		return 0;
	this->Is_active = 0;
	if (this->card == GFX_SOFTWARE)
	{
		this->Aspect_x = 1.0;
		this->Aspect_y = 1.0;
	}
	else
	{
		this->Aspect_x = (float)Width / (float)this->Render_w;
		this->Aspect_y = (float)Height / (float)this->Render_h;
	}
	// copy for quick recover
	this->ScreenX_old = this->XSize;
	this->ScreenY_old = this->YSize;
	this->Is_fullscreen_old = this->Is_fullscreen;
	this->BitDepth_old = this->BitDepth;
	// assign new sizes
	this->XSize = Width;
	this->YSize = Height;
	this->BitDepth = BitsPerPixel;
	this->ClearBuffers();
	// check if this initializes fine
	if (!this->InitAll())
	{
		// failed, try to fall back to previous mode
		this->XSize = this->ScreenX_old;
		this->YSize = this->ScreenY_old;
		this->BitDepth = this->BitDepth_old;
		this->ClearBuffers();
		if (!this->InitAll())
		{
			// can't go back, oops
			this->ClearBuffers();
			return 0;
		}
	}

	if (this->ReloadTexture())
	{
		this->Is_active = 1;
		return 1;
	}

	// couldn't load textures in this resolution, fall back to previous
	this->XSize = this->ScreenX_old;
	this->YSize = this->ScreenY_old;
	this->BitDepth = this->BitDepth_old;
	this->ClearBuffers();
	if (!this->InitAll())
	{
		// can't go back, oops
		this->ClearBuffers();
		return 0;
	}

	if (this->ReloadTexture())
	{
		// recovered successfully
		this->Is_active = 1;
		return 1;
	}

	// we're fucked pretty badly
	this->ClearBuffers();
	return 0;
}

//void CMarni::Clear()
//{
//}

void CMarni::ClientToScreen()
{
	POINT p0, p1;

	p0.y = 0;
	p0.x = 0;
	::ClientToScreen(this->hWnd, &p0);
	p1.x = this->XSize;
	p1.y = this->YSize;
	::ClientToScreen(this->hWnd, &p1);
	SetRect(&this->Resolution_rect, p0.x, p0.y, p1.x - 1, p1.y - 1);
}

void CMarni::Destroy()
{
	this->ClearBuffers();
	for (int i = 0; i < 512; i++)
		this->Release(i);
	if (this->field_1D80)
	{
		// some call to ->Release, find what object it is
		this->field_1D80 = NULL;
	}

	if (this->Clipper)
	{
		this->Clipper->Release();
		this->Clipper = NULL;
	}
	this->MarniBitsDst.Release();
	this->MarniBitsMain.Release();

	if (this->Is_fullscreen)
	{
		this->Is_busy_ = 1;
		this->pDirectDraw->RestoreDisplayMode();
		SetCooperativeLevel(this->hWnd, 0, this->pDirectDraw);
		this->Is_busy_ = 0;
	}

	if (this->pZBuffer_)
	{
		this->pZBuffer_->Release();
		this->pZBuffer_ = NULL;
	}

	if (this->pDirectDraw)
	{
		this->pDirectDraw->Release();
		this->pDirectDraw = NULL;
	}
}

int CMarni::Resize(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (!this->Is_gpu_init)
		return 1;
	if (wParam == SIZE_MINIMIZED)
	{
		this->Is_minimized = 1;
		DefWindowProcA(hWnd, Msg, SIZE_MINIMIZED, lParam);
		return 1;
	}

	if (this->Is_fullscreen)
	{
		this->Is_busy_ = 1;
		error = SetCooperativeLevel(this->hWnd, this->Is_fullscreen, this->pDirectDraw);
		this->Is_busy_ = 1;
		error = this->pDirectDraw->SetDisplayMode(
			this->Resolutions[this->display_mode].W,
			this->Resolutions[this->display_mode].H,
			this->Resolutions[this->display_mode].Depth);
		this->Is_busy_ = 0;
		if (error)
		{
			this->Is_fullscreen = 0;
			return 0;
		}
		else
		{
			if (this->card != GFX_SOFTWARE || this->Softfull_rescnt + 2 > this->display_mode)
			{
				this->XSize = this->Resolutions[this->display_mode].W;
				this->YSize = this->Resolutions[this->display_mode].H;
				SetRect(&this->Resolution_rect, 0, 0, this->XSize, this->YSize);
			}
			else
			{
				this->XSize = this->Render_w;
				this->YSize = this->Render_h;
				SetRect(&this->Resolution_rect, 0, 0, this->Resolutions[this->display_mode].W, this->Resolutions[this->display_mode].H);
			}
			this->RestoreLostSurface();
			this->ReloadTexture();
			this->Is_minimized = 0;
			DefWindowProcA(hWnd, Msg, wParam, lParam);
			this->MarniBitsDst.ClearBg(0, 0, 0);
			this->field_308 = 0;
		}
	}
	else if (this->Is_minimized)
	{
		this->Is_minimized = 0;
		DefWindowProcA(hWnd, Msg, wParam, lParam);
		return 1;
	}
	else if (this->Is_active)
	{
		this->Is_active = 0;
		this->XSize = LOWORD(lParam);
		this->YSize = HIWORD(lParam);
		this->Aspect_x = (float)LOWORD(lParam) / (float)this->Render_w;
		this->Aspect_y = (float)HIWORD(lParam) / (float)this->Render_h;
		this->ClearBuffers();
		if (this->InitAll())
		{
			this->ReloadTexture();
			this->Is_active = 1;
			DefWindowProcA(hWnd, Msg, wParam, lParam);
			return 1;
		}
	}

	return 0;
}

int CMarni::RestoreLostSurface()
{
#define IS_LOST(x)	((x) \
		&& (x)->IsLost() == DDERR_SURFACELOST \
		&& (x)->Restore())

	if (IS_LOST(this->MarniBitsDst.DDsurface))
		return 0;
	if (IS_LOST(this->MarniBitsMain.DDsurface))
		return 0;
	// no more checks in software mode
	if (this->card == GFX_SOFTWARE)
		return 1;
	// keep going if hardware mode
	if (IS_LOST(this->MarniBits1.DDsurface))
		return 0;
	for (int i = 0; i < 512; i++)
	{
		CMarni216 *p = this->pSurfaces[i];
		if (p && p->pSurfEx->Is_open && p->pSurfEx->DDsurface->IsLost() == DDERR_SURFACELOST && !this->ReloadTexture(i))
			return 0;
	}
	return 1;
}

void CMarni::ClearBuffers()
{
	this->Is_gpu_init = 0;
	this->Is_active = 0;
	//if (this->card == GFX_SOFTWARE)
	{
		CMarni216** p= this->pSurfaces;
		for (int i = 0; i < 513; i++)
		{
			if (p[i])
			{
				if (p[i]->pSurfEx) delete p[i]->pSurfEx;
				p[i]->pSurfEx = NULL;
			}
		}
	}
	//else
	//{
	//	v6 = (float *)p->pSurfaces;
	//	v7 = 512;
	//	do
	//	{
	//		if (*(_DWORD *)v6)
	//		{
	//			v8 = *(void **)(*(_DWORD *)v6 + 4);
	//			if (v8)
	//			{
	//				MarniSurfaceEx::deconstructor();
	//				operator delete(v8);
	//			}
	//			*(_DWORD *)(*(_DWORD *)v6 + 4) = 0;
	//		}
	//		++v6;
	//		--v7;
	//	} while (v7);
	//}

	if (this->pD3DDevice)
	{
		this->pD3DDevice->DeleteMatrix(this->MatrixHandle1);
		this->pD3DDevice->DeleteMatrix(this->MatrixHandle0);
		this->pD3DDevice->DeleteMatrix(this->MatrixHandle2);
		this->pD3DDevice->DeleteMatrix(this->MatrixHandle3);
		this->pD3DDevice->DeleteMatrix(this->MatrixHandle4);
		this->pD3DDevice->DeleteMatrix(this->MatrixHandle5);
		this->MatrixHandle1 = 0;
		this->MatrixHandle0 = 0;
		this->MatrixHandle2 = 0;
		this->MatrixHandle4 = 0;
		this->MatrixHandle5 = 0;
		this->MatrixHandle3 = 0;
	}
	if (this->pViewport)
	{
		for(int i = 0; i<this->Light_max; i++)
		{
			this->pViewport->DeleteLight(this->pD3DLight[i]);
			if (this->pD3DLight[i])
			{
				this->pD3DLight[i]->Release();
				this->pD3DLight[i] = 0;
			}
		}
	}

	if (this->Material)
	{
		this->Material->Release();
		this->Material = 0;
	}

	if (this->pExecuteBuffer)
	{
		this->pExecuteBuffer->Release();
		this->pExecuteBuffer = 0;
	}

	if (this->pViewport)
	{
		this->pViewport->Release();
		this->pViewport = 0;
	}

	if (this->pD3DDevice)
	{
		this->pD3DDevice->Release();
		this->pD3DDevice = 0;
	}

	if (this->Clipper)
	{
		this->Clipper->Release();
		this->Clipper = 0;
	}
	this->MarniBitsMain.Release();
	this->MarniBits1.Release();
	this->MarniBitsDst.Release();
}

static int double_call = 0;

CMarni* CMarni::Init(HWND hWnd, int screen_w, int screen_h, int display_mode, int display_driver)
{
	int enumerate;

	try
	{
		this->Initialize(hWnd, screen_w, screen_h);
		this->display_mode = display_mode;

		// drop nocard2 type here
		if (display_driver == GFX_NOCARD2)
		{
			this->card = GFX_SOFTWARE;
			this->Use_VRAM = 0;
		}
		else
		{
			this->card = display_driver;
			this->Use_VRAM = 1;
		}
		// the original code here would go fullscreen for other cards, fuck that

		if (double_call)
		{
			Marni2Out("Can't call this twice", "MarniSystem Direct3D Class");
			exit(2);
		}
		double_call++;

		this->cnt_pPtr_2K = 512;
		this->pPtr_2KBuffer = new int*[512];

		LoadIdentity(&this->Matrix0);
		LoadIdentity(&this->Matrix1);
		LoadIdentity(&this->Matrix2);
		LoadIdentity(&this->Matrix3);
		LoadIdentity(&this->Matrix4);
		this->Matrix4.m[10] = this->Matrix4.m[11] = (float)this->Render_w / (float)this->Scaling;
		LoadIdentity(&this->Matrix5);

		switch (this->card)
		{
		case GFX_MONSTER:
			enumerate = 1;
			this->adjust_v = -0.0000099999997f;
			this->adjust_u = 0.f;
			this->field_1114 = 1;
			break;
		case GFX_MYSTIQUE:
			enumerate = 0;
			this->adjust_v = -0.0000099999997f;
			this->adjust_u = 0.f;
			this->field_1114 = 1;
			break;
		case GFX_TOTAL:
		case GFX_BLASTER:
			enumerate = 0;
			this->adjust_v = 0.f;
			this->adjust_u = -0.40000001f;
			this->field_1114 = 1;
			break;
		case GFX_RIGHTEOUS:
			enumerate = 0;
			this->adjust_v = 0.f;
			this->adjust_u = -0.40000001f;
			this->field_1114 = 0;
			break;
		case GFX_SOFTWARE:
			enumerate = 0;
			this->adjust_v = 0.f;
			this->adjust_u = 0.f;
			this->field_1114 = 0;
			this->pTexture_buffer = new WORD[64 * 1024];
			for (int y = 0; y < 256; y++)
			{
				for (int x = 0; x < 128; x++)
				{

				}
			}
			//GETDWORD(0x4BDD40) = (DWORD)this->pTexture_buffer;
			break;
		case GFX_INTENSE:
			enumerate = 0;
			this->adjust_v = 0.f;
			this->adjust_u = 0.f;
			this->field_1114 = 0;
			break;
		case GFX_UNKNOWN:
			enumerate = 0;
			this->adjust_v = 0.f;
			this->adjust_u = 0.f;
			this->field_1114 = 1;
			break;
		}

		//GETDWORD(0x7E0E18) = 0;
		this->field_110C = 1;

		for (int i = 0; i < 6; i++)
		{
			this->Soft_Light[i * 2].z = 1.f;
			this->Soft_Light[i * 2].r = 1.f;
			this->Soft_Light[i * 2].g = 1.f;
			this->Soft_Light[i * 2].b = 1.f;
		}

		if (this->BitDepth < 16)
		{
			Marni1Out("the depth of screen is wrong. you should make change to more than", "MarniSystem Direct3D::Direct3D");
			throw 0;
		}

		static DWORD someshit;
		CreateDevice(enumerate, &this->pDirectDraw, &someshit/*(DWORD*)&pExe[0x7e0e18 - 0x400000]*/);
		//QueryInterface(this->pDirectDraw, &this->g_pDD);
		DDCAPS caps;
		caps.dwSize = sizeof(caps);
		this->pDirectDraw->GetCaps(&caps, NULL);
		//DebugEnumerateModes(&caps);
		this->VRAM_available = caps.dwVidMemTotal;
		Marni1Out("available VRAM %d MB", this->VRAM_available / 1024 / 1024);

		// now create software resolutions
		// in software mode create two non-fullscreen resolutions of 320x240 and doubled
		if (this->card == GFX_SOFTWARE)
		{
			MARNI_RES *pRes = &this->Resolutions[0];

			pRes[0].W = 1920;//pMarni->Render_w;
			pRes[0].H = 1080;//pMarni->Render_h;
			pRes[0].Depth = 32;
			pRes[0].Fullscreen = 0;

			pRes[1].W = 640;
			pRes[1].H = 480;
			pRes[1].Depth = 32;
			pRes[1].Fullscreen = 0;

			pRes[2].W = 800;
			pRes[2].H = 600;
			pRes[2].Depth = 32;
			pRes[2].Fullscreen = 0;

			pRes[3].W = 1024;;
			pRes[3].H = 768;;
			pRes[3].Depth = 32;
			pRes[3].Fullscreen = 0;

			this->Resolution_count = 4;
			this->Softfull_rescnt = this->Resolution_count;
		}
		else
		{
			MARNI_RES *pRes = &this->Resolutions[0];

			pRes[0].W = 640;
			pRes[0].H = 480;
			pRes[0].Depth = 32;
			pRes[0].Fullscreen = 0;

			pRes[1].W = 800;
			pRes[1].H = 600;
			pRes[1].Depth = 32;
			pRes[1].Fullscreen = 0;

			this->Resolution_count = 2;
			this->Softfull_rescnt = 2;
		}

		if (this->display_mode >= this->Resolution_count)
		{
			Marni1Out("You specified an invalid resolution, resorting to first one");
			this->display_mode = 0;
		}

		if (!this->MD3DCreateDirect3D() || !this->DetectGpuDriver())
		{
			throw 0;
		}

		// here there was code debugouting driver names, we don't need that shit either
		if (!this->InitAll())
			throw 0;

		this->Bpp_compatible = Marni_device[this->DeviceID].Bpp_compatible;
		// here there was more debug shit that printed available resolutions

		// more initialization crap for software
		if (this->card == GFX_SOFTWARE)
		{
			this->MarniSrf2.ClearBg(0, 1, 0);
			this->MarniSrf2.Lock(0, 0);
			// set palette
			//for (int i = 0; i < 256; i++)
			//	this->MarniSrf2.SetPaletteColor(i, 0xffffff, 0);
			this->MarniSrf2.Unlock();
		}
		// and here goes for hardware
		else
		{
			this->D3DHELDevDesc.dwSize = sizeof(this->D3DHELDevDesc);
			this->D3DHWDevDesc.dwSize = sizeof(this->D3DHWDevDesc);
			this->pD3DDevice->GetCaps((D3DDEVICEDESC*)&this->D3DHWDevDesc, (D3DDEVICEDESC*)&this->D3DHELDevDesc);
			this->Is_active = 1;
			this->Is_gpu_init = 1;
			this->RequestVideoMemory();	// doesn't really do much
			this->MarniSrf2.CreateWork(16, 16, 32, 0);
			this->MarniSrf2.sdesc.field_16 = 0;
			this->MarniSrf2.sdesc.dwRGBAlphaBitMask_setcnt = 0;
			this->MarniSrf2.sdesc.field_14 = 0;
			this->MarniSrf2.ClearBg(0, 0x808080, 0);

			// creates the first texture surface
			CMarni216 *p216 = new CMarni216;
			this->pSurfaces[0] = p216;
			p216->pSurfEx = NULL;
			p216->field_D0 = 1;
			if (!p216->Surface.copy(this->MarniSrf2))
				throw 0;
			if (!this->ReloadTexture(0))
				throw 0;
		}
	}
	catch (int)
	{
		return NULL;
	}

	return this;
}

void CMarni::Initialize(HWND hWnd, int screen_w, int screen_h)
{
	this->hWnd = hWnd;
	this->Render_w = this->XSize = screen_w;
	this->Render_h = this->YSize = screen_h;
	this->Scaling = 260;
	this->Aspect_x = (float)((double)this->XSize / (double)screen_w);
	this->Aspect_y = (float)((double)this->YSize / (double)screen_h);
	this->Texture_w = screen_w / 2;
	this->Texture_h = screen_h / 2;
	this->Light_max = 1;
	this->Light_ambient = 0x20202020;
	this->Desktop_w = GetSystemMetrics(SM_CXSCREEN);
	this->Desktop_h = GetSystemMetrics(SM_CYSCREEN);
	// get desktop bpp
	HDC hdc = GetDC(NULL);
	int bpp = GetDeviceCaps(hdc, PLANES) * GetDeviceCaps(hdc, BITSPIXEL);
	this->BitDepth = this->BitsPerPixel = bpp;
	ReleaseDC(NULL, hdc);
}

int CMarni::InitAll()
{
	try
	{
		MARNI_RES *pRes = &this->Resolutions[this->display_mode];
		this->XSize = pRes->W;
		this->YSize = pRes->H;
		this->Is_fullscreen = pRes->Fullscreen;
		this->BitDepth = pRes->Depth;

		this->Is_busy_ = 1;
		error = SetCooperativeLevel(this->hWnd, this->Is_fullscreen, this->pDirectDraw);
		this->Is_busy_ = 0;

		if (error)
		{
			this->Is_gpu_init = 0;
			throw 0;
		}

		this->GetZDepthCaps();
		InvalidateWindow(this->hWnd, this->XSize, this->YSize, this->Is_fullscreen, &this->Resolution_rect);
		this->Resolution_rect.right--;
		this->Resolution_rect.bottom--;

		// switch resolution
		if (this->Is_fullscreen)
		{
			this->Is_busy_ = 1;
			error = this->pDirectDraw->SetDisplayMode(this->XSize, this->YSize, this->BitDepth);
			this->Is_busy_ = 0;
			if (error)
			{
				this->Is_fullscreen = 0;
				throw 0;
			}
			if (this->card != GFX_SOFTWARE || this->Softfull_rescnt + 2 > this->display_mode)
			{
				MARNI_RES *pRes = &this->Resolutions[this->display_mode];
				this->XSize = pRes->W;
				this->YSize = pRes->H;
				SetRect(&this->Resolution_rect, 0, 0, pRes->W, pRes->H);
				if (this->card != GFX_SOFTWARE)
				{
					this->Aspect_x = (float)((double)this->XSize / (double)this->Render_w);
					this->Aspect_y = (float)((double)this->YSize / (double)this->Render_h);
				}
			}
			else
			{
				MARNI_RES *pRes = &this->Resolutions[this->display_mode];
				this->XSize = pRes->W;
				this->YSize = pRes->H;
				SetRect(&this->Resolution_rect, 0, 0, pRes->W, pRes->H);
			}
		}

		error = CreatePrimarySurface(this->hWnd, this->XSize, this->YSize, this->Is_fullscreen, this->pDirectDraw,
			&this->MarniBitsDst.DDsurface, &this->MarniBitsMain.DDsurface, (LPDIRECTDRAWCLIPPER *)&this->Clipper, this->Use_VRAM);
		if (error)
		{
			this->Is_gpu_init = 0;
			throw 0;
		}

		DDSURFACEDESC dsdesc;
		error = GetSurfaceDesc(&dsdesc, this->MarniBitsMain.DDsurface);
		if (error) throw 0;

		this->MarniBitsMain.pPalette = 0;
		CalcPixelFormat(&dsdesc, &this->MarniBitsMain.sdesc);
		// try to create a compatible main surface
		this->MarniBitsMain.sdesc.field_16 = 0;	// reset alpha
		this->MarniBitsMain.sdesc.dwRGBBitCount = (char)dsdesc.ddpfPixelFormat.dwRGBBitCount;
		this->MarniBitsMain.sdesc.bpp = 0;
		this->MarniBitsMain.dwWidth = dsdesc.dwWidth;	// need to be changed later
		this->MarniBitsMain.dwHeight = dsdesc.dwHeight;	// same
		this->MarniBitsMain.lPitch = dsdesc.lPitch;
		this->MarniBitsMain.field_44 = 1;
		this->MarniBitsMain.Has_palette = 0;
		this->MarniBitsMain.field_4C = 0;
		this->MarniBitsMain.Is_open = 1;
		this->MarniBitsMain.ClearBg(0, 0, 0);

		error = GetSurfaceDesc(&dsdesc, this->MarniBitsDst.DDsurface);
		if (error) throw 0;

		this->MarniBitsDst.pPalette = 0;
		CalcPixelFormat(&dsdesc, &this->MarniBitsDst.sdesc);
		// do more crap to description
		this->MarniBitsDst.sdesc.field_16 = 0;
		this->MarniBitsDst.sdesc.dwRGBBitCount = (char)dsdesc.ddpfPixelFormat.dwRGBBitCount;
		this->MarniBitsDst.sdesc.bpp = 0;
		this->MarniBitsDst.dwWidth = dsdesc.dwWidth;
		this->MarniBitsDst.dwHeight = dsdesc.dwHeight;
		this->MarniBitsDst.lPitch = dsdesc.lPitch;
		this->MarniBitsDst.field_44 = 1;
		this->MarniBitsDst.Has_palette = 0;
		this->MarniBitsDst.field_4C = 0;
		this->MarniBitsDst.Is_open = 1;

		// more software mode shanenigas
		if (this->card == GFX_SOFTWARE)
		{
			// fullscreen requires a background clear
			if (this->Is_fullscreen)
				this->MarniBitsDst.ClearBg(NULL, 0, 0);
			this->Is_gpu_init = 1;
		}
		else
		{
			// not sure why this
			if (this->MarniBits1.DDsurface)
			{
				this->MarniBits1.DDsurface->Release();
				this->MarniBits1.DDsurface = NULL;
			}
			// tries to create a z-buffer
			LPDIRECTDRAWSURFACE srf = NULL;
			if (!this->MD3DCreateZBuffer(this->XSize, this->YSize, &srf))
				throw 0;

			this->MarniBits1.DDsurface = srf;
			this->MarniBits1.pPalette = 0;
			this->MarniBits1.Has_palette = 0;
			this->MarniBits1.field_4C = 0;
			this->MarniBits1.field_44 = 1;
			this->MarniBits1.Is_open = 1;
			this->MarniBits1.dwHeight = this->YSize;
			this->MarniBits1.dwWidth = this->XSize;
#if 0
			// old 16 bit surface
			this->MarniBits1.lPitch = 2 * this->XSize;
			this->MarniBits1.sdesc.dwRBitMask_setcnt = 0;
			this->MarniBits1.sdesc.field_2 = 31;
			this->MarniBits1.sdesc.field_8 = 31;
			this->MarniBits1.sdesc.field_E = 31;
			this->MarniBits1.sdesc.field_4 = 5;
			this->MarniBits1.sdesc.dwBBitMask_setcnt = 10;
			this->MarniBits1.sdesc.dwRGBBitCount = 16;
			this->MarniBits1.sdesc.dwRBitMask_cnt = 5;
			this->MarniBits1.sdesc.field_A = 5;
			this->MarniBits1.sdesc.field_10 = 5;
			this->MarniBits1.sdesc.bpp = 0;
#else
			// new 32 bit surface
			this->MarniBits1.lPitch = 4 * this->XSize;
			this->MarniBits1.sdesc.dwRBitMask_setcnt = 16;
			this->MarniBits1.sdesc.field_2 = 0xff;
			this->MarniBits1.sdesc.field_4 = 8;
			this->MarniBits1.sdesc.dwRBitMask_cnt = 8;
			this->MarniBits1.sdesc.field_8 = 0xff;
			this->MarniBits1.sdesc.field_A = 8;
			this->MarniBits1.sdesc.dwBBitMask_setcnt = 0;
			this->MarniBits1.sdesc.field_E = 0xff;
			this->MarniBits1.sdesc.field_10 = 8;
			/* --- */
			this->MarniBits1.sdesc.dwRGBBitCount = 0;
			this->MarniBits1.sdesc.bpp = 0;
#endif

			if (!this->createDevice())
				throw 0;
			// viewport creation
			error = ((IDirect3D*)this->pZBuffer_)->CreateViewport(&this->pViewport, NULL);
			if (error) throw 0;
			error = this->pD3DDevice->AddViewport(this->pViewport);
			if (error) throw 0;
			D3DVIEWPORT vpdata;
			memset(&vpdata, 0, sizeof(vpdata));
			vpdata.dwSize = sizeof(vpdata);
			vpdata.dwWidth = this->XSize;
			vpdata.dwHeight = this->YSize;
			vpdata.dvScaleX = 1.f;
			vpdata.dvScaleY = 1.f;
			vpdata.dwX = 0;
			vpdata.dwY = 0;
			vpdata.dvMaxX = (float)this->XSize * 0.5f;
			vpdata.dvMaxY = (float)this->YSize * 0.5f;
			this->pViewport->SetViewport(&vpdata);

			// matrix creation
			MARNI_MATRIX m;
			LoadIdentity(&m);
			if (this->pD3DDevice->CreateMatrix(&this->MatrixHandle1)) throw 0;
			if (this->pD3DDevice->SetMatrix(this->MatrixHandle1, (D3DMATRIX*)&m)) throw 0;
			if (this->pD3DDevice->CreateMatrix(&this->MatrixHandle0)) throw 0;
			if (this->pD3DDevice->SetMatrix(this->MatrixHandle0, (D3DMATRIX*)&this->Matrix0)) throw 0;
			if (this->pD3DDevice->CreateMatrix(&this->MatrixHandle2)) throw 0;
			if (this->pD3DDevice->SetMatrix(this->MatrixHandle2, (D3DMATRIX*)&this->Matrix2)) throw 0;
			if (this->pD3DDevice->CreateMatrix(&this->MatrixHandle3)) throw 0;
			if (this->pD3DDevice->SetMatrix(this->MatrixHandle3, (D3DMATRIX*)&this->Matrix3)) throw 0;
			if (this->pD3DDevice->CreateMatrix(&this->MatrixHandle4)) throw 0;
			if (this->pD3DDevice->SetMatrix(this->MatrixHandle4, (D3DMATRIX*)&this->Matrix5)) throw 0;
			if (this->pD3DDevice->CreateMatrix(&this->MatrixHandle5)) throw 0;
			if (this->pD3DDevice->SetMatrix(this->MatrixHandle5, (D3DMATRIX*)&this->Matrix4)) throw 0;

			// light creation
			D3DLIGHT light;
			memset(&light, 0, sizeof(light));
			light.dwSize = sizeof(light);
			light.dcvColor.r = 1.f;
			light.dcvColor.g = 1.f;
			light.dcvColor.b = 1.f;
			light.dcvColor.a = 1.f;
			light.dvDirection.x = 0.f;
			light.dvDirection.y = 0.f;
			light.dvDirection.z = 1.f;
			light.dvAttenuation0 = 1.f;
			light.dvAttenuation1 = 0.f;
			light.dvAttenuation2 = 0.f;
			light.dltType = D3DLIGHT_DIRECTIONAL;
			for (int i = 0; i < this->Light_max; i++)
			{
				if (((LPDIRECT3D)this->pZBuffer_)->CreateLight(&this->pD3DLight[i], NULL)) throw 0;
				if (this->pD3DLight[i]->SetLight(&light)) throw 0;
				if (this->pViewport->AddLight(this->pD3DLight[i])) throw 0;
			}

			D3DEXECUTEBUFFERDESC exedesc;
			exedesc.dwSize = sizeof(exedesc);
			//exedesc.dwFlags = 0;
			exedesc.dwCaps = 0;
			exedesc.lpData = NULL;
			exedesc.dwFlags = D3DDEB_BUFSIZE;
			exedesc.dwBufferSize = 0x90000;
			if (this->pD3DDevice->CreateExecuteBuffer(&exedesc, &this->pExecuteBuffer, NULL)) throw 0;
			if (((LPDIRECT3D)this->pZBuffer_)->CreateMaterial(&this->Material, NULL)) throw 0;
			if (this->Material->GetHandle(this->pD3DDevice, &this->MaterialHandle)) throw 0;
			if (this->pExecuteBuffer->Lock(&exedesc)) throw 0;
			// needs buffer population here
			void *ptr = exedesc.lpData;
			PUTD3DINSTRUCTION(D3DOP_STATETRANSFORM, sizeof(D3DSTATE), 3, ptr);
			STATE_DATA(D3DTRANSFORMSTATE_PROJECTION, this->MatrixHandle0, ptr);
			STATE_DATA(D3DTRANSFORMSTATE_VIEW, this->MatrixHandle1, ptr);
			STATE_DATA(D3DTRANSFORMSTATE_WORLD, this->MatrixHandle2, ptr);
			PUTD3DINSTRUCTION(D3DOP_EXIT, 0, 0, ptr);
			//
			if (this->pExecuteBuffer->Unlock()) throw 0;
			D3DEXECUTEDATA exedata;
			exedata.dwSize = sizeof(exedata);
			exedata.dwInstructionOffset = 0;
			exedata.dwInstructionLength = (char*)ptr - (char*)exedesc.lpData;
			this->pExecuteBuffer->SetExecuteData(&exedata);

			if (this->pD3DDevice->BeginScene()) throw 0;
			DWORD dwFlags = this->card == GFX_INTENSE ? 2 : 1;
			if (this->pD3DDevice->Execute(this->pExecuteBuffer, this->pViewport, dwFlags)) throw 0;
			if (this->pD3DDevice->EndScene()) throw 0;

			this->Is_gpu_init = 1;
		}
	}
	catch (int)
	{
		return 0;
	}

	return 1;
}

void CMarni::GetZDepthCaps()
{
	DDCAPS halcaps;
	DDCAPS helcaps;

	memset(&helcaps, 0, sizeof(helcaps));
	helcaps.dwSize = 316;
	memset(&halcaps, 0, sizeof(halcaps));
	halcaps.dwSize = 316;
	if (this->pDirectDraw->GetCaps(&halcaps, &helcaps))// glDirectDraw1::GetCaps
	{
		//MarniOut2(aGetcapsFailedI, aMarnisystemD_6);
		return;
	}
	if (!halcaps.dwZBufferBitDepths)
		goto LABEL_16;
	if (halcaps.dwZBufferBitDepths & 0x100)
	{
		this->Caps_depth = 32;
		goto LABEL_13;
	}
	if (halcaps.dwZBufferBitDepths & 0x200)
	{
		this->Caps_depth = 24;
		goto LABEL_13;
	}
	if (halcaps.dwZBufferBitDepths & 0x400)
	{
	LABEL_16:
		this->Caps_depth = 16;
		goto LABEL_13;
	}
	if (!(halcaps.dwZBufferBitDepths & 0x800))
	{
		//MarniOut2(aNoValidZBuffer, aMarnisystemD_6);
		return;
	}
	this->Caps_depth = 8;
LABEL_13:
	this->ddscapsSize = halcaps.ddsOldCaps.dwCaps;
}

int  CMarni::Test_textures()
{
	if (this->ReloadTexture0(0x21))
		return 1;
	if (this->ReloadTexture0(0x22))
		return 1;
	if (this->ReloadTexture0(0x01))
		return 1;
	if (this->ReloadTexture0(0x02))
		return 1;

	return 0;
}

void CMarni::Vmem_stats()
{
	if (this->card == 5)
	{
		this->field_48 = 0.0;
		return;
	}

	int total_cnt = 0;
	int vram_cnt = 0;
	CMarni216 **pp = this->pSurfaces;
	int i = 512;
	do
	{
		if (*pp)
		{
			++total_cnt;
			vram_cnt += (*pp)->pSurfEx->Is_vmem ? 1 : 0;
		}
		++pp;
		--i;
	} while (i);
	this->field_48 = (float)vram_cnt / (float)total_cnt;
}

int CMarni::ReloadTexture()
{
	try
	{
		if (!this->Is_gpu_init)
			throw 0;

		int i = 0;
		while (1)
		{
			CMarni216 *pSurf = this->pSurfaces[i];

			if (pSurf)
			{
				pSurf->field_D4 = 0;
				if (!this->ReloadTexture(i))
					break;
			}
			if (++i >= 512)
			{
				this->RequestVideoMemory();
				this->Vmem_stats();
				return 1;
			}
		}
		throw 0;
	}
	catch (int)
	{
		return 0;
	}

	// failed to reload
	return 1;
}

int CMarni::ReloadTexture(int index)
{
	try
	{
		if (!this->Is_gpu_init) throw 0;

		CMarniSurfaceEx surf_ex;
		//pDirectDrawSurface.constructor();

		CMarni216 *pSurf = this->pSurfaces[index];
		if (!pSurf) throw 0;

		// not really useful for now
		if (this->card == GFX_SOFTWARE)
		{
			CMarniSurface *pS;
			if (pSurf->pSurfEx)
			{
				pS = pSurf->pSurfEx;
				delete pS;
			}
			pS = new CMarniSurface;
			pSurf->pSurfEx = (CMarniSurfaceEx*)pS;	// not sure if casting like this will work
			if (pS->CreateOffscreenSurface(this->pDirectDraw, pSurf->Surface.dwWidth, pSurf->Surface.dwHeight))
			{
				delete pS;
				pSurf->pSurfEx = NULL;
				throw 0;
			}
			RECT rc;
			SetRect(&rc, 0, 0, pSurf->Surface.dwWidth, pSurf->Surface.dwHeight);
			pS->Blt(&rc, &rc, &pSurf->Surface, 0, 0);
		}
		else
		{
			if (!pSurf->Surface.Is_open)
				throw 0;
			if (pSurf->pSurfEx)
			{
				delete pSurf->pSurfEx;
				throw 0;
			}

			DDSURFACEDESC ddesc;
			memset(&ddesc, 0, sizeof(ddesc));
			if (!this->SearchMatchTextureFormat(&pSurf->Surface, &ddesc))
				throw 0;
			ddesc.dwFlags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
			ddesc.dwWidth = pSurf->Surface.dwWidth;
			ddesc.dwHeight = pSurf->Surface.dwHeight;
			ddesc.ddsCaps.dwCaps = DDSCAPS_ALLOCONLOAD | DDSCAPS_TEXTURE;
			if (!Marni_device[this->DeviceID].Is_hal)
				ddesc.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
			CMarniSurfaceEx *pSex = new CMarniSurfaceEx;
			//pSurf->pSurfEx = pSex->constructor();
			pSurf->pSurfEx = pSex;
			pSex->CreateCompatibleSurface(this->pDirectDraw, &ddesc);
			if (!pSex->Is_open)
				throw 0;
			// failed to create surface in vram, let's try and find a better method
			if (Marni_device[this->DeviceID].Is_hal && !pSex->Is_vmem)
			{
				// this stuff is actually buggy and can't find shit
				while (1)
				{
					pSex->Release();
					if (!this->Test_textures())
					{
						Marni1Out("really can't create this shitty texture");
						throw 0;
					}
					// found a suitable hardware texture
					if (pSex->Is_open && pSex->Is_vmem)
						break;
				}
			}
			memcpy(&pSurf->desc, &ddesc, sizeof(ddesc));
			pSex->CreateTextureObject();
			ddesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
			surf_ex.CreateCompatibleSurface(this->pDirectDraw, &ddesc);
			// perform blit tests
			RECT r0, r1;
			SetRect(&r0, 0, 0, pSurf->Surface.dwWidth - 1, pSurf->Surface.dwHeight - 1);
			SetRect(&r1, 0, 0, pSurf->Surface.dwWidth - 1, pSurf->Surface.dwHeight - 1);
			if (pSurf->field_D0 & 0x1 || pSurf->field_D0 & 0x20)
				surf_ex.Blt(NULL, NULL, &pSurf->Surface, 0, 0);
			else surf_ex.Blt(&r0, &r1, &pSurf->Surface, 0, 0);
			if (surf_ex.Has_palette)
				surf_ex.PalBlt(&pSurf->Surface, 0, 0, 0);
			// perform palette tests
			pSurf->field_C8 = surf_ex.dwWidth;
			pSurf->field_CC = surf_ex.dwHeight;
			if (pSurf->field_D0 & 8 && !pSex->Has_palette)
			{
				surf_ex.Lock(NULL, NULL);
				for (int y = 0; y < surf_ex.dwHeight; y++)
				{
					for (int x = 0; x < surf_ex.dwWidth; x++)
					{
						u32 p;
						surf_ex.GetCurrentColor(x, y, &p);
						if (!p) surf_ex.SetCurrentColor(x, y, 0x080808, 0);
					}
				}
				surf_ex.Unlock();
			}
			if (surf_ex.sdesc.field_16 && !surf_ex.Has_palette)
			{
				surf_ex.Lock(NULL, NULL);
				for (int y = 0; y < surf_ex.dwHeight; y++)
				{
					for (int x = 0; x < surf_ex.dwWidth; x++)
					{
						u32 p;
						surf_ex.GetCurrentColor(x, y, &p);
						//p = (0xFF000000 - (p & 0xFF000000)) ^ (p ^ -(p & 0xFF000000)) & 0xFFFFFF;
						p = p & 0xffffff;
						surf_ex.SetCurrentColor(x, y, p, 0);
					}
				}
				surf_ex.Unlock();
			}
			/* other tests */
			surf_ex.CreateTextureObject();
			if (pSex->DDtexture->Load(surf_ex.DDtexture))
				throw 0;
			pSex->GetTextureHandle(this->pD3DDevice);
		}
	}
	catch (int)
	{
		return 0;
	}

	return 1;
}

int CMarni::ReloadTexture0(DWORD dwFlags)
{
	try
	{
		bool found = false;
		CMarniSurfaceEx surf;

		// search for a surface description that matches
		CMarni216 *p;
		for (int i = 0; i < 512; i++)
		{
			p = this->pSurfaces[i];
			if (!p) continue;
			if (p->pSurfEx->Is_open && (p->field_D0 & dwFlags) == dwFlags && !p->field_D4)
			{
				break;
				found = true;
			}
		}
		if (!found) throw 0;
		// copy what was found
		DDSURFACEDESC ddesc;
		memcpy(&ddesc, &p->desc, sizeof(ddesc));
		ddesc.dwWidth /= 2;
		ddesc.dwHeight /= 2;
	}
	catch (int)
	{
		return 0;
	}

	return 1;
}

HRESULT __stdcall cbEnumDevices(GUID *guid, char *description, char *name, D3DDEVICEDESC *hal_desc, D3DDEVICEDESC *hel_desc, void *ctx)
{
	MARNI_DEVICE *pDev = &Marni_device[DeviceId];

	pDev->guid = *guid;
	//strcpy_s(pDev->description, sizeof(pDev->description), description);
	memcpy(pDev->description, description, sizeof(pDev->description));
	pDev->description[49] = '\0';
	strcpy_s(pDev->name, sizeof(pDev->name), name);

	// use actual hardware mode
	if (hal_desc->dcmColorModel)
	{
		pDev->Is_hal = 1;
		memcpy(&pDev->device_desc, hal_desc, sizeof(pDev->device_desc));
	}
	// emulate stuff via software
	else
	{
		pDev->Is_hal = 0;
		memcpy(&pDev->device_desc, hel_desc, sizeof(pDev->device_desc));
	}

	return ++DeviceId - 5 >= 1;
}

int CMarni::DetectGpuDriver()
{
	if (this->card == GFX_SOFTWARE)
		return 1;

	IDirect3D *pZ = (IDirect3D *)this->pZBuffer_;
	DeviceId = 0;
	error = pZ->EnumDevices(cbEnumDevices, NULL);

	if (error)
	{
		Marni1Out("Failed to detect devices");
		return 0;
	}

	MARNI_DEVICE *pD = &Marni_device[0];
	for (int i = 0; i < (int)DeviceId; i++, pD++)
	{
		pD->Is_mono = pD->device_desc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_PERSPECTIVE;
		pD->has_zbuffer = pD->device_desc.dwDeviceZBufferBitDepth >= 1;
		pD->Bpp_compatible = (pD->device_desc.dwDeviceRenderBitDepth & D3DIBPPToDDBD(BitDepth)) >= 1;
		//if (GETDWORD(0x7E0E18) == 0) pD->Bpp_compatible = 0;
	}

	this->DeviceID = 2;

	return 1;
}
int CMarni::MD3DCreateDirect3D()
{
	if (this->card == GFX_SOFTWARE)
		return 1;

	error = this->pDirectDraw->QueryInterface(IID_IDirect3D, (void**)&this->pZBuffer_);
	if (error)
	{
		Marni1Out("can't do shit here", "MarniSystem Direct3D::MD3DCreateDirect3D");
		return 0;
	}

	return 1;
}

int CMarni::MD3DCreateZBuffer(int Width, int Height, LPDIRECTDRAWSURFACE *ppSurface)
{
	try
	{
		if (this->card == GFX_SOFTWARE)
			return 1;

		if (!Marni_device[this->DeviceID].has_zbuffer)
		{
			*ppSurface = NULL;
			throw 0;
		}

		DDSURFACEDESC dsdesc;
		memset(&dsdesc, 0, sizeof(dsdesc));
		dsdesc.dwHeight = Height;
		dsdesc.dwWidth = Width;
		dsdesc.dwSize = sizeof(dsdesc);
		dsdesc.dwFlags = 71;
		dsdesc.ddsCaps.dwCaps = 0x24000;
		if (!Marni_device[this->DeviceID].Is_hal)
			dsdesc.ddsCaps.dwCaps = 0x20800;

		u32 bpp = Marni_device[this->DeviceID].device_desc.dwDeviceZBufferBitDepth;
		int zdepth;

		if (bpp & 0x100) zdepth = 32;
		else if (bpp & 0x200) zdepth = 24;
		else if (bpp & 0x400) zdepth = 16;
		else if (bpp & 0x800) zdepth = 8;
		else
		{
			ppSurface = NULL;
			throw 0;
		}

		assert(*ppSurface == NULL);
		if (this->pDirectDraw->CreateSurface(&dsdesc, ppSurface, NULL))
			throw 0;

		if (this->MarniBitsMain.DDsurface->AddAttachedSurface(*ppSurface))
			throw 0;

		if (GetSurfaceDesc(&dsdesc, *ppSurface))
			throw 0;

		this->field_4A8 = (dsdesc.ddsCaps.dwCaps & 0x4000) >> 14;
	}
	catch (int)
	{
		if (!*ppSurface)
		{
			(*ppSurface)->Release();
			*ppSurface = NULL;
		}
		return 0;
	}

	return 1;
}

int CMarni::RequestDisplayRect(int id, MARNI_RES *res)
{
	if (!this->Is_gpu_init)
		return 0;
	if (id >= this->Resolution_count)
		return 0;

	memcpy32(res, &this->Resolutions[id], sizeof(MARNI_RES) / 4);
	return 1;
}

int CMarni::RequestDisplayModeCount()
{
	if (this->Is_gpu_init)
		return this->Resolution_count;

	return 0;
}

int CMarni::createDevice()
{
	void **v1 = (void**)&this->pD3DDevice;

	try
	{
		if (this->pD3DDevice)
			throw 0;

		if (this->card == GFX_SOFTWARE)
			return 1;

		MARNI_DEVICE *pDev = &Marni_device[this->DeviceID];
		error = this->MarniBitsMain.DDsurface->QueryInterface((const IID)pDev->guid, v1);
		if (error) throw 1;

		this->Surf_desc_cnt = 0;
		if (pDev->Is_mono)
		{
			DDSURFACEDESC ddesc[20];
			int desc_cnt = D3DEnumTextureFormats(this->pD3DDevice, 20, ddesc);
			if (!desc_cnt) throw 0;
			DDSURFACEDESC *pDesc = this->Surf_desc;
			for (int i = 0; i < desc_cnt; i++)
			{
				//if (ddesc[i].ddpfPixelFormat.dwRGBBitCount <= 16)
				{
					MARNI_SURFACE_DESC sdesc;
					CalcPixelFormat(&ddesc[i], &sdesc);
					// check alpha bitsize?
					if (sdesc.field_16 >= 1)
					{
						memcpy(pDesc, &ddesc[i], sizeof(DDSURFACEDESC));
						this->Surf_desc_cnt++;
						pDesc++;
					}
				}
			}
		}

		//DDSURFACEDESC *pDesc = (DDSURFACEDESC*)&this->Surf_desc;
		//for (int i = 0; i < this->Surf_desc_cnt; i++)
		//{
		//	if (pDesc->ddpfPixelFormat.dwFlags & 0x28)
		//		Marni1Out("Palette index %d", pDesc->ddpfPixelFormat.dwRGBBitCount);
		//	else
		//	{
		//		MARNI_SURFACE_DESC sdesc;
		//		CalcPixelFormat(pDesc, &sdesc);
		//		Marni1Out("ARGB %d %d %d %d",
		//			sdesc.field_16,
		//			sdesc.field_4,
		//			sdesc.field_A,
		//			sdesc.field_10);
		//	}
		//	pDesc++;
		//}
	}
	catch (int e)
	{
		switch (e)
		{
		case 0:
			this->pD3DDevice->Release();
			this->pD3DDevice = NULL;
			return 0;
		case 1:
			return 0;
		}
	}

	return 1;
}

//int RequestVideoMemory();
int CMarni::SearchMatchTextureFormat(CMarniSurface2 *pSurface, DDSURFACEDESC *pDesc)
{
	int bot[10];
	int bit[10][10];
	int cnt, found;

	memset(bot, 0, sizeof(bot));
	memset(bit, 0, sizeof(bit));

	cnt = found= this->Surf_desc_cnt;
	memset32(bit, 1, cnt);
	bot[0] = cnt;
	memcpy32(bit[1], bit[0], cnt);

	if (pSurface->Has_palette)
	{
		DDSURFACEDESC *pd = this->Surf_desc;
		for (int i = 0; i < cnt; i++, pd++)
		{
			if (bit[0][i] && !(pd->dwFlags & 0x20) || pSurface->sdesc.dwRGBBitCount > pd->ddpfPixelFormat.dwRGBBitCount)
			{
				found--;
				bit[1][i] = 0;
			}
		}

		if (found && cnt > 0)
		{
			int i;
			for (i = 0; i < cnt; i++)
				if (bit[1][i] == 1)
					break;
			if (i < cnt)
			{
				memcpy(pDesc, &this->Surf_desc[i], sizeof(*pDesc));
				if (this->System_memory)
					pDesc->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
				return 1;
			}
		}
	}

	bot[1] = found;
	memcpy32(bit[2], bit[1], cnt);
	if (pSurface->sdesc.field_16)
	{
		DDSURFACEDESC *pd = this->Surf_desc;
		MARNI_SURFACE_DESC ddesc;
		for (int i = 0; i < cnt; i++, pd++)
		{
			if (bit[1][i])
			{
				CalcPixelFormat(pd, &ddesc);
				if (!pd->ddpfPixelFormat.dwRGBAlphaBitMask || ddesc.field_16 == 1)
				{
					bit[2][i] = 0;
					found--;
				}
			}
		}
	}

	bot[2] = found;
	memcpy32(bit[3], bit[2], cnt);
	if (cnt > 0)
	{
		DDSURFACEDESC *pd = this->Surf_desc;
		for (int i = 0; i < cnt; i++)
		{
			if (bit[2][i] == 1 &&
				pSurface->Has_palette &&
				pSurface->sdesc.bpp > pd->ddpfPixelFormat.dwRGBBitCount ||
				pSurface->sdesc.dwRGBBitCount > pd->ddpfPixelFormat.dwRGBBitCount)
			{
				bit[3][i] = 0;
				found--;
			}
		}
	}

	bot[3] = found;
	memcpy32(bit[4], bit[3], cnt);
	if (cnt > 0)
	{
		DDSURFACEDESC *pd = this->Surf_desc;
		for (int i = 0; i < cnt; i++)
		{
			if (bit[3][i] == 1 &&
				pSurface->Has_palette &&
				pSurface->sdesc.bpp > pd->ddpfPixelFormat.dwRGBBitCount ||
				pSurface->sdesc.dwRGBBitCount > pd->ddpfPixelFormat.dwRGBBitCount)
			{
				bit[4][i] = 0;
				found--;
			}
		}
	}

	bot[4] = found;
	memcpy32(bit[5], bit[4], cnt);

	int i;
	int *pbot = &bot[4];
	int *pbit = bit[4];
	while (1)
	{
		if (*pbot)
		{
			i = 0;
			if (cnt > 0)
				break;
		}
label50:
		pbit -= 10;
		--pbot;
		if (pbot < bot)
			return 0;
	}
	int *pbet = pbit;
	while (*pbet != 1)
	{
		pbet++;
		if (++i >= cnt)
			goto label50;
	}

	memcpy(pDesc, &this->Surf_desc[i], sizeof(*pDesc));
	if (this->System_memory)
		pDesc->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;

	return 1;
}
