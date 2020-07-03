#include <iostream>
#include <math.h>
#include <windows.h>

#pragma warning(disable : 4996)

using namespace std;

void DrawCDF(float cdf[256], int x_origin, int y_origin);


HWND hwnd;
HDC hdc;

void MemoryClear(UCHAR **buf) {
	if (buf) {
		free(buf[0]);
		free(buf);
		buf = NULL;
	}
}

/* 이미지를 저장하기 위한 2차원 UCHAR 배열 할당 */
UCHAR** memory_alloc2D(int width, int height)
{
	UCHAR** ppMem2D = 0;
	int	i;

	//arrary of pointer
	ppMem2D = (UCHAR**)calloc(sizeof(UCHAR*), height);
	if (ppMem2D == 0) {
		return 0;
	}

	*ppMem2D = (UCHAR*)calloc(sizeof(UCHAR), height * width);
	if ((*ppMem2D) == 0) {//free the memory of array of pointer        
		free(ppMem2D);
		return 0;
	}

	for (i = 1; i < height; i++) {
		ppMem2D[i] = ppMem2D[i - 1] + width;
	}

	return ppMem2D;
}

/* get inverse CDF of G ; G^-1(z)*/
int inverse_cdf_z(float p, float z_cdf[256]) {
	float low_z = 0.0;	// 변환된 결과의 범위의 최소값 = 0
	float low_p = 0.0;	// 확률의 최소값 0
	float hi_z = 256.0;	// 변환된 결과의 최대값 = 255.0
	float hi_p = 1.0;	// 확률의 최대값 1

	float target_z;	// 구하고자 하는 값
	float target_p;	// 비교할 확률

	while (hi_z - low_z > 1) {	//인덱스차이가 1보다 크면 반복
		// binary search method
		target_z = (low_z + hi_z) / 2;	//계속 절반하여 찾는 방식
		target_p = z_cdf[(int)target_z];	//G(z)를 통해 값 비교
		if (target_p < p) {	// 원하는 확률보다 작으면
			low_z = target_z;	//범위 아래쪽으로 재조정
			low_p = target_p;
		}
		else if (target_p > p) {	//크면 위로 재조정
			hi_z = target_z;
			hi_p = target_p;
		}
		else break;
	}
	return (int)target_z;	//해당 index반환
}

/* raw file to histogram */
void MakeHistogram(UCHAR** imgbuf, float histogram[256], int width, int height)
{
	for (int CurX = 0; CurX < width; CurX++) {
		for (int CurY = 0; CurY < height; CurY++) {
			UCHAR value = imgbuf[CurX][CurY];	//raw값 받아와서
			int result = (int)value;	//int형으로 바꾸어
			histogram[result]++;	//count한다.
		}
	}
	for (int i = 0; i < 256; i++) {
		histogram[i] /= width * height;	//화면에 보기좋게 범위조정
	}
}

void DrawCDF(float cdf[256], int x_origin, int y_origin) {
	for (int CurX = 0; CurX < 256; CurX++) {
		for (int CurY = 0; CurY < cdf[CurX]; CurY++) {
			MoveToEx(hdc, x_origin + CurX, y_origin, 0);
			SetPixel(hdc, x_origin + CurX, y_origin - cdf[CurX] * 100, RGB(0, 0, 255));
		}
	}
}

void DrawHistogram(float histogram[256], int x_origin, int y_origin) {
	MoveToEx(hdc, x_origin, y_origin, 0);
	LineTo(hdc, x_origin + 255, y_origin);

	MoveToEx(hdc, x_origin, 100, 0);
	LineTo(hdc, x_origin, y_origin);

	for (int CurX = 0; CurX < 256; CurX++) {
		for (int CurY = 0; CurY < histogram[CurX]; CurY++) {
			MoveToEx(hdc, x_origin + CurX, y_origin, 0);
			LineTo(hdc, x_origin + CurX, y_origin - histogram[CurX] * 5000);
		}
	}
}

/* histogram equalization */
UCHAR** MakeHistogramEqualization(UCHAR** imgbuf, UCHAR** equalized_imgBuf, float histogram[256], float Equal_histogram[256], int width, int height)
{
	// Step 1. Get CDF of histogram
	float cdf[256] = { 0, };
	cdf[0] = histogram[0];
	for (int i = 1; i < 256; i++) {
		cdf[i] = histogram[i] + cdf[i - 1];
	}
	DrawCDF(cdf, 30, 700);	//Draw CDF to check if it calculated correctly

	// Step 2. Transforming
	for (int CurX = 0; CurX < width; CurX++) {	// 가로번
		for (int CurY = 0; CurY < height; CurY++) {	//세로번
			UCHAR value = imgbuf[CurX][CurY];	//이미지 raw값을 받아와서
			int r = (int)value;	//int형으로 변환하고
			float s = cdf[r];	//s = T(r) 하여 
			int index = (int)(s * 255);	//값을 histogram 배열 사이즈에 맞게 변환한 후
			Equal_histogram[index]++;	//counting한다.
			equalized_imgBuf[CurX][CurY] = (UCHAR)index;	//실제 새로운 2D-array에 저장
		}
	}
	for (int i = 0; i < 256; i++) {	//counting 한 값을 화면에 보기좋게 출력하기 위해 
		Equal_histogram[i] /= width * height;	//범위를 조정한다.
	}

	return equalized_imgBuf;	//새로운 이미지 반환
}

/* histogram matching */
UCHAR** HistogramMatching(UCHAR** imgbuf, UCHAR** matched_imgBuf, float histogram[256], float z_Histogram[256], float Matching_histogram[256], int width, int height)
{
	// Step 1. Get CDF of r ;T(r)
	float r_cdf[256] = { 0, };
	r_cdf[0] = histogram[0];
	for (int i = 1; i < 256; i++) {
		r_cdf[i] = histogram[i] + r_cdf[i - 1];
	}

	// Step 2. Get inverse CDF of z ;G(z)
	float z_cdf[256] = { 0, };
	z_cdf[0] = z_Histogram[0];
	for (int i = 1; i < 256; i++) {
		z_cdf[i] = z_Histogram[i] + z_cdf[i - 1];
	}
	DrawCDF(z_cdf, 1200, 700);

	// Step 3. z = G^-1(T(r)) Transforming
	for (int CurX = 0; CurX < width; CurX++) {
		for (int CurY = 0; CurY < height; CurY++) {
			UCHAR value = imgbuf[CurX][CurY];
			int r = (int)value;
			float Tr = r_cdf[r];		//get T(r)
			int z = inverse_cdf_z(Tr, z_cdf); // get z = G^-1(T(r))
			Matching_histogram[z]++;
			matched_imgBuf[CurX][CurY] = (UCHAR)z;
		}
	}
	for (int i = 0; i < 256; i++) {
		Matching_histogram[i] /= width * height;
	}

	return matched_imgBuf;
}

int main(void)
{
	system("color F0");
	hwnd = GetForegroundWindow();
	hdc = GetWindowDC(hwnd);

	int width = 512;
	int height = 512;

	float Image_Histogram[256] = { 0, };
	float Image_equal_Histogram[256] = { 0, };
	float z_Histogram[256] = { 0, };
	float Image_matching_Histogram[256] = { 0, };

	// Step 1. Get raw type images
	// 바꿀 이미지.
	FILE* fp_InputImg = fopen("RAW\\gray\\barbara(512x512).raw", "rb");	//barbara 이미지 = 바꿀 이미지
	if (!fp_InputImg) {
		printf("Can not open file.");
	}
	// 원하는 이미지
	FILE* fp_expectImg = fopen("RAW\\gray\\Couple(512x512).raw", "rb");	//histogram matching용 이미지
	if (!fp_expectImg) {
		printf("Can not open file.");
	}
	UCHAR** Input_imgBuf = memory_alloc2D(width, height); // 메모리 동적할당
	fread(&Input_imgBuf[0][0], sizeof(UCHAR), width * height, fp_InputImg); // 2차원 배열에 이미지 읽어오기

	UCHAR** Expect_imgBuf = memory_alloc2D(width, height); // 메모리 동적할당
	fread(&Expect_imgBuf[0][0], sizeof(UCHAR), width * height, fp_expectImg); // 2차원 배열에 이미지 읽어오기

	// Step 2. s = T(r) -> r은 바꿀 이미지, s는 Uniform 
	UCHAR** equalized_imgBuf = memory_alloc2D(width, height);
	MakeHistogram(Input_imgBuf, &Image_Histogram[0], width, height);	//이미지 Histogram으로 만들기
	equalized_imgBuf = MakeHistogramEqualization(Input_imgBuf, equalized_imgBuf, Image_Histogram, &Image_equal_Histogram[0], width, height);	//histogram equalization
	
	// Step 3. [+option] Histogram Matching s = G(z) -> z는 원하는 이미지, s는 Uniform
	// -> G(z) = T(r) -> z = G^-1(T(r))
	UCHAR** matched_imgBuf = memory_alloc2D(width, height);
	MakeHistogram(Expect_imgBuf, &z_Histogram[0], width, height);	//이미지 Histogram으로 만들기
	matched_imgBuf = HistogramMatching(Input_imgBuf, matched_imgBuf, Image_Histogram, z_Histogram, &Image_matching_Histogram[0], width, height);	//histogram matching

	DrawHistogram(Image_Histogram, 30, 700); // 히스토그램 그래프
	DrawHistogram(Image_equal_Histogram, 400, 700); // 히스토그램 평활화 그래프
	DrawHistogram(Image_matching_Histogram, 770, 700);	//매칭된 히스토그램 그래프
	DrawHistogram(z_Histogram, 1200, 700); // 타겟 히스토그램 그래프

	FILE* fp_outputImg = fopen("output.raw", "wb"); // 결과 저장하기
	fwrite(&equalized_imgBuf[0][0], sizeof(UCHAR), width * height, fp_outputImg);

	FILE* fp_matchedImg = fopen("output2.raw", "wb");	//결과 저장
	fwrite(&matched_imgBuf[0][0], sizeof(UCHAR), width * height, fp_matchedImg);

	//메모리 할당 해제
	MemoryClear(Input_imgBuf);
	MemoryClear(Expect_imgBuf);
	MemoryClear(matched_imgBuf);
	MemoryClear(equalized_imgBuf);
	//파일 닫기
	fclose(fp_outputImg);
	fclose(fp_InputImg);
	return 0;
}