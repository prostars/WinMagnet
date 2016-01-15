// WinMagnet.cpp : DLL 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include <CommCtrl.h>
#include <Shellapi.h>
//-----------------------------------------------------------------------------


struct sDisplaysInfo
{
	int		count;
	RECT	rects[8];
};


#pragma data_seg(".kbdata")
POINT			g_ptStartPos	= { 0, };
sDisplaysInfo	g_displaysInfo	= { 0, };
BOOL			g_bSubclassing	= FALSE;
HHOOK			g_HMouseHook	= NULL;
HINSTANCE		g_hModule		= NULL;
BOOL			g_Strong		= FALSE;
HWND			g_hTargetWnd	= NULL;
#pragma data_seg()

#pragma comment(linker, "/SECTION:.kbdata,RWS")
//-----------------------------------------------------------------------------


#ifdef _MANAGED
#pragma managed(push, off)
#endif

BOOL APIENTRY DllMain( HINSTANCE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved )
{
	switch ( ul_reason_for_call )
	{
	case DLL_PROCESS_ATTACH :
		{
			g_hModule = hModule;

			#ifdef _DEBUG
				WCHAR buf[1024] = {0, };
				wsprintf( buf, L"DLL_PROCESS_ATTACH, module: %x", hModule );
				OutputDebugStringW( buf );
			#endif
		}
		break;

	case DLL_PROCESS_DETACH :
		{
			g_hModule = NULL;

			#ifdef _DEBUG
				WCHAR buf[1024] = {0, };
				wsprintf( buf, L"DLL_PROCESS_DETACH" );
				OutputDebugStringW( buf );
			#endif
		}
		break;
	}

    return TRUE;
}

#ifdef _MANAGED
#pragma managed(pop)
#endif
//-----------------------------------------------------------------------------


BOOL CALLBACK MonitorEnumProc( HMONITOR _hMonitor, HDC _hdcMonitor, LPRECT _pMonitor, LPARAM _pData )
{
	sDisplaysInfo *pDisplaysInfo = reinterpret_cast<sDisplaysInfo*>( _pData );

	int idx = pDisplaysInfo->count;

	pDisplaysInfo->rects[idx] = *_pMonitor;
	pDisplaysInfo->count++;

	return TRUE;
}
//-----------------------------------------------------------------------------


bool getDisplayMonitorsInfo( sDisplaysInfo& _rDisplaysInfo )
{
	bool result = TRUE == EnumDisplayMonitors( NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>( &_rDisplaysInfo ) );

	return result;
}
//-----------------------------------------------------------------------------


LRESULT CALLBACK NewWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
	const int	MagnetRange = (g_Strong) ? 40 : 20;
	LPRECT		pRect;
	int			width, height;
	LRESULT		lResult;
	POINT		ptMouse;

	lResult = DefSubclassProc( hWnd, uMsg, wParam, lParam );

	if ( uMsg == WM_MOVING )
	{
		GetCursorPos( &ptMouse );

		pRect	= (LPRECT)lParam;	// current position of the window, in screen coordinates
		width	= pRect->right - pRect->left;
		height	= pRect->bottom - pRect->top;

		// target window position
		for ( int idx = 0; idx < g_displaysInfo.count; ++idx )
		{
			if ( pRect->left <= g_displaysInfo.rects[idx].left + MagnetRange && pRect->left >= g_displaysInfo.rects[idx].left - MagnetRange )
			{
				pRect->left		= g_displaysInfo.rects[idx].left;
				pRect->right	= g_displaysInfo.rects[idx].left + width;
			}
			else if ( pRect->right <= g_displaysInfo.rects[idx].right + MagnetRange && pRect->right >= g_displaysInfo.rects[idx].right - MagnetRange )
			{
				pRect->right	= g_displaysInfo.rects[idx].right;
				pRect->left		= g_displaysInfo.rects[idx].right - width;
			}

			if ( pRect->top <= g_displaysInfo.rects[idx].top + MagnetRange && pRect->top >= g_displaysInfo.rects[idx].top - MagnetRange )
			{
				pRect->top		= g_displaysInfo.rects[idx].top;
				pRect->bottom	= g_displaysInfo.rects[idx].top + height;
			}
			else if ( pRect->bottom <= g_displaysInfo.rects[idx].bottom + MagnetRange && pRect->bottom >= g_displaysInfo.rects[idx].bottom - MagnetRange )
			{
				pRect->bottom	= g_displaysInfo.rects[idx].bottom;
				pRect->top		= g_displaysInfo.rects[idx].bottom - height;
			}
		}

		// mouse postion
		if ( ((ptMouse.x - pRect->left - g_ptStartPos.x) >= MagnetRange ) ||
			 ((ptMouse.x - pRect->left - g_ptStartPos.x) <= -MagnetRange ) )
		{
			pRect->left = ptMouse.x - g_ptStartPos.x;
			pRect->right = pRect->left + width;
		}
		if ( ((ptMouse.y - pRect->top - g_ptStartPos.y) >= MagnetRange ) ||
			 ((ptMouse.y - pRect->top - g_ptStartPos.y) <= -MagnetRange ) )
		{
			pRect->top = ptMouse.y - g_ptStartPos.y;
			pRect->bottom = pRect->top + height;
		}
	}

    return lResult;
}
//-----------------------------------------------------------------------------


void SubClassingStart( HWND hWnd, int nX, int nY )
{
    RECT rtSelfRect;

    GetWindowRect( hWnd, &rtSelfRect );

    g_ptStartPos.x = nX;
    g_ptStartPos.y = nY;
    g_ptStartPos.x = g_ptStartPos.x - rtSelfRect.left;
    g_ptStartPos.y = g_ptStartPos.y - rtSelfRect.top;

	g_displaysInfo.count = 0;
	getDisplayMonitorsInfo( g_displaysInfo );

	APPBARDATA appBarData = { sizeof( APPBARDATA ), };
	::SHAppBarMessage( ABM_GETTASKBARPOS, &appBarData );

	RECT rect;

	for ( int cnt = 0; cnt < g_displaysInfo.count; ++cnt )
		if ( TRUE == ::IntersectRect( &rect, &g_displaysInfo.rects[cnt], &appBarData.rc ) )
			if ( TRUE == ::SubtractRect( &rect, &g_displaysInfo.rects[cnt], &appBarData.rc ) )
			{
				g_displaysInfo.rects[cnt] = rect;
				break;
			}


	#ifdef _DEBUG
		WCHAR text[1024] = { 0, };
		WCHAR buf[1024] = {0, };
		::GetWindowText( hWnd, text, 1000 );
		wsprintf( buf, L"hwnd: [%d], caption: [%s], NewWndProc: [%x]\n", hWnd, text, NewWndProc );
		OutputDebugStringW( buf );
	#endif

	DWORD_PTR temp;
	if ( FALSE == GetWindowSubclass( hWnd, NewWndProc, 0, &temp ) )
	{
		g_bSubclassing = SetWindowSubclass( hWnd, NewWndProc, 0, 0 );
		if ( FALSE == g_bSubclassing )
		{
			#ifdef _DEBUG
				WCHAR buf[1024] = { 0, };
				wsprintf( buf, L"NewWndProc setting fail. error: [%d]", GetLastError() );
				OutputDebugStringW( buf );
			#endif
		}
	}
}
//-----------------------------------------------------------------------------


void SubClassingClose( HWND hWnd )
{
	DWORD_PTR temp;
	if ( TRUE == GetWindowSubclass( hWnd, NewWndProc, 0, &temp ) )
	{
		BOOL result = RemoveWindowSubclass( hWnd, NewWndProc, 0 );

		#ifdef _DEBUG
			WCHAR buf[1024] = {0, };
			wsprintf( buf, L"RemoveWindowSubclass result: %d", result );
			OutputDebugStringW( buf );
		#endif
	}
}
//-----------------------------------------------------------------------------


LRESULT CALLBACK MouseHookProc( INT nCode, WPARAM wp, LPARAM lp )
{
    if ( nCode == HC_ACTION )
    {
        switch ( wp )
        {
		case	WM_LBUTTONDOWN :
		case    WM_NCLBUTTONDOWN :
			{
				#ifdef _DEBUG
					WCHAR buf[1024] = {0, };
					wsprintf( buf, L"mouse lbutton down\n" );
					OutputDebugStringW( buf );
				#endif
				DWORD processID = 0;
				::GetWindowThreadProcessId( ((MOUSEHOOKSTRUCT*)lp)->hwnd, &processID );
				if ( processID )
				{
					HANDLE	hProcess	= ::OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, processID );
					BOOL	Wow64		= FALSE;
					::IsWow64Process( hProcess, &Wow64 );
					::CloseHandle( hProcess );

					#ifdef WIN64
						if ( TRUE == Wow64 )		// 타겟 프로세스가 wow64로 실행 중이면 32bit 프로세스므로 패스한다.
						{
							WCHAR buf[1024] = {0, };
							wsprintf( buf, L"wow64 process, pass" );
							OutputDebugStringW( buf );
							break;
						}
						else
						{
							WCHAR buf[1024] = {0, };
							wsprintf( buf, L"x64 process, keep going" );
							OutputDebugStringW( buf );
						}
					#else
						BOOL CurrentWow64 = FALSE;
						::IsWow64Process( ::GetCurrentProcess(), &CurrentWow64 );
						if ( TRUE == CurrentWow64 )	// 32bit로 빌드된 exe가 wow64로 실행 중인게 아니면 32bit windows다.
						{
							if ( FALSE == Wow64 )	// 여기에 걸리면 64bit 프로세스므로 패스한다.
							{
								#ifdef _DEBUG
									WCHAR buf[1024] = {0, };
									wsprintf( buf, L"pure 64bit process" );
									OutputDebugStringW( buf );
								#endif

								break;
							}
						}
					#endif
				}

				if ( ((MOUSEHOOKSTRUCT*)lp)->wHitTestCode == HTCAPTION )
				{
					#ifdef _DEBUG
						WCHAR buf[1024] = {0, };
						wsprintf( buf, L"HTCAPTION mouse lbutton down\n" );
						OutputDebugStringW( buf );
					#endif

					if ( (GetWindowLongPtr( ((MOUSEHOOKSTRUCT*)lp)->hwnd, GWL_STYLE ) & WS_CHILD) == 0 )
					{
						#ifdef _DEBUG
							WCHAR buf[1024] = {0, };
							wsprintf( buf, L"SubClassingStart call( %d, %d, %d )\n", ((MOUSEHOOKSTRUCT*)lp)->hwnd, ((MOUSEHOOKSTRUCT*)lp)->pt.x, ((MOUSEHOOKSTRUCT*)lp)->pt.y );
							OutputDebugStringW( buf );
						#endif

						g_hTargetWnd = ((MOUSEHOOKSTRUCT*)lp)->hwnd;
						SubClassingStart( g_hTargetWnd, ((MOUSEHOOKSTRUCT*)lp)->pt.x, ((MOUSEHOOKSTRUCT*)lp)->pt.y );
					}
					else
					{
						HWND hWnd = ::GetAncestor( ((MOUSEHOOKSTRUCT*)lp)->hwnd, GA_ROOT );
						if ( hWnd )
						{
							#ifdef _DEBUG
								WCHAR buf[1024] = {0, };
								wsprintf( buf, L"not caption area, SubClassingStart call( %d, %d, %d )\n", ((MOUSEHOOKSTRUCT*)lp)->hwnd, ((MOUSEHOOKSTRUCT*)lp)->pt.x, ((MOUSEHOOKSTRUCT*)lp)->pt.y );
								OutputDebugStringW( buf );
							#endif

							g_hTargetWnd = hWnd;
							SubClassingStart( g_hTargetWnd, ((MOUSEHOOKSTRUCT*)lp)->pt.x, ((MOUSEHOOKSTRUCT*)lp)->pt.y );
						}
						else
						{
							#ifdef _DEBUG
								WCHAR buf[1024] = {0, };
								wsprintf( buf, L"not caption area, hWnd: %d, error: %d\n", hWnd, GetLastError() );
								OutputDebugStringW( buf );
							#endif
						}
					}
				}
				else
				{
					HWND hWnd = ::GetAncestor( ((MOUSEHOOKSTRUCT*)lp)->hwnd, GA_ROOT );
					if ( hWnd )
					{
						#ifdef _DEBUG
							WCHAR buf[1024] = {0, };
							wsprintf( buf, L"not caption area, SubClassingStart call( %d, %d, %d )\n", ((MOUSEHOOKSTRUCT*)lp)->hwnd, ((MOUSEHOOKSTRUCT*)lp)->pt.x, ((MOUSEHOOKSTRUCT*)lp)->pt.y );
							OutputDebugStringW( buf );
						#endif

						g_hTargetWnd = hWnd;
						SubClassingStart( g_hTargetWnd, ((MOUSEHOOKSTRUCT*)lp)->pt.x, ((MOUSEHOOKSTRUCT*)lp)->pt.y );
					}
					else
					{
						#ifdef _DEBUG
							WCHAR buf[1024] = {0, };
							wsprintf( buf, L"not caption area, hWnd: %d, error: %d\n", hWnd, GetLastError() );
							OutputDebugStringW( buf );
						#endif
					}
				}				
			}
			break;
        case    WM_NCLBUTTONUP :
        case    WM_NCRBUTTONUP :
        case    WM_NCMBUTTONUP :
        case    WM_LBUTTONUP :
        case    WM_RBUTTONUP :
        case    WM_MBUTTONUP :
			DWORD processID = 0;
			::GetWindowThreadProcessId( ((MOUSEHOOKSTRUCT*)lp)->hwnd, &processID );
			if ( processID )
			{
				HANDLE	hProcess	= ::OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, processID );
				BOOL	Wow64		= FALSE;
				::IsWow64Process( hProcess, &Wow64 );
				::CloseHandle( hProcess );

				#ifdef WIN64
					if ( TRUE == Wow64 )		// 타겟 프로세스가 wow64로 실행 중이면 32bit 프로세스므로 패스한다.
					{
						WCHAR buf[1024] = {0, };
						wsprintf( buf, L"wow64 process, pass" );
						OutputDebugStringW( buf );
						break;
					}
					else
					{
						WCHAR buf[1024] = {0, };
						wsprintf( buf, L"x64 process, keep going" );
						OutputDebugStringW( buf );
					}
				#else
					BOOL CurrentWow64 = FALSE;
					::IsWow64Process( ::GetCurrentProcess(), &CurrentWow64 );
					if ( TRUE == CurrentWow64 )	// 32bit로 빌드된 exe가 wow64로 실행 중인게 아니면 32bit windows다.
					{
						if ( FALSE == Wow64 )	// 여기에 걸리면 64bit 프로세스므로 패스한다.
						{
							#ifdef _DEBUG
								WCHAR buf[1024] = {0, };
								wsprintf( buf, L"pure 64bit process" );
								OutputDebugStringW( buf );
							#endif

							break;
						}
					}
				#endif
			}

			#ifdef _DEBUG
				WCHAR buf[1024] = {0, };
				wsprintf( buf, L"wp: [%d], SubClassingClose call( %d a)\n", wp, ((MOUSEHOOKSTRUCT*)lp)->hwnd );
				OutputDebugStringW( buf );
			#endif

            SubClassingClose( g_hTargetWnd );
			break;
        }
    }

    return CallNextHookEx( g_HMouseHook, nCode, wp, lp );
}
//-----------------------------------------------------------------------------


extern "C" __declspec(dllexport) void InstallMouseHook( BOOL _Strong )
{
	g_Strong		= _Strong;
	g_HMouseHook	= ::SetWindowsHookEx( WH_MOUSE, MouseHookProc, g_hModule, 0 );
	//g_HMouseHook	= ::SetWindowsHookEx( WH_CBT, CBTProc, g_hModule, 0 );
	if ( NULL == g_HMouseHook )
	{
		#ifdef _DEBUG
			WCHAR buf[1024] = {0, };
			wsprintf( buf, L"SetWindowsHookEx fail. error: %d ", GetLastError() );
			OutputDebugStringW( buf );
		#endif
	}
	#ifdef _DEBUG
		else
		{
			WCHAR buf[1024] = {0, };
			wsprintf( buf, L"SetWindowsHookEx success. g_HMouseHook: %x, g_hModule: %x, error: %d", g_HMouseHook, g_hModule, GetLastError() );
			OutputDebugStringW( buf );
		}
	#endif
}
//-----------------------------------------------------------------------------


extern "C" __declspec(dllexport) void UninstallMouseHook( void )
{
	::UnhookWindowsHookEx( g_HMouseHook );
}
//-----------------------------------------------------------------------------