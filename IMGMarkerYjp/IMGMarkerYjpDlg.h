
// IMGMarkerYjpDlg.h: 헤더 파일
//

#pragma once
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>
#include "CvvImage.h"
#include "CMarkerProcess.h"

#define CAM_TIMER 1


// CIMGMarkerYjpDlg 대화 상자
class CIMGMarkerYjpDlg : public CDialogEx
{
// 생성입니다.
public:
	CIMGMarkerYjpDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_IMGMARKERYJP_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	DECLARE_MESSAGE_MAP()
public:
	CStatic m_stDisplay;

	CvCapture* m_pCapture;
	IplImage* m_pImage;
	CvvImage m_vImage;

	BOOL m_bMonitor;

	bool m_bLatticeCode;
	bool m_bColorCode;
	bool m_bTemplateMatching;

	afx_msg void OnBnClickedButton1();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	virtual BOOL DestroyWindow();
	afx_msg void OnBnClickedMonitor();
	
	afx_msg void OnBnClickedLatticeCode();
	afx_msg void OnBnClickedColorCode();
	afx_msg void OnBnClickedTemplateMatching();
};
