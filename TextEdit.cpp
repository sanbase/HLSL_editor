// TextEdit.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "TextEdit.h"
#include "MainFrm.h"
#include <chrono>
#include <cctype>

#include <algorithm>    // std::binary_search, std::sort
#include <mutex>
#include <chrono>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CTextEditApp

BEGIN_MESSAGE_MAP(CTextEditApp, CWinApp)
	//{{AFX_MSG_MAP(CTextEditApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
END_MESSAGE_MAP()

#define WM_CUSTOM_REDRAW_MSG (WM_USER+0)

#define ON_WM_CUSTOM_PAINT() \
	{ WM_CUSTOM_REDRAW_MSG, 0, 0, 0, AfxSig_vv, \
		(AFX_PMSG)(AFX_PMSGW) \
		(static_cast< void (AFX_MSG_CALL CWnd::*)(void) > ( &ThisClass :: OnRedraw)) },


BEGIN_MESSAGE_MAP(InfoPanel, CRichEditCtrl)
	//{{AFX_MSG_MAP(HLSLEdit)
	ON_WM_CHAR()
	ON_WM_LBUTTONDOWN()
	ON_WM_CUSTOM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_MESSAGE_MAP(HLSLEdit, CRichEditCtrl)
	//{{AFX_MSG_MAP(HLSLEdit)
	ON_WM_CHAR()
	ON_WM_CUSTOM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

LONG m_tick_ms = 0;

void InfoPanel::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CRichEditCtrl::OnChar(nChar, nRepCnt, nFlags);
}

void InfoPanel::OnRedraw()
{
	RedrawWindow();
}


void InfoPanel::SetColor(int index, int length, COLORREF color)
{
	CHARFORMAT2 cf;
	//	GetDefaultCharFormat(cf);
	ZeroMemory(&cf, sizeof cf);
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;

	int nOldFirstVisibleLine = GetFirstVisibleLine();

	cf.crTextColor = color;
	SetSel(index, length<0 ? -1 : (index + length));
	SetSelectionCharFormat(cf);
	//	::SendMessage(GetSafeHwnd(), EM_SETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&cf);

	// Prevent the auto-scroll of the control when calling SetSel()
	int nNewFirstVisibleLine = GetFirstVisibleLine();

	if (nOldFirstVisibleLine != nNewFirstVisibleLine)
		LineScroll(nOldFirstVisibleLine - nNewFirstVisibleLine);
}

void HLSLEdit::OnCopy()
{
	Copy();
}

void InfoPanel::OnSetFocus(CWnd* pWnd)
{
	// forward focus to the RichEdit window
	SetFocus();
}
void InfoPanel::OnCopy()
{
	Copy();
}

void HLSLEdit::OnSetFocus(CWnd* pWnd)
{
	// forward focus to the RichEdit window
	SetFocus();
}

void InfoPanel::OnLButtonDown(UINT nFlags, CPoint point)
{
	CPaintDC dc(this);
	TEXTMETRIC lpMetrics;
	dc.GetTextMetrics(&lpMetrics);
	int line = point.y / lpMetrics.tmHeight;
	if (line < m_err.size())
	{
		m_parent->MarkErrorPos(m_err[line].pos);
	}
}

void InfoPanel::SetBGColor(int index, int length, COLORREF color)
{
	CHARFORMAT2 cf;
	ZeroMemory(&cf, sizeof cf);
	//	GetDefaultCharFormat(cf);
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_BACKCOLOR;

	int nOldFirstVisibleLine = GetFirstVisibleLine();

	cf.crBackColor = color;
	SetSel(index, length<0 ? -1 : (index + length));
	SetSelectionCharFormat(cf);

	//	::SendMessage(GetSafeHwnd(), EM_SETCHARFORMAT, (WPARAM)SCF_SELECTION, (LPARAM)&cf);

	// Prevent the auto-scroll of the control when calling SetSel()
	int nNewFirstVisibleLine = GetFirstVisibleLine();

	if (nOldFirstVisibleLine != nNewFirstVisibleLine)
		LineScroll(nOldFirstVisibleLine - nNewFirstVisibleLine);
}

void HLSLEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CRichEditCtrl::OnChar(nChar, nRepCnt, nFlags);
	m_tick_ms = GetTickCount();
}


void HLSLEdit::OnRedraw()
{
	UpdateDoc();
}

extern bool compEx(KeyWord a, KeyWord b);
bool HLSLEdit::MarkKeyWord(CString line, CString  word, int index, int pos)
{
	KeyWord key = { word, 0 };

	vector<KeyWord>::iterator a = std::lower_bound(type_table.begin(), type_table.end(), key, compEx);

	if (a!= type_table.end() &&  a->name==key.name)
	{
		if (a->type == 0)
		{
			SetColor(index + pos, word.GetLength(), RGB(100, 162, 230));
			return TRUE;
		}
		if (a->type == 1)
		{
			int j;
			for (j = pos + word.GetLength(); line[j]; j++)
			{
				if (line[j] != ' ' && line[j] != '\t')
					break;
			}
			if (line[j] == '(')
			{
				SetColor(index + pos, word.GetLength(), RGB(200, 100, 180)); //RGB(32, 160, 160));
				return TRUE;
			}
		}
		if (a->type == 2)
		{
			SetColor(index + pos, word.GetLength(), RGB(130, 130, 130));
			return TRUE;
		}
		if (a->type == 3)
		{
			SetColor(index + pos, word.GetLength(), RGB(220, 180, 80)); // RGB(80, 200, 120));
			return TRUE;
		}
	}

	int len = word.GetLength();
	for (int i = 0; i<len; i++)
	{
		if ((word[i]<'0' || word[i]>'9') && word[i] != '.')
			return FALSE;
	}
	SetColor(index + pos, len, RGB(220, 140, 120));  // RGB(200, 180, 100));
	return TRUE;
}

bool HLSLEdit::ParseLine(int line, char *ch, bool comment)
{
	CHARFORMAT2 cf;
	GetDefaultCharFormat(cf);
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;
	int index, i=0;
	CString str = ch;

	int m_comment = comment;
	int comment_end = 0;
	index = LineIndex(line);
	int comment_start = index;
	int comm_begin = 100000, comm_end = 0;

	int j = 0;
	i = 0;

	if (!m_comment)
	{
		i = str.Find("//", 0);
		if (i != -1)
		{
			comment_start = index + i;
			comment_end = index + str.GetLength();

			SetColor(comment_start, comment_end - comment_start, RGB(50, 140, 42));
			comm_begin = i;
			comm_end = 100000;
		}
	}

	i = str.Find("/*", 0);
	if (i != -1 && i < comm_begin)
	{
		comment_start = index + i;
		m_comment = true;
		comm_begin = comm_begin<i ? comm_begin : i;
	}
	else i = 0;
	if (m_comment)
	{
		i = str.Find("*/", i);
		if (i == -1)
		{
			comment_end = index + str.GetLength();
			comm_end = 100000;
			i = 0;
		}
		else
		{
			m_comment = false;
			comment_end = index + i + 2;
//			str = ch + i;
			if(i>comm_end)
				comm_end = i;
		}
	}
	i = 0;
	for (CString word = str.Tokenize(" (){},;\t+/-*=<>\r:", i); word != ""; word = str.Tokenize(" (){},;\t+/-*=<>\r:", i))
	{
		while (str[j] != word[0])
			j++;
		if (i > comm_begin && i < comm_end)
			continue;
		bool res = MarkKeyWord(str, word, index, j);
		j = i;
	}

	if (comment_end > 0)
	{
		SetColor(comment_start, comment_end - comment_start, RGB(50, 140, 42));
	}

	if (m_comment)
		return true;

	return false;
}

CTextEditApp::CTextEditApp()
{

}

_AFX_RICHEDITEX_STATE::_AFX_RICHEDITEX_STATE()
{
	m_hInstRichEdit20 = NULL;
}
_AFX_RICHEDITEX_STATE::~_AFX_RICHEDITEX_STATE()
{
	if (m_hInstRichEdit20 != NULL)
	{
		::FreeLibrary(m_hInstRichEdit20);
	}
}

//******************************************************************************
//
// Global Data
//
//******************************************************************************

_AFX_RICHEDITEX_STATE _afxRichEditStateEx;


BOOL PASCAL AfxInitRichEditEx()
{
	if (!::AfxInitRichEdit())
	{
		return FALSE;
	}

	_AFX_RICHEDITEX_STATE* l_pState = &_afxRichEditStateEx;

	if (l_pState->m_hInstRichEdit20 == NULL)
	{
		l_pState->m_hInstRichEdit20 = LoadLibraryA("RICHED20.DLL");
	}

	return l_pState->m_hInstRichEdit20 != NULL;
}

void DrawThread(void *arg)
{
	HLSLEdit *editor= reinterpret_cast<HLSLEdit *>(arg);

//	std::unique_lock<std::mutex> g_Lock(m_Mutex);
	for (;editor!=nullptr && !editor->m_terminating;)
	{
		Sleep(100);
		if(m_tick_ms!=0)
		{
			int duration = GetTickCount() - m_tick_ms;
			if (duration > 1000)
			{

				m_tick_ms = 0;
				::SendMessage(editor->GetSafeHwnd(), WM_CUSTOM_REDRAW_MSG, NULL, NULL);
			}
		}
	}
}

int comp(KeyWord *a, KeyWord *b)
{

	return strcmp(a->name, b->name);
}

HLSLEdit::HLSLEdit()
{
	KeyWord init_value[] = {
	{ "bool", 0 },
	{ "int", 0 },
	{ "float", 0 },
	{ "float2", 0 },
	{ "float3", 0 },
	{ "float4", 0 },
	{ "double", 0},
	{ "double2", 0 },
	{ "double3", 0 },
	{ "double4", 0 },
	{ "vec2", 0 },
	{ "vec3", 0 },
	{ "vec4", 0 },
	{ "dvec2", 0 },
	{ "dvec3", 0 },
	{ "dvec4", 0 },
	{ "half", 0 },
	{ "half2", 0 },
	{ "half3", 0 },
	{ "half4", 0 },
	{"uint64_t", 0 },
	{ "uint64_t2", 0 },
	{ "uint64_t3", 0 },
	{ "uint64_t4", 0 },
	{ "void", 0 },
	{ "mat2", 0 },
	{ "mat3", 0 },
	{ "mat4", 0 },
	{ "return", 0 },
	{ "Texture2D", 0 },
	{ "RWTexture2D", 0 },
	{ "Texture1D", 0 },
	{ "RWTexture1D", 0 },
	{ "static", 0 },
	{ "cbuffer", 0 },
	{ "register", 0 },
	{ "struct", 0 },
	{ "float4x4", 0 },
	
	{ "abort", 1 },
	{ "abs", 1 },
	{ "acos", 1 },
	{ "all", 1 },
	{ "AllMemoryBarrier", 1 },
	{ "AllMemoryBarrierWithGroupSync", 1 },
	{ "any", 1 },
	{ "asdouble", 1 },
	{ "asfloat", 1 },
	{ "asin", 1 },
	{ "asint", 1 },
	{ "asint", 1 },
	{ "asuint", 1 },
	{ "asuint", 1 },
	{ "atan", 1 },
	{ "atan2", 1 },
	{ "ceil", 1 },
	{ "CheckAccessFullyMapped", 1 },
	{ "clamp", 1 },
	{ "clip", 1 },
	{ "cos", 1 },
	{ "cosh", 1 },
	{ "countbits", 1 },
	{ "cross", 1 },
	{ "D3DCOLORtoUBYTE4", 1 },
	{ "ddx", 1 },
	{ "ddx_coarse", 1 },
	{ "ddx_fine", 1 },
	{ "ddy", 1 },
	{ "ddy_coarse", 1 },
	{ "ddy_fine", 1 },
	{ "degrees", 1 },
	{ "determinant", 1 },
	{ "DeviceMemoryBarrier", 1 },
	{ "DeviceMemoryBarrierWithGroupSync", 1 },
	{ "distance", 1 },
	{ "dot", 1 },
	{ "dst", 1 },
	{ "errorf", 1 },
	{ "EvaluateAttributeAtCentroid", 1 },
	{ "EvaluateAttributeAtSample", 1 },
	{ "EvaluateAttributeSnapped", 1 },
	{ "exp", 1 },
	{ "exp2", 1 },
	{ "f16tof32", 1 },
	{ "f32tof16", 1 },
	{ "faceforward", 1 },
	{ "firstbithigh", 1 },
	{ "firstbitlow", 1 },
	{ "floor", 1 },
	{ "fma", 1 },
	{ "fmod", 1 },
	{ "frac", 1 },
	{ "frexp", 1 },
	{ "fwidth", 1 },
	{ "GetRenderTargetSampleCount", 1 },
	{ "GetRenderTargetSamplePosition", 1 },
	{ "GroupMemoryBarrier", 1 },
	{ "GroupMemoryBarrierWithGroupSync", 1 },
	{ "InterlockedAdd", 1 },
	{ "InterlockedAnd", 1 },
	{ "InterlockedCompareExchange", 1 },
	{ "InterlockedCompareStore", 1 },
	{ "InterlockedExchange", 1 },
	{ "InterlockedMax", 1 },
	{ "InterlockedMin", 1 },
	{ "InterlockedOr", 1 },
	{ "InterlockedXor", 1 },
	{ "isfinite", 1 },
	{ "isinf", 1 },
	{ "isnan", 1 },
	{ "ldexp", 1 },
	{ "length", 1 },
	{ "lerp", 1 },
	{ "lit", 1 },
	{ "log", 1 },
	{ "log10", 1 },
	{ "log2", 1 },
	{ "mad", 1 },
	{ "max", 1 },
	{ "min", 1 },
	{ "modf", 1 },
	{ "msad4", 1 },
	{ "mul", 1 },
	{ "noise", 1 },
	{ "normalize", 1 },
	{ "pow", 1 },
	{ "printf", 1 },
	{ "Process2DQuadTessFactorsAvg", 1 },
	{ "Process2DQuadTessFactorsMax", 1 },
	{ "Process2DQuadTessFactorsMin", 1 },
	{ "ProcessIsolineTessFactors", 1 },
	{ "ProcessQuadTessFactorsAvg", 1 },
	{ "ProcessQuadTessFactorsMax", 1 },
	{ "ProcessQuadTessFactorsMin", 1 },
	{ "ProcessTriTessFactorsAvg", 1 },
	{ "ProcessTriTessFactorsMax", 1 },
	{ "ProcessTriTessFactorsMin", 1 },
	{ "radians", 1 },
	{ "rcp", 1 },
	{ "reflect", 1 },
	{ "refract", 1 },
	{ "reversebits", 1 },
	{ "round", 1 },
	{ "rsqrt", 1 },
	{ "saturate", 1 },
	{ "sign", 1 },
	{ "sin", 1 },
	{ "sincos", 1 },
	{ "sinh", 1 },
	{ "smoothstep", 1 },
	{ "sqrt", 1 },
	{ "step", 1 },
	{ "tan", 1 },
	{ "tanh", 1 },
	{ "tex1D", 1 },
	{ "tex1D", 1 },
	{ "tex1Dbias", 1 },
	{ "tex1Dgrad", 1 },
	{ "tex1Dlod", 1 },
	{ "tex1Dproj", 1 },
	{ "tex2D", 1 },
	{ "tex2D", 1 },
	{ "tex2Dbias", 1 },
	{ "tex2Dgrad", 1 },
	{ "tex2Dlod", 1 },
	{ "tex2Dproj", 1 },
	{ "tex3D", 1 },
	{ "tex3D", 1 },
	{ "tex3Dbias", 1 },
	{ "tex3Dgrad", 1 },
	{ "tex3Dlod", 1 },
	{ "tex3Dproj", 1 },
	{ "texCUBE", 1 },
	{ "texCUBE", 1 },
	{ "texCUBEbias", 1 },
	{ "texCUBEgrad", 1 },
	{ "texCUBElod", 1 },
	{ "texCUBEproj", 1 },
	{ "transpose", 1 },
	{ "trunc", 1 },

	{ "#define", 2 },
	{ "#include", 2 },
	{ "if" , 2 },
	{ "else", 2 },
	{ "do", 2 },
	{ "break", 2 },
	{ "continue", 2 },
	{ "switch", 2 },
	{ "while", 2 },
	{ "discard", 2 },
	{ "for", 2 }
	};
	for (int i = 0; i < sizeof(init_value) / sizeof KeyWord; i++)
	{
		type_table.push_back(init_value[i]);
	}

	std::sort(type_table.begin(), type_table.end(), compEx);

	timerHandle = CreateWaitableTimer(NULL, TRUE, NULL);

	hThread = CreateThread(
		nullptr,
		0,
		reinterpret_cast<LPTHREAD_START_ROUTINE>(DrawThread),
		this,
		CREATE_SUSPENDED,
		nullptr);

	ResumeThread(hThread);
}


HLSLEdit::~HLSLEdit()
{
}


BOOL InfoPanel::Create(
	DWORD		in_dwStyle,
	const RECT& in_rcRect,
	CWnd*		in_pParentWnd,
	UINT		in_nID)
{
	if (!::AfxInitRichEditEx())
	{
		return FALSE;
	}

	CWnd* l_pWnd = this;

	BOOL ret = l_pWnd->Create(_T("RichEdit20A"), NULL, in_dwStyle, in_rcRect, in_pParentWnd, in_nID);

	CClientDC dc(this);

	int tabSize = 7;
	int tabTwips = int(dc.GetTextExtent(" ", 1).cx) * tabSize * 1440 / dc.GetDeviceCaps(LOGPIXELSX);

	PARAFORMAT pf;
	pf.dwMask = PFM_TABSTOPS;
	pf.cTabCount = MAX_TAB_STOPS;
	for (int i = 0; i < MAX_TAB_STOPS; i++)
	{
		pf.rgxTabs[i] = tabTwips * (i + 1);
	}

	pf.cbSize = sizeof(PARAFORMAT);
	SetParaFormat(pf);

	return ret;
}


void HLSLEdit::UpdateDoc()
{

	SetRedraw(FALSE);

//	std::chrono::high_resolution_clock::time_point t0 = std::chrono::high_resolution_clock::now();

//	CHARFORMAT2 cf;
//	long lMinSel, lMaxSel;
	int nOldFirstVisibleLine = GetFirstVisibleLine();

	CPoint pos = GetCaretPos();
	int cursor = CharFromPos(pos);
	SetSel(cursor, cursor);

	SetColor(0, -1, RGB(200, 200, 200));
	SetBGColor(0, -1, RGB(32,32,32));

	// Prevent the auto-scroll of the control when calling SetSel()
	int nNewFirstVisibleLine = GetFirstVisibleLine();

	if (nOldFirstVisibleLine != nNewFirstVisibleLine)
		LineScroll(nOldFirstVisibleLine - nNewFirstVisibleLine);

	int num_lines = GetLineCount();
	bool comment = false;
	char Buffer[1024];

	for (int l = 0; l < num_lines; l++)
	{
		GetLine(l, Buffer, sizeof Buffer);
		comment = ParseLine(l, Buffer, comment);
	}

//	std::chrono::milliseconds t2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t0);

	SetSel(cursor, cursor);
	SetRedraw(TRUE);
	RedrawWindow();
}

// The one and only CTextEditApp object

CTextEditApp theApp;

// CTextEditApp initialization

class CParseCommandLineInfo : public CCommandLineInfo
{
private:
	enum CommandTypes
	{
		eNone = -1,
		eShader = 0,
		eEntryPoint = 1,
		eErrorFile = 2,
		eErrorPoint = 3,
		eErrorMsg = 4
	};
	CommandTypes m_nNextCommandType;

public:
	CParseCommandLineInfo(CMainFrame *Editor)
	{
		m_nNextCommandType = eNone;
		m_strFileName = "";
		m_Editor = Editor;
	}

	virtual void ParseParam(const char* pszParam, BOOL bFlag, BOOL bLast)
	{
		if (m_nNextCommandType == eNone)
		{
			if (pszParam[0] == 's')
				m_nNextCommandType = eShader;
			else if (pszParam[0] == 'p')
				m_nNextCommandType = eEntryPoint;
/*
			if (pszParam[0] == 'f')
				m_nNextCommandType = eErrorFile;
			else if (pszParam[0] == 'l')
				m_nNextCommandType = eErrorPoint;
			else if (pszParam[0] == 'e')
				m_nNextCommandType = eErrorMsg;
*/
		}
		else
		{
			switch (m_nNextCommandType)
			{
			case eShader:
				m_Editor->m_strShaderName = pszParam;
				break;
			case eEntryPoint:
				m_Editor->m_entryPoint = pszParam;
				break;
/*
			case eErrorFile:
				m_Editor->m_strInputFileName = pszParam;
				break;
			case eErrorPoint:
				m_Editor->m_errorPos = pszParam;
				break;
			case eErrorMsg:
				m_Editor->m_Error_line = pszParam;
				break;
*/
			}
			m_nNextCommandType = eNone;
		}
	}
private:
	CMainFrame *m_Editor;
};

BOOL CTextEditApp::InitInstance()
{
	InitCommonControls();

	AfxInitRichEdit2();

	CWinApp::InitInstance();

	CMainFrame* pFrame = new CMainFrame;
	// To create the main window, this code creates a new frame window
	// object and then sets it as the application's main window object

	m_pMainWnd = pFrame;

	CParseCommandLineInfo objParseCommandLineInfo(pFrame);
	ParseCommandLine(objParseCommandLineInfo);

	// create and load the frame with its resources

	pFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, NULL);

	// The one and only window has been initialized, so show and update it
//	pFrame->DrawMenuBar();
	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();
	pFrame->ReadFile();
	if(pFrame->m_strShaderName.GetLength())
		pFrame->OnCompile();
/*
	int line, pos, len;
	sscanf_s(pFrame->m_errorPos, "%d,%d-%d", &line, &pos, &len);

	int i = pFrame->m_RichEdit.LineIndex(line-1) + pos - 1;

	CHARFORMAT cf = { sizeof(cf) };

	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = RGB(255, 80, 80);
	pFrame->m_RichEdit.SetSel(i, i+len);
	BOOL res = pFrame->m_RichEdit.SetSelectionCharFormat(cf);

	pFrame->m_RichEdit.SetSel(i, i);

	pFrame->SetMessage(pFrame->m_Error_line);
*/
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CTextEditApp message handlers

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
		// No message handlers
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CTextEditApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CFindDlg::CFindDlg(CWnd* pParent /*=NULL*/)
	: CFindReplaceDialog()
{
	//{{AFX_DATA_INIT(CFindDlg)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CFindDlg::DoDataExchange(CDataExchange* pDX)
{
	CFindReplaceDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFindDlg)
	// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFindDlg, CFindReplaceDialog)
	//{{AFX_MSG_MAP(CFindDlg)
	ON_BN_CLICKED(IDC_CHECK1, OnCheck1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMyFindDlg message handlers

void CFindDlg::OnCheck1()
{
	AfxMessageBox("Wrap Around is checked...", MB_OK | MB_ICONINFORMATION);
	// TODO: Add your control notification handler code here
}


/////////////////////////////////////////////////////////////////////////////
// CTextEditApp message handlers


