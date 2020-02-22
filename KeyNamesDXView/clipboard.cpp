#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <tchar.h>
#include <shlobj.h>
#include "clipboard.h"

BOOL SetClipTextW(LPCWSTR unitext)
{
	size_t len = wcslen(unitext);
	BOOL   ret = FALSE;

	if (OpenClipboard(NULL)) {
		HGLOBAL hMem;

		EmptyClipboard();

		hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * 2 );
		if (hMem) {
			LPWSTR pBuf = (LPWSTR)GlobalLock(hMem);
			if (pBuf) {
				wcscpy(pBuf, unitext);
				GlobalUnlock(hMem);
				SetClipboardData(CF_UNICODETEXT, hMem);
				ret = TRUE;
			}
		}

		CloseClipboard();
	}

	return ret;
}
