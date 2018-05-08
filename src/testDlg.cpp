
// testDlg.cpp : implementation file
//

#include "stdafx.h"
#include "test.h"
#include "testDlg.h"
#include "afxdialogex.h"
#include "plot.h"

#include <iostream>
#include <array>
#include <numeric>
#include <atomic>
#include <thread>
#include <ctime>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define WM_UPDATEDATA_MESSAGE (WM_USER + 1000)

using namespace common;

namespace
{

    simple_list_plot input_plot, deconv_plot, impulse_response_plot, conv_plot;

    std::atomic<bool> working;
}

CtestDlg::CtestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CtestDlg::IDD, pParent)
    , sample_count(32)
    , mhj_accuracy(1e-6)
    , mhj_steps(100000)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    
    deconv_plot.color = RGB(180, 0, 0);

    input_deconv_plot_ctrl.plot_layer.with(
        ((plot::plot_builder() << deconv_plot) << input_plot)
        .with_ticks(plot::palette::pen(RGB(150, 150, 0)))
        .with_x_ticks(0, 10, 0)
        .with_y_ticks(0, 5, 5)
        .build()
    );
    impulse_response_plot_ctrl.plot_layer.with(
        (plot::plot_builder() << impulse_response_plot)
        .with_ticks(plot::palette::pen(RGB(150, 150, 0)))
        .with_x_ticks(0, 10, 0)
        .with_y_ticks(0, 5, 5)
        .build()
    );
    conv_plot_ctrl.plot_layer.with(
        (plot::plot_builder() << conv_plot)
        .with_ticks(plot::palette::pen(RGB(150, 150, 0)))
        .with_x_ticks(0, 10, 0)
        .with_y_ticks(0, 5, 5)
        .build()
    );

    input_params[0] = { 1, 3, 6 };
    input_params[1] = { 2, 1, 20 };
    input_params[2] = { 2, 1, 25 };
    impulse_response_params = { 1, 1, 0 };
}

void CtestDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_INPUT_DECONV_PLOT, input_deconv_plot_ctrl);
    DDX_Control(pDX, IDC_CONVOLUTION_PLOT, conv_plot_ctrl);
    DDX_Control(pDX, IDC_IMPULSE_RESPONSE_PLOT, impulse_response_plot_ctrl);
    DDX_Text(pDX, IDC_SAMPLE_COUNT, sample_count);
    DDX_Text(pDX, IDC_SNR, snr_percents);
    DDX_Text(pDX, IDC_REL_ERROR_X, x_error);
    DDX_Text(pDX, IDC_REL_ERROR_Y, y2_error);
    DDX_Text(pDX, IDC_MHJ_ACC, mhj_accuracy);
    DDX_Text(pDX, IDC_MHJ_STEPS, mhj_steps);
    DDX_Text(pDX, IDC_SIGNAL_A1, input_params[0].a);
    DDX_Text(pDX, IDC_SIGNAL_A2, input_params[1].a);
    DDX_Text(pDX, IDC_SIGNAL_A3, input_params[2].a);
    DDX_Text(pDX, IDC_SIGNAL_S1, input_params[0].s);
    DDX_Text(pDX, IDC_SIGNAL_S2, input_params[1].s);
    DDX_Text(pDX, IDC_SIGNAL_S3, input_params[2].s);
    DDX_Text(pDX, IDC_SIGNAL_t1, input_params[0].t0);
    DDX_Text(pDX, IDC_SIGNAL_t2, input_params[1].t0);
    DDX_Text(pDX, IDC_SIGNAL_t3, input_params[2].t0);
    DDX_Text(pDX, IDC_SIGNAL_A0, impulse_response_params.a);
    DDX_Text(pDX, IDC_SIGNAL_S0, impulse_response_params.s);
    DDX_Text(pDX, IDC_SIGNAL_t0, impulse_response_params.t0);
}

BEGIN_MESSAGE_MAP(CtestDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_START_PAUSE, &CtestDlg::OnBnClickedStartPause)
    ON_BN_CLICKED(IDC_STOP, &CtestDlg::OnBnClickedStop)
    ON_MESSAGE(WM_UPDATEDATA_MESSAGE, &CtestDlg::OnUpdateDataMessage)
END_MESSAGE_MAP()


// CtestDlg message handlers

BOOL CtestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    UpdateData(FALSE);
    UpdateData(TRUE);

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CtestDlg::OnPaint()
{
	CDialog::OnPaint();
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CtestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CtestDlg::OnBnClickedStartPause()
{
    bool expected = false;
    if (!working.compare_exchange_strong(expected, true))
    {
        return;
    }
    std::thread worker(&CtestDlg::DoWork, this);
    worker.detach();
}


void CtestDlg::OnBnClickedStop()
{
    working = false;
}


void CtestDlg::DoWork()
{
    DoWork0();
}


void CtestDlg::DoWork0()
{
    RequestUpdateData(TRUE);
    
    srand(clock());

    // setup periods and bounds

    sampled_t input_sampled = allocate_sampled(sample_count, 1);
    sampled_t impulse_response_sampled = allocate_sampled(sample_count, 1);
    sampled_t conv_sampled = allocate_sampled(sample_count, 1);
    sampled_t deconv_sampled = allocate_sampled(sample_count, 1);
    sampled_t noise_sampled = allocate_sampled(sample_count, 1);
    sampled_t lambda = allocate_sampled(sample_count, 1);

    continuous_t input_signals[] = { gaussian(input_params[0]), gaussian(input_params[1]), gaussian(input_params[2]) };
    sample(combine(3, input_signals), input_sampled);

    setup(input_plot, input_sampled);
    input_deconv_plot_ctrl.RedrawWindow();

    sample(gaussian(impulse_response_params), impulse_response_sampled, sample_count / 2);
    for (size_t i = 0; i < sample_count / 2; i++)
    {
        impulse_response_sampled.samples[sample_count - i - 1] = impulse_response_sampled.samples[i];
    }

    setup(impulse_response_plot, impulse_response_sampled);
    impulse_response_plot_ctrl.RedrawWindow();

    double signal_power = convolute(input_sampled, impulse_response_sampled, conv_sampled);
    double noise_power = sample(noise(), noise_sampled);
    double alpha = std::sqrt(snr_percents / 100. * signal_power / noise_power);
    map(conv_sampled, noise_sampled, [alpha](size_t, double a, double b) { return std::abs(a + b * alpha); });

    setup(conv_plot, conv_sampled);
    conv_plot_ctrl.RedrawWindow();

    double func_value;
    mhj_method::function functional = [this, &impulse_response_sampled, &conv_sampled, &deconv_sampled, &func_value] (double *lambda) {
        sampled_t lmb = { lambda, sample_count, 1 };
        convolute(lmb, impulse_response_sampled, deconv_sampled, [] (size_t, double s) { return std::exp(-1 - s); });
        double result = 0;
        for (size_t i = 0; i < sample_count; i++)
        {
            double e_i = conv_sampled.samples[i] - convolute(deconv_sampled, impulse_response_sampled, i);
            result += e_i * e_i;
        }
        func_value = result;
        return result;
    };

    mhj_method mhj(functional, { lambda.samples, sample_count }, mhj_accuracy, mhj_accuracy, mhj_steps);
    while(!++mhj && working)
    {
        setup(deconv_plot, deconv_sampled);
        input_deconv_plot_ctrl.RedrawWindow();
        x_error = 0;
        for (size_t i = 0; i < sample_count; i++)
        {
            double t = input_sampled.samples[i] - deconv_sampled.samples[i];
            x_error += t * t;
        }
        x_error /= sample_count;
        x_error = std::sqrt(x_error);
        y2_error = func_value;
        RequestUpdateData(FALSE);
    }

    setup(deconv_plot, deconv_sampled);
    input_deconv_plot_ctrl.RedrawWindow();

    free_sampled(noise_sampled);
    free_sampled(lambda);
    free_sampled(input_sampled);
    free_sampled(impulse_response_sampled);
    free_sampled(conv_sampled);
    free_sampled(deconv_sampled);

    working = false;
}


LRESULT CtestDlg::OnUpdateDataMessage(WPARAM wpD, LPARAM lpD)
{
    UpdateData(wpD == TRUE);
    return 0;
}

void CtestDlg::RequestUpdateData(BOOL saveAndValidate)
{
    SendMessage(WM_UPDATEDATA_MESSAGE, saveAndValidate);
}
