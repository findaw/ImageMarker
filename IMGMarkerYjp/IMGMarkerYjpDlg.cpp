
// IMGMarkerYjpDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "IMGMarkerYjp.h"
#include "IMGMarkerYjpDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CIMGMarkerYjpDlg 대화 상자



CIMGMarkerYjpDlg::CIMGMarkerYjpDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_IMGMARKERYJP_DIALOG, pParent)
	, m_bLatticeCode(false)
	, m_bColorCode(false)
	, m_bTemplateMatching(false)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CIMGMarkerYjpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ST_DISPLAY, m_stDisplay);
	/*
	DDX_Control(pDX, IDC_LATTICE_CODE, m_bLatticeCode);
	DDX_Control(pDX, IDC_COLOR_CODE, m_bColorCode);
	DDX_Control(pDX, IDC_TEMPLATE_MATCHING, m_bTemplateMatching);
	*/
}

BEGIN_MESSAGE_MAP(CIMGMarkerYjpDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_AUG_DATA, &CIMGMarkerYjpDlg::OnBnClickedButton1)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_MONITOR, &CIMGMarkerYjpDlg::OnBnClickedMonitor)
	ON_BN_CLICKED(IDC_LATTICE_CODE, &CIMGMarkerYjpDlg::OnBnClickedLatticeCode)
	ON_BN_CLICKED(IDC_COLOR_CODE, &CIMGMarkerYjpDlg::OnBnClickedColorCode)
	ON_BN_CLICKED(IDC_TEMPLATE_MATCHING, &CIMGMarkerYjpDlg::OnBnClickedTemplateMatching)
END_MESSAGE_MAP()


// CIMGMarkerYjpDlg 메시지 처리기

BOOL CIMGMarkerYjpDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	//m_pCapture = new cv::VideoCapture(0);
	m_pCapture = cvCaptureFromCAM(CV_CAP_ANY);
	m_pImage = NULL;

	m_bMonitor = FALSE;
	
	m_bLatticeCode = FALSE;
	m_bColorCode = FALSE;
	m_bTemplateMatching = FALSE;

	SetTimer(CAM_TIMER, 10, NULL);

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CIMGMarkerYjpDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CIMGMarkerYjpDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		
		if (m_pImage) {
			CDC* pDC;
			CRect rect;


			pDC = m_stDisplay.GetDC();
			m_stDisplay.GetClientRect(&rect);

			m_vImage.CopyOf(m_pImage, m_pImage->nChannels*8);
			m_vImage.DrawToHDC(pDC->m_hDC, rect);

			ReleaseDC(pDC);
		}
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CIMGMarkerYjpDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CIMGMarkerYjpDlg::OnBnClickedButton1()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

	IplImage* img = cvLoadImage("pyj.jpg");

	if (img != NULL) {
		cvNamedWindow("증강사진");
		cvShowImage("증강사진", img);
		cvReleaseImage(&img);
	}

}


void CIMGMarkerYjpDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	if (m_pCapture != NULL) {
		m_pImage = cvQueryFrame(m_pCapture);
	}
	
	if (m_bMonitor) {
		CMarkerProcess marker;

		//웹캠 흑백 영상 생성 : 컬러 캡쳐영상을 흑백 영상으로 변환 복사
		IplImage* imageGray = cvCreateImage(cvSize(m_pImage->width, m_pImage->height), IPL_DEPTH_8U, 1);
		cvCvtColor(m_pImage, imageGray, CV_BGR2GRAY);

		//영상 이진화 : 임계화 & 모폴로지
		marker.ImageBinarization(imageGray);

		//마커 후보 영역 분할 : 레이블링(8-이웃)
		marker.RegionSegmentation(imageGray);

		//마커 후보 영역 필터링 : MER의 크기/경계
		marker.RegionFiltering(imageGray);

		//마커 후보 윤곽 추출 : 꼭지점 검출
		marker.ContourExtraction(imageGray);

		//마커 검증 : 마커의 MER / 꼭지점 / 테두리 검증
		marker.Verification(imageGray);

		//마커 패턴 인식 : 격자코드 / 컬러코드 / 형판 정합
		marker.PatternRecognition(imageGray, m_pImage, m_bLatticeCode, m_bColorCode, m_bTemplateMatching);

		//디스플레이 컬러 영상 생성 : 흑백 영상을 컬러 영상으로 변환 복사
		IplImage* imageColor = cvCreateImage(cvSize(imageGray->width, imageGray->height), IPL_DEPTH_8U, 3);
		cvCvtColor(imageGray, imageColor, CV_GRAY2BGR);

		marker.DrawRegion(imageGray, imageColor);		//마커 후보 영역 그리기
		cvShowImage("처리화면", imageColor);		//화면 디스플레이
			   
		cvReleaseImage(&imageColor);		//컬러 영상 해제
		cvReleaseImage(&imageGray);			//흑백 영상 해제

	}
	
	Invalidate(FALSE);

	CDialogEx::OnTimer(nIDEvent);
}

void CIMGMarkerYjpDlg::OnBnClickedMonitor()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_bMonitor = !m_bMonitor;

	if (m_bMonitor) {
		cvNamedWindow("처리화면");
	}
	else {
		cvDestroyWindow("처리화면");
	}
}

BOOL CIMGMarkerYjpDlg::DestroyWindow()
{
	// TODO: 여기에 특수화된 코드를 추가 및/또는 기본 클래스를 호출합니다.
	KillTimer(CAM_TIMER);

	if (m_pCapture != NULL) {
		cvReleaseCapture(&m_pCapture);
	}

	cvDestroyAllWindows();
	return CDialogEx::DestroyWindow();
}



void CIMGMarkerYjpDlg::OnBnClickedLatticeCode()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_bLatticeCode = TRUE;
	m_bColorCode = FALSE;
	m_bTemplateMatching = FALSE;

	UpdateData(FALSE);
}


void CIMGMarkerYjpDlg::OnBnClickedColorCode()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_bLatticeCode = FALSE;
	m_bColorCode = TRUE;
	m_bTemplateMatching = FALSE;

	UpdateData(FALSE);
}


void CIMGMarkerYjpDlg::OnBnClickedTemplateMatching()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	m_bLatticeCode = FALSE;
	m_bColorCode = FALSE;
	m_bTemplateMatching = TRUE;

	UpdateData(FALSE);
}
