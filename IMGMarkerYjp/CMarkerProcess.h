#pragma once

#include <cv.h>
#include <cxcore.h>
#include <highgui.h>

#define T_BINARY 128		//����ȭ �Ӱ�ġ �ʱⰪ
#define MAX_NUM 256			//���̺� ���Ѱ� (0:���, 1~255:���̺�)
#define MIN_LENGTH 30		//�ĺ� ������ �ּ� ũ�� �Ӱ�ġ(> 2* DISPLACEMENT)
#define MAX_LENGTH 1000		//�ּ� �Ÿ� �ʱⰪ (>���� ũ��)
#define DISPLACEMENT 5		//���� ����� ���� ����, �ڳ� ����
#define T_SLOPE_CHANGE 5	//���� ��ȭ(�����Ÿ� ������ ����) �Ӱ�ġ

#define NW 0		//North-West �ڳ�/������
#define NE 1		//North-East�ڳ�/������
#define SE 2		//South-West �ڳ�/������
#define SW 3		//South-West �ڳ�/������

#define TS 0		//Top Side ��
#define RS 1		//Right Side ��
#define BS 2		//Bottom Side ��
#define LS 3		//Left Side ��

#define M9 9		//��Ŀ ũ��
#define B7 7		//���Ÿ� ��� ��ġ
#define B2 2		//�ٰŸ� ��� ��ġ
#define OFFSET 0.35	//��迡���� ���� (< 0.5)


class CMarkerProcess
{
public:
	CMarkerProcess(void);
	~CMarkerProcess(void);

public:
	int m_maxLabel;					//�ִ� ��ȣ ���̺�
	CvRect m_MER[MAX_NUM];			// ��Ŀ �ĺ��� MER: Minimum Enclosing Rectangle (x,y,width, height)
	CvPoint m_Vertex[MAX_NUM][4];	//��Ŀ�� �ĺ� ������:[NW, NE, SE, SW] -> (x,y)

	void ImageBinarization(IplImage* image);		//���� ����ȭ : �Ӱ�ȭ & ��������
	void RegionSegmentation(IplImage* imgae);		//��Ŀ �ĺ� ���� ���� : ���̺�(8-�̿�)
	void RegionFiltering(IplImage* image);			//��Ŀ �ĺ� ���� ���͸� : MER�� ũ��/���
	void ContourExtraction(IplImage* image);			//��Ŀ �ĺ� ���� ����:������ ����
	void Verification(IplImage* image);				//��Ŀ ���� : ������ / MER / �׵θ�
	void PatternRecognition(IplImage* imageGray, IplImage* m_pImage, bool m_bLatticeCode, bool m_bColorCode, bool m_bTemplateMatching);
	void DrawRegion(IplImage* imageGray, IplImage* imageColor);		//��Ŀ �ĺ� ���� �׸���

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

