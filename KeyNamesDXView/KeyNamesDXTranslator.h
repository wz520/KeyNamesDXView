#pragma once

#include <windows.h>
#include <tchar.h>
#include <atlstr.h>
#include <map>

class CKeyNamesDXTranslator {
public:
	explicit CKeyNamesDXTranslator();

	enum {
		INVALID_KEYCODE = UINT_MAX
	};

	enum WORK_MODE {
		WORK_MODE_KEYNAME,
		WORK_MODE_KEYCODE,
		WORK_MODE_KEYCODE_10
	};

	// translate keycode specified by @code to keyname
	// return an empty string(LPCTSTR) if this keycode is undetermined.
	LPCTSTR translate(UINT code);

	// dwKeyCount [out]: returns the count of pressed key.
	// Can be NULL if not needed.
	//
	// uLastestPressedKeyCode [out]: returns the key code of the latest pressed key in @buffer.
	// If no key is pressed(dwKeyCount==0), it will be set to CKeyNamesDXTranslator::INVALID_KEYOCDE, 
	CString translate_all(const BYTE* buffer, DWORD* pdwKeyCount=NULL, UINT* uLastestPressedKeyCode=NULL);

	WORK_MODE m_workmode;

private:
	TCHAR s_scancode_buffer[16];
	void init_map();

protected:
	static std::map<UINT, LPCTSTR> s_map;
};
