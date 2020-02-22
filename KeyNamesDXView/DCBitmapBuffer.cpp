#include "DCBitmapBuffer.h"

#include <atlstr.h>

#define call_func_then_null(funcname, v)	if ((v) != NULL) { funcname(v); (v)=NULL; }

CDCBitmapBuffer::CDCBitmapBuffer(HDC dc, int w, int h)
{
	m_hdc = CreateCompatibleDC(dc);
	m_width = w;
	m_height = h;
	m_hBitmap = CreateCompatibleBitmap(dc, w, h);
	m_oldbitmap = (HBITMAP)SelectObject(m_hdc, m_hBitmap);
}

CDCBitmapBuffer::~CDCBitmapBuffer()
{
	this->Release();
}

void CDCBitmapBuffer::Release()
{
	if (m_oldbitmap && m_hdc) {
		SelectObject(m_hdc, m_oldbitmap);
	}
	m_oldbitmap = NULL;

	call_func_then_null(DeleteObject, m_hBitmap);
	call_func_then_null(DeleteDC, m_hdc);
}
