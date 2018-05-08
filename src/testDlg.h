
// testDlg.h : header file
//

#pragma once
#include "afxwin.h"
#include "afxcmn.h"

#include "common.h"
#include "mhj.h"

#include "PlotStatic.h"

// CtestDlg dialog
class CtestDlg : public CDialog
{
// Construction
public:
	CtestDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_TEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CWnd plot;
    afx_msg void OnBnClickedStartPause();
    afx_msg void OnBnClickedStop();
    PlotStatic input_deconv_plot_ctrl;
    PlotStatic conv_plot_ctrl;
    PlotStatic impulse_response_plot_ctrl;
    size_t sample_count;
    double snr_percents;
    double x_error;
    double y2_error;
    double mhj_accuracy;
    int mhj_steps;
    common::gaussian_t input_params[3];
    common::gaussian_t impulse_response_params;
    void DoWork();
    LRESULT OnUpdateDataMessage(WPARAM wpD, LPARAM lpD);
    void RequestUpdateData(BOOL saveAndValidate);
    void DoWork0();
};
