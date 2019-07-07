#include "StdAfx.h"
#include "Bitmap.h"

///////////////////////////////////////////////////////////////////////
LPRECT C32BitImageProcessor::btnpos = 0;
UINT ID_BK;

#define ICON_WIDTH 100

C32BitImageProcessor::C32BitImageProcessor(BOOL bEnableWeighting) : m_bWeightingEnabled(bEnableWeighting)
{
	m_bBk = FALSE;
	bk = new CEnBitmap;
	bk->LoadBitmap(ID_BK);
	bkrgbx = bk->GetDIBits32();
	BITMAP BM;
	bk->GetBitmap(&BM);
	bk->bksize = CSize(BM.bmWidth, BM.bmHeight);
}

void C32BitImageProcessor::SetBkColor(int R, int G, int B)
{
	m_bBk = TRUE;
	m_nRed = R;
	m_nGreen = G;
	m_nBlue = B;
}

C32BitImageProcessor::~C32BitImageProcessor()
{
}

CSize C32BitImageProcessor::CalcDestSize(CSize sizeSrc)
{
	return sizeSrc; // default
}

BOOL C32BitImageProcessor::IsBkColor(RGBX*p)
{
	if (!m_bBk)
		return FALSE;
	if (p->btRed == m_nRed && p->btGreen == m_nGreen && p->btBlue == m_nBlue)
		return TRUE;
	return FALSE;
}

BOOL C32BitImageProcessor::ProcessPixels(RGBX* pSrcPixels, CSize, RGBX* pDestPixels, CSize sizeDest)
{
	CopyMemory(pDestPixels, pSrcPixels, sizeDest.cx * 4 * sizeDest.cy); // default
	return TRUE;
}

// C32BitImageProcessor::CalcWeightedColor(...) is inlined in EnBitmap.h

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CEnBitmap::CEnBitmap(COLORREF crBkgnd) : m_crBkgnd(crBkgnd)
{

}

CEnBitmap::~CEnBitmap()
{

}

BOOL CEnBitmap::GrayImage()
{
	CImageGrayer gr;
	return ProcessImage(&gr);
}


BOOL CEnBitmap::ProcessImage(C32BitImageProcessor* pProcessor)
{
	C32BIPArray aProcessors;

	aProcessors.Add(pProcessor);

	return ProcessImage(aProcessors);
}

BOOL CEnBitmap::ProcessImage(C32BIPArray& aProcessors)
{
	ASSERT(GetSafeHandle());

	if (!GetSafeHandle())
		return FALSE;

	if (!aProcessors.GetSize())
		return TRUE;

	int nProcessor, nCount = (int)aProcessors.GetSize();

	// retrieve src and final dest sizes
	BITMAP BM;

	if (!GetBitmap(&BM))
		return FALSE;

	CSize sizeSrc(BM.bmWidth, BM.bmHeight);
	CSize sizeDest(sizeSrc), sizeMax(sizeSrc);

	for (nProcessor = 0; nProcessor < nCount; nProcessor++)
	{
		sizeDest = aProcessors[nProcessor]->CalcDestSize(sizeDest);
		sizeMax = CSize(max(sizeMax.cx, sizeDest.cx), max(sizeMax.cy, sizeDest.cy));
	}

	// prepare src and dest bits
	RGBX* pSrcPixels = GetDIBits32();

	if (!pSrcPixels)
		return FALSE;

	RGBX* pDestPixels = new RGBX[sizeMax.cx * sizeMax.cy];

	if (!pDestPixels)
		return FALSE;

	Fill(pDestPixels, sizeMax, m_crBkgnd);

	BOOL bRes = TRUE;
	sizeDest = sizeSrc;

	// do the processing
	for (nProcessor = 0; bRes && nProcessor < nCount; nProcessor++)
	{
		// if its the second processor or later then we need to copy
		// the previous dest bits back into source.
		// we also need to check that sizeSrc is big enough
		if (nProcessor > 0)
		{
			if (sizeSrc.cx < sizeDest.cx || sizeSrc.cy < sizeDest.cy)
			{
				delete[] pSrcPixels;
				pSrcPixels = new RGBX[sizeDest.cx * sizeDest.cy];
			}

			CopyMemory(pSrcPixels, pDestPixels, sizeDest.cx * 4 * sizeDest.cy); // default
			Fill(pDestPixels, sizeDest, m_crBkgnd);
		}

		sizeSrc = sizeDest;
		sizeDest = aProcessors[nProcessor]->CalcDestSize(sizeSrc);

		bRes = aProcessors[nProcessor]->ProcessPixels(pSrcPixels, sizeSrc, pDestPixels, sizeDest);
	}

	// update the bitmap
	if (bRes)
	{
		// set the bits
		HDC hdc = GetDC(NULL);
		HBITMAP hbmSrc = ::CreateCompatibleBitmap(hdc, sizeDest.cx, sizeDest.cy);

		if (hbmSrc)
		{
			BITMAPINFO bi;

			if (PrepareBitmapInfo32(bi, hbmSrc))
			{
				if (SetDIBits(hdc, hbmSrc, 0, sizeDest.cy, pDestPixels, &bi, DIB_RGB_COLORS))
				{
					// delete the bitmap and attach new
					DeleteObject();
					bRes = Attach(hbmSrc);
				}
			}

			::ReleaseDC(NULL, hdc);

			if (!bRes)
				::DeleteObject(hbmSrc);
		}
	}

	delete[] pSrcPixels;
	delete[] pDestPixels;

	return bRes;
}

RGBX* CEnBitmap::GetDIBits32()
{
	BITMAPINFO bi;

	int nHeight = PrepareBitmapInfo32(bi);

	if (!nHeight)
		return FALSE;

	BYTE* pBits = (BYTE*)new BYTE[bi.bmiHeader.biSizeImage];
	HDC hdc = GetDC(NULL);

	if (!GetDIBits(hdc, (HBITMAP)GetSafeHandle(), 0, nHeight, pBits, &bi, DIB_RGB_COLORS))
	{
		delete pBits;
		pBits = NULL;
	}

	::ReleaseDC(NULL, hdc);

	return (RGBX*)pBits;
}

BOOL CEnBitmap::PrepareBitmapInfo32(BITMAPINFO& bi, HBITMAP hBitmap)
{
	if (!hBitmap)
		hBitmap = (HBITMAP)GetSafeHandle();

	BITMAP BM;

	if (!::GetObject(hBitmap, sizeof(BM), &BM))
		return FALSE;

	bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
	bi.bmiHeader.biWidth = BM.bmWidth;
	bi.bmiHeader.biHeight = -BM.bmHeight;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 32; // 32 bit
	bi.bmiHeader.biCompression = BI_RGB; // 32 bit
	bi.bmiHeader.biSizeImage = BM.bmWidth * 4 * BM.bmHeight; // 32 bit
	bi.bmiHeader.biClrUsed = 0;
	bi.bmiHeader.biClrImportant = 0;

	return BM.bmHeight;
}

BOOL CEnBitmap::Fill(RGBX* pPixels, CSize size, COLORREF color)
{
	if (!pPixels)
		return FALSE;

	if (color == -1 || color == RGB(255, 255, 255))
		FillMemory(pPixels, size.cx * 4 * size.cy, 255); // white

	else if (color == 0)
		FillMemory(pPixels, size.cx * 4 * size.cy, 0); // black

	else
	{
		// fill the first line with the color
		RGBX* pLine = &pPixels[0];
		int nSize = 1;

		pLine[0] = RGBX(color);

		while (1)
		{
			if (nSize > size.cx)
				break;

			// else
			int nAmount = min(size.cx - nSize, nSize) * 4;

			CopyMemory(&pLine[nSize], pLine, nAmount);
			nSize *= 2;
		}

		// use that line to fill the rest of the block
		int nRow = 1;

		while (1)
		{
			if (nRow > size.cy)
				break;

			// else
			int nAmount = min(size.cy - nRow, nRow) * size.cx * 4;

			CopyMemory(&pPixels[nRow * size.cx], pPixels, nAmount);
			nRow *= 2;
		}
	}

	return TRUE;
}
BOOL CEnBitmap::MakeDisabled(COLORREF bk)
{
	int R = GetRValue(bk);
	int G = GetGValue(bk);
	int B = GetBValue(bk);

	CImageHigh high(0.08f);
	high.SetBkColor(R, G, B);
	CImageGrayer gray;
	gray.SetBkColor(R, G, B);

	C32BIPArray aProcessors;
	aProcessors.Add(&gray);
	for (int i = 0; i<0/*3*/; i++)
		aProcessors.Add(&high);

	return ProcessImage(aProcessors);
}

BOOL CEnBitmap::MakeNormal(COLORREF bk)
{
	int R = GetRValue(bk);
	int G = GetGValue(bk);
	int B = GetBValue(bk);

	C32BIPArray aProcessors;
	CImageNormal normal;
	normal.SetBkColor(R, G, B);
	aProcessors.Add(&normal);
	return ProcessImage(aProcessors);

	return TRUE;
}

BOOL CEnBitmap::MakeNotActive(COLORREF bk)
{
	int R = GetRValue(bk);
	int G = GetGValue(bk);
	int B = GetBValue(bk);

	C32BIPArray aProcessors;

	CImageHigh high2(0.04f);
	high2.SetBkColor(R, G, B);
	aProcessors.Add(&high2);
	
	return ProcessImage(aProcessors);
}


BOOL CImageGrayer::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize)
{
	int x, y;
	for (int nX = 0; nX < sizeSrc.cx; nX++)
	{
		for (int nY = 0; nY < sizeSrc.cy; nY++)
		{
			RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];
			if (IsBkColor(pRGBSrc))
			{
				//----------
				x = btnpos[nX / ICON_WIDTH].left + nX % ICON_WIDTH;
				y = btnpos[nX / ICON_WIDTH].top + nY;
				RGBX* bkSrc = &bkrgbx[y*bk->bksize.cx + x];
				*pRGBDest = *bkSrc;
				//----------
				//*pRGBDest = *pRGBSrc;
			}
			else
			{

				*pRGBDest = pRGBSrc->Gray();
			}
		}
	}

	return TRUE;
}

BOOL CImageNormal::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize)
{
	int x, y;
	for (int nX = 0; nX < sizeSrc.cx; nX++)
	{

		for (int nY = 0; nY < sizeSrc.cy; nY++)
		{
			RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];
			if (IsBkColor(pRGBSrc))
			{
				//----------
				x = btnpos[nX / ICON_WIDTH].left + nX % ICON_WIDTH;
				y = btnpos[nX / ICON_WIDTH].top + nY;
				RGBX* bkSrc = &bkrgbx[y*bk->bksize.cx + x];
				*pRGBDest = *bkSrc;
				//----------
				//*pRGBDest = *pRGBSrc;
			}
			else
			{

				*pRGBDest = *pRGBSrc;
			}
		}
	}

	return TRUE;
}

CImageHigh::CImageHigh(float nL)
{
	m_fLumDecr = nL;
}

BOOL CImageHigh::ProcessPixels(RGBX* pSrcPixels, CSize sizeSrc, RGBX* pDestPixels, CSize)
{
	CColor cnv;
	int x, y;
	for (int nX = 0; nX < sizeSrc.cx; nX++)
	{
		for (int nY = 0; nY < sizeSrc.cy; nY++)
		{
			RGBX* pRGBSrc = &pSrcPixels[nY * sizeSrc.cx + nX];
			RGBX* pRGBDest = &pDestPixels[nY * sizeSrc.cx + nX];
			if (IsBkColor(pRGBSrc))
			{
				//----------
				x = btnpos[nX / ICON_WIDTH].left + nX % ICON_WIDTH;
				y = btnpos[nX / ICON_WIDTH].top + nY;
				RGBX* bkSrc = &bkrgbx[y*bk->bksize.cx + x];
				*pRGBDest = *bkSrc;
				//----------
				//*pRGBDest = *pRGBSrc;
			}
			else
			{
				cnv.SetRGB(pRGBSrc->btRed, pRGBSrc->btGreen, pRGBSrc->btBlue);
				float L = cnv.GetLuminance();

				if (m_fLumDecr>0 && L<1.0)
				{
					L = min(1, L + m_fLumDecr);
					cnv.SetLuminance(L);
				}
				else if (m_fLumDecr<0 && L>0)
				{
					L = max(0, L + m_fLumDecr);
					cnv.SetLuminance(L);
				}

				pRGBDest->btRed = (BYTE)cnv.GetRed();
				pRGBDest->btBlue = (BYTE)cnv.GetBlue();
				pRGBDest->btGreen = (BYTE)cnv.GetGreen();
			}
		}
	}

	return TRUE;
}

void	CToolBar24::SetFullColorImage(UINT ID, COLORREF rgbBack, int* num)
{

	//----------
	if (num != NULL)
	{
		int i, j, k, btn = num[0];
		C32BitImageProcessor::btnpos = new RECT[btn];
		memset(C32BitImageProcessor::btnpos, 0, btn * sizeof(RECT));
		k = 3;//6+3.5=9.5
		C32BitImageProcessor::btnpos[0].top = 3.5;// 7/2=3.5

												
		for (j = i = 0; i<btn; i++)
		{
			if (num[i + 1] == 0)
			{
				C32BitImageProcessor::btnpos[j].left = k;
				k += 31;//24+7=31
				C32BitImageProcessor::btnpos[j].top = 3.5;
				//btnpos[j].top=btnpos[j-1].top+High;
				j++;
			}
			else
			{
				k += 6;
			}
		}

	}
	else
	{
		m_bmToolbar.DeleteObject();
		m_imgToolbar.DeleteImageList();
		m_bmToolbarDis.DeleteObject();
		m_imgToolbarDis.DeleteImageList();
		m_bmToolbarNA.DeleteObject();
		m_imgToolbarNA.DeleteImageList();
	}
	//----------

	m_bmToolbar.LoadBitmap(ID);
	m_bmToolbar.MakeNormal(RGB(0, 0, 0));
	m_imgToolbar.Create(ICON_WIDTH, 30, ILC_COLOR24 | ILC_MASK, 1, 1);
	m_imgToolbar.Add(&m_bmToolbar, rgbBack);
	GetToolBarCtrl().SetImageList(&m_imgToolbar);//A

	m_bmToolbarDis.LoadBitmap(ID);
	m_bmToolbarDis.MakeDisabled(RGB(0, 0, 0));
	m_imgToolbarDis.Create(ICON_WIDTH, 30, ILC_COLOR24 | ILC_MASK, 1, 1);
	m_imgToolbarDis.Add(&m_bmToolbarDis, rgbBack);
	GetToolBarCtrl().SetDisabledImageList(&m_imgToolbarDis);

	m_bmToolbarNA.LoadBitmap(ID);
	m_bmToolbarNA.MakeNotActive(RGB(0, 0, 0));
	m_imgToolbarNA.Create(ICON_WIDTH, 30, ILC_COLOR24 | ILC_MASK, 1, 1);
	m_imgToolbarNA.Add(&m_bmToolbarNA, rgbBack);
	GetToolBarCtrl().SetHotImageList(&m_imgToolbarNA);
}

BEGIN_MESSAGE_MAP(CToolBar24, CToolBar)
	//{{AFX_MSG_MAP(CBmpToolBar)
	ON_WM_ERASEBKGND()
//	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CToolBar24::UpdateBk(UINT ID, COLORREF rgbBack, UINT newbk)
{
	ID_BK = newbk;
	CBitmap tmp;
	tmp.LoadBitmap(ID_BK);
	brush.DeleteObject();
	brush.CreatePatternBrush(&tmp);
	SetFullColorImage(ID, rgbBack, NULL);
	Invalidate();
}

BOOL CToolBar24::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default

	RECT rect;
	GetWindowRect(&rect);
	ScreenToClient(&rect);
	pDC->FillRect(&rect, &brush);

	return TRUE;
	return CToolBar::OnEraseBkgnd(pDC);
}
/*
void CToolBar24::OnPaint()
{
	return CToolBar::OnPaint();
}
*/
CToolBar24::CToolBar24()
{
	ID_BK = IDB_BITMAP;
	CBitmap tmp;
	tmp.LoadBitmap(ID_BK);
	brush.CreatePatternBrush(&tmp);
}

CColor::CColor(COLORREF cr)
	: m_bIsRGB(true), m_bIsHLS(false), m_colorref(cr)
{}

CColor::operator COLORREF() const
{
	const_cast<CColor*>(this)->ToRGB();
	return m_colorref;
}

// RGB

void CColor::SetRed(int red)
{
	ASSERT(0 <= red && red <= 255);
	ToRGB();
	m_color[c_red] = static_cast<unsigned char>(red);
	m_bIsHLS = false;
}

void CColor::SetGreen(int green)
{
	ASSERT(0 <= green && green <= 255);
	ToRGB();
	m_color[c_green] = static_cast<unsigned char>(green);
	m_bIsHLS = false;
}

void CColor::SetBlue(int blue)
{
	ASSERT(0 <= blue && blue <= 255);
	ToRGB();
	m_color[c_blue] = static_cast<unsigned char>(blue);
	m_bIsHLS = false;
}

void CColor::SetRGB(int red, int blue, int green)
{
	ASSERT(0 <= red && red <= 255);
	ASSERT(0 <= green && green <= 255);
	ASSERT(0 <= blue && blue <= 255);

	m_color[c_red] = static_cast<unsigned char>(red);
	m_color[c_green] = static_cast<unsigned char>(green);
	m_color[c_blue] = static_cast<unsigned char>(blue);
	m_bIsHLS = false;
	m_bIsRGB = true;
}

int CColor::GetRed() const
{
	const_cast<CColor*>(this)->ToRGB();
	return m_color[c_red];
}

int CColor::GetGreen() const
{
	const_cast<CColor*>(this)->ToRGB();
	return m_color[c_green];
}

int CColor::GetBlue() const
{
	const_cast<CColor*>(this)->ToRGB();
	return m_color[c_blue];
}

// HSL

void CColor::SetHue(float hue)
{
	ASSERT(hue >= 0.0 && hue <= 360.0);

	ToHLS();
	m_hue = hue;
	m_bIsRGB = false;
}

void CColor::SetSaturation(float saturation)
{
	ASSERT(saturation >= 0.0 && saturation <= 1.0); // 0.0 ist undefiniert

	ToHLS();
	m_saturation = saturation;
	m_bIsRGB = false;
}

void CColor::SetLuminance(float luminance)
{
	ASSERT(luminance >= 0.0 && luminance <= 1.0);

	ToHLS();
	m_luminance = luminance;
	m_bIsRGB = false;
}

void CColor::SetHLS(float hue, float luminance, float saturation)
{
	ASSERT(hue >= 0.0 && hue <= 360.0);
	ASSERT(luminance >= 0.0 && luminance <= 1.0);
	ASSERT(saturation >= 0.0 && saturation <= 1.0); // 0.0 ist undefiniert

	m_hue = hue;
	m_luminance = luminance;
	m_saturation = saturation;
	m_bIsRGB = false;
	m_bIsHLS = true;
}

float CColor::GetHue() const
{
	const_cast<CColor*>(this)->ToHLS();
	return m_hue;
}

float CColor::GetSaturation() const
{
	const_cast<CColor*>(this)->ToHLS();
	return m_saturation;
}

float CColor::GetLuminance() const
{
	const_cast<CColor*>(this)->ToHLS();
	return m_luminance;
}

// Konvertierung

void CColor::ToHLS()
{
	if (!m_bIsHLS)
	{
		// Konvertierung
		unsigned char minval = min(m_color[c_red], min(m_color[c_green], m_color[c_blue]));
		unsigned char maxval = max(m_color[c_red], max(m_color[c_green], m_color[c_blue]));
		float mdiff = float(maxval) - float(minval);
		float msum = float(maxval) + float(minval);

		m_luminance = msum / 510.0f;

		if (maxval == minval)
		{
			m_saturation = 0.0f;
			m_hue = 0.0f;
		}
		else
		{
			float rnorm = (maxval - m_color[c_red]) / mdiff;
			float gnorm = (maxval - m_color[c_green]) / mdiff;
			float bnorm = (maxval - m_color[c_blue]) / mdiff;

			m_saturation = (m_luminance <= 0.5f) ? (mdiff / msum) : (mdiff / (510.0f - msum));

			if (m_color[c_red] == maxval) m_hue = 60.0f * (6.0f + bnorm - gnorm);
			if (m_color[c_green] == maxval) m_hue = 60.0f * (2.0f + rnorm - bnorm);
			if (m_color[c_blue] == maxval) m_hue = 60.0f * (4.0f + gnorm - rnorm);
			if (m_hue > 360.0f) m_hue = m_hue - 360.0f;
		}
		m_bIsHLS = true;
	}
}

void CColor::ToRGB()
{
	if (!m_bIsRGB)
	{
		if (m_saturation == 0.0) // Grauton, einfacher Fall
		{
			m_color[c_red] = m_color[c_green] = m_color[c_blue] = unsigned char(m_luminance * 255.0);
		}
		else
		{
			float rm1, rm2;

			if (m_luminance <= 0.5f) rm2 = m_luminance + m_luminance * m_saturation;
			else                     rm2 = m_luminance + m_saturation - m_luminance * m_saturation;
			rm1 = 2.0f * m_luminance - rm2;
			m_color[c_red] = ToRGB1(rm1, rm2, m_hue + 120.0f);
			m_color[c_green] = ToRGB1(rm1, rm2, m_hue);
			m_color[c_blue] = ToRGB1(rm1, rm2, m_hue - 120.0f);
		}
		m_bIsRGB = true;
	}
}

unsigned char CColor::ToRGB1(float rm1, float rm2, float rh)
{
	if (rh > 360.0f) rh -= 360.0f;
	else if (rh <   0.0f) rh += 360.0f;

	if (rh <  60.0f) rm1 = rm1 + (rm2 - rm1) * rh / 60.0f;
	else if (rh < 180.0f) rm1 = rm2;
	else if (rh < 240.0f) rm1 = rm1 + (rm2 - rm1) * (240.0f - rh) / 60.0f;

	return static_cast<unsigned char>(rm1 * 255);
}
