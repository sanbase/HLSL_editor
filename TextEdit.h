// TextEdit.h : main header file for the TEXTEDIT application
//

#if !defined(AFX_TEXTEDIT_H__D30520A9_A3CB_40F3_B739_1124E3AF98F6__INCLUDED_)
#define AFX_TEXTEDIT_H__D30520A9_A3CB_40F3_B739_1124E3AF98F6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CTextEditApp:
// See TextEdit.cpp for the implementation of this class
//

class CTextEditApp : public CWinApp
{
public:
	CTextEditApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTextEditApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

public:
	//{{AFX_MSG(CTextEditApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEXTEDIT_H__D30520A9_A3CB_40F3_B739_1124E3AF98F6__INCLUDED_)
