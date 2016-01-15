// WinMagnetApp.cpp : ���� ���α׷��� ���� �������� �����մϴ�.
//

#include "stdafx.h"
#include "WinMagnetApp.h"
#include "hyperLinkControl.h"
//-----------------------------------------------------------------------------

#define MAX_LOADSTRING	100
#define WM_TASKICON		WM_APP + 100
//-----------------------------------------------------------------------------

// ���� ����:
HINSTANCE	hInst;							// ���� �ν��Ͻ��Դϴ�.
HWND		g_hWnd;
HWND		g_hWnd64;						// 32bit app�� �����Ų 64bit app�� window handle
TCHAR		szTitle[MAX_LOADSTRING];		// ���� ǥ���� �ؽ�Ʈ�Դϴ�.
TCHAR		szWindowClass[MAX_LOADSTRING];	// �⺻ â Ŭ���� �̸��Դϴ�.
UINT		ShellRestart;
BOOL		MagnetUse = TRUE;
BOOL		MagnetIntensityStrong;
HMENU		hMenu;
HMENU		hPopupMenu;
HINSTANCE	g_hInstDll;
typedef void (*pInstallMouseHook)( BOOL _Strong );
typedef void (*pUninstallMouseHook)( void );
pInstallMouseHook	InstallMouseHook;
pUninstallMouseHook	UninstallMouseHook;
//-----------------------------------------------------------------------------

// �� �ڵ� ��⿡ ��� �ִ� �Լ��� ������ �����Դϴ�.
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
BOOL				addTaskIcon( HWND _hWnd );
void				saveMagnetStatus( BOOL _MagnetUse, BOOL _IntensityStrong );
void				loadMagnetStatus( BOOL& _rUse, BOOL& _rIntensityStrong );
void				startMagnet( void );
void				closeMagnet( void );
//-----------------------------------------------------------------------------

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// ���� ���ڿ��� �ʱ�ȭ�մϴ�.
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_WINMAGNETAPP, szWindowClass, MAX_LOADSTRING);

#ifdef WIN64
	wcscat( szTitle, L"64" );
	wcscat( szWindowClass, L"64" );
#else
	wcscat( szTitle, L"32" );
	wcscat( szWindowClass, L"32" );
#endif

	HANDLE Mutex = OpenMutex( MUTEX_ALL_ACCESS, false, szTitle );

	if ( NULL == Mutex )
		Mutex = CreateMutex( NULL, true, szTitle );
	else
	{
		MessageBox( NULL, L"already running.", L"Warning", MB_ICONWARNING | MB_OK );
		return 0;
	}
	
	MSG msg;

	MyRegisterClass(hInstance);

	// ���� ���α׷� �ʱ�ȭ�� �����մϴ�.
	if (!InitInstance(hInstance, nCmdShow))
		return FALSE;

	// �⺻ �޽��� �����Դϴ�.
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	ReleaseMutex( Mutex );

	return (int) msg.wParam;
}
//-----------------------------------------------------------------------------

ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= 0;//CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_MAGNET));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= NULL;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_MAGNET));

	return RegisterClassEx(&wcex);
}
//-----------------------------------------------------------------------------

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // �ν��Ͻ� �ڵ��� ���� ������ �����մϴ�.

   hWnd = CreateWindow(szWindowClass, szTitle, WS_ICONIC,
      0, 0, 0, 0, NULL, NULL, hInstance, NULL);

   if (!hWnd)
      return FALSE;

   g_hWnd = hWnd;

  // BOOL Wow64 = FALSE;
  // ::IsWow64Process( ::GetCurrentProcess(), &Wow64 );

  // if ( TRUE == Wow64 )
		//::WinExec( "WinMagnetApp64.exe", SW_HIDE );

   loadMagnetStatus( MagnetUse, MagnetIntensityStrong );

   hMenu		= ::LoadMenu( hInst, MAKEINTRESOURCE( IDR_MENU_TRY_POPUP ) );
   hPopupMenu	= ::GetSubMenu( hMenu, 0 );

   if ( MagnetUse )
   {
	   ::CheckMenuItem( hPopupMenu, ID_POPUP_MAGNET_EFFECT, MF_BYCOMMAND | MF_CHECKED );
	   startMagnet();
   }
   else
	   ::CheckMenuItem( hPopupMenu, ID_POPUP_MAGNET_EFFECT, MF_BYCOMMAND | MF_UNCHECKED );

   if ( MagnetIntensityStrong )
	   ::CheckMenuItem( hPopupMenu, ID_POPUP_STRONG, MF_BYCOMMAND | MF_CHECKED );
   else
	   ::CheckMenuItem( hPopupMenu, ID_POPUP_STRONG, MF_BYCOMMAND | MF_UNCHECKED );

   addTaskIcon( hWnd );

   ShowWindow( hWnd, SW_HIDE );

   return TRUE;
}
//-----------------------------------------------------------------------------


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;

	if ( message == ShellRestart )
		addTaskIcon( hWnd );

	switch (message)
	{
	case WM_TASKICON:
		switch ( lParam )
		{
		case WM_RBUTTONUP:
			{
				POINT pt;
				::GetCursorPos( &pt );
				::SetForegroundWindow( hWnd );
				::TrackPopupMenu( hPopupMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTALIGN, pt.x, pt.y, 0, hWnd, NULL );
				::SetForegroundWindow( hWnd );
			}
			break;
		}
		break;

	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);

		switch (wmId)
		{
		case ID_POPUP_MAGNET_EFFECT:
			MagnetUse = !MagnetUse;

			if ( MagnetUse )
			{
				::CheckMenuItem( hPopupMenu, ID_POPUP_MAGNET_EFFECT, MF_BYCOMMAND | MF_CHECKED );
				startMagnet();
			}
			else
			{
				::CheckMenuItem( hPopupMenu, ID_POPUP_MAGNET_EFFECT, MF_BYCOMMAND | MF_UNCHECKED );
				closeMagnet();
			}

			saveMagnetStatus( MagnetUse, MagnetIntensityStrong );
			break;

		case ID_POPUP_STRONG:
			MagnetIntensityStrong = !MagnetIntensityStrong;

			if ( MagnetIntensityStrong )
				::CheckMenuItem( hPopupMenu, ID_POPUP_STRONG, MF_BYCOMMAND | MF_CHECKED );
			else
				::CheckMenuItem( hPopupMenu, ID_POPUP_STRONG, MF_BYCOMMAND | MF_UNCHECKED );

			saveMagnetStatus( MagnetUse, MagnetIntensityStrong );

			if ( MagnetUse)
			{
				closeMagnet();
				startMagnet();
			}
			break;

		case ID_POPUP_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;

		case ID_POPUP_EXIT:
			DestroyWindow( hWnd );
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_DESTROY:
		{
			closeMagnet();

			NOTIFYICONDATA nid;
			nid.cbSize	= sizeof( NOTIFYICONDATA );
			nid.hWnd	= hWnd;
			nid.uID		= 1;
			::Shell_NotifyIcon( NIM_DELETE, &nid );

			::DestroyMenu( hPopupMenu );
			::DestroyMenu( hMenu );

			PostQuitMessage(0);
		}
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}
//-----------------------------------------------------------------------------

BOOL addTaskIcon( HWND _hWnd )
{
	NOTIFYICONDATA nid;

	nid.cbSize				= sizeof( nid );										// ����ü�� ������
	nid.hWnd				= _hWnd;												// �޼����� �޾Ƶ��̴� ������ �ڵ�
	nid.uCallbackMessage	= WM_TASKICON;											// �����쿡 �������� �޼���
	nid.uFlags				= NIF_ICON | NIF_MESSAGE | NIF_TIP;						// �÷���
	nid.hIcon				= ::LoadIcon( hInst, MAKEINTRESOURCE( IDI_MAGNET ) );	// ǥ���ϴ� �������� �ڵ�
	nid.uID					= 1;													// ������ �ĺ���
	wcscpy_s( nid.szTip, sizeof( nid.szTip ), szTitle );							// ǥ���ϴ� ��Ʈ

	ShellRestart	= ::RegisterWindowMessage( L"TaskbarCreated" );

	return ::Shell_NotifyIcon( NIM_ADD, &nid );
}

//-----------------------------------------------------------------------------

void saveMagnetStatus( BOOL _MagnetUse, BOOL _IntensityStrong )
{
	WCHAR path[_MAX_PATH + 1] = { NULL, };

	::GetModuleFileName( NULL, path, _MAX_PATH );	

	WCHAR* pPos = wcsrchr( path, '\\' );
	wcscpy( pPos, L"\\WinMagnet.ini" );

	WCHAR status[2] = { NULL, };

	if ( _MagnetUse )	status[0] = '1';
	else				status[0] = '0';

	::WritePrivateProfileString( L"WinMagnet", L"MagnetUse", status, path );

	if ( _IntensityStrong )	status[0] = '1';
	else					status[0] = '0';

	::WritePrivateProfileString( L"WinMagnet", L"MagnetIntensityStrong", status, path );
}
//-----------------------------------------------------------------------------

void loadMagnetStatus( BOOL& _rUse, BOOL& _rIntensityStrong )
{
	WCHAR path[260 + 1] = { NULL, };

	::GetModuleFileName( NULL, path, _MAX_PATH );	

	WCHAR* pPos = wcsrchr( path, '\\' );
	wcscpy( pPos, L"\\WinMagnet.ini" );

	_rUse				= ::GetPrivateProfileInt( L"WinMagnet", L"MagnetUse", 1, path );
	_rIntensityStrong	= ::GetPrivateProfileInt( L"WinMagnet", L"MagnetIntensityStrong", 0, path );	
}
//-----------------------------------------------------------------------------

void startMagnet( void )
{
	WCHAR path[_MAX_PATH] = { NULL, };

#ifdef WIN64
	wcscpy( path, L"WinMagnet64.dll" );
#else
	wcscpy( path, L"WinMagnet.dll" );
#endif

	g_hInstDll = ::LoadLibrary( path );
	if ( !g_hInstDll )
	{
		WCHAR buf[1024] = { 0, };
		wsprintf( buf, L"%s Loading Error: [%d]", path, GetLastError() );
		OutputDebugStringW( buf );

		::MessageBox( NULL, L"dll Loading Error", L"Error", MB_OK | MB_ICONERROR );
		::DestroyWindow( g_hWnd );
		return;
	}

	InstallMouseHook = (pInstallMouseHook)GetProcAddress( g_hInstDll, "InstallMouseHook" );
	if ( InstallMouseHook == NULL )
	{
		::MessageBox( NULL, L"InstallMouseHook Error", L"Error", MB_OK | MB_ICONERROR );
		::DestroyWindow( g_hWnd );
		return;
	}

	UninstallMouseHook = (pUninstallMouseHook)GetProcAddress( g_hInstDll, "UninstallMouseHook" );
	if ( UninstallMouseHook == NULL )
	{
		::MessageBox( NULL, L"UninstallMouseHook Error", L"Error", MB_OK | MB_ICONERROR );
		::DestroyWindow( g_hWnd );
		return;
	}

	InstallMouseHook( MagnetIntensityStrong );
}
//-----------------------------------------------------------------------------

void closeMagnet( void )
{
	UninstallMouseHook();

	if ( g_hInstDll != NULL )
		::FreeLibrary( g_hInstDll );
}
//-----------------------------------------------------------------------------

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	switch (message)
	{
	case WM_INITDIALOG:
		{
			cHyperLinkControl hyperLinkControl;
			hyperLinkControl.setTargetStaticControl( hDlg, IDC_STATIC_HYPER_LINK, L"http://prostars.net" );
			hyperLinkControl.setTargetStaticControl( hDlg, IDC_IMAGE, L"http://shinebody.net" );
		}
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
//-----------------------------------------------------------------------------