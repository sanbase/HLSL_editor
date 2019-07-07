// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__0BC0FB9E_F0B7_486A_A939_5894B9E590A9__INCLUDED_)
#define AFX_MAINFRM_H__0BC0FB9E_F0B7_486A_A939_5894B9E590A9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Resource.h"
#include "SimpleSplitter.h"
#include "Bitmap.h"
#include <thread>

DWORD CALLBACK MyStreamInCallback(CFile* dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb);
DWORD CALLBACK MyStreamOutCallback(CFile* dwCookie, LPBYTE pbBuff, LONG cb, LONG *pcb);

/*
#define FINDDLGORD              1540
#define REPLACEDLGORD           1541
*/

#include <vector>
using namespace std;

class _AFX_RICHEDITEX_STATE
{
public:

	_AFX_RICHEDITEX_STATE();
	virtual ~_AFX_RICHEDITEX_STATE();

public:

	HINSTANCE m_hInstRichEdit20;
};

BOOL PASCAL AfxInitRichEditEx();


struct KeyWord
{
	CString name;
	int type;
};


struct vec2
{
	int x;
	int y;
};

struct vec3
{
	int x;
	int y;
	int z;
};

struct vec4
{
	int x;
	int y;
	int z;
	int w;
};


class CMainFrame;
struct ERR_MESSAGE
{
	vec4 pos;
	CString info_line;
};

class InfoPanel : public CRichEditCtrl
{
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL( InfoPanel )
public:
	virtual BOOL Create(DWORD in_dwStyle, const RECT& in_rcRect, CWnd* in_pParentWnd, UINT in_nID);
	//}}AFX_VIRTUAL

	void SetColor(int index, int length, COLORREF color);
	void SetBGColor(int index, int length, COLORREF color);
	void SetMessage(CString msg) 
	{ 
		ReplaceSel(msg); 
		RedrawWindow();
	};
	std::vector<ERR_MESSAGE> m_err;
	CMainFrame *m_parent;
protected:
	//{{AFX_MSG( InfoPanel )
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnRedraw();
	afx_msg void OnCopy();
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
//	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class HLSLEdit : public InfoPanel // CRichEditCtrl
{
	// Construction
public:

	HLSLEdit();
	virtual ~HLSLEdit();

public:

	void UpdateDoc();


	LONG volatile m_terminating = 0;
	HANDLE hThread = nullptr;
	HANDLE timerHandle;

	std::vector<KeyWord> type_table;

protected:
	//{{AFX_MSG( HLSLEdit )
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnRedraw();
	afx_msg void OnCopy();
	afx_msg void OnSetFocus(CWnd *pOldWnd);
//	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	bool ParseLine(int line, char *ch, bool comment);
	bool MarkKeyWord(CString line, CString str, int index, int pos);
};

class CFindDlg : public CFindReplaceDialog
{
	// Construction
public:
	CFindDlg(CWnd* pParent = NULL);   // standard constructor									  

									  // Dialog Data

									  //{{AFX_DATA(CFindDlg)
	enum { IDD = IDD_FINDDLG };
	// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFindDlg)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
														//}}AFX_VIRTUAL

														// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFindDlg)
	afx_msg void OnCheck1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CChildWnd : public CWnd
{
public:
	CChildWnd();
	virtual ~CChildWnd();
	void SetMessage(CString msg)
	{
		m_message = msg;
		OnPaint();
		RedrawWindow();
	}
	std::vector<vec4> m_err;
	CMainFrame *m_parent;
private:
	CString m_message;
protected:
	//{{AFX_MSG(CChildWnd)
	afx_msg void OnPaint();
	afx_msg void OnWindowPosChanging(WINDOWPOS FAR* lpwndpos);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

class CMainFrame : public  CFrameWnd
{

public:
	CMainFrame();

	HLSLEdit m_RichEdit;
	InfoPanel m_wndInfo;

//	CEdit m_Text;

	void ReadFile();
	void WriteFile();
	void SetWindowTitle();
	void SetMessage(CString msg)
	{
		m_wndInfo.ReplaceSel(msg);
//		m_wndInfo.SetMessage(msg);
	}
	void MarkErrorPos(vec4 pos);
protected:
	DECLARE_DYNAMIC(CMainFrame)

	// Attributes
public:
	void OpenFile(CString file);
	void SetName(CString name);
	void ChangeDir();
	CFindDlg* m_pFindDialog = nullptr;
private:
	//	bool ParseLine(int line, char *ch, bool comment);
	// Operations
public:
	CStringW m_strShaderName;
	CString m_entryPoint;
/*
	CString m_strInputFileName;
	CString m_errorPos;
	CString m_Error_line;
*/
	// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	void OnCompile();
	// Implementation
public:
	virtual ~CMainFrame();
	void Print(bool bShowPrintDialog);
	LRESULT OnFindReplaceMessage(WPARAM wParam, LPARAM lParam);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar24	m_wndToolBar;
	CString		m_strPathname;
	CString		m_directory;

	//	CMyMenu* pMyMenu = nullptr;
	//	CMyMenuBar		m_wndMenuBar;

private:
	FINDTEXTEX ft;
	BOOL m_active;
	BOOL m_enable;
	HICON m_hIcon;
	int m_Width;
	int m_Height;

	void UpdateFunctionList();
	bool ParseFinctions(int line, const char *ch, bool comment);
	bool on_init = true;
	// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSetFocus(CWnd *pOldWnd);
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
	afx_msg void OnEditCut();
	afx_msg void OnFilePrint();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnFileOpen();
	afx_msg void OnFileSave();
	afx_msg void OnFileNew();
//	afx_msg void OnCompile();
	afx_msg void OnFileSaveas();
	afx_msg void OnAppClose();
	afx_msg void OnEditUndo();
	afx_msg void OnUpdatePage(CCmdUI *pCmdUI);
	afx_msg void OnDestroy();
	afx_msg void OnFind();
	afx_msg void OnNcPaint();
	afx_msg BOOL OnNcActivate(BOOL bActive);
	afx_msg void OnActivate(UINT, CWnd*, BOOL);

//	afx_msg void OnClick(NMHDR *pNMHDR, LRESULT* pResult);

	//	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//	afx_msg virtual BOOL PreTranslateMessage(MSG* msg);

	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

	CSimpleSplitter m_wndSplitterVert;
//	CChildWnd	m_wndInfo;
//	CChildWnd	m_wndMenu;
	CBrush m_InfoToolbar;
	//	CSplitterWnd m_wndSplitter;
	//	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
};



/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__0BC0FB9E_F0B7_486A_A939_5894B9E590A9__INCLUDED_)

