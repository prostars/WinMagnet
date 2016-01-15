#include "stdafx.h"
#include <ShellAPI.h>
#include "hyperLinkControl.h"
//-----------------------------------------------------------------------------

cHyperLinkControl::cHyperLinkControl()
{
}
//-----------------------------------------------------------------------------

cHyperLinkControl::~cHyperLinkControl()
{
}
//-----------------------------------------------------------------------------

bool cHyperLinkControl::setTargetStaticControl( HWND _hDialog, UINT _id, const WCHAR* _szUrl )
{
	// static control subclassing
	HWND hControl =  ::GetDlgItem( _hDialog, _id );
	if ( NULL == hControl ) 
		return false;

	HGLOBAL hProp = ::GetProp( hControl, L"controlData" );
	if ( NULL != hProp)
	{
		sControlData* pData = reinterpret_cast<sControlData*>( hProp );
		delete[] pData->szUrl;
		int len = wcslen( _szUrl ) + 1;
		pData->szUrl = new WCHAR[len];
		wcscpy_s( pData->szUrl, len, _szUrl );
	}
	else
	{
		sControlData* pData = new sControlData;

		int len = wcslen( _szUrl ) + 1;

		pData->szUrl	= new WCHAR[len];
		pData->oldProc	= reinterpret_cast<WNDPROC>( ::GetWindowLongPtr( hControl, GWLP_WNDPROC ) );
		pData->hBrush	= NULL;
		pData->hFont	= NULL;

		wcscpy_s( pData->szUrl, len, _szUrl );

		::SetProp( hControl, L"controlData", pData );
	}

	if ( FALSE == ::SetWindowText( hControl, _szUrl ) )
		return false;

	if ( 0 == ::SetWindowLongPtr( hControl, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>( controlWindowProc ) ) )
		return false;

	// dialog subclassing
	hProp = ::GetProp( _hDialog, L"dialogData" );
	if ( NULL == hProp )
	{
		sControlData* pData = new sControlData;

		pData->szUrl	= NULL;
		pData->oldProc	= reinterpret_cast<WNDPROC>( ::GetWindowLongPtr( _hDialog, GWLP_WNDPROC ) );
		pData->hBrush	= ::CreateSolidBrush( GetSysColor( COLOR_BTNFACE ) );
		pData->hFont	= NULL;

		::SetProp( _hDialog, L"dialogData", pData );

		if ( 0 == ::SetWindowLongPtr( _hDialog, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>( dialogWindowProc ) ) )
			return false;
	}

	return true;
}
//-----------------------------------------------------------------------------

LRESULT CALLBACK cHyperLinkControl::controlWindowProc( HWND _hWnd, UINT _msg, WPARAM _wParam, LPARAM _lParam )
{
	sControlData* pData = reinterpret_cast<sControlData*>( ::GetProp( _hWnd, L"controlData" ) );
	if ( NULL == pData )
		return ::DefWindowProc( _hWnd, _msg, _wParam, _lParam );

	switch ( _msg )
	{
	case WM_DESTROY:
		{
			::RemoveProp( _hWnd, L"controlData" );
			if ( NULL != pData->szUrl )
				delete[] pData->szUrl;
		}
		break;

	case WM_LBUTTONDOWN:
		{
			HWND hDialog = ::GetParent( _hWnd );
			if ( 0 == hDialog )
				hDialog = _hWnd;
			::ShellExecute( hDialog, L"open", pData->szUrl, NULL, NULL, SW_SHOWNORMAL );
		}
		break;

	case WM_SETCURSOR:
		::SetCursor( ::LoadCursor( NULL, MAKEINTRESOURCE( IDC_HAND ) ) );
		return TRUE;

	case WM_NCHITTEST:
		return HTCLIENT;
	}

	return ::CallWindowProc( pData->oldProc, _hWnd, _msg, _wParam, _lParam );
}
//-----------------------------------------------------------------------------

LRESULT CALLBACK cHyperLinkControl::dialogWindowProc( HWND _hWnd, UINT _msg, WPARAM _wParam, LPARAM _lParam )
{
	sControlData* pData = reinterpret_cast<sControlData*>( ::GetProp( _hWnd, L"dialogData" ) );
	if ( NULL == pData )
		return ::DefWindowProc( _hWnd, _msg, _wParam, _lParam );

	switch ( _msg )
	{
	case WM_DESTROY:
		::RemoveProp( _hWnd, L"dialogData" );
		if ( NULL != pData->hBrush )
			::DeleteObject( pData->hBrush );
		if ( NULL != pData->hFont )
			::DeleteObject( pData->hFont );
		break;

	case WM_CTLCOLORSTATIC:
		{
			HDC				hdc				= reinterpret_cast<HDC>( _wParam );
			HWND			hControl		= reinterpret_cast<HWND>( _lParam );
			sControlData*	pControlData	= reinterpret_cast<sControlData*>( ::GetProp( hControl, L"controlData" ) );
			long			style			= ::GetWindowLong( hControl, GWL_STYLE );

			if ( NULL == pControlData || SS_BITMAP & style )
				return ::DefWindowProc( _hWnd, _msg, _wParam, _lParam );

			::SetTextColor( hdc, RGB( 0, 0, 255 ) );
			::SetBkColor( hdc, GetSysColor( COLOR_BTNFACE ) );
			if ( NULL == pControlData->hFont )
			{
				TEXTMETRIC tm;
				::GetTextMetrics( hdc, &tm );

				LOGFONT lf;
				lf.lfHeight			= tm.tmHeight;
				lf.lfWidth			= 0;
				lf.lfEscapement		= 0;
				lf.lfOrientation	= 0;
				lf.lfWeight			= tm.tmWeight;
				lf.lfItalic			= tm.tmItalic;
				lf.lfUnderline		= TRUE;
				lf.lfStrikeOut		= tm.tmStruckOut;
				lf.lfCharSet		= tm.tmCharSet;
				lf.lfOutPrecision	= OUT_DEFAULT_PRECIS;
				lf.lfClipPrecision	= CLIP_DEFAULT_PRECIS;
				lf.lfQuality		= DEFAULT_QUALITY;
				lf.lfPitchAndFamily	= tm.tmPitchAndFamily;
				::GetTextFace( hdc, LF_FACESIZE, lf.lfFaceName );

				pData->hFont = ::CreateFontIndirect( &lf );
			}

			::SelectObject( hdc, pData->hFont );
			return reinterpret_cast<LRESULT>( pData->hBrush );
		}
		break;
	}

	return ::CallWindowProc( pData->oldProc, _hWnd, _msg, _wParam, _lParam );
}
//-----------------------------------------------------------------------------