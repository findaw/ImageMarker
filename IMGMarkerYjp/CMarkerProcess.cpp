#include "pch.h"
#include "CMarkerProcess.h"


CMarkerProcess::CMarkerProcess(void) {
	
	int nL, nY;

	//�ִ� ��ȿ ���̺� �ʱ�ȭ
	m_maxLabel = 0;

	//��Ŀ �ĺ��� MER �ʱ�ȭ:(x,y,width,height)
	for (nL = 0; nL < MAX_NUM; nL++) {
		m_MER[nL] = cvRect(0, 0, 0, 0);
	}

	//��Ŀ �ĺ��� ������ �ʱ�ȭ: (x,y)
	for (nL = 0; nL < MAX_NUM; nL++)
		for (nY = 0; nY < 4; nY++)
			m_Vertex[nL][nY] = cvPoint(-1, -1);
}

CMarkerProcess::~CMarkerProcess(void) {

}

void CMarkerProcess::ImageBinarization(IplImage* image) {

	//�Ӱ�ȭ : Otsu �˰���
	cvThreshold(image, image, T_BINARY, 255, CV_THRESH_BINARY_INV | CV_THRESH_OTSU);

	//�������� : Open & Close
	IplImage* imageTemp = cvCreateImage(cvSize(image->width, image->height), IPL_DEPTH_8U, 1);	//�ӽÿ��� ����
	IplConvKernel* element = cvCreateStructuringElementEx(3, 3, 1, 1, CV_SHAPE_RECT, NULL);		//���� ��� ����

	cvMorphologyEx(image, image, imageTemp, element, CV_MOP_OPEN, 1);			//����
	cvMorphologyEx(image, image, imageTemp, element, CV_MOP_CLOSE, 1);			//����

	cvReleaseStructuringElement(&element);		//������� ����
	cvReleaseImage(&imageTemp);					//�ӽÿ��� ����
}

void CMarkerProcess::RegionSegmentation(IplImage* image) {

	int i, j;									//�ε���
	int x, y, nx, ny;							//����/�̿� ȭ�� ��ǥ
	int nMERx, nMERy, nMERw, nMERh;				//�ּ� ���̺� ������ MER (x,y,width, height)
	int cMERx1, cMERy1, cMERx2, cMERy2;			//���� ���̺� ������ MER �»��/���ϴ� ��ǥ
	int dMERx1, dMERy1, dMERx2, dMERy2;			//���� ���̺� ������ MER �»��/���ϴ� ��ǥ
	int nLabel, nMinLabel, nL;					//���� ���̺� / �ּ� ���̺� / �����̿� ���̺�
	int vNeighborLabel[4];						//���� �̿��� ���̺� : 4-�̿�(NW, N, NE, W)
	BYTE uData;									//ȭ�Ұ�
	BOOL bConnected;							//���� �÷���
	int vLCT[MAX_NUM];							//���̺� ���� ���̺� (1~255:���̺�, 0:���)

	nLabel = 0;										//���̺� ��ȣ �ʱ�ȭ
	for (i = 0; i < MAX_NUM; i++)	vLCT[i] = 0;	// ���̺� ���� ���̺� �ʱ�ȭ


	for (y = 0; y < image->height; y++) {
		for (x = 0; x < image->width; x++) {

			uData = (BYTE)image->imageData[y * image->widthStep + x];		//ĳ����(BYTE) ����

			if (uData) {

				//�̿� ȭ���� ���̺� �� �ּ� ���̺� ����
				bConnected = FALSE;								//���� �÷��� �ʱ�ȭ
				nMinLabel = MAX_NUM;							//�ּ� ���̺� �ʱ�ȭ(256)
							
				for (i = 0; i < 4; i++)	vNeighborLabel[i] = 0;		//���� �̿� ���̺� �ʱ�ȭ

				//8-�̿� �߿���
				for (j = -1; j <= 1; j++) {
					for (i = -1; i <= 1; i++) {

						//���� 4-�̿��� ���� : NW, N, NE, W
						if ((j == -1) || (j == 0 && i == -1)) {

							nx = x + i;		//�̿��� x��ǥ
							ny = y + j;		//�̿��� y��ǥ

							//�̿� ȭ�Ұ� ���� ������ ���Եǰ�
							if ((nx >= 0) && (ny >= 0) && (nx < image->width) && (ny < image->height)) {

								uData = (BYTE)image->imageData[ny * image->widthStep + nx];

								//�̿� ȭ���� ���� ���̺�(1~255)�� ���
								if (uData > 0) {
									bConnected = TRUE;								//���� �÷��� ����
									if (uData < nMinLabel)	nMinLabel = uData;		//�ּ� ���̺� ����
									vNeighborLabel[(j + 1) * 3 + (i + 1)] = uData;		//���� �̿� ���̺�(0, 1,2,3) 
								}
							}
						}
					}
				}


				//����� ���� �̿��� ���� ��� : ���ο� ��Ŀ �ĺ� ���
				if (!bConnected) {

					nLabel++;					//���̺� ����
					if (nLabel >= MAX_NUM) nLabel = MAX_NUM - 1;		//���̺��� �ִ밪�� 255�� ����
					image->imageData[y * image->widthStep + x] = (BYTE)nLabel;	//���� ȭ���� ���� ���̺�� ����

					m_MER[nLabel] = cvRect(x, y, 1, 1);		//��Ŀ�� MER ����
					vLCT[nLabel] = nLabel;			//���̺� ���� ���̺��� �ش� �׸��� �ڽ��� ���̺�� ����


				}
				//����� ���� �̿��� �ִ� ��� : ��Ŀ �ĺ��� MER ����
				else {

					//���� ȭ�Ҹ� �ּ� ������ ���Խ�Ŵ
					image->imageData[y * image->widthStep + x] = (BYTE)nMinLabel;	//���� ȭ���� ���� ����� �ּ� ���̺�� ����

					nMERx = m_MER[nMinLabel].x;		//�ּ� ���̺� ������ MER x��ǥ
					nMERy = m_MER[nMinLabel].y;		//�ּ� ���̺� ������ MER y��ǥ
					nMERw = m_MER[nMinLabel].width;		//�ּ� ���̺� ������ MER ��
					nMERh = m_MER[nMinLabel].height;		//�ּ� ���̺� ������ MER ����


					if (x < nMERx) {	//���� ȭ�Ұ� ���� MER�� ������ ��ġ�ϴ� ���

						m_MER[nMinLabel].x = x;				//�ּ� ���̺� ������ MER x��ǥ ����
						m_MER[nMinLabel].width = nMERw + (nMERx - x);		//�ּ� ���̺� ������ MER �� ����

					}
					else if (x > nMERx + nMERw - 1) {		//���� ȭ�Ұ� ���� MER�� ������ ��ġ�ϴ� ���

						m_MER[nMinLabel].width = x - nMERx + 1;		//�ּ� ���̺� ������ MER �� ����

					}
					else if (y < nMERy) {		//���� ȭ�Ұ� ���� MER�� ������ ��ġ�ϴ� ���

						m_MER[nMinLabel].y = y;				//�ּ� ���̺� ������ MER y��ǥ ����
						m_MER[nMinLabel].height = nMERh + (nMERy - y);		//�ּ� ���̺� ������ MER ���� ����

					}
					else if (y > nMERy + nMERh - 1) {		//���� ȭ�Ұ� ���� MER�� ������ ��ġ�ϴ� ���

						m_MER[nMinLabel].height = y - nMERy + 1;		//�ּ� ���̺� ������ MER shvdl ����

					}

					//���� ȭ�ҿ� ����� ������ ������ ���� ���̺� ���� ���̺� ����
					for (i = 0; i < 4; i++) {

						nL = vNeighborLabel[i];		//���� �̿� ���̺� ����


						if ((nL > 0) && (vLCT[nL] > nMinLabel)) {	//���� �̿� ���̺��� ��ȿ�ϰ�, ���̺� ���� ���̺��� ���� �ּ� ���̺��� ū ���
							vLCT[nL] = nMinLabel;	//�ּ� ���̺�� ����
						}

					}
				}
			}
		}
	}



	//2 �ܰ� : ���̺� ������迡 ���� ���� ���� ����

	//���̺� ���� ���̺��� �������� ����ȭ : ���� ���̺�� ���� ����
	for (i = 1; i <= nLabel; i++) {
		j = i;		//���� ���̺��� Ž�� ���̺�� ����
		while (j > vLCT[j]) {	//Ž�� ���̺��� �� ���� ���̺� ����Ǿ� �ִ� ���
			j = vLCT[j];		//����� ���̺��� ��� ����
		}
		vLCT[i] = j;		//���� ������ �ּ� ���̺��� ���� ���̺� ���� ����
	}

	//���� ������ ȭ�� ���̺� ����
	for (y = 0; y < image->height; y++) {
		for (x = 0; x < image->width; x++) {		//�� ȭ�ҿ� ����

			uData = (BYTE)image->imageData[y * image->widthStep + x];

			if (uData > 0) {		//���� ȭ���� ���� ���̺�(1~255) �̰�
				if (vLCT[uData] < uData) {	//���� ȭ���� ���̺��� �� ���� ���̺� ����Ǿ� �ִ� ���
					image->imageData[y * image->widthStep + x] = (BYTE)vLCT[uData];	//���� ȭ���� ���̺� ����
				}
			}
		}
	}

	//���� ������ MER ����
	for (i = 1; i <= nLabel; i++) {		
		if (i > vLCT[i]) {		//���� ���̺��� �� ���� ���̺� ����Ǿ� �ִ� ���

			j = vLCT[i];		//���� ���̺��� �� ���� ���̺� ����Ǿ� �ִ� ���

			cMERx1 = m_MER[i].x;					//���� ���̺� ������ MER �»�� x��ǥ
			cMERy1 = m_MER[i].y;					//���� ���̺� ������ MER �»�� y��ǥ
			cMERx2 = cMERx1 + m_MER[i].width - 1;	//���� ���̺� ������ MER ���ϴ� x��ǥ
			cMERy2 = cMERy1 + m_MER[i].height - 1;	//���� ���̺� ������ MER ���ϴ� y��ǥ

			dMERx1 = m_MER[j].x;			//���� ���̺� ������ MER �»�� x��ǥ
			dMERy1 = m_MER[j].y;			//���� ���̺� ������ MER �»�� y��ǥ
			dMERx2 = dMERx1 + m_MER[j].width - 1;			//���� ���̺� ������ MER ���ϴ� x��ǥ
			dMERy2 = dMERy1 + m_MER[j].height - 1;			//���� ���̺� ������ MER ���ϴ� y��ǥ

			if (cMERx1 < dMERx1) {			//MER ���� Ȯ���� ���
				m_MER[j].x = cMERx1;				//���� ���̺� ������ MER x��ǥ ����
				m_MER[j].width = dMERx2 - cMERx1 + 1;		//���� ���̺� ������ MER �� ����
			}
			if (cMERy1 < dMERy1) {		//MER ���� Ȯ���� ���
				m_MER[j].y = cMERy1;					//���� ���̺� ������ MER y ��ǥ ����
				m_MER[j].height = dMERy2 - cMERy1 + 1;		//���� ���Ը� ������ MER ���� ����
			}
			if (cMERx2 > dMERx2) {		//MER ���� Ȯ���� ���
				m_MER[j].width = cMERx2 - m_MER[j].x + 1;		//���� ���̺� ������ MER �� ����
			}
			if (cMERy2 > dMERy2) {
				m_MER[j].height = cMERy2 - m_MER[j].y + 1;		//���� ���̺� ������ MER ���� ����
			}

			m_MER[i] = cvRect(0, 0, 0, 0);		//���� ���̺� ������ MER ����
			vLCT[i] = 0;						//���̺� ���� ���̺��� ���� ���̺� ����

		}
	}

	m_maxLabel = nLabel;		//�ִ� ��ȣ ���̺� ����

}

void CMarkerProcess::RegionFiltering(IplImage* image) {

	int nL;				//���̺� �ε���
	int nWidth, nHeight;			//MER ���� ����
	int nMERx1, nMERy1, nMERx2, nMERy2;		//MER�� �»��/���ϴ� ��ǥ

	//�ĺ� ������ ũ�� ���͸�
	for (nL = 1; nL <= m_maxLabel; nL++) {
		nWidth = m_MER[nL].width;
		nHeight = m_MER[nL].height;

		if ((nWidth > 0) && (nHeight > 0)) {		//�ĺ� ������ �����ϰ�,

			//�ĺ� ���� MER ũ�Ⱑ ��ȿ���� ���� �������
			if ((nWidth < MIN_LENGTH) || (nHeight < MIN_LENGTH)) {	
				m_MER[nL] = cvRect(0, 0, 0, 0);		//���� ���̺� ������ MER ����
			}

		}
	}

	//�ĺ� ������ ��� ���͸�
	for (nL = 1; nL <= m_maxLabel; nL++) {
		nWidth = m_MER[nL].width;
		nHeight = m_MER[nL].height;

		if ((nWidth > 0) && (nHeight > 0)) {		//�ĺ� ������ �����ϰ�,

			nMERx1 = m_MER[nL].x;		//���� ���̺� ������ MER �»�� x��ǥ
			nMERy1 = m_MER[nL].y;		//���� ���̺� ������ MER �»�� y��ǥ

			nMERx2 = nMERx1 + m_MER[nL].width -1;		//���� ���̺� ������ MER ���ϴ� x��ǥ
			nMERy2 = nMERy1 + m_MER[nL].height -1;		//���� ���̺� ������ MER ���ϴ� y��ǥ

			//�ĺ� ���� MER�� ���� ��躸�� ���ʿ� ��ġ���� ������� ����
			if (!(nMERx1 > 0) && (nMERy1 > 0) && (nMERx2 < image->width - 1) && (nMERy2 < image->height - 1)) {
				m_MER[nL] = cvRect(0, 0, 0, 0);		//���� ���̺� ������ MER ����
			}

		}

	}

}

void CMarkerProcess::DrawRegion(IplImage* imageGray, IplImage* imageColor) {

	int nL;					//���̺� �ε���
	int x, y;				//ȭ�� ��ǥ
	BYTE uData;				//ȭ�Ұ�
	int nWidth, nHeight;			//MER ���� ����
	CvPoint pt1, pt2, pt3, pt4;		//MER�� ������
	CvScalar color;						//�÷�
	

	//��Ŀ �ĺ� ������ ȭ�� ǥ��
	for (y = 0; y < imageGray->height; y++) {
		for (x = 0; x < imageGray->width; x++) {		//�� ȭ�ҿ� ����

			uData = (BYTE)imageGray->imageData[y * imageGray->widthStep + x];

			if (uData > 0) {		//ȭ�Ұ��� ���̺�(1~255)�� ���
				imageColor->imageData[y * imageColor->widthStep + x * 3] = (BYTE)255;		//������(whilte)���� : B ����
				imageColor->imageData[y * imageColor->widthStep + x * 3+1] = (BYTE)255;		//������(whilte)���� : G ����
				imageColor->imageData[y * imageColor->widthStep + x * 3+2] = (BYTE)255;		//������(whilte)���� : R ����
			}
		}
	}

	//��Ŀ �ĺ� ������ MER ǥ��
	for (nL = 1; nL <= m_maxLabel; nL++) {		//�� ���̺� ����

		nWidth = m_MER[nL].width;		//MER�� �� ����
		nHeight = m_MER[nL].height;		//MER�� ���� ����

		if ((nWidth > 0) && (nHeight > 0)) {		//�ĺ� ������ �����ϴ� ���

			//��Ŀ �ĺ� ������ MER ǥ��
			color = cvScalar(0, 255, 0);			//MER ǥ�û� ���� : �ʷϻ�

			pt1 = cvPoint(m_MER[nL].x, m_MER[nL].y);			//MER�� �»��� ����
			pt2 = cvPoint(m_MER[nL].x + m_MER[nL].width -1, m_MER[nL].y + m_MER[nL].height -1);		//MER�� ������ ����

			cvDrawRect(imageColor, pt1, pt2, color);		//MER �׸���

			color = cvScalar(0, 0, 255);

			pt1 = m_Vertex[nL][NW];
			pt2 = m_Vertex[nL][NE];
			pt3 = m_Vertex[nL][SE];
			pt4 = m_Vertex[nL][SW];

			cvLine(imageColor, pt1, pt2, color, 2);
			cvLine(imageColor, pt2, pt3, color, 2);
			cvLine(imageColor, pt3, pt4, color, 2);
			cvLine(imageColor, pt4, pt1, color, 2);
		}
	}

}

//��Ŀ �ĺ� ���� ���� : ������ ����
void CMarkerProcess::ContourExtraction(IplImage* image) {

	int i;
	int nL;
	int nWidth, nHeight;
	CvPoint ptCorner[4];
	CvPoint ptReflect[4];
	CvPoint ptRefract[4];

	for (nL = 1; nL <= m_maxLabel; nL++) {
		nWidth = m_MER[nL].width;
		nHeight = m_MER[nL].height;

		if ((nWidth > 0) && (nHeight > 0)) {
			for (i = 0; i < 4; i++) {
				ptCorner[i] = cvPoint(-1, -1);
				ptReflect[i] = cvPoint(-1, -1);
				ptRefract[i] = cvPoint(-1, -1);

			}

			SearchVertexAtCorner(image, nL, ptCorner);
			SearchVertexAtSide(image, nL, ptReflect, ptRefract);

			RemoveOverlapVertext(ptCorner, ptReflect, ptRefract);

			AssignCornerVertexToMarker(nL, ptCorner);
			AssignReflectiveVertexToMarker(nL, ptReflect);
			AssignRefractiveVertexToMarker(nL, ptRefract);

			RemoveIncompleteMarker(nL);

		}
	}
}

//MER�� �ڳʿ��� �ڳ� ������ Ž��
void CMarkerProcess::SearchVertexAtCorner(IplImage* image, int nL, CvPoint ptCorner[]) {
	int cID;
	CvPoint cLocation;
	CvRect cRegion;
	int x, y;
	int minD, curD;
	BYTE uData;

	for (cID = 0; cID < 4; cID++) {
		switch (cID) {
		case NW:
			cLocation = cvPoint(m_MER[nL].x, m_MER[nL].y);
			cRegion = cvRect(m_MER[nL].x, m_MER[nL].y, DISPLACEMENT, DISPLACEMENT);
			break;
		case NE:
			cLocation = cvPoint(m_MER[nL].x + m_MER[nL].width - 1, m_MER[nL].y);
			cRegion = cvRect(m_MER[nL].x + m_MER[nL].width - DISPLACEMENT, m_MER[nL].y, DISPLACEMENT, DISPLACEMENT);
			break;
		case SE:
			cLocation = cvPoint(m_MER[nL].x + m_MER[nL].width - 1, m_MER[nL].y + m_MER[nL].height - 1);
			cRegion = cvRect(m_MER[nL].x + m_MER[nL].width - DISPLACEMENT, m_MER[nL].y + m_MER[nL].height - DISPLACEMENT, DISPLACEMENT, DISPLACEMENT);
			break;
		case SW:
			cLocation = cvPoint(m_MER[nL].x, m_MER[nL].y + m_MER[nL].height - 1);
			cRegion = cvRect(m_MER[nL].x, m_MER[nL].y + m_MER[nL].height - DISPLACEMENT, DISPLACEMENT, DISPLACEMENT);
			break;
		}

		minD = MAX_LENGTH;

		for (x = cRegion.x; x < cRegion.x + cRegion.width; x++) {
			for (y = cRegion.y; y < cRegion.y + cRegion.height; y++) {

				uData = (BYTE)image->imageData[y * image->widthStep + x];

				if (uData == nL) {
					curD = abs(x - cLocation.x) + abs(y - cLocation.y);
					if (curD < minD) {
						minD = curD;
						ptCorner[cID] = cvPoint(x, y);
					}
				}
			}
		}


	}
}

//MER�� ������ �ݻ� �� ���� ������ Ž��
void CMarkerProcess::SearchVertexAtSide(IplImage* image, int nL, CvPoint ptReflect[], CvPoint ptRefract[]) {
	
	int sID;
	int sideS, sideL;
	int runS, runL;
	int step;
	int nProfile[MAX_LENGTH];

	for (sID = 0; sID < 4; sID++) {
		switch (sID) {
		case TS:
			sideS = m_MER[nL].x;
			sideL = m_MER[nL].width;
			runS = m_MER[nL].y;
			runL = m_MER[nL].height;
			step = 1;
			break;
		case RS:
			sideS = m_MER[nL].y;
			sideL = m_MER[nL].height;
			runS = m_MER[nL].x + m_MER[nL].width -1;
			runL = m_MER[nL].width;
			step = -1;
			break;
		case BS:
			sideS = m_MER[nL].x;
			sideL = m_MER[nL].width;
			runS = m_MER[nL].y + m_MER[nL].height -1;
			runL = m_MER[nL].height;
			step = -1;
			break;
		case LS:
			sideS = m_MER[nL].y;
			sideL = m_MER[nL].height;
			runS = m_MER[nL].x;
			runL = m_MER[nL].width;
			step = 1;
			break;

		}

		// �����Ÿ� �������� ���� : �� ������ ��üȭ�ұ����� �����Ÿ� �������� ����
		ProduceDistanceProfile(image, nL, sID, sideS, sideL, runS, runL, step, nProfile);

		//�ݻ� ������ Ž�� : �� ������ ���� ��ȭ�� ���� ū �ݻ������� ������ Ž��
		SearchReflectiveVertex(image, sID, sideS, sideL, runS, step, nProfile, ptReflect);

		//���� ������ Ž�� : �� ������ ���� ��ȭ�� ���� ū ���������� ������ Ž��
		SearchRefractiveVertex(image, sID, sideS, sideL, runS, step, nProfile, ptReflect, ptRefract);

	}
}

//�����Ÿ� �������� ���� : �� ������ ��üȭ�ұ����� �����Ÿ� �����θ� ����
void CMarkerProcess::ProduceDistanceProfile(IplImage* image, int nL, int sID, int sideS, int sideL, int runS, int runL, int step, int nProfile[]){

	int i;
	int x, y;
	BYTE uData;

	for (i = 0; i < MAX_LENGTH; i++) nProfile[i] = 0;

	switch (sID) {
	case TS:
	case BS:
		for (x = sideS; x < sideS + sideL; x++) {
			for (i = 0, y = runS; i < runL; i++, y += step) {
				uData = (BYTE)image->imageData[y * image->widthStep + x];
				if (uData == nL)
					break;
			}
			nProfile[x] = abs(y - runS);
		}
		break;

	case RS:
	case LS:
		for (y = sideS; y < sideS + sideL; y++) {
			for (i = 0, x = runS; i < runL; i++, x += step) {
				uData = (BYTE)image->imageData[y * image->widthStep + x];
				if (uData == nL)
					break;
			}
			nProfile[y] = abs(x - runS);
		}
		break;

	}

}

//�ݻ� ������ Ž�� : �� ������ ���� ��ȭ�� ���� ū �ݻ������� ������ Ž��
void CMarkerProcess::SearchReflectiveVertex(IplImage* image, int sID, int sideS, int sideL, int runS, int step, int nProfile[], CvPoint ptReflect[]) {

	int i;
	int d0, d1, d2;
	int curValue, maxValue;
	int maxSideLoc, maxRunLoc;

	maxValue = 0;

	for (i = sideS + DISPLACEMENT; i < (sideS + sideL - 2 * DISPLACEMENT); i++) {
		d0 = nProfile[i];
		d1 = nProfile[i - DISPLACEMENT];
		d2 = nProfile[i + DISPLACEMENT];

		if ((d0 <= d1) && (d0 <= d2)) {
			curValue = abs((d2 - d0) - (d0 - d1));

			if (curValue > maxValue) {
				maxValue = curValue;
				maxSideLoc = i;
				maxRunLoc = d0;
			}
		}
	}

	if (maxValue >= T_SLOPE_CHANGE) {
		switch (sID) {
		case TS:
		case BS:
			ptReflect[sID].x = maxSideLoc;
			ptReflect[sID].y = runS + step * maxRunLoc;

			break;
		case RS:
		case LS:
			ptReflect[sID].x = runS + step * maxRunLoc;
			ptReflect[sID].y = maxSideLoc;
			break;

		}
	}
}

//���� ������ Ž�� : �� ������ ���� ��ȭ�� ���� ū ���������� ������ Ž��
void CMarkerProcess::SearchRefractiveVertex(IplImage* image, int sID, int sideS, int sideL, int runS, int step, int nProfile[], CvPoint ptReflect[], CvPoint ptRefract[]) {
	
	int i;
	int d0, d1, d2;
	int curValue, maxValue;
	int maxSideLoc, maxRunLoc;
	int preSideLoc;

	maxValue = 0;

	switch (sID) {
	case TS:
	case BS:
		preSideLoc = ptReflect[sID].x;
		break;
	case RS:
	case LS:
		preSideLoc = ptReflect[sID].y;
		break;

	}

	for (i = sideS + DISPLACEMENT; i < (sideS + sideL - 2 * DISPLACEMENT); i++) {
		if ((preSideLoc >= 0) && (abs(preSideLoc - i)) < DISPLACEMENT) {
			i = preSideLoc + DISPLACEMENT;
				continue;
		}

		d0 = nProfile[i];
		d1 = nProfile[i - DISPLACEMENT];
		d2 = nProfile[i + DISPLACEMENT];

		if ((d0 > d1) || (d0 > d2)) {
			curValue = abs((d2 - d0) - (d0 - d1));

			if (curValue > maxValue) {
				maxValue = curValue;
				maxSideLoc = i;
				maxRunLoc = d0;
			}
		}
	}

	if (maxValue >= T_SLOPE_CHANGE) {
		switch (sID) {
		case TS:
		case BS:
			ptRefract[sID].x = maxSideLoc;
			ptRefract[sID].y = runS + step * maxRunLoc;

			break;
		case RS:
		case LS:
			ptRefract[sID].x = runS + step * maxRunLoc;
			ptRefract[sID].y = maxSideLoc;
			break;

		}
	}

}

//�ߺ� ������ ����
void CMarkerProcess::RemoveOverlapVertext(CvPoint ptCorner[], CvPoint ptReflect[], CvPoint ptRefract[]) {

	int i, j;
	int nL;
	int nWidth, nHeight;

	for (nL = 1; nL <= m_maxLabel; nL++) {
		nWidth = m_MER[nL].width;
		nHeight = m_MER[nL].height;

		if ((nWidth > 0) && (nHeight > 0)) {

			for (i = 0; i < 4; i++) {
				if ((ptReflect[i].x >= 0) && (ptReflect[i].y >= 0)) {

					for (j = 0; j < 4; j++) {
						if ((ptCorner[j].x >= 0) && (ptCorner[j].y >= 0)) {

							if (abs(ptReflect[i].x - ptCorner[j].x) < DISPLACEMENT &&
								(ptReflect[i].y - ptCorner[j].y) < DISPLACEMENT) {

								ptReflect[i] = cvPoint(-1, -1);
								break;
							}

						}

						
					}
				}
			}

			for (i = 0; i < 4; i++) {
				if ((ptRefract[i].x >= 0) && (ptRefract[i].y >= 0)) {

					for (j = 0; j < 4; j++) {
						if ((ptCorner[j].x >= 0) && (ptCorner[j].y >= 0)) {

							if (abs(ptRefract[i].x - ptCorner[j].x) < DISPLACEMENT &&
								(ptRefract[i].y - ptCorner[j].y) < DISPLACEMENT) {

								ptRefract[i] = cvPoint(-1, -1);
								break;
							}

						}
						if ((ptReflect[j].x >= 0) && (ptReflect[j].y >= 0)) {

							if (abs(ptRefract[i].x - ptReflect[j].x) < DISPLACEMENT &&
								(ptRefract[i].y - ptReflect[j].y) < DISPLACEMENT) {

								ptRefract[i] = cvPoint(-1, -1);
								break;
							}

						}
					}
				}
			}


		}
	}
}

//�ڳ� �������� ��Ŀ�� ����
void CMarkerProcess::AssignCornerVertexToMarker(int nL, CvPoint ptCorner[]) {
	int cID;
	for (cID = 0; cID < 4; cID++) {
		if ((ptCorner[cID].x >= 0) && (ptCorner[cID].y >= 0)) {
			m_Vertex[nL][cID] = ptCorner[cID];
		}
	}
}


//�ݻ� �������� ��Ŀ�� ����
void CMarkerProcess::AssignReflectiveVertexToMarker(int nL, CvPoint ptReflect[]) {

	int cID, nID;
	CvPoint cLoc, nLoc;
	int cDist, nDist;

	for (cID = 0; cID < 4; cID++) {
		if (ptReflect[cID].x >= 0) {
			nID = (cID + 1) % 4;

			switch (cID) {
			case TS:
				cLoc = cvPoint(m_MER[nL].x, m_MER[nL].y);
				nLoc = cvPoint(m_MER[nL].x + m_MER[nL].width - 1, m_MER[nL].y);
				break;
			case RS:
				cLoc = cvPoint(m_MER[nL].x + m_MER[nL].width -1, m_MER[nL].y);
				nLoc = cvPoint(m_MER[nL].x + m_MER[nL].width - 1, m_MER[nL].y + m_MER[nL].height -1);
				break;
			case BS:
				cLoc = cvPoint(m_MER[nL].x + m_MER[nL].width - 1, m_MER[nL].y + m_MER[nL].height - 1);
				nLoc = cvPoint(m_MER[nL].x, m_MER[nL].y + m_MER[nL].height - 1);
				break;

			case LS:
				cLoc = cvPoint(m_MER[nL].x, m_MER[nL].y + m_MER[nL].height - 1);
				nLoc = cvPoint(m_MER[nL].x, m_MER[nL].y);
				break; 

			}

			cDist = abs((ptReflect[cID].x - cLoc.x) + (ptReflect[cID].y - cLoc.y));
			nDist = abs((ptReflect[cID].x - nLoc.x) + (ptReflect[cID].y - nLoc.y));

			if (cDist <= nDist) {
				if (m_Vertex[nL][cID].x == -1) {
					m_Vertex[nL][cID] = ptReflect[cID];
				}
				else if (m_Vertex[nL][nID].x == -1) {
					m_Vertex[nL][nID] = ptReflect[cID];
				}
			}
			else {
				if (m_Vertex[nL][nID].x == -1) {
					m_Vertex[nL][nID] = ptReflect[cID];
				}
				else if (m_Vertex[nL][cID].x == -1) {
					m_Vertex[nL][cID] = ptReflect[cID];
				}
			}

		}
	}
}

//�۲� �������� ��Ŀ�� ����
void CMarkerProcess::AssignRefractiveVertexToMarker(int nL, CvPoint ptRefract[]) {
	int cID, nID;
	CvPoint cLoc, nLoc;
	int cDist, nDist;

	for (cID = 0; cID < 4; cID++) {
		if (ptRefract[cID].x >= 0) {
			nID = (cID + 1) % 4;


			switch (cID) {
			case TS:
				cLoc = cvPoint(m_MER[nL].x, m_MER[nL].y);
				nLoc = cvPoint(m_MER[nL].x + m_MER[nL].width - 1, m_MER[nL].y);
				break;
			case RS:
				cLoc = cvPoint(m_MER[nL].x + m_MER[nL].width - 1, m_MER[nL].y);
				nLoc = cvPoint(m_MER[nL].x + m_MER[nL].width - 1, m_MER[nL].y + m_MER[nL].height - 1);
				break;
			case BS:
				cLoc = cvPoint(m_MER[nL].x + m_MER[nL].width - 1, m_MER[nL].y + m_MER[nL].height - 1);
				nLoc = cvPoint(m_MER[nL].x, m_MER[nL].y + m_MER[nL].height - 1);
				break;

			case LS:
				cLoc = cvPoint(m_MER[nL].x, m_MER[nL].y + m_MER[nL].height - 1);
				nLoc = cvPoint(m_MER[nL].x, m_MER[nL].y);
				break;

			}

			cDist = abs((ptRefract[cID].x - cLoc.x) + (ptRefract[cID].y - cLoc.y));
			nDist = abs((ptRefract[cID].x - nLoc.x) + (ptRefract[cID].y - nLoc.y));

			if (cDist <= nDist) {
				if (m_Vertex[nL][cID].x == -1) {
					m_Vertex[nL][cID] = ptRefract[cID];
				}
				
			}
			else {
				if (m_Vertex[nL][nID].x == -1) {
					m_Vertex[nL][nID] = ptRefract[cID];
				}
			}
		}

	}
}

//�ҿ��� ��Ŀ �ĺ� ���� : �Ϻ� �������� ���� �˻�
void CMarkerProcess::RemoveIncompleteMarker(int nL) {
	int nY;
	BOOL bVerify;

	bVerify = TRUE;

	for (nY = 0; nY < 4; nY++) {
		if (m_Vertex[nL][nY].x < 0 || m_Vertex[nL][nY].y < 0) {
			bVerify = FALSE;
			break;
		}
	}

	if (!bVerify) {
		m_MER[nL] = cvRect(0, 0, 0, 0);

		for (nY = 0; nY < 4; nY++) {
			m_Vertex[nL][nY] = cvPoint(-1, -1);
		}
	}

}

//��Ŀ ���� : ��Ŀ �ĺ��� ������  / MER / �׵θ� ����
void CMarkerProcess::Verification(IplImage* image) {

	VerifyMarkerVertext(image);
	VerifyMarkerMER(image);
	VerifyMarkerBorder(image);
}

//��Ŀ ������ ���� : �� �������� �ð���� �迭 �˻�
void CMarkerProcess::VerifyMarkerVertext(IplImage* image) {

	int nL, nY;
	int nWidth, nHeight;
	BOOL bVerify;

	for (nL = 1; nL <= m_maxLabel; nL++) {
		nWidth = m_MER[nL].width;
		nHeight = m_MER[nL].height;

		if ((nWidth > 0) && (nHeight > 0)) {
			bVerify = TRUE;

			if (!(m_Vertex[nL][NW].x < m_Vertex[nL][NE].x)) bVerify = FALSE;
			if (!(m_Vertex[nL][NE].y < m_Vertex[nL][SE].y)) bVerify = FALSE;
			if (!(m_Vertex[nL][SE].x > m_Vertex[nL][SW].x)) bVerify = FALSE;
			if (!(m_Vertex[nL][SW].y > m_Vertex[nL][NW].y)) bVerify = FALSE;

			if (!bVerify) {
				m_MER[nL] = cvRect(0, 0, 0, 0);

				for (nY = 0; nY < 4; nY++)	m_Vertex[nL][nY] = cvPoint(-1, -1);
			}
		}
	}
}

//��Ŀ MER ���� : ���̺� ������ MER�� �� �������� MER�� ���� ��ġ �˻�
void CMarkerProcess::VerifyMarkerMER(IplImage* image) {

	int nL, nY;
	int nWidth, nHeight;
	BOOL bVerify;
	int nMERx1, nMERy1, nMERx2, nMERy2;
	int vMERx1, vMERy1, vMERx2, vMERy2;

	for (nL = 1; nL <= m_maxLabel; nL++) {
		nWidth = m_MER[nL].width;
		nHeight = m_MER[nL].height;

		if ((nWidth > 0) && (nHeight > 0)) {
			bVerify = TRUE;

			nMERx1 = m_MER[nL].x;
			nMERy1 = m_MER[nL].y;
			nMERx2 = nMERx1 + nWidth - 1;
			nMERy2 = nMERy1 + nHeight - 1;

			vMERx1 = MAX_LENGTH;
			vMERy1 = MAX_LENGTH;
			vMERx2 = 0;
			vMERy2 = 0;

			for (nY = 0; nY < 4; nY++) {
				if ((m_Vertex[nL][nY].x < vMERx1))	vMERx1 = m_Vertex[nL][nY].x;
				if ((m_Vertex[nL][nY].y < vMERy1))	vMERy1 = m_Vertex[nL][nY].y;
				if ((m_Vertex[nL][nY].x > vMERx2))	vMERx2 = m_Vertex[nL][nY].x;
				if ((m_Vertex[nL][nY].y > vMERy2))	vMERy2 = m_Vertex[nL][nY].y;
			}

		
			if (abs(nMERx1 - vMERx1) > DISPLACEMENT) bVerify = FALSE;
			if (abs(nMERy1 - vMERy1) > DISPLACEMENT) bVerify = FALSE;
			if (abs(nMERx2 - vMERx2) > DISPLACEMENT) bVerify = FALSE;
			if (abs(nMERy2 - vMERy2) > DISPLACEMENT) bVerify = FALSE;


			if (!bVerify) {
				m_MER[nL] = cvRect(0, 0, 0, 0);

				for (nY = 0; nY < 4; nY++)	m_Vertex[nL][nY] = cvPoint(-1, -1);
			}
		}
	}
}

//��Ŀ �׵θ� ���� : �׵θ��� ������ ��輱�� ������ �밢���� ���� ���� �˻�
//��Ŀ ũ�� * 0, ���� ũ�� *  5, ���� ũ��*4 --> �׵θ� �β� 2, ���� �β� 0.5
// --> �ٰŸ� ���(N2) = 2, ���Ÿ� ���(N7) = 7, ��迡���� ����(OFFSET) < 0.5
void CMarkerProcess::VerifyMarkerBorder(IplImage* image) {

	int nL, nY;
	int nVerify;
	int nWidth, nHeight;
	CvPoint ptD1, ptD2;
	CvPoint ptB1, ptW1, ptB2, ptW2;
	int x, y;

	for (nL = 1; nL <= m_maxLabel; nL++) {
		nWidth = m_MER[nL].width;
		nHeight= m_MER[nL].height;

		if ((nWidth > 0) && (nHeight > 0)) {

			nVerify = 0;

			ptD1 = m_Vertex[nL][NW];
			ptD2 = m_Vertex[nL][SE];

			x = ptD1.x + (int)(abs(ptD2.x - ptD1.x) * (B2 - OFFSET) / M9);
			y = ptD1.y + (int)(abs(ptD2.y - ptD1.y) * (B2 - OFFSET) / M9);
			if (CheckPixelLabel(image, x, y, nL)) nVerify++;

			x = ptD1.x + (int)(abs(ptD2.x - ptD1.x) * (B2 + OFFSET) / M9);
			y = ptD1.y + (int)(abs(ptD2.y - ptD1.y) * (B2 + OFFSET) / M9);
			if (CheckPixelLabel(image, x, y, 0)) nVerify++;

			x = ptD1.x + (int)(abs(ptD2.x - ptD1.x) * (B7 + OFFSET) / M9);
			y = ptD1.y + (int)(abs(ptD2.y - ptD1.y) * (B7 + OFFSET) / M9);
			if (CheckPixelLabel(image, x, y, nL)) nVerify++;

			x = ptD1.x + (int)(abs(ptD2.x - ptD1.x) * (B7 - OFFSET) / M9);
			y = ptD1.y + (int)(abs(ptD2.y - ptD1.y) * (B7 - OFFSET) / M9);
			if (CheckPixelLabel(image, x, y, 0)) nVerify++;


			//2�ܰ�
			ptD1 = m_Vertex[nL][NE];
			ptD2 = m_Vertex[nL][SW];

			x = ptD2.x + (int)(abs(ptD1.x - ptD1.x) * (B7 + OFFSET) / M9);
			y = ptD1.y + (int)(abs(ptD2.y - ptD1.y) * (B2 - OFFSET) / M9);
			if (CheckPixelLabel(image, x, y, nL)) nVerify++;

			x = ptD2.x + (int)(abs(ptD1.x - ptD2.x) * (B7 - OFFSET) / M9);
			y = ptD1.y + (int)(abs(ptD2.y - ptD1.y) * (B2 + OFFSET) / M9);
			if (CheckPixelLabel(image, x, y, 0)) nVerify++;

			x = ptD2.x + (int)(abs(ptD1.x - ptD2.x) * (B2 - OFFSET) / M9);
			y = ptD1.y + (int)(abs(ptD2.y - ptD1.y) * (B7 + OFFSET) / M9);
			if (CheckPixelLabel(image, x, y, nL)) nVerify++;

			x = ptD2.x + (int)(abs(ptD1.x - ptD2.x) * (B2 + OFFSET) / M9);
			y = ptD1.y + (int)(abs(ptD2.y - ptD1.y) * (B7 - OFFSET) / M9);
			if (CheckPixelLabel(image, x, y, 0)) nVerify++;


			if (nVerify < 7) {
				m_MER[nL] = cvRect(0, 0, 0, 0);

				for (nY = 0; nY < 4; nY++) m_Vertex[nL][nY] = cvPoint(-1, -1);
			}
		}
	}


}

//ȭ�� ���̺� �˻�
BOOL CMarkerProcess::CheckPixelLabel(IplImage* image, int x, int y, int nL) {

	BYTE uData;

	uData = (BYTE)image->imageData[y * image->widthStep + x];

	if (uData == nL) return TRUE;
	else			return FALSE;

}

void CMarkerProcess::PatternRecognition(IplImage* imageGray, IplImage* m_pImage, bool m_bLatticeCode, bool m_bColorCode, bool m_bTemplateMatching) {
	if (m_bLatticeCode)
		RecognitionByLatticeCode(imageGray);
	else if (m_bColorCode)
		RecognitionByColorCode(m_pImage);
	else if (m_bTemplateMatching)
		RecognitionByTemplateMatching(imageGray);

}

void CMarkerProcess::RecognitionByLatticeCode(IplImage* imageGray) {
	//���⿡ �����ڵ� �ν� �˰����� �ۼ��Ͻÿ�.

}

void CMarkerProcess::RecognitionByColorCode(IplImage* imageGray) {
	//���⿡ �÷��ڵ� �ν� �˰����� �ۼ��Ͻÿ�.

}

void CMarkerProcess::RecognitionByTemplateMatching(IplImage* imageGray) {
	//���⿡ �������� �ν� �˰����� �ۼ��Ͻÿ�.

}