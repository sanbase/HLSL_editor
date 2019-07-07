// EnBitmap.h: interface for the CEnBitmap class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ENBITMAP_H__1FDE0A4E_8AB4_11D6_95AD_EFA89432A428__INCLUDED_)
#define AFX_ENBITMAP_H__1FDE0A4E_8AB4_11D6_95AD_EFA89432A428__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "StdAfx.h"

////////////////////////////////////////////////////////////////////////////////////
// helper struct. equates to COLORREF

#pragma pack(push)
#pragma pack(1)


#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CBitmapToolBarResrch_enApp:
// See BitmapToolBarResrch_en.cpp for the implementation of this class
//

class CBitmapToolBarResrch_enApp : public CWinApp
{
public:
	CBitmapToolBarResrch_enApp();

	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBitmapToolBarResrch_enApp)
public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

	// Implementation
	//{{AFX_MSG(CBitmapToolBarResrch_enApp)
	afx_msg void OnAppAbout();
	// NOTE - the ClassWizard will add and remove member functions here.
	//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};



struct RGBX
{
public:
	RGBX() { btRed = btBlue = btGreen = btUnused = 0; }
	RGBX(BYTE red, BYTE green, BYTE blue) { btRed = red; btBlue = blue; btGreen = green; btUnused = 0; }
	RGBX(COLORREF color) { btRed = GetRValue(color); btBlue = GetBValue(color); btGreen = GetGValue(color); btUnused = 0; }

	BYTE btBlue;
	BYTE btGreen;
	BYTE btRed;

protected:
	BYTE btUnused;

public:
	inline BOOL Equals(const RGBX& rgb) { return (btRed == rgb.btRed && btGreen == rgb.btGreen && btBlue == rgb.btBlue); }
	inline BOOL IsGray() const { return (btRed == btGreen && btGreen == btBlue); }

	RGBX Gray()
	{
		BYTE btGray = (BYTE)(((int)btBlue + (int)btGreen * 6 + (int)btRed * 3) / 10);//Í¼Ïñ±ä»ÒËã·¨
		return RGBX(btGray, btGray, btGray);
	}

};

#pragma pack(pop)

/////////////////////////////////////////////////////////////////////////////////////
// base class for image processing

class CEnBitmap;
class C32BitImageProcessor
{
public:
	C32BitImageProcessor(BOOL bEnableWeighting = FALSE);
	virtual ~C32BitImageProcessor();

	void SetBkColor(int R, int G, int B);
	virtual CSize CalcDestSize(CSize sizeSrc);
	virtual BOOL ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest);

	inline RGBX CalcWeightedColor(RGBX* pPixels, CSize size, double dX, double dY);

	//---------
	friend class CToolBar24;
protected:
	BOOL IsBkColor(RGBX*p);
	BOOL m_bWeightingEnabled;
	BOOL m_bBk;
	int  m_nRed;
	int  m_nGreen;
	int  m_nBlue;
	//-----------
	static LPRECT btnpos;
	CEnBitmap *bk;
	RGBX *bkrgbx;
	//-----------
};


typedef CArray<C32BitImageProcessor*, C32BitImageProcessor*> C32BIPArray;

//////////////////////////////////////////////////////////////////////////////////////////

class CEnBitmap : public CBitmap
{
public:
	CEnBitmap(COLORREF crBkgnd = RGB(255, 255, 255));
	virtual ~CEnBitmap();

	BOOL LoadImage(LPCTSTR szImagePath, COLORREF crBack = -1);
	BOOL LoadImage(UINT uIDRes, LPCTSTR szResourceType, HMODULE hInst = NULL, COLORREF crBack = -1);

	BOOL GrayImage();
	BOOL MakeDisabled(COLORREF bk);
	BOOL MakeNotActive(COLORREF bk);
	BOOL MakeNormal(COLORREF bk);

	BOOL ProcessImage(C32BitImageProcessor* pProcessor);
	BOOL ProcessImage(C32BIPArray& aProcessors);

	//----------
	RGBX* GetDIBits32();
	CSize bksize;
	//----------

	/*	// helpers
	static HBITMAP LoadImageFile(LPCTSTR szImagePath, COLORREF crBack = -1);
	static HBITMAP LoadImageResource(UINT uIDRes, LPCTSTR szResourceType, HMODULE hInst = NULL, COLORREF crBack = 0);
	static BOOL GetResource(LPCTSTR lpName, LPCTSTR lpType, HMODULE hInst, void* pResource, int& nBufSize);
	static IPicture* LoadFromBuffer(BYTE* pBuff, int nSize);
	*/

protected:
	COLORREF m_crBkgnd;

protected:
	//	RGBX* GetDIBits32();
	BOOL PrepareBitmapInfo32(BITMAPINFO& bi, HBITMAP hBitmap = NULL);

	static HBITMAP ExtractBitmap(IPicture* pPicture, COLORREF crBack);
	static int GetFileType(LPCTSTR szImagePath);
	static BOOL Fill(RGBX* pPixels, CSize size, COLORREF color);
};

// inline weighting function
inline RGBX C32BitImageProcessor::CalcWeightedColor(RGBX* pPixels, CSize size, double dX, double dY)
{
	ASSERT(m_bWeightingEnabled);

	// interpolate between the current pixel and its pixel to the right and down
	int nX = (int)dX;
	int nY = (int)dY;

	if (dX < 0 || dY < 0)
		return pPixels[max(0, nY) * size.cx + max(0, nX)]; // closest

	RGBX* pRGB = &pPixels[nY * size.cx + nX]; // current

	double dXFraction = dX - nX;
	double dX1MinusFraction = 1 - dXFraction;

	double dYFraction = dY - nY;
	double dY1MinusFraction = 1 - dYFraction;

	RGBX* pRGBXP = &pPixels[nY * size.cx + min(nX + 1, size.cx - 1)]; // x + 1
	RGBX* pRGBYP = &pPixels[min(nY + 1, size.cy - 1) * size.cx + nX]; // y + 1

	int nRed = (int)((dX1MinusFraction * pRGB->btRed +
		dXFraction * pRGBXP->btRed +
		dY1MinusFraction * pRGB->btRed +
		dYFraction * pRGBYP->btRed) / 2);

	int nGreen = (int)((dX1MinusFraction * pRGB->btGreen +
		dXFraction * pRGBXP->btGreen +
		dY1MinusFraction * pRGB->btGreen +
		dYFraction * pRGBYP->btGreen) / 2);

	int nBlue = (int)((dX1MinusFraction * pRGB->btBlue +
		dXFraction * pRGBXP->btBlue +
		dY1MinusFraction * pRGB->btBlue +
		dYFraction * pRGBYP->btBlue) / 2);

	return RGBX((BYTE)nRed, (BYTE)nGreen, (BYTE)nBlue);
}

class CImageNormal : public C32BitImageProcessor
{
public:
	virtual BOOL ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest);
};

class CImageGrayer : public C32BitImageProcessor
{
public:
	virtual BOOL ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest);
};

class CImageHigh : public C32BitImageProcessor
{
public:
	CImageHigh(float fL = 0.1);
	virtual BOOL ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize sizeDest);
protected:
	float m_fLumDecr;
};

class CToolBar24 : public CToolBar
{
public:
	CToolBar24();
	void	SetFullColorImage(UINT ID, COLORREF rgbBack, int* num);
	void UpdateBk(UINT ID, COLORREF rgbBack, UINT newbk);
protected:
	CImageList	m_imgToolbar;
	CImageList	m_imgToolbarDis;
	CImageList	m_imgToolbarNA;
	CEnBitmap		m_bmToolbar;//-----------
	CEnBitmap	m_bmToolbarDis;
	CEnBitmap	m_bmToolbarNA;
	CBrush brush;
protected:
	//{{AFX_MSG(CBmpToolBar)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
//	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CColor
{
public:

	// COLORREF Konvertierung
	// ----------------------
	CColor(COLORREF cr = 0);
	operator COLORREF() const;

	// RGB - Routinen
	// --------------
	void SetRed(int red);     // 0..255  
	void SetGreen(int green); // 0..255
	void SetBlue(int blue);   // 0..255
	void SetRGB(int red, int blue, int green);

	int GetRed() const;   // 0..255
	int GetGreen() const; // 0..255
	int GetBlue() const;  // 0..255

						  // HLS - Routinen
						  // --------------
	void SetHue(float hue);               // 0.0 .. 360.0
	void SetLuminance(float luminance);   // 0.0 .. 1.0
	void SetSaturation(float saturation); // 0.0 .. 1.0
	void SetHLS(float hue, float luminance, float saturation);

	float GetHue() const;        // 0.0 .. 360.0
	float GetLuminance() const;  // 0.0 .. 1.0
	float GetSaturation() const; // 0.0 .. 1.0

private:

	// Konvertierung
	// -------------
	void ToRGB(); // logisch konstant, nicht physikalisch
	void ToHLS(); // logisch konstant, nicht physikalisch
	static unsigned char ToRGB1(float rm1, float rm2, float rh);

	// Daten
	// -----
	union // Byteweiser Zugriff auf die COLORREF Struktur
	{
		COLORREF      m_colorref;
		unsigned char m_color[4];
	};
	enum { c_red = 0, c_green = 1, c_blue = 2, c_null = 3 }; // enum Hack für colorbyte-Index

	float m_hue;         // 0.0 .. 360.0  // Winkel
	float m_saturation;  // 0.0 .. 1.0    // Prozent
	float m_luminance;   // 0.0 .. 1.0    // Prozent

						 // Flag für Lazy Evaluation
	bool m_bIsRGB;
	bool m_bIsHLS;
};


#endif // !defined(AFX_ENBITMAP_H__1FDE0A4E_8AB4_11D6_95AD_EFA89432A428__INCLUDED_)

