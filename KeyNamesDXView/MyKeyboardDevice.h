#pragma once

#define DIRECTINPUT_VERSION			0x0800

#include <windows.h>
#include <tchar.h>
#include <dinput.h>

class CMyKeyboardDevice {
public:
	struct lasterror_t {
		HRESULT hr;
		LPCTSTR func_name;
		inline lasterror_t() {
			clear();
		}

		inline void set(HRESULT hr, LPCTSTR func_name) {
			this->hr = hr;
			this->func_name = func_name;
		}

		inline void clear() {
			hr = 0;
			func_name = NULL;
		}

		inline bool isFailed() const {
			return hr != 0;
		}
	};

	explicit CMyKeyboardDevice(HWND hWnd, LPDIRECTINPUT8 pDirectInput);

	bool dev_read();

	bool read();

	inline lasterror_t getLastError() const {
		return m_lasterror;
	}

	inline BYTE* getState() {
		return m_buffer;
	}

	inline bool isLastReadingFailed() const {
		return m_isLastReadingFailed;
	}

	inline bool isKeyDown(BYTE bytScanCode) {
		return m_buffer[bytScanCode] & 0x80 ? true : false;
	}

	~CMyKeyboardDevice();

private:
	// return true if hr has failed
	bool setError(HRESULT hr, LPCTSTR func_name);

	LPDIRECTINPUTDEVICE8 m_kbd;
	lasterror_t m_lasterror;
	bool m_isLastReadingFailed;
	HWND m_hWnd;

	BYTE m_buffer[256];
};
