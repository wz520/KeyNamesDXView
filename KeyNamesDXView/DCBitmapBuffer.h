#ifndef __DCBITMAPBUFFER_HEl4ohRHM0MaYCxZb5
#define __DCBITMAPBUFFER_HEl4ohRHM0MaYCxZb5

#include <Windows.h>

class CDCBitmapBuffer
{
public:
	explicit CDCBitmapBuffer(HDC dc, int w, int h);
	~CDCBitmapBuffer();

	void Release();

	inline HDC getBufferDC() { return m_hdc; }
	inline int getWidth() { return m_width; }
	inline int getHeight() { return m_height; }

private:
	HDC m_hdc;
	int m_width;
	int m_height;
	HBITMAP m_oldbitmap;

	HBITMAP m_hBitmap;
};


#endif //__DCBITMAPBUFFER_HEl4ohRHM0MaYCxZb5
