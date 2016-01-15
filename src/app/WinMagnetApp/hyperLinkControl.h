//#include <string>
//using namespace std;
//-----------------------------------------------------------------------------

struct sControlData
{
	WCHAR*	szUrl;
	WNDPROC oldProc;
	HFONT	hFont;
	HBRUSH	hBrush;
};
//-----------------------------------------------------------------------------

class cHyperLinkControl
{
public:
	cHyperLinkControl();
	~cHyperLinkControl();

	bool setTargetStaticControl( HWND _hDialog, UINT _id, const WCHAR* _szUrl );

	static LRESULT CALLBACK controlWindowProc( HWND _hWnd, UINT _msg, WPARAM _wParam, LPARAM _lParam );
	static LRESULT CALLBACK dialogWindowProc( HWND _hWnd, UINT _msg, WPARAM _wParam, LPARAM _lParam );
};