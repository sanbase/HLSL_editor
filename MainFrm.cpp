// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "TextEdit.h"
#include "MainFrm.h"
#include <string>

#include <dxgi1_3.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <afxrich.h>
#include <afxdlgs.h>

#include <fstream>
#include <iostream>
#include <vector>
#include <algorithm>    // std::binary_search, std::sort

using namespace std;
using namespace DirectX;
using Microsoft::WRL::ComPtr;

#pragma comment(lib, "D3dcompiler.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


/////////////////////////////////////////////////////////////////////////////
// CChildWnd

BEGIN_MESSAGE_MAP(CChildWnd, CWnd)
	//{{AFX_MSG_MAP(CChildWnd)
	ON_WM_LBUTTONDOWN()
	ON_WM_PAINT()
	ON_WM_WINDOWPOSCHANGING()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CChildWnd::CChildWnd()
{
}

CChildWnd::~CChildWnd()
{}

void CChildWnd::OnPaint()
{
	CPaintDC dc(this);
//	CFont font;
	CRect rcClient;
	TEXTMETRIC lpMetrics;

	dc.SetBkColor(RGB(32,32,32));
	dc.GetTextMetrics(&lpMetrics);

	dc.SetTextColor(RGB(255, 64, 64));
	dc.SetTextAlign(TA_TOP | TA_LEFT);
	GetClientRect(&rcClient); //rcClient.CenterPoint().x
	int windows_height = rcClient.bottom;
	rcClient.bottom = lpMetrics.tmHeight;

	int p = 0;
	CString line = m_message.Tokenize(_T("\n"), p);

	while (line != _T(""))
	{
		if (line.Find(_T("warning"))>=0)
		{
			dc.SetTextColor(RGB(200, 120, 64));
		}
		else
			dc.SetTextColor(RGB(255, 64, 64));

		dc.DrawText(line, rcClient, DT_LEFT);
		line = m_message.Tokenize(_T("\n"), p);
		rcClient.top = rcClient.bottom;
		rcClient.bottom += lpMetrics.tmHeight;
		if (rcClient.bottom > windows_height)
			break;
	}
}

void CChildWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	CPaintDC dc(this);
	TEXTMETRIC lpMetrics;
	dc.GetTextMetrics(&lpMetrics);
	int line = point.y / lpMetrics.tmHeight;
	if (line < m_err.size())
	{
		vec4 pos = m_err[line];
		m_parent->MarkErrorPos(pos);
	}
}

void CChildWnd::OnWindowPosChanging(WINDOWPOS FAR* lpwndpos)
{
	lpwndpos->flags |= SWP_NOCOPYBITS;
	CWnd::OnWindowPosChanging(lpwndpos);
}

// CMainFrame

static UINT indicators[] =
{
	ID_INDICATOR_PAGE,
//	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,

};

static UINT FindReplaceDialogMessage = ::RegisterWindowMessage(FINDMSGSTRING);

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_WM_SETFOCUS()
	ON_WM_DROPFILES()
	ON_WM_DESTROY()
	ON_WM_NCPAINT()
	ON_WM_NCACTIVATE()
	ON_WM_ACTIVATE()

//	ON_NOTIFY(NM_CLICK, 0, OnClick)

	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_FIND, OnFind)
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_SHADER_COMPILE, OnCompile)
	ON_COMMAND(ID_FILE_SAVEAS, OnFileSaveas)
	ON_COMMAND(ID_APP_CLOSE, OnAppClose)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)

	ON_REGISTERED_MESSAGE(FindReplaceDialogMessage, OnFindReplaceMessage)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_PAGE, &CMainFrame::OnUpdatePage)
	//}}AFX_MSG_MAP

END_MESSAGE_MAP()


// CMainFrame construction/destruction

CMainFrame::CMainFrame():
	m_InfoToolbar(RGB(32,32,32)),
	m_wndSplitterVert(2, SSP_VERT, 30, 1)
{
	m_strPathname = "";
}

CMainFrame::~CMainFrame()
{
//	if(pMyMenu!=nullptr)
//		delete pMyMenu;
}

//#pragma comment(lib, "Dwmapi.lib")

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CFrameWnd::PreCreateWindow(cs))
		return FALSE;

	int width = GetSystemMetrics(SM_CXFULLSCREEN);
	int height = GetSystemMetrics(SM_CYFULLSCREEN);
	cs.cx = (int)(width>1000 ? 1000 : width*0.9);
	cs.cy = (int)(height*0.9);
	cs.x = (width - cs.cx) / 2;
	cs.y = 50;

	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
	cs.lpszClass = AfxRegisterWndClass(0, 0, 0, LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME)));
	cs.hMenu = NULL;

	m_Width = cs.cx;
	m_Height = cs.cy;
	return TRUE;
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	BOOL m_enable = true;

	// Set extended style: 3D look - ie. sunken edge
	::SetWindowLongPtr(m_hWnd, GWL_STYLE, GetWindowLong(m_hWnd, GWL_STYLE) & ~WS_MINIMIZEBOX & ~WS_MAXIMIZEBOX);
	::SetWindowLongPtr(m_RichEdit.GetSafeHwnd(), GWL_EXSTYLE, WS_EX_CLIENTEDGE);

	RECT lpRectBorder;
	ZeroMemory(&lpRectBorder, sizeof lpRectBorder);
	lpRectBorder.left = 10;
	NegotiateBorderSpace(3, &lpRectBorder);


	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	int *num, i, j = m_wndToolBar.GetToolBarCtrl().GetButtonCount();
	num = new int[j + 1];
	memset(num, 0, (j + 1) * sizeof(int));
	num[0] = j;//button numbers,include the separators.
	for (i = 0; i<j; i++)
	{
		if (ID_SEPARATOR == m_wndToolBar.GetItemID(i))
			num[i + 1] = 1;
	}//get the button info.zero represent the 
	 //separators.the info is used to calc 
	 //the position of the buttons.

	m_wndToolBar.SetFullColorImage(IDR_MAINFRAME,
		RGB(32, 32, 32), num); //the first Parameter 
								//specify the resource id of button bitmap , 
								//second is the backgroud color of  the toolbar.

	// TODO: Delete these three lines if you don't want the toolbar to
	//  be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);

	//	m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC);

	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	//Set Charactistics of the Status Bar
	UINT nID, nStyle;
	int cxWidth;
	if (m_wndStatusBar.m_hWnd != nullptr)
	{
		m_wndStatusBar.GetPaneInfo(0, nID, nStyle, cxWidth);
		m_wndStatusBar.SetPaneInfo(0, nID, SBPS_NORMAL | SBPS_STRETCH, cxWidth);
	}
	//Set the default font
	CHARFORMAT cfDefault;
	cfDefault.cbSize = sizeof(cfDefault);
	cfDefault.dwEffects = CFE_PROTECTED;
	cfDefault.dwMask = CFM_BOLD | CFM_FACE | CFM_SIZE | CFM_CHARSET | CFM_PROTECTED;
	cfDefault.yHeight = 200;
	cfDefault.bCharSet = 0x00;
	strcpy_s(cfDefault.szFaceName, _T("Consolas"));


//	m_RichEdit.SetFont();
	//Support Drag and Drop
	DragAcceptFiles();
	m_strPathname = m_strShaderName; // m_strInputFileName;

	m_strPathname.Replace(_T("\\"), _T("/"));

	ChangeDir();

	SetWindowTitle();
//	ReadFile();

	HCURSOR crsArrow = ::LoadCursor(0, IDC_ARROW);
	CString classInfo = AfxRegisterWndClass(CS_DBLCLKS, crsArrow, (HBRUSH)m_InfoToolbar, 0);


	WINDOWINFO main_win;
	GetWindowInfo(&main_win);

	int height = main_win.rcClient.bottom - main_win.rcClient.top;

	const int vert_splitter_sizes[2] = {20, 2 };
	m_wndSplitterVert.Create(this); // &m_wndSplitterHorz);
	m_wndSplitterVert.SetPaneSizes(vert_splitter_sizes);

	BOOL res = m_wndSplitterVert.CreatePane(0, &m_RichEdit);
	res = m_wndSplitterVert.CreatePane(1, &m_wndInfo); // , 0, WS_EX_CLIENTEDGE, classInfo);

	m_wndInfo.m_parent = this;

	m_RichEdit.SetDefaultCharFormat(cfDefault);
	m_RichEdit.SetEventMask(ENM_CHANGE | ENM_SELCHANGE | ENM_PROTECTED);
	m_RichEdit.SetBackgroundColor(false, RGB(32, 32, 32));

	m_wndInfo.SetDefaultCharFormat(cfDefault);
	m_wndInfo.SetEventMask(ENM_CHANGE | ENM_SELCHANGE | ENM_PROTECTED);
	m_wndInfo.SetBackgroundColor(false, RGB(32, 32, 32));

	CHARFORMAT2 cf;
	ZeroMemory(&cf, sizeof(cf));
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = RGB(200, 200, 200);
	m_RichEdit.SetDefaultCharFormat(cf);
	m_wndInfo.SetDefaultCharFormat(cf);

	m_hIcon = (HICON)LoadImage(AfxGetApp()->m_hInstance,
		MAKEINTRESOURCE(IDR_MAINFRAME),
		IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTCOLOR);

	//restore windows positions
	WINDOWPLACEMENT wp;
	wp.length = sizeof(WINDOWPLACEMENT);
	ZeroMemory(&wp, sizeof(WINDOWPLACEMENT));

	RECT rect = { 0, 0, m_Width, m_Height };
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
	const int windowWidth = (rect.right - rect.left);
	const int windowHeight = (rect.bottom - rect.top);

	char *buff;
	size_t size;
	errno_t err = _dupenv_s(&buff, &size, "APPDATA");
	if (err == 0)
	{
		std::string path = buff; 
		path += "\\SanBaseStudio\\HLSL_Editor\\HLSL_Edit.ini";

		GetPrivateProfileStruct(TEXT("Section1"), TEXT("FirstKey"), LPBYTE(&wp), sizeof(wp), path.c_str());
		wp.rcNormalPosition.right = wp.rcNormalPosition.left + windowWidth;
		wp.rcNormalPosition.bottom = wp.rcNormalPosition.top + windowHeight;
		free(buff);
	}
	res = SetWindowPlacement(&wp);
	if (!res)
	{
		DWORD err = GetLastError();
		int i = 0;
	}
/*
	int scroll = COLOR_SCROLLBAR;
	DWORD color[1] = { GetSysColor(COLOR_SCROLLBAR) };
	color[0] = RGB(32, 32, 32);
	BOOL ret_c = SetSysColors(1, &scroll, color);
*/
/*
	int ret_c = 0;
	COLORREF clr = RGB(32, 32, 32);
	ret_c = SetSysColors(1, COLOR_SCROLLBAR, &clr);
*/
	return 0;
}

void CMainFrame::ChangeDir()
{
	int pos = m_strPathname.ReverseFind('/');
	if (pos != -1)
	{
		m_directory = m_strPathname.Left(pos);
		m_directory.Replace(_T("/"), _T("\\"));
	}

}
void CMainFrame::OpenFile(CString file)
{
	m_strPathname = file;
	m_strPathname.Replace(_T("\\"), _T("/"));
	ChangeDir();
	OutputDebugStringA(file);
	//		SetWindowTitle();
	//		ReadFile();
}
void CMainFrame::SetName(CString name)
{
	m_strPathname = name;
	m_strPathname.Replace(_T("\\"), _T("/"));
	ChangeDir();

	ReadFile();
}

#define LIGHT_COLOR 64
#define DARK_COLOR 64

void CMainFrame::OnNcPaint()
{
//	if (m_enable)
	{
		CWindowDC _dc(this);
		CRect rectWindow;
		CSize size;
		CRect editWindow, toolbarWindow, infoWindow;

		m_wndToolBar.GetWindowRect(&toolbarWindow); //78
		m_RichEdit.GetWindowRect(&editWindow); //78
		m_wndInfo.GetWindowRect(&infoWindow);
		GetWindowRect(&rectWindow); // 50
		rectWindow.OffsetRect(-rectWindow.left, -rectWindow.top);
		if(on_init)
			FillRect(_dc.GetSafeHdc(), rectWindow, CreateSolidBrush(RGB(32,32,32)));  //(HBRUSH)GetStockObject(GRAY_BRUSH));
		on_init = false;

		if (::IsThemeActive() && ::IsAppThemed())
		{
			HWND hWnd = GetSafeHwnd();

			HANDLE hTheme = OpenThemeData(hWnd, VSCLASS_WINDOW);
			int iState = m_active ? CS_ACTIVE : CS_INACTIVE;
			if (hTheme != NULL)
			{
				HDC hDC = _dc.GetSafeHdc();
				HRESULT hr = S_OK;

				int iOffset = 3;

				hr = GetThemePartSize(hTheme, hDC, WP_CAPTION, iState, NULL, TS_TRUE, &size);
				int cyCaption = hr == S_OK ? size.cy : GetSystemMetrics(SM_CYCAPTION);
				hr = GetThemePartSize(hTheme, hDC, WP_FRAMELEFT, iState, NULL, TS_TRUE, &size);
				int cxFrameL = hr == S_OK ? size.cx : GetSystemMetrics(SM_CXFRAME);
				hr = GetThemePartSize(hTheme, hDC, WP_FRAMELEFT, iState, NULL, TS_TRUE, &size);
				int cxFrameR = hr == S_OK ? size.cx : GetSystemMetrics(SM_CXFRAME);
				hr = GetThemePartSize(hTheme, hDC, WP_FRAMEBOTTOM, iState, NULL, TS_TRUE, &size);
				int cyFrameB = hr == S_OK ? size.cy : GetSystemMetrics(SM_CYFRAME);
				int cyFrameT = cyFrameB + iOffset;

				int cxIcon = GetSystemMetrics(SM_CXSMICON);
				int cyIcon = GetSystemMetrics(SM_CYSMICON);

				hr = GetThemePartSize(hTheme, hDC, WP_CLOSEBUTTON, iState, NULL, TS_TRUE, &size);
				int cxClose = size.cx;
				int cyClose = size.cy;

				rectWindow.bottom = cyCaption - iOffset;

				if (1)
				{
					CRect rect;
					GetClientRect(rect);
					CDC* dc;
					CRect rc1, rc2;
					dc = GetWindowDC();
					GetWindowRect((LPRECT)&rc2);

					CRect recta;
					GetClientRect(recta);

					CRect rctest;
					GetWindowRect(&rctest);
					ScreenToClient(&rctest);

					rc1.top = 0;
					rc1.bottom = 70; // ((rc2.bottom - rc2.top) - recta.bottom) + cyCaption + iOffset;  //69;
					rc1.left = 0;
					rc1.right = rc2.right - rc2.left;
					dc->FillSolidRect(rc1, RGB(64, 64, 64));

					// draw top border [title bar]

					int window_width = rc2.right - rc2.left;
					int window_height = rc2.bottom - rc2.top;

					CRect exp = { 0, rc1.bottom, (window_width - recta.right) / 2, window_height };
					dc->FillSolidRect(exp, RGB(64, 64, 64));

					exp = { (window_width - recta.right) / 2, rc1.bottom, (window_width - recta.right) + 2, window_height};
					dc->FillSolidRect(exp, RGB(32, 32, 32));


					exp = { recta.right + (window_width - recta.right) / 2, 0, rc2.right - rc2.left, window_height };
					dc->FillSolidRect(exp, RGB(64, 64, 64));

					exp = { 0, rectWindow.bottom ,rc2.right - rc2.left, rectWindow.bottom + 24};
					dc->FillSolidRect(exp, RGB(64, 64, 64));

					exp = { 0, window_height - 6, window_width, window_height };
					dc->FillSolidRect(exp, RGB(64, 64, 64));

					exp = { 0, 27, window_width, 28 };
					dc->FillSolidRect(exp, RGB(86, 86, 86));

				}

				if (SUCCEEDED(hr))
				{
					LOGFONTW logFont;
					memset(&logFont, 0, sizeof(logFont));

					hr = GetThemeSysFont(hTheme, TMT_CAPTIONFONT, &logFont);
					HFONT hOldFont = NULL, hNewFont = NULL;
					if (SUCCEEDED(hr))
					{
						hNewFont = CreateFontIndirectW(&logFont);
						if (hNewFont != NULL)
							hOldFont = (HFONT)SelectObject(hDC, hNewFont);
					}

					rectWindow.top = 5;
					rectWindow.left = cxFrameL + cxIcon + iOffset;
					rectWindow.bottom = cyCaption;

					CString Title;
					if (m_strPathname == "") Title = "HLSL Editor";
					else Title = "HLSL Editor: " + CString(m_strPathname);

					SetTextColor(hDC, RGB(200, 200, 200));
					SetBkMode(hDC, TRANSPARENT);

					DrawText(hDC, Title, Title.GetLength(), rectWindow, DT_VCENTER);

				}
				if (SUCCEEDED(hr))
				{

					CRect rcContent = rectWindow;
					rcContent.top = cyFrameT+iOffset;
					rcContent.bottom = rcContent.top + cyIcon;
					rcContent.left = rcContent.right - cxClose;
					rcContent.right = rcContent.left + cxIcon;


					hr = DrawThemeBackground(hTheme, hDC, WP_CLOSEBUTTON, FS_ACTIVE, &rcContent, 0);

				}
				CloseThemeData(hTheme);
			}
		}
	}

	m_wndToolBar.Invalidate();
	m_wndToolBar.UpdateWindow();
//	CFrameWnd::OnNcPaint();
}


BOOL CMainFrame::OnNcActivate(BOOL bActive)
{
	m_active = bActive;
	if (!bActive)
	{
//		CFrameWnd::OnNcActivate(bActive);
		// Add code here to draw caption when window is inactive.
		return TRUE;

		// Fall through if wParam == TRUE, i.e., window is active.     
	}
	return FALSE;
//	return CFrameWnd::OnNcActivate(bActive);
}
void CMainFrame::OnActivate(UINT nState, CWnd* pWndOther, BOOL  bActive)
{
	static UINT status = nState;
//	if(!bActive)
//		OnNcPaint();
//	status = nState;
	CFrameWnd::OnActivate(nState, pWndOther, bActive);
}


// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG


// CMainFrame message handlers

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// let the RichEdit have first crack at the command
	if (m_RichEdit.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	// otherwise, do default handling

	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
/*
void CMainFrame::OnClick(NMHDR *pNMHDR, LRESULT* pResult)
{

	::AfxMessageBox(_T("Hurrah!!"));
	ASSERT(NM_CLICK == pNMHDR->code);

	NMITEMACTIVATE *pNMIA = (NMITEMACTIVATE *)pNMHDR;

	*pResult = 1;
}
*/
void CMainFrame::OnUpdatePage(CCmdUI *pCmdUI)
{
	int nLine = m_RichEdit.LineFromChar(-1); // line index of current line 
	long lStart, lEnd;
	m_RichEdit.GetSel(lStart, lEnd);
	int nFirst = m_RichEdit.LineIndex();
	// Get caret column
	int nCol = lStart - nFirst;

	pCmdUI->Enable(true);
	CString strPage;
	strPage.Format(_T("   Current pos:  Line=%d Col=%d"), nLine, nCol);
	pCmdUI->SetText(strPage);
}


void CMainFrame::OnSetFocus(CWnd* pWnd)
{
	// forward focus to the RichEdit window
	//if(pWnd == m_RichEdit)
	m_RichEdit.SetFocus();
}

void CMainFrame::OnEditPaste()
{
	m_RichEdit.PasteSpecial(CF_TEXT);
}

void CMainFrame::OnEditCut()
{
	m_RichEdit.Cut();
}

void CMainFrame::OnEditCopy()
{
//	m_wndInfo.Copy();
	m_RichEdit.Copy();
}

void CMainFrame::OnEditUndo()
{
	m_RichEdit.Undo();
}


void CMainFrame::OnFind()
{
	m_pFindDialog = new CFindDlg();
	ZeroMemory(&ft, sizeof(ft));
//	m_pFindDialog->DoModal();

	m_pFindDialog->m_fr.lpTemplateName = MAKEINTRESOURCE(IDD_FINDDLG);
	BOOL res = m_pFindDialog->Create(FALSE, NULL, NULL, FR_ENABLETEMPLATE | FR_HIDEUPDOWN, this);
	if (res)
	{
		m_pFindDialog->SetActiveWindow();
		res = m_pFindDialog->ShowWindow(TRUE);
	}

	WINDOWINFO info, main_win;
	GetWindowInfo(&main_win);

	m_pFindDialog->GetWindowInfo(&info);
	int width = info.rcWindow.right - info.rcWindow.left;
	int height = info.rcWindow.bottom - info.rcWindow.top;

	RECT rect;
	rect.top = main_win.rcClient.top;
	rect.left = main_win.rcClient.right - width;
	rect.bottom = height + rect.top;
	rect.right = main_win.rcClient.right;

	m_pFindDialog->MoveWindow(&rect);
	// TODO: Add your command handler code here
//	delete m_pFindDialog;
	m_RichEdit.UpdateDoc();

	m_pFindDialog->SetActiveWindow();
	m_pFindDialog->ShowWindow(true);
}

LRESULT CMainFrame::OnFindReplaceMessage(WPARAM wParam, LPARAM lParam)
{
	CFindReplaceDialog* pFindReplace = CFindReplaceDialog::GetNotifier(lParam);
	ASSERT(pFindReplace != NULL);
	long pos = 0;
	if (pFindReplace->IsTerminating())
	{
		pFindReplace = NULL;

	}
	else if (pFindReplace->FindNext())
	{
		CString csFindString = m_pFindDialog->GetFindString();
		CString csReplaceString = m_pFindDialog->GetReplaceString();

		char str[256];
		sprintf_s(str, "%x", m_pFindDialog->m_fr.Flags);

		bool bFindAll = m_pFindDialog->MatchCase() == TRUE;
		bool bMatchWholeWord = m_pFindDialog->MatchWholeWord() == TRUE;

		ft.chrg.cpMin = bFindAll?0:ft.chrgText.cpMax;
		ft.chrg.cpMax = -1;
		ft.lpstrText = csFindString;

		if (csReplaceString.GetLength() == 0)
		{
			if (!bFindAll)
			{
				pos = m_RichEdit.FindText(FR_DOWN | (bMatchWholeWord ? FR_WHOLEWORD :0), &ft);
				if (pos != -1)
				{
					m_RichEdit.SetSel(pos, pos + csFindString.GetLength());
					m_RichEdit.SetFocus();
				}
			}
			else
			{
				m_RichEdit.SetRedraw(FALSE);
				CPoint pos_orig = m_RichEdit.GetCaretPos();
				int cursor = m_RichEdit.CharFromPos(pos_orig);
				m_RichEdit.SetSel(cursor, cursor);
				while (pos = m_RichEdit.FindText(FR_DOWN | (bMatchWholeWord ? FR_WHOLEWORD : 0), &ft) != -1)
				{
					ft.chrg.cpMin = ft.chrgText.cpMax;
					m_RichEdit.SetColor(ft.chrgText.cpMin, ft.chrgText.cpMax - ft.chrgText.cpMin, RGB(255, 255, 255));
					m_RichEdit.SetBGColor(ft.chrgText.cpMin, ft.chrgText.cpMax - ft.chrgText.cpMin, RGB(64, 120, 220));
				}
				m_RichEdit.SetRedraw(TRUE);
				pFindReplace->EndDialog(0);
				pFindReplace = NULL;
				m_RichEdit.SetSel(cursor, cursor);

				return 0;
			}
		}
		else
		{
			if (!bFindAll)
			{
				pos = m_RichEdit.FindText(FR_DOWN | (bMatchWholeWord ? FR_WHOLEWORD : 0), &ft);
				if (pos != -1)
				{
					m_RichEdit.SetSel(pos, pos + csFindString.GetLength());
					m_RichEdit.ReplaceSel(csReplaceString);
					m_RichEdit.SetFocus();
				}
			}
			else
			{
				m_RichEdit.SetRedraw(FALSE);
				CPoint pos_orig = m_RichEdit.GetCaretPos();
				int cursor = m_RichEdit.CharFromPos(pos_orig);
				m_RichEdit.SetSel(cursor, cursor);
				while (pos = m_RichEdit.FindText(FR_DOWN | (bMatchWholeWord ? FR_WHOLEWORD : 0), &ft) != -1)
				{
					ft.chrg.cpMin = ft.chrgText.cpMax;
					m_RichEdit.SetSel(ft.chrgText.cpMin, ft.chrgText.cpMax);
					m_RichEdit.ReplaceSel(csReplaceString);
				}
				m_RichEdit.SetRedraw(TRUE);
				pFindReplace->EndDialog(0);
				pFindReplace = NULL;
				m_RichEdit.SetSel(cursor, cursor);

				return 0;
			}
		}
	}

	return 0;

}


void CMainFrame::OnFilePrint()
{
	Print(true);
}

void CMainFrame::Print(bool bShowPrintDialog)
{
	CPrintDialog printDialog(false);

	if (bShowPrintDialog)
	{
		if(printDialog.DoModal() == IDCANCEL)
			return; // User pressed cancel, don't print.
	}
	else
	{
		printDialog.GetDefaults();
	}

	HDC hPrinterDC = printDialog.GetPrinterDC();

	// This code basically taken from MS KB article Q129860

	FORMATRANGE fr;
	  int	  nHorizRes = GetDeviceCaps(hPrinterDC, HORZRES);
	  int	  nVertRes = GetDeviceCaps(hPrinterDC, VERTRES);
      int     nLogPixelsX = GetDeviceCaps(hPrinterDC, LOGPIXELSX);
	  int     nLogPixelsY = GetDeviceCaps(hPrinterDC, LOGPIXELSY);
	  LONG	      lTextLength;   // Length of document.
	  LONG	      lTextPrinted;  // Amount of document printed.

	 // Ensure the printer DC is in MM_TEXT mode.
	  SetMapMode ( hPrinterDC, MM_TEXT );

	 // Rendering to the same DC we are measuring.
	  ZeroMemory(&fr, sizeof(fr));
	  fr.hdc = fr.hdcTarget = hPrinterDC;

	 // Set up the page.
	  fr.rcPage.left     = fr.rcPage.top = 0;
	  fr.rcPage.right    = (nHorizRes/nLogPixelsX) * 1440;
	  fr.rcPage.bottom   = (nVertRes/nLogPixelsY) * 1440;

	 // Set up 0" margins all around.
	  fr.rc.left   = fr.rcPage.left ;//+ 1440;  // 1440 TWIPS = 1 inch.
	  fr.rc.top    = fr.rcPage.top ;//+ 1440;
	  fr.rc.right  = fr.rcPage.right ;//- 1440;
	  fr.rc.bottom = fr.rcPage.bottom ;//- 1440;

	 // Default the range of text to print as the entire document.
	  fr.chrg.cpMin = 0;
	  fr.chrg.cpMax = -1;
      m_RichEdit.FormatRange(&fr,true);

	  // Set up the print job (standard printing stuff here).
	DOCINFO di;
	ZeroMemory(&di, sizeof(di));
	di.cbSize = sizeof(DOCINFO);

	di.lpszDocName = m_strPathname;

	// Do not print to file.
	di.lpszOutput = NULL;


	 // Start the document.
	  StartDoc(hPrinterDC, &di);

	 // Find out real size of document in characters.
	  lTextLength = m_RichEdit.GetTextLength();

	 do
	 {
	     // Start the page.
	     StartPage(hPrinterDC);

	    // Print as much text as can fit on a page. The return value is
	     // the index of the first character on the next page. Using TRUE
	     // for the wParam parameter causes the text to be printed.

		lTextPrinted =m_RichEdit.FormatRange(&fr,true);
		m_RichEdit.DisplayBand(&fr.rc );

	    // Print last page.
	     EndPage(hPrinterDC);

	    // If there is more text to print, adjust the range of characters
	    // to start printing at the first character of the next page.
		if (lTextPrinted < lTextLength)
		{
			fr.chrg.cpMin = lTextPrinted;
			fr.chrg.cpMax = -1;
		}
	  }
	  while (lTextPrinted < lTextLength);

	 // Tell the control to release cached information.
	  m_RichEdit.FormatRange(NULL,false);

	 EndDoc (hPrinterDC);

	DeleteDC(hPrinterDC);

}

void CMainFrame::OnDropFiles(HDROP hDropInfo)
{
	CString FileName;

	::DragQueryFile(hDropInfo, 0, FileName.GetBufferSetLength(_MAX_PATH),_MAX_PATH);
	FileName.ReleaseBuffer();
	::DragFinish(hDropInfo);

	m_strPathname = FileName;

	SetWindowTitle();
	ReadFile();
}

void CMainFrame::ReadFile()
{
	CString FileName = m_strPathname;

	if (FileName.IsEmpty())
		return;

	UpdateFunctionList();

	// Convert full filename characters from "\" to "\\"
//	FileName.Replace(_T("\\"), _T("\\\\"));
//	FileName.Replace(_T("\\"), _T("/"));

	// The file from which to load the contents of the rich edit control.
	CFile cFile(FileName, CFile::modeRead);
	EDITSTREAM es;

	es.dwCookie =  (DWORD_PTR) &cFile;
	es.pfnCallback = (EDITSTREAMCALLBACK) MyStreamInCallback;

	m_RichEdit.StreamIn(SF_TEXT,es); // Perform the streaming
	m_RichEdit.UpdateDoc();

}

bool compEx(KeyWord a, KeyWord b)
{

	return (strcmp(a.name, b.name)<0);
}


bool CMainFrame::ParseFinctions(int line, const char *ch, bool comment)
{
	int i = 0;
	CString str = ch;
	int comment_start=100000, comment_end = 0;

	int m_comment = comment;

	if (!m_comment)
	{
		i = str.Find("//", 0);
		if (i != -1)
		{
			comment_start = i;
//			return false;
		}
	}

	i = str.Find("/*", 0);
	if (i != -1 && i < comment_start)
	{
		m_comment = true;
		comment_start = comment_start<i?comment_start:i;
	}
	else i = 0;
	if (m_comment)
	{
		i = str.Find("*/", i);
		if (i == -1)
		{
			comment_end = 10000;
			i = 0;
		}
		else
		{
			m_comment = false;
			comment_end = i;
		}
	}

	if (m_comment)
		return true;

	int j = 0;
//	i = 0;
//	str.Remove(' ');
//	str.Remove('\t');

	int len = (int)m_RichEdit.type_table.size(); // / (sizeof KeyWord);
	i = 0;
	for (CString word = str.Tokenize(" \t{},;+/-*=<>\r):", i); word != ""; word = str.Tokenize(" \t{},;+/-*=<>\r:)", i))
	{
		while (str[j] != word[0])
			j++;
		if ((i > comment_start) && (i < comment_end))
			continue;
		int p;
		if ((p=word.Find('(')) != -1)
		{
			word.Truncate(p);
			KeyWord key = { word, 3 };

			vector<KeyWord>::iterator a = std::lower_bound(m_RichEdit.type_table.begin(), m_RichEdit.type_table.end(), key, compEx);

			if (a != m_RichEdit.type_table.end() && key.name == a->name)
				continue;

			m_RichEdit.type_table.push_back(key);
			std::sort(m_RichEdit.type_table.begin(), m_RichEdit.type_table.end(), compEx);

		}
		//		bool res = MarkKeyWord(str, word, index, j);
		j = i;
	}
	return false;
}


void CMainFrame::UpdateFunctionList()
{
	int num_lines = m_RichEdit.GetLineCount();
	bool comment = false;
	char Buffer[1024];
	CString folderPath;

	int pos = m_strPathname.ReverseFind('/');
	if (pos != -1)
	{
		folderPath = m_strPathname.Left(pos)+"/";
	}

	for (int l = 0; l < num_lines; l++)
	{
		m_RichEdit.GetLine(l, Buffer, sizeof Buffer);
		CString str = Buffer;
		int i;
		if ((i = str.Find("#include ", 0)) != -1)
		{
			CString file = Buffer + i + 8;
			int j = file.Find('\"', 0);
			file = Buffer + i + 8 + j + 1;
			j = file.Find('\"', 0);
			file.Truncate(j);
			CString file_name = folderPath + file;

			string line;
			ifstream myfile(file_name);
			bool comment = false;

			if (myfile)
			{
				int l = 0;
				while (getline(myfile, line)) 
				{
					comment = ParseFinctions(l, line.c_str(), comment);
					l++;
				}
				myfile.close();
			}
		}
	}
	string line;
	ifstream myfile(m_strPathname);
	comment = false;

	if (myfile)
	{
		int l = 0;
		while (getline(myfile, line))
		{
			comment = ParseFinctions(l, line.c_str(), comment);
			l++;
		}
		myfile.close();
	}
}


void CMainFrame::WriteFile()
{

	// Convert full filename characters from "\" to "\\"
	CString Pathname = m_strPathname;
	Pathname.Replace(_T("\\"), _T("/"));

	// The file from which to load the contents of the rich edit control.
	CFile cFile(Pathname, CFile::modeCreate|CFile::modeWrite);
	EDITSTREAM es;

	es.dwCookie =  (DWORD_PTR) &cFile;
	es.pfnCallback = (EDITSTREAMCALLBACK) MyStreamOutCallback;

	m_RichEdit.StreamOut(SF_TEXT,es); // Perform the streaming

}

#include <dlgs.h>
////////////////////////////////////////////////////////////////////// 
// CMyDlg dialog

class CMyDlg : public CFileDialog
{
	// Construction
public:
	CMyDlg(BOOL bOpenFileDialog,
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		CWnd* pParentWnd = NULL,
		DWORD dwSize = 0,
		BOOL bVistaStyle = TRUE);   // standard constructor

									// Add a CBrush pointer to store the new background brush
	CBrush m_pBkBrush;

	// Dialog Data
	//<AngularNoBind>{{</AngularNoBind>AFX_DATA(CMyDlg)
	enum { IDD = FILEOPENORD };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// Implementation
protected:
	virtual void DoDataExchange(CDataExchange* pDX);  // DDX/DDV support
													  // Generated message map functions
													  //<AngularNoBind>{{</AngularNoBind>AFX_MSG(CMyDlg)
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

////////////////////////////////////////////////////////////////////// 
// CMyDlg dialog

CMyDlg::CMyDlg(BOOL bOpenFileDialog,
	LPCTSTR lpszDefExt,
	LPCTSTR lpszFileName,
	DWORD dwFlags,
	LPCTSTR lpszFilter,
	CWnd* pParentWnd,
	DWORD dwSize,
	BOOL bVistaStyle)
	: CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd)
{
	//<AngularNoBind>{{</AngularNoBind>AFX_DATA_INIT(CMyDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

void CMyDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//<AngularNoBind>{{</AngularNoBind>AFX_DATA_MAP(CMyDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CMyDlg, CFileDialog)
	//<AngularNoBind>{{</AngularNoBind>AFX_MSG_MAP(CMyDlg)
	ON_WM_CTLCOLOR()
	//<AngularNoBind>}}</AngularNoBind>AFX_MSG_MAP
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////////// 
// CMyDlg message handlers

HBRUSH CMyDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	switch (nCtlColor) {

	case CTLCOLOR_STATIC:
		// Set the static text to white on blue.
		pDC->SetTextColor(RGB(255, 255, 255));
		pDC->SetBkColor(RGB(32, 32, 32));
		// Drop through to return the background brush.

	case CTLCOLOR_DLG:
		return (HBRUSH)(m_pBkBrush.GetSafeHandle());

	default:
		return CFileDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	}
}

void CMainFrame::OnFileOpen()
{
	// szFilters is a text string that includes two file name filters:
	// "*.my" for "MyType Files" and "*.*' for "All Files."
	TCHAR szFilters[]= _T("Text Files (*.hlsl)|*.hlsl|All Files (*.*)|*.*||");

	// Create an Open dialog; the default file name extension is ".my".

	CMyDlg fileDlg (TRUE, "hlsl", NULL, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY, szFilters, this);

	CString initial_dir = m_strPathname;

	int pos = initial_dir.ReverseFind('/');
	if (pos != -1)
	{
		initial_dir = initial_dir.Left(pos) + "/";
	}
	initial_dir.Replace(_T("/"), _T("\\"));
	fileDlg.m_ofn.lpstrInitialDir = initial_dir;

	// Display the file dialog. When user clicks OK, fileDlg.DoModal()
	// returns IDOK.
	if( fileDlg.DoModal ()==IDOK )
	{
		m_strPathname = fileDlg.GetPathName();
		m_strPathname.Replace(_T("\\"), _T("/"));

		int pos1 = m_strPathname.ReverseFind('/');
		int pos2 = m_strShaderName.ReverseFind('/');
		CString a = m_strShaderName.Left(pos2);
	
		if(a.GetLength()==0 || m_strPathname.Left(pos1).Compare(a))
			m_strShaderName = m_strPathname;

		m_entryPoint = "PS";
		//Change the window's title to the opened file's title.
		SetWindowTitle();
		ReadFile();
//		m_RichEdit.UpdateDoc();
	}
}


DWORD CALLBACK MyStreamInCallback(CFile* dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	// Required for StreamIn
	CFile* pFile = (CFile*) dwCookie;

	*pcb = pFile->Read(pbBuff, cb);

	return 0;
}


DWORD CALLBACK MyStreamOutCallback(CFile* dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb)
{
	// Required for StreamOut
	CFile* pFile = (CFile*) dwCookie;

	pFile->Write(pbBuff, cb);
	*pcb = cb;

	return 0;
}
void CMainFrame::OnFileSave()
{
	if (m_strPathname.GetLength() && !(GetFileAttributes(m_strPathname)&FILE_ATTRIBUTE_DIRECTORY))
	{
		WriteFile();
	}
	else
		OnFileSaveas();
}

void CMainFrame::OnFileNew()
{
	//Clear the Richedit text
	m_RichEdit.SetWindowText("");

	int pos = m_strPathname.ReverseFind('/');
	if (pos != -1)
	{
		m_strPathname = m_strPathname.Left(pos)+"/"; 
	}
	else
		m_strPathname = "";

	SetWindowTitle();
}

void CMainFrame::OnDestroy()
{
	WINDOWPLACEMENT wp;

	wp.length = sizeof(WINDOWPLACEMENT);
	ZeroMemory(&wp, sizeof(WINDOWPLACEMENT));

	GetWindowPlacement(&wp);

	char *buff;
	size_t size;
	errno_t err = _dupenv_s(&buff, &size, "APPDATA");
	if (err == 0)
	{
		std::string path = buff;
		path += "\\SanBaseStudio";

		if (!PathFileExists(path.c_str()))
		{
			CreateDirectory(path.c_str(), NULL);
		}
		path += "\\HLSL_Editor";

		if (!PathFileExists(path.c_str()))
		{
			CreateDirectory(path.c_str(), NULL);
		}
		std::string key = path + "\\HLSL_Edit.ini";

		BOOL res = WritePrivateProfileStruct(TEXT("Section1"), TEXT("FirstKey"), LPBYTE(&wp), sizeof(wp), key.c_str());
		TCHAR pBuf[256];
		int bytes = GetModuleFileName(NULL, pBuf, sizeof pBuf);

		key = path += "\\Editor_Path";

		FILE *fd;
		fopen_s(&fd, key.c_str(), "wb");
		fwrite(pBuf, 1, bytes, fd);
		fclose(fd);

		free(buff);
	}
	CFrameWnd::OnDestroy();
	AfxPostQuitMessage(0);
}

void CMainFrame::MarkErrorPos(vec4 pos)
{
	CHARFORMAT2 cf;
	m_RichEdit.GetDefaultCharFormat(cf);

	int i = m_RichEdit.LineIndex(pos.x - 1) + pos.y - 1;

	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;
	if(pos.w==0)
		cf.crTextColor = RGB(200, 120, 64);
	else
		cf.crTextColor = RGB(255, 80, 80);
	m_RichEdit.SetSel(i, i + pos.z + 1 - pos.y);
	BOOL res = m_RichEdit.SetSelectionCharFormat(cf);

	m_RichEdit.SetSel(i, i);
}

struct {
	bool operator()(ERR_MESSAGE a, ERR_MESSAGE b)
	{
		return a.pos.w > b.pos.w;
	}
} 
sort_key;

void CMainFrame::OnCompile()
{
	CHARFORMAT2 cf;
	WriteFile();

	m_RichEdit.GetDefaultCharFormat(cf);

	ComPtr<ID3D10Blob> error;
	UINT flag = 0;
#if _DEBUG
	flag |= D3DCOMPILE_DEBUG;
#endif /* _DEBUG */

	ComPtr<ID3DBlob> Shader;

	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;

	BOOL res = m_RichEdit.SetSelectionCharFormat(cf);

	CString  s_Target = m_entryPoint + "_5_1";
	s_Target.MakeLower();

	m_RichEdit.UpdateDoc();

	m_wndInfo.SetSel(0, -1);
	m_wndInfo.Clear();
	m_wndInfo.SetColor(0, 1, RGB(200,200,200));

	HRESULT hr = D3DCompileFromFile(m_strShaderName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, m_entryPoint, s_Target, flag, 0, &Shader, &error);
	if (hr != S_OK)
	{
		if (error != nullptr)
		{
			char *err = (char*)error.Get()->GetBufferPointer();
			OutputDebugStringA(err);

			CString str = err;
			str.Replace(_T("\\"), _T("/"));

			std::vector<CString> m_strings;
			str = err;
			int p = 0;
			bool selected = false;
			CString line = str.Tokenize(_T("\n"), p);

			int lin, pos, len;
			vec4 vec;

			m_wndInfo.m_err.clear();

			while (line != _T(""))
			{
				struct ERR_MESSAGE m_line;
				m_strings.push_back(line);

				int m_p = 0;
				line.Tokenize(_T("("), m_p);
				CString file_name = line.Mid(0, m_p-1);
				CString m_str = line.Mid(m_p, line.GetLength() - m_p);
				m_p = 0;
				lin = 0;
				pos = 0;
				len = 0;
				sscanf_s(m_str, "%d,%d-%d", &lin, &pos, &len);
				m_str.Tokenize(_T(")"), m_p);
				m_str = m_str.Mid(m_p, m_str.GetLength() - m_p);
				CString m_pos;
				m_pos.Format(_T(" Line=%d Col=%d "), lin, pos);

				vec.x = lin;
				vec.y = pos;
				vec.z = len;
				if (vec.z <= 0)
					vec.z = pos;
				vec.w = 0;

				if (line.Find(_T("error"), 0) >= 0)
				{
					vec.w = 1;
					if (selected == false)
					{
						if (file_name != CW2A(m_strShaderName, CP_UTF8))
						{
							if(file_name[1]==':')
								m_strPathname = file_name;
							else
							{
								int i = m_strPathname.ReverseFind('/');
								if(i==-1)
									i = m_strPathname.ReverseFind('\\');

								m_strPathname.Truncate(i);

								m_strPathname += L"/";
								m_strPathname += file_name;
							}

							SetWindowTitle();
							ReadFile();
						}
						selected = true;
					}
				}
				m_line.pos = vec;
				m_line.info_line = m_pos + m_str + _T("\n");
				m_wndInfo.m_err.push_back(m_line);

				line = str.Tokenize(_T("\n"), p);

			}
			int info_index = 0;
			std::sort(m_wndInfo.m_err.begin(), m_wndInfo.m_err.end(), sort_key);
			for (int i = 0; i< m_wndInfo.m_err.size(); i++)
			{
				if (m_wndInfo.m_err[i].info_line.Find(_T("warning")) >= 0)
					m_wndInfo.SetColor(info_index, m_wndInfo.m_err[i].info_line.GetLength(), RGB(200, 120, 64));
				else
					m_wndInfo.SetColor(info_index, m_wndInfo.m_err[i].info_line.GetLength(), RGB(255, 64, 64));

				m_wndInfo.SetMessage(m_wndInfo.m_err[i].info_line);
				info_index += m_wndInfo.m_err[i].info_line.GetLength();

				MarkErrorPos(m_wndInfo.m_err[i].pos);
			}
			MarkErrorPos(m_wndInfo.m_err[0].pos);
		}
	}
	else
		m_wndInfo.SetMessage("Success!");
}

void CMainFrame::OnFileSaveas()
{
	// szFilters is a text string that includes two file name filters:
	// "*.txt" for "Text Files" and "*.*" for "All Files."
	char szFilters[]= "Text Files (*.hlsl)|*.hlsl|All Files (*.*)|*.*||";

	// Create an Open dialog; the default file name extension is ".my".
	CFileDialog fileDlg (FALSE, "hlsl", NULL, OFN_OVERWRITEPROMPT| OFN_HIDEREADONLY, szFilters, this);

	CString initial_dir = m_strPathname;
	initial_dir.Replace(_T("/"), _T("\\"));
	fileDlg.m_ofn.lpstrInitialDir = initial_dir;

	// Display the file dialog. When user clicks OK, fileDlg.DoModal()
	// returns IDOK.
	if( fileDlg.DoModal ()==IDOK )
	{
		m_strPathname = fileDlg.GetPathName();

		//Change the window's title to include the saved file's title.
		SetWindowTitle();

		WriteFile();
	}
}

void CMainFrame::OnAppClose()
{
	CFrameWnd::OnClose();
}

void CMainFrame::SetWindowTitle()
{
	CString Title;

	if (m_strPathname == "") Title = "HLSL_Edit - Untitled";
	else Title = "HLSL Editor: " + m_strPathname;
	SetWindowText(Title);
}

