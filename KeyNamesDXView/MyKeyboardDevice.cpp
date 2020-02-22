#include "MyKeyboardDevice.h"

#define delete_then_null(v)			((delete (v)), (v)=NULL)
#define release_then_null(v)		if ((v) != NULL) { (v)->Release(); (v)=NULL; }
#define is_empty_LPTSTR(v)			( (v)[0] == _T('\0') )
#define set_LPTSTR_to_empty(v)		( (v)[0] = _T('\0') )

CMyKeyboardDevice::CMyKeyboardDevice(HWND hWnd, LPDIRECTINPUT8 pDirectInput) : m_lasterror(), m_isLastReadingFailed(false) {
	HRESULT hr;
	m_hWnd = hWnd;
	ZeroMemory(m_buffer, sizeof(m_buffer));
	hr = pDirectInput->CreateDevice(GUID_SysKeyboard, &m_kbd, NULL);
	if (!setError(hr, _T("CreateDevice"))) {
		hr = m_kbd->SetDataFormat(&c_dfDIKeyboard);
		if (!setError(hr, _T("SetDataFormat"))) {
			hr = m_kbd->SetCooperativeLevel(hWnd, DISCL_FOREGROUND | DISCL_NONEXCLUSIVE);
			setError(hr, _T("SetCooperativeLevel"));
		}
	}
}

bool CMyKeyboardDevice::dev_read() {
	HRESULT hr;

	m_kbd->Poll();
	m_kbd->Acquire();

	if (SUCCEEDED(hr = m_kbd->GetDeviceState(sizeof(m_buffer), m_buffer)))
		return true;
	if (hr != DIERR_INPUTLOST && hr != DIERR_NOTACQUIRED) return false;
	if (FAILED(m_kbd->Acquire())) return false;

	return false;
}

bool CMyKeyboardDevice::read() {
	Sleep(7);
	if (dev_read()) {
		m_isLastReadingFailed = false;
	}
	else {
		m_isLastReadingFailed = true;
	}

	return !m_isLastReadingFailed;
}

CMyKeyboardDevice::~CMyKeyboardDevice() {
	release_then_null(m_kbd);
}



//// private:

// return true if hr is failed
bool CMyKeyboardDevice::setError(HRESULT hr, LPCTSTR func_name) {
	if (FAILED(hr)) {
		m_lasterror.set(hr, func_name);
		return true;
	}
	else
		return false;
}
