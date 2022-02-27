#pragma once

#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

#define T_BINARY 128		//이진화 임계치 초기값
#define MAX_NUM 256			//레이블 상한값 (0:배경, 1~255:레이블)
#define MIN_LENGTH 30		//후보 영역의 최소 크기 임계치(> 2* DISPLACEMENT)
#define MAX_LENGTH 1000		//최소 거리 초기값 (>영상 크기)
#define DISPLACEMENT 5		//기울기 계산을 위한 변위, 코너 구간
#define T_SLOPE_CHANGE 5	//기울기 변화(수직거리 차이의 차이) 임계치

#define NW 0		//North-West 코너/꼭지점
#define NE 1		//North-East코너/꼭지점
#define SE 2		//South-West 코너/꼭지점
#define SW 3		//South-West 코너/꼭지점

#define TS 0		//Top Side 변
#define RS 1		//Right Side 변
#define BS 2		//Bottom Side 변
#define LS 3		//Left Side 변

#define M9 9		//마커 크기
#define B7 7		//원거리 경계 위치
#define B2 2		//근거리 경계 위치
#define OFFSET 0.35	//경계에서의 변위 (< 0.5)


class CMarkerProcess
{
public:
	CMarkerProcess(void);
	~CMarkerProcess(void);

public:
	int m_maxLabel;					//최대 유호 레이블
	CvRect m_MER[MAX_NUM];			// 마커 후보의 MER: Minimum Enclosing Rectangle (x,y,width, height)
	CvPoint m_Vertex[MAX_NUM][4];	//마커의 후보 꼭지점:[NW, NE, SE, SW] -> (x,y)

	void ImageBinarization(IplImage* image);		//영상 이진화 : 임계화 & 모폴로지
	void RegionSegmentation(IplImage* imgae);		//마커 후보 영역 분할 : 레이블링(8-이웃)
	void RegionFiltering(IplImage* image);			//마커 후보 영역 필터링 : MER의 크기/경계
	void ContourExtraction(IplImage* image);			//마커 후보 윤곽 추출:꼭지점 검출
	void Verification(IplImage* image);				//마커 검증 : 꼭지점 / MER / 테두리
	void PatternRecognition(IplImage* imageGray, IplImage* m_pImage, bool m_bLatticeCode, bool m_bColorCode, bool m_bTemplateMatching);
	void DrawRegion(IplImage* imageGray, IplImage* imageColor);		//마커 후보 영역 그리기

private:
	void SearchVertexAtCorner(IplImage* image, int nL, CvPoint ptCorner[]);
	void SearchVertexAtSide(IplImage* image, int nL, CvPoint ptReflect[], CvPoint ptRefract[]);
	void ProduceDistanceProfile(IplImage* image, int nL, int sID, int sideS, int sideL, int runS, int runL, int step, int nProfile[]);
	void SearchReflectiveVertex(IplImage* image, int sID, int sideS, int sideL, int runS, int step, int nProfile[], CvPoint ptReflect[]);
	void SearchRefractiveVertex(IplImage* image, int sID, int sideS, int sideL, int runS, int step, int nProfile[], CvPoint ptReflect[], CvPoint ptRefract[]);
	void RemoveOverlapVertext(CvPoint ptCorner[], CvPoint ptReflect[], CvPoint ptRefract[]);
	void AssignCornerVertexToMarker(int nL, CvPoint ptCorner[]);
	void AssignReflectiveVertexToMarker(int nL, CvPoint ptReflect[]);
	void AssignRefractiveVertexToMarker(int nL, CvPoint ptRefract[]);
	void RemoveIncompleteMarker(int nL);
	void VerifyMarkerVertext(IplImage* image);
	void VerifyMarkerMER(IplImage* image);
	void VerifyMarkerBorder(IplImage* image);
	BOOL CheckPixelLabel(IplImage* image, int x, int y, int nL);
	void RecognitionByLatticeCode(IplImage* imageGray);
	void RecognitionByColorCode(IplImage* imageGray);
	void RecognitionByTemplateMatching(IplImage* imageGray);

};

