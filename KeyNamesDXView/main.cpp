#include "KeyNamesDXTranslator.h"
#include "clipboard.h"
#include "MyKeyboardDevice.h"
#include "DCBitmapBuffer.h"

#include <map>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "msimg32.lib")


static LPDIRECTINPUT8 g_DirectInput = NULL;

#define CLSNAME		_T("WZWinBase")
#define WNDNAME		_T("KeyNamesDXView - DirectInput8键位名称查询工具")

static LPCTSTR g_title = _T("KeyNamesDXView");

static HWND				g_hwnd = NULL;
static HINSTANCE		g_hInst = NULL;
static HFONT			g_hFont = NULL;

void InitWinClass(WNDCLASS& wndcls);
LRESULT CALLBACK WinProc(HWND, UINT, WPARAM, LPARAM);


#define delete_then_null(v)					((delete (v)), (v)=NULL)
#define release_then_null(v)				if ((v) != NULL) { (v)->Release(); (v)=NULL; }
#define is_empty_LPTSTR(v)					( (v)[0] == _T('\0') )
#define set_LPTSTR_to_empty(v)				( (v)[0] = _T('\0') )
#define lengthof(arr)						(sizeof(arr)/sizeof(arr[0]))
#define last_element_of(arr)				( arr[lengthof(arr)-1] )
#define call_func_then_null(funcname, v)	if ((v) != NULL) { (funcname)(v); (v)=NULL; }

inline LONG PointToLogical(int lPoint, HDC dc)
{
	return -MulDiv(lPoint, ::GetDeviceCaps(dc, LOGPIXELSY), 72);
}

void initFont(HFONT& hFont, HWND hwnd, int fontsize, BOOL bBold=FALSE)
{
	NONCLIENTMETRICS ncm;
	HDC dc = GetDC(hwnd);

	memset(&ncm, 0, sizeof(ncm));
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

	//更改字体大小和粗细
	ncm.lfMessageFont.lfHeight = PointToLogical(fontsize, dc);
	ncm.lfMessageFont.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
	ReleaseDC(hwnd, dc);

	hFont = CreateFontIndirect(&ncm.lfMessageFont);
}


void exit_if_failed(HRESULT hr, LPCTSTR szFuncName)
{
	if (FAILED(hr)) {
		CString strMsg;
		strMsg.Format(_T("%s() FAILED!(hr=0x%x)"), szFuncName, hr);

		MessageBox(g_hwnd, strMsg, _T("Error"), 16);

		if (g_DirectInput != NULL) g_DirectInput->Release();
		exit(1);
	}
}







////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////


static CMyKeyboardDevice* g_pKbd = NULL;

static const int g_nInitWindowW = 800;
static const int g_nInitWindowH = 360;

#define ID_TIMER_CLEAR_STATUS			(42)

#define MESSAGE_LENGTH					(512)
static TCHAR g_szMessage[MESSAGE_LENGTH];

#define STATUS_MESSAGE_LENGTH			(128)
static TCHAR g_szStatusMessage[STATUS_MESSAGE_LENGTH];
static COLORREF g_clrStatusColors[2] = { 0xBB80FF ,  0xFF80BB };
static int g_nCurrentStatusColorIndex;
static bool g_bIsNewStatus = false;
static int g_nStatusSpeed = 0;

static UINT g_uMessageAdditionalFlags = DT_CENTER;

#define WZFLAG_HORIZONTAL_REVERSE		1
#define WZFLAG_VERTICAL_REVERSE			2
static UINT g_uFlags = 0;

static CKeyNamesDXTranslator::WORK_MODE g_workMode = CKeyNamesDXTranslator::WORK_MODE_KEYNAME;

void processInput();


int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR lpCmdLine, int nCmdShow)
{
	g_hInst = hInst;

	WNDCLASS wndcls;
	InitWinClass(wndcls);
	if (!RegisterClass(&wndcls)) {
		MessageBox(NULL, _T("RegisterClass Error"), _T("Error"), 16);
		return 128;
	}
	g_hwnd = CreateWindowEx(0, CLSNAME, WNDNAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, g_nInitWindowW, g_nInitWindowH, NULL, NULL, hInst, NULL);
	if (!g_hwnd) {
		MessageBox(NULL, _T("CreateWindowEx Error"), _T("Error"), 16);
		return 127;
	}

	ShowWindow(g_hwnd, nCmdShow);
	UpdateWindow(g_hwnd);

	HRESULT hr = DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, reinterpret_cast<void**>(&g_DirectInput), NULL);
	if (!g_hwnd) {
		MessageBox(NULL, _T("Cannot create DirectInput8 device"), _T("Error"), 16);
		return 126;
	}


	g_pKbd = new CMyKeyboardDevice(g_hwnd, g_DirectInput);
	CMyKeyboardDevice::lasterror_t le = g_pKbd->getLastError();
	exit_if_failed(le.hr, le.func_name);

	InvalidateRect(g_hwnd, NULL, TRUE);

	MSG msg = {0};
	while (msg.message != WM_QUIT) {
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			processInput();
			Sleep(7);
		}
	}

	delete_then_null(g_pKbd);
	return (int)msg.wParam;
}



VOID CALLBACK TimerProc(
	HWND hwnd,         // handle to window
	UINT uMsg,         // WM_TIMER message
	UINT_PTR idEvent,  // timer identifier
	DWORD dwTime       // current system time
)
{
	switch (idEvent)
	{
	case ID_TIMER_CLEAR_STATUS:
		set_LPTSTR_to_empty(g_szStatusMessage);
		KillTimer(hwnd, ID_TIMER_CLEAR_STATUS);
	}
}

void setStatusText(LPCTSTR text, UINT uTimeMS, int speed)
{
	if (text != g_szStatusMessage)
		_tcscpy_s(g_szStatusMessage, STATUS_MESSAGE_LENGTH, text);
	g_nCurrentStatusColorIndex = g_nCurrentStatusColorIndex == 1 ? 0 : 1;
	g_bIsNewStatus = true;
	g_nStatusSpeed = speed;
	SetTimer(g_hwnd, ID_TIMER_CLEAR_STATUS, uTimeMS, TimerProc);
}

void processInput()
{
	if (!g_pKbd->read())
		return;

	static CKeyNamesDXTranslator translator;
	static BYTE bytPrevKeyCodes[256];
	static UINT nPrevKeyCode = translator.INVALID_KEYCODE;
	static int nSameKeyStateCount = 0;
	static DWORD dwPrevKeyCount = 0;

	DWORD dwKeyCount = 0;
	UINT nKeyCode = translator.INVALID_KEYCODE;
	CString strKeyName = translator.translate_all(g_pKbd->getState(), &dwKeyCount, &nKeyCode);
	_tcscpy_s(g_szMessage, MESSAGE_LENGTH, strKeyName);

	if (dwKeyCount == 1) {
		if (dwPrevKeyCount == 0) {
			if (nKeyCode != translator.INVALID_KEYCODE && nKeyCode == nPrevKeyCode) {
				if (++nSameKeyStateCount >= 2) {
					nPrevKeyCode = translator.INVALID_KEYCODE;
					nSameKeyStateCount = 0;

					// 单键连按 3 次成功
					if (SetClipTextW(g_szMessage)) {
						CString strFmt;
						strFmt.Format(_T("复制“%s”到剪贴板成功！"), g_szMessage);
						setStatusText(strFmt, 5000, 19);
					}
				}
			}
			else {
				nPrevKeyCode = nKeyCode;
				nSameKeyStateCount = 0;
			}
		}
	}
	else if (dwKeyCount > 1) {
		nPrevKeyCode = translator.INVALID_KEYCODE;
		nSameKeyStateCount = 0;

		const bool is_same = memcmp(bytPrevKeyCodes, g_pKbd->getState(), 256) == 0;
		if (!is_same && dwKeyCount == 2) {
#define KDOWN(k)		(g_pKbd->isKeyDown(k))
			bool bShift = KDOWN(DIK_LSHIFT) || KDOWN(DIK_RSHIFT);
			if (bShift) {
				if (KDOWN(DIK_S)) {
					CKeyNamesDXTranslator::WORK_MODE mode_map[] = {
						CKeyNamesDXTranslator::WORK_MODE_KEYNAME,
						CKeyNamesDXTranslator::WORK_MODE_KEYCODE,
						CKeyNamesDXTranslator::WORK_MODE_KEYCODE_10,
					};

					int i = 0;
					for (auto &m : mode_map) {
						if (g_workMode == m) {
							auto new_m = (m != last_element_of(mode_map)) ? mode_map[i + 1] : mode_map[0];
							g_workMode = translator.m_workmode = new_m;
							break;
						}
						++i;
					}
				}
				else if (KDOWN(DIK_LEFT)) {
					g_uMessageAdditionalFlags = DT_LEFT;
				}
				else if (KDOWN(DIK_RIGHT)) {
					g_uMessageAdditionalFlags = DT_RIGHT;
				}
				else if (KDOWN(DIK_DOWN) || KDOWN(DIK_UP)) {
					g_uMessageAdditionalFlags = DT_CENTER;
				}
				else if (KDOWN(DIK_A)) {
					CString strFmt;
					strFmt.Format(_T("编译日期：" __DATE__  " 作者：%c%c%u<wingzero%u@gmail.com>"), _T('w'), _T('z'), 520, 1040);
					setStatusText(strFmt, 20000, 30);
				}
			}
			else if (KDOWN(DIK_SPACE)) {
				if (KDOWN(DIK_LEFT) || KDOWN(DIK_RIGHT)) {
					g_uFlags ^= WZFLAG_HORIZONTAL_REVERSE;
				}
				else if (KDOWN(DIK_DOWN) || KDOWN(DIK_UP)) {
					g_uFlags ^= WZFLAG_VERTICAL_REVERSE;
				}
			}
#undef KDOWN
		}
	}
	dwPrevKeyCount = dwKeyCount;
	memcpy(bytPrevKeyCodes, g_pKbd->getState(), 256);

	InvalidateRect(g_hwnd, NULL, FALSE);
}


//COLORREF2vertexcolor
#define CLR2V_R(color)		( (COLOR16)(((color) & 0xFF)<<8) )
#define CLR2V_G(color)		( (COLOR16)(((color) & 0xFF00)) )
#define CLR2V_B(color)		( (COLOR16)(((color) & 0xFF0000)>>8) )

//背景色
#define BKCOLOR1			(0x401005)
#define BKCOLOR2			(0x054010)



void drawBackground(HDC dc, int w, int h)
{
	//背景色
	static TRIVERTEX g_vertbk[2] = {
		{ 0, 0, CLR2V_R(BKCOLOR1), CLR2V_G(BKCOLOR1), CLR2V_B(BKCOLOR1), 0x7f00 },
		{ 0, 0, CLR2V_R(BKCOLOR2), CLR2V_G(BKCOLOR2), CLR2V_B(BKCOLOR2), 0x7f00 }
	};

	static GRADIENT_RECT gRect = { 0,1 }; //UpperLeft&RightBottom

	g_vertbk[1].x = w;
	g_vertbk[1].y = h;

	GradientFill(dc, g_vertbk, 2, &gRect, 1, GRADIENT_FILL_RECT_V);
}

void drawScreen(HDC dc, int w, int h, RECT& rect)
{
	HDC &memDC = dc;
	if (g_pKbd != NULL) {

		TCHAR bufferText[MESSAGE_LENGTH] = { 0 };

		SelectObject(memDC, g_hFont);
		SetBkMode(memDC, TRANSPARENT);

		SetTextColor(memDC, 0x0099FF);
		LPCTSTR const szWorkMode = (g_workMode == CKeyNamesDXTranslator::WORK_MODE_KEYNAME)
			? _T("键位名称") : g_workMode == CKeyNamesDXTranslator::WORK_MODE_KEYCODE
			? _T("扫描码(16进制)") : _T("扫描码(10进制)");
		LPCTSTR const szTopHintFormat =
			_T("请按键盘上的任意键（可同时多个），会显示其【%s】。\r\n"
				"【单个键连按3次】：可将其【%s】复制到剪贴板中。\r\n"
				"【Shift+S】：切换 键位名称/扫描码(16进制)/扫描码(10进制) 的显示及复制\r\n"
				"【Shift+A】：关于本程序\r\n"
				"【Shift+↑或↓或←或→】：更改【%s】的对齐方式。其中↑↓键均为居中对齐\r\n"
				"【空格+↑或↓或←或→】：鬼人正邪"
			);
		_stprintf_s(bufferText, MESSAGE_LENGTH, szTopHintFormat, szWorkMode, szWorkMode, szWorkMode);
		DrawTextEx(memDC, bufferText, -1, &rect, DT_NOPREFIX | DT_END_ELLIPSIS | DT_WORDBREAK | DT_WORD_ELLIPSIS, NULL);

		UINT uFormat = DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS | DT_WORDBREAK | DT_WORD_ELLIPSIS;

		// draw message
		if (!is_empty_LPTSTR(g_szMessage)) {
			SetTextColor(memDC, 0xFFCC66);
			rect.top = rect.bottom / 2 - 10;
			DrawTextEx(memDC, g_szMessage, -1, &rect, uFormat | g_uMessageAdditionalFlags, NULL);
		}

		// Draw status message
		if (!is_empty_LPTSTR(g_szStatusMessage)) {
			SIZE size = { 0 };
			GetTextExtentPoint32(memDC, g_szStatusMessage, _tcslen(g_szStatusMessage), &size);

			static int status_x = 0;
			if (g_bIsNewStatus) {
				status_x = -size.cx - 10;
				g_bIsNewStatus = false;
			}

			rect.top = rect.bottom - 50;
			rect.left = (status_x > 10) ? 10 : status_x;
			rect.right = rect.left + size.cx;
			rect.bottom = rect.top + size.cy;
			status_x += g_nStatusSpeed;

			const auto clrStatus = g_clrStatusColors[g_nCurrentStatusColorIndex];
			SetTextColor(memDC, clrStatus);

			DrawTextEx(memDC, g_szStatusMessage, -1, &rect, uFormat, NULL);

			// draw rect
			InflateRect(&rect, 10, 10);
			SelectObject(dc, GetStockObject(NULL_BRUSH));
			SelectObject(dc, GetStockObject(DC_PEN));
			SetDCPenColor(dc, clrStatus);
			Rectangle(dc, rect.left, rect.top, rect.right, rect.bottom);
		}
	}
}


LRESULT CALLBACK WinProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(hwnd, &ps);
			
			RECT rect = { 0 };
			GetClientRect(g_hwnd, &rect);
			int w = rect.right;
			int h = rect.bottom;

			// Using double buffer for HDC
			CDCBitmapBuffer dcbb(dc, w, h);
			HDC memDC = dcbb.getBufferDC();
			drawBackground(memDC, w, h);
			drawScreen(memDC, w, h, rect);

			const bool bHorzReverse = g_uFlags & WZFLAG_HORIZONTAL_REVERSE ? true : false;
			const bool bVertReverse = g_uFlags & WZFLAG_VERTICAL_REVERSE ? true : false;
			StretchBlt(dc, 0, 0, w, h, memDC,
				bHorzReverse ? w - 1 : 0,
				bVertReverse ? h - 1 : 0,
				bHorzReverse ? -w : w,
				bVertReverse ? -h : h, SRCCOPY);

			dcbb.Release();
			EndPaint(hwnd, &ps);
			return 0;
		}
	case WM_CREATE:
		initFont(g_hFont, hwnd, 14);
		return g_hFont ? 0 : 1;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}




void InitWinClass(WNDCLASS& wndcls)
{
	wndcls.cbClsExtra = 0;
	wndcls.cbWndExtra = 0;
	wndcls.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wndcls.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndcls.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndcls.hInstance = g_hInst;
	wndcls.lpfnWndProc = WinProc;
	wndcls.lpszClassName = CLSNAME;
	wndcls.lpszMenuName = NULL;
	wndcls.style = CS_VREDRAW | CS_HREDRAW;
}
