#include "pch.h"
#include "CMarkerProcess.h"


CMarkerProcess::CMarkerProcess(void) {
	
	int nL, nY;

	//최대 유효 레이블 초기화
	m_maxLabel = 0;

	//마커 후보의 MER 초기화:(x,y,width,height)
	for (nL = 0; nL < MAX_NUM; nL++) {
		m_MER[nL] = cvRect(0, 0, 0, 0);
	}

	//마커 후보의 꼭지점 초기화: (x,y)
	for (nL = 0; nL < MAX_NUM; nL++)
		for (nY = 0; nY < 4; nY++)
			m_Vertex[nL][nY] = cvPoint(-1, -1);
}

CMarkerProcess::~CMarkerProcess(void) {

}

void CMarkerProcess::ImageBinarization(IplImage* image) {

	//임계화 : Otsu 알고리즘
	cvThreshold(image, image, T_BINARY, 255, CV_THRESH_BINARY_INV | CV_THRESH_OTSU);

	//모폴로지 : Open & Close
	IplImage* imageTemp = cvCreateImage(cvSize(image->width, image->height), IPL_DEPTH_8U, 1);	//임시영상 생성
	IplConvKernel* element = cvCreateStructuringElementEx(3, 3, 1, 1, CV_SHAPE_RECT, NULL);		//구조 요소 생성

	cvMorphologyEx(image, image, imageTemp, element, CV_MOP_OPEN, 1);			//열림
	cvMorphologyEx(image, image, imageTemp, element, CV_MOP_CLOSE, 1);			//닫힘

	cvReleaseStructuringElement(&element);		//구조요소 해제
	cvReleaseImage(&imageTemp);					//임시영상 해제
}

void CMarkerProcess::RegionSegmentation(IplImage* image) {

	int i, j;									//인덱스
	int x, y, nx, ny;							//현재/이웃 화소 좌표
	int nMERx, nMERy, nMERw, nMERh;				//최소 레이블 영역의 MER (x,y,width, height)
	int cMERx1, cMERy1, cMERx2, cMERy2;			//현재 레이블 영역의 MER 좌상단/우하단 좌표
	int dMERx1, dMERy1, dMERx2, dMERy2;			//연결 레이블 영역의 MER 좌상단/우하단 좌표
	int nLabel, nMinLabel, nL;					//현재 레이블 / 최소 레이블 / 선행이웃 레이블
	int vNeighborLabel[4];						//선행 이웃의 레이블 : 4-이웃(NW, N, NE, W)
	BYTE uData;									//화소값
	BOOL bConnected;							//연결 플래그
	int vLCT[MAX_NUM];							//레이블 연결 테이블 (1~255:레이블, 0:배경)

	nLabel = 0;										//레이블 번호 초기화
	for (i = 0; i < MAX_NUM; i++)	vLCT[i] = 0;	// 레이블 연결 테이블 초기화


	for (y = 0; y < image->height; y++) {
		for (x = 0; x < image->width; x++) {

			uData = (BYTE)image->imageData[y * image->widthStep + x];		//캐스팅(BYTE) 핆수

			if (uData) {

				//이웃 화소의 레이블 중 최소 레이블 성정
				bConnected = FALSE;								//연결 플래그 초기화
				nMinLabel = MAX_NUM;							//최소 레이블 초기화(256)
							
				for (i = 0; i < 4; i++)	vNeighborLabel[i] = 0;		//선행 이웃 레이블 초기화

				//8-이웃 중에서
				for (j = -1; j <= 1; j++) {
					for (i = -1; i <= 1; i++) {

						//선행 4-이웃에 대해 : NW, N, NE, W
						if ((j == -1) || (j == 0 && i == -1)) {

							nx = x + i;		//이웃의 x좌표
							ny = y + j;		//이웃의 y좌표

							//이웃 화소가 영상 범위에 포함되고
							if ((nx >= 0) && (ny >= 0) && (nx < image->width) && (ny < image->height)) {

								uData = (BYTE)image->imageData[ny * image->widthStep + nx];

								//이웃 화소의 값이 레이블(1~255)인 경우
								if (uData > 0) {
									bConnected = TRUE;								//연결 플래그 설정
									if (uData < nMinLabel)	nMinLabel = uData;		//최소 레이블 갱신
									vNeighborLabel[(j + 1) * 3 + (i + 1)] = uData;		//선행 이웃 레이블(0, 1,2,3) 
								}
							}
						}
					}
				}


				//연결된 선행 이웃의 없는 경우 : 새로운 마커 후보 등록
				if (!bConnected) {

					nLabel++;					//레이블 증가
					if (nLabel >= MAX_NUM) nLabel = MAX_NUM - 1;		//레이블의 최대값을 255로 제한
					image->imageData[y * image->widthStep + x] = (BYTE)nLabel;	//현재 화소의 값을 레이블로 변경

					m_MER[nLabel] = cvRect(x, y, 1, 1);		//마커의 MER 설정
					vLCT[nLabel] = nLabel;			//레이블 연결 테이블의 해당 항목을 자신의 레이블로 설정


				}
				//연결된 선행 이웃이 있는 경우 : 마커 후보의 MER 갱신
				else {

					//현재 화소를 최소 영역에 포함시킴
					image->imageData[y * image->widthStep + x] = (BYTE)nMinLabel;	//현재 화소의 값을 연결된 최소 레이블로 변경

					nMERx = m_MER[nMinLabel].x;		//최소 레이블 영역의 MER x좌표
					nMERy = m_MER[nMinLabel].y;		//최소 레이블 영역의 MER y좌표
					nMERw = m_MER[nMinLabel].width;		//최소 레이블 영역의 MER 폭
					nMERh = m_MER[nMinLabel].height;		//최소 레이블 영역의 MER 높이


					if (x < nMERx) {	//현재 화소가 기존 MER의 좌측에 위치하는 경우

						m_MER[nMinLabel].x = x;				//최소 레이블 영역의 MER x좌표 갱신
						m_MER[nMinLabel].width = nMERw + (nMERx - x);		//최소 레이블 영역의 MER 폭 갱신

					}
					else if (x > nMERx + nMERw - 1) {		//현재 화소가 기존 MER의 우측에 위치하는 경우

						m_MER[nMinLabel].width = x - nMERx + 1;		//최소 레이블 영역의 MER 폭 갱신

					}
					else if (y < nMERy) {		//현재 화소가 기존 MER의 상측에 위치하는 경우

						m_MER[nMinLabel].y = y;				//최소 레이블 영역의 MER y좌표 갱신
						m_MER[nMinLabel].height = nMERh + (nMERy - y);		//최소 레이블 영역의 MER 높이 갱신

					}
					else if (y > nMERy + nMERh - 1) {		//현재 화소가 기존 MER의 하측에 위치하는 경우

						m_MER[nMinLabel].height = y - nMERy + 1;		//최소 레이블 영역의 MER shvdl 갱신

					}

					//현재 화소에 연결된 영역의 통합을 위한 레이블 연결 테이블 갱신
					for (i = 0; i < 4; i++) {

						nL = vNeighborLabel[i];		//선행 이웃 레이블 설정


						if ((nL > 0) && (vLCT[nL] > nMinLabel)) {	//선행 이웃 레이블이 유효하고, 레이블 연결 테이블의 값이 최소 레이블보다 큰 경우
							vLCT[nL] = nMinLabel;	//최소 레이블로 갱신
						}

					}
				}
			}
		}
	}



	//2 단계 : 레이블 연결관계에 의한 분할 영역 통합

	//레이블 연결 테이블의 추적관계 최적화 : 최종 레이블로 직접 연결
	for (i = 1; i <= nLabel; i++) {
		j = i;		//현재 레이블을 탐색 레이블로 설정
		while (j > vLCT[j]) {	//탐색 레이블이 더 작은 레이블에 연결되어 있는 경우
			j = vLCT[j];		//연결된 레이블을 계속 추적
		}
		vLCT[i] = j;		//최종 추적된 최소 레이블을 현재 레이블에 직접 연결
	}

	//연결 영역의 화소 레이블 통합
	for (y = 0; y < image->height; y++) {
		for (x = 0; x < image->width; x++) {		//각 화소에 대해

			uData = (BYTE)image->imageData[y * image->widthStep + x];

			if (uData > 0) {		//현재 화소의 값이 레이블(1~255) 이고
				if (vLCT[uData] < uData) {	//현재 화소의 레이블이 더 작은 레이블에 연결되어 있는 경우
					image->imageData[y * image->widthStep + x] = (BYTE)vLCT[uData];	//현재 화소의 레이블 갱신
				}
			}
		}
	}

	//연결 영역의 MER 통합
	for (i = 1; i <= nLabel; i++) {		
		if (i > vLCT[i]) {		//현재 레이블이 더 작은 레이블에 연결되어 있는 경우

			j = vLCT[i];		//현재 레이블이 더 작은 레이블에 연결되어 있는 경우

			cMERx1 = m_MER[i].x;					//현재 레이블 영역의 MER 좌상단 x좌표
			cMERy1 = m_MER[i].y;					//현재 레이블 영역의 MER 좌상단 y좌표
			cMERx2 = cMERx1 + m_MER[i].width - 1;	//현재 레이블 영역의 MER 우하단 x좌표
			cMERy2 = cMERy1 + m_MER[i].height - 1;	//현재 레이블 영역의 MER 우하단 y좌표

			dMERx1 = m_MER[j].x;			//연결 레이블 영역의 MER 좌상단 x좌표
			dMERy1 = m_MER[j].y;			//연결 레이블 영역의 MER 좌상단 y좌표
			dMERx2 = dMERx1 + m_MER[j].width - 1;			//연결 레이블 영역의 MER 우하단 x좌표
			dMERy2 = dMERy1 + m_MER[j].height - 1;			//연결 레이블 영역의 MER 우하단 y좌표

			if (cMERx1 < dMERx1) {			//MER 좌측 확장의 경우
				m_MER[j].x = cMERx1;				//연결 레이블 영역의 MER x좌표 갱신
				m_MER[j].width = dMERx2 - cMERx1 + 1;		//연결 레이블 영역의 MER 폭 갱신
			}
			if (cMERy1 < dMERy1) {		//MER 상측 확장의 경우
				m_MER[j].y = cMERy1;					//연결 레이블 영역의 MER y 좌표 갱신
				m_MER[j].height = dMERy2 - cMERy1 + 1;		//연결 레입르 영역의 MER 높이 갱신
			}
			if (cMERx2 > dMERx2) {		//MER 우측 확장의 경우
				m_MER[j].width = cMERx2 - m_MER[j].x + 1;		//연결 레이블 영역의 MER 폭 갱신
			}
			if (cMERy2 > dMERy2) {
				m_MER[j].height = cMERy2 - m_MER[j].y + 1;		//연결 레이블 영역의 MER 높이 갱신
			}

			m_MER[i] = cvRect(0, 0, 0, 0);		//현재 레이블 영역의 MER 리셋
			vLCT[i] = 0;						//레이블 연결 테이블의 현재 테이블 리셋

		}
	}

	m_maxLabel = nLabel;		//최대 유호 레이블 저장

}

void CMarkerProcess::RegionFiltering(IplImage* image) {

	int nL;				//레이블 인덱스
	int nWidth, nHeight;			//MER 폭과 높이
	int nMERx1, nMERy1, nMERx2, nMERy2;		//MER의 좌상단/우하단 좌표

	//후보 영역의 크기 필터링
	for (nL = 1; nL <= m_maxLabel; nL++) {
		nWidth = m_MER[nL].width;
		nHeight = m_MER[nL].height;

		if ((nWidth > 0) && (nHeight > 0)) {		//후보 영역이 존재하고,

			//후보 영역 MER 크기가 유효하지 않은 경우제외
			if ((nWidth < MIN_LENGTH) || (nHeight < MIN_LENGTH)) {	
				m_MER[nL] = cvRect(0, 0, 0, 0);		//현재 레이블 영역의 MER 리셋
			}

		}
	}

	//후보 영역의 경계 필터링
	for (nL = 1; nL <= m_maxLabel; nL++) {
		nWidth = m_MER[nL].width;
		nHeight = m_MER[nL].height;

		if ((nWidth > 0) && (nHeight > 0)) {		//후보 영역이 존재하고,

			nMERx1 = m_MER[nL].x;		//현재 레이블 영역의 MER 좌상단 x좌표
			nMERy1 = m_MER[nL].y;		//현재 레이블 영역의 MER 좌상단 y좌표

			nMERx2 = nMERx1 + m_MER[nL].width -1;		//현재 레이블 영역의 MER 우하단 x좌표
			nMERy2 = nMERy1 + m_MER[nL].height -1;		//현재 레이블 영역의 MER 우하단 y좌표

			//후보 영역 MER이 영상 경계보다 안쪽에 위치하지 않은경우 제외
			if (!(nMERx1 > 0) && (nMERy1 > 0) && (nMERx2 < image->width - 1) && (nMERy2 < image->height - 1)) {
				m_MER[nL] = cvRect(0, 0, 0, 0);		//현재 레이블 영역의 MER 리셋
			}

		}

	}

}

void CMarkerProcess::DrawRegion(IplImage* imageGray, IplImage* imageColor) {

	int nL;					//레이블 인덱스
	int x, y;				//화소 좌표
	BYTE uData;				//화소값
	int nWidth, nHeight;			//MER 폭과 높이
	CvPoint pt1, pt2, pt3, pt4;		//MER의 꼭지점
	CvScalar color;						//컬러
	

	//마커 후보 영역의 화소 표시
	for (y = 0; y < imageGray->height; y++) {
		for (x = 0; x < imageGray->width; x++) {		//각 화소에 대해

			uData = (BYTE)imageGray->imageData[y * imageGray->widthStep + x];

			if (uData > 0) {		//화소값이 레이블(1~255)인 경우
				imageColor->imageData[y * imageColor->widthStep + x * 3] = (BYTE)255;		//영역색(whilte)설정 : B 성분
				imageColor->imageData[y * imageColor->widthStep + x * 3+1] = (BYTE)255;		//영역색(whilte)설정 : G 성분
				imageColor->imageData[y * imageColor->widthStep + x * 3+2] = (BYTE)255;		//영역색(whilte)설정 : R 성분
			}
		}
	}

	//마커 후보 영역의 MER 표시
	for (nL = 1; nL <= m_maxLabel; nL++) {		//각 레이블에 대해

		nWidth = m_MER[nL].width;		//MER의 폭 설정
		nHeight = m_MER[nL].height;		//MER의 높이 설정

		if ((nWidth > 0) && (nHeight > 0)) {		//후보 영역이 존재하는 경우

			//마커 후보 영역의 MER 표시
			color = cvScalar(0, 255, 0);			//MER 표시색 설정 : 초록색

			pt1 = cvPoint(m_MER[nL].x, m_MER[nL].y);			//MER의 좌상점 설정
			pt2 = cvPoint(m_MER[nL].x + m_MER[nL].width -1, m_MER[nL].y + m_MER[nL].height -1);		//MER의 우하점 설정

			cvDrawRect(imageColor, pt1, pt2, color);		//MER 그리기

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

//마커 후보 윤곽 추출 : 꼭지점 검출
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

//MER의 코너에서 코너 꼭지점 탐색
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

//MER의 변에서 반사 및 굴절 꼭지점 탐색
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

		// 수직거리 프로파일 산축 : 각 변에서 물체화소까지의 수직거리 프로파일 산출
		ProduceDistanceProfile(image, nL, sID, sideS, sideL, runS, runL, step, nProfile);

		//반사 꼭지점 탐색 : 각 변에서 기울기 변화가 가장 큰 반사패턴의 꼭지점 탐색
		SearchReflectiveVertex(image, sID, sideS, sideL, runS, step, nProfile, ptReflect);

		//굴절 꼭지점 탐색 : 각 변에서 기울기 변화가 가장 큰 굴절패턴의 꼭지점 탐색
		SearchRefractiveVertex(image, sID, sideS, sideL, runS, step, nProfile, ptReflect, ptRefract);

	}
}

//수직거리 프로파일 산출 : 각 변에서 물체화소까지의 수직거리 프로팡리 산출
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

//반사 꼭지점 탐색 : 각 변에서 기울기 변화가 가장 큰 반사패턴의 꼭지점 탐색
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

//굴절 꼭지점 탐색 : 각 변에서 기울기 변화가 가장 큰 굴잴패턴의 꼭지점 탐색
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

//중복 꼭지점 제거
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

//코너 꼭지점을 마커에 배정
void CMarkerProcess::AssignCornerVertexToMarker(int nL, CvPoint ptCorner[]) {
	int cID;
	for (cID = 0; cID < 4; cID++) {
		if ((ptCorner[cID].x >= 0) && (ptCorner[cID].y >= 0)) {
			m_Vertex[nL][cID] = ptCorner[cID];
		}
	}
}


//반사 꼭지점을 마커에 배정
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

//글꼴 꼭지점을 마커에 배정
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

//불완전 마커 후보 제거 : 일부 꼭지점의 누락 검사
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

//마커 검증 : 마커 후보의 꼭지점  / MER / 테두리 검증
void CMarkerProcess::Verification(IplImage* image) {

	VerifyMarkerVertext(image);
	VerifyMarkerMER(image);
	VerifyMarkerBorder(image);
}

//마커 꼭지점 검증 : 네 꼭지점의 시계방향 배열 검사
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

//마커 MER 검증 : 레이블 영역의 MER과 네 꼭지점의 MER에 대한 일치 검사
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

//마커 테두리 검증 : 테두리와 여백의 경계선과 꼭지점 대각선의 교차 지역 검사
//마커 크기 * 0, 여백 크기 *  5, 패턴 크기*4 --> 테두리 두께 2, 여백 두께 0.5
// --> 근거리 경계(N2) = 2, 원거리 경계(N7) = 7, 경계에서의 변위(OFFSET) < 0.5
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


			//2단계
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

//화소 레이블 검사
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
	//여기에 격자코드 인식 알고리즘을 작성하시오.

}

void CMarkerProcess::RecognitionByColorCode(IplImage* imageGray) {
	//여기에 컬러코드 인식 알고리즘을 작성하시오.

}

void CMarkerProcess::RecognitionByTemplateMatching(IplImage* imageGray) {
	//여기에 형판정합 인식 알고리즘을 작성하시오.

}