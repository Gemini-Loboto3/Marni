// MarniTest.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "MarniTest.h"
#include "Config.h"

#include "Marni.h"
#include "PsxTexture.h"
#include "hardwarebp.h"
#include <timeapi.h>
#include "debug_new.h"

#define FRAMES_PER_SECOND	(60)
#define TICKS_PER_FRAME		(1)
#define TICKS_PER_SECOND	(TICKS_PER_FRAME * FRAMES_PER_SECOND)

double TIME_Frequency, TIME_Ticks;

void Ticks()
{
	LARGE_INTEGER counter;

	QueryPerformanceCounter(&counter);
	TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
}

bool TIME_Init()
{
	LARGE_INTEGER frequency;

	if (!QueryPerformanceFrequency(&frequency))
		return false;

	TIME_Frequency = (double)frequency.QuadPart / (double)TICKS_PER_SECOND;
	Ticks();
	return true;
}

DWORD Sync()
{
	DWORD lastTicks, currentTicks;
	LARGE_INTEGER counter;

	QueryPerformanceCounter(&counter);
	lastTicks = (DWORD)TIME_Ticks;
	TIME_Ticks = (double)counter.QuadPart / TIME_Frequency;
	currentTicks = (DWORD)TIME_Ticks;

	return (currentTicks > lastTicks) ? currentTicks - lastTicks : 0;
}

#define newclear(TYPE)		new(calloc(sizeof(TYPE), 1)) TYPE();

static void Exit();

//CMarni *pMarni = NULL;

CConfig config;
CMarni *pMarni = NULL;
DWORD time_init;

void DXGL_Attach();

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
CHAR szTitle[MAX_LOADSTRING];                  // The title bar text
CHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
HWND				hWnd = nullptr;
u8*					buffer = nullptr;

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	DXGL_Attach();

    // TODO: Place code here.

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_MARNITEST, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

	buffer = new u8[10 * 1024 * 1024];

	// initialize crap
	CMarni *ppMarni = new CMarni();
	pMarni = ppMarni->Init(hWnd, 320, 240, 0, GFX_TOTAL);
	if (!pMarni) DestroyWindow(hWnd);

	SetDisplayRect();
	//SetDriverNames();

    MSG msg;

	pMarni->ClearBG_ = 1;

	CPsxTexture psx;
	LoadFile("..\\USA\\Data\\SIDE06.TIM", buffer);
	//psx.Store(buffer);
	psx.StorePng("01.png");

	RECT rc;
	SetRect(&rc, 0, 0, psx.dwWidth - 1, psx.dwHeight - 1);

	time_init = timeGetTime();
	TIME_Init();
    // Main message loop:
    while (1)
    {
		PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE);
		if (msg.message == WM_QUIT)
		{
			DestroyWindow(hWnd);
			break;
		}

		if (IsGpuActive())
		{
			//DWORD time_now = timeGetTime();
			if (Sync()/*time_now - time_init >= 31*/)
			{
				//time_init = time_now;
				//pMarni->MarniBitsMain.ClearBg(NULL, 0x884032, 0);
				pMarni->MarniBitsMain.BltSurface(NULL, &rc, &psx.surf, 0, NULL);
				pMarni->Render();
			}
		}

		TranslateMessage(&msg);
		DispatchMessage(&msg);
    }

	delete[] buffer;
    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MARNITEST));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // Store instance handle in our global variable

	hWnd = CreateWindowExA(0, szWindowClass, szTitle,
		//WS_CLIPCHILDREN |  WS_POPUP
		WS_CAPTION | WS_SYSMENU | WS_GROUP,
		CW_USEDEFAULT, 0,
		CW_USEDEFAULT, 0,
		nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
		return FALSE;

	ShowWindow(hWnd, nCmdShow);
	//UpdateWindow(hWnd);

	return TRUE;
}

void Exit()
{
	static int Exited = 0;

	if (!Exited)
	{
		Exited = 1;
		if (pMarni)
			delete pMarni;
		pMarni = NULL;
	}
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_SIZE)
		printf("Resizing\n");

	if(pMarni)
	{
		if (!pMarni->Message(hWnd, message, wParam, lParam))
			return 0;
	}

    switch (message)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
		Exit();
		hWnd = nullptr;
        PostQuitMessage(0);
        break;
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_F7:
			{
				DWORD timer = timeGetTime();
				if (timer + 10000 > time_init)
				{
					Display_mode--;
					if (Display_mode < 0)
						Display_mode = Max_resolutions - 1;
					SwitchResolution(Display_mode);
				}
			}
			break;
		case VK_F8:
			{
				DWORD timer = timeGetTime();
				if (timer + 10000 > time_init)
				{
					Display_mode++;
					if (Display_mode >= Max_resolutions)
						Display_mode = 0;
					SwitchResolution(Display_mode);
				}
			}
			break;
		}
		break;
	case WM_KEYUP:
		switch (wParam)
		{
		case VK_SNAPSHOT:
			pMarni->MarniBitsMain.WriteBitmap("test.png");
			break;
		}
		break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
