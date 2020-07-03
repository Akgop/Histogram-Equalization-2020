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

/* �̹����� �����ϱ� ���� 2���� UCHAR �迭 �Ҵ� */
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
	float low_z = 0.0;	// ��ȯ�� ����� ������ �ּҰ� = 0
	float low_p = 0.0;	// Ȯ���� �ּҰ� 0
	float hi_z = 256.0;	// ��ȯ�� ����� �ִ밪 = 255.0
	float hi_p = 1.0;	// Ȯ���� �ִ밪 1

	float target_z;	// ���ϰ��� �ϴ� ��
	float target_p;	// ���� Ȯ��

	while (hi_z - low_z > 1) {	//�ε������̰� 1���� ũ�� �ݺ�
		// binary search method
		target_z = (low_z + hi_z) / 2;	//��� �����Ͽ� ã�� ���
		target_p = z_cdf[(int)target_z];	//G(z)�� ���� �� ��
		if (target_p < p) {	// ���ϴ� Ȯ������ ������
			low_z = target_z;	//���� �Ʒ������� ������
			low_p = target_p;
		}
		else if (target_p > p) {	//ũ�� ���� ������
			hi_z = target_z;
			hi_p = target_p;
		}
		else break;
	}
	return (int)target_z;	//�ش� index��ȯ
}

/* raw file to histogram */
void MakeHistogram(UCHAR** imgbuf, float histogram[256], int width, int height)
{
	for (int CurX = 0; CurX < width; CurX++) {
		for (int CurY = 0; CurY < height; CurY++) {
			UCHAR value = imgbuf[CurX][CurY];	//raw�� �޾ƿͼ�
			int result = (int)value;	//int������ �ٲپ�
			histogram[result]++;	//count�Ѵ�.
		}
	}
	for (int i = 0; i < 256; i++) {
		histogram[i] /= width * height;	//ȭ�鿡 �������� ��������
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
	for (int CurX = 0; CurX < width; CurX++) {	// ���ι�
		for (int CurY = 0; CurY < height; CurY++) {	//���ι�
			UCHAR value = imgbuf[CurX][CurY];	//�̹��� raw���� �޾ƿͼ�
			int r = (int)value;	//int������ ��ȯ�ϰ�
			float s = cdf[r];	//s = T(r) �Ͽ� 
			int index = (int)(s * 255);	//���� histogram �迭 ����� �°� ��ȯ�� ��
			Equal_histogram[index]++;	//counting�Ѵ�.
			equalized_imgBuf[CurX][CurY] = (UCHAR)index;	//���� ���ο� 2D-array�� ����
		}
	}
	for (int i = 0; i < 256; i++) {	//counting �� ���� ȭ�鿡 �������� ����ϱ� ���� 
		Equal_histogram[i] /= width * height;	//������ �����Ѵ�.
	}

	return equalized_imgBuf;	//���ο� �̹��� ��ȯ
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
	// �ٲ� �̹���.
	FILE* fp_InputImg = fopen("RAW\\gray\\barbara(512x512).raw", "rb");	//barbara �̹��� = �ٲ� �̹���
	if (!fp_InputImg) {
		printf("Can not open file.");
	}
	// ���ϴ� �̹���
	FILE* fp_expectImg = fopen("RAW\\gray\\Couple(512x512).raw", "rb");	//histogram matching�� �̹���
	if (!fp_expectImg) {
		printf("Can not open file.");
	}
	UCHAR** Input_imgBuf = memory_alloc2D(width, height); // �޸� �����Ҵ�
	fread(&Input_imgBuf[0][0], sizeof(UCHAR), width * height, fp_InputImg); // 2���� �迭�� �̹��� �о����

	UCHAR** Expect_imgBuf = memory_alloc2D(width, height); // �޸� �����Ҵ�
	fread(&Expect_imgBuf[0][0], sizeof(UCHAR), width * height, fp_expectImg); // 2���� �迭�� �̹��� �о����

	// Step 2. s = T(r) -> r�� �ٲ� �̹���, s�� Uniform 
	UCHAR** equalized_imgBuf = memory_alloc2D(width, height);
	MakeHistogram(Input_imgBuf, &Image_Histogram[0], width, height);	//�̹��� Histogram���� �����
	equalized_imgBuf = MakeHistogramEqualization(Input_imgBuf, equalized_imgBuf, Image_Histogram, &Image_equal_Histogram[0], width, height);	//histogram equalization
	
	// Step 3. [+option] Histogram Matching s = G(z) -> z�� ���ϴ� �̹���, s�� Uniform
	// -> G(z) = T(r) -> z = G^-1(T(r))
	UCHAR** matched_imgBuf = memory_alloc2D(width, height);
	MakeHistogram(Expect_imgBuf, &z_Histogram[0], width, height);	//�̹��� Histogram���� �����
	matched_imgBuf = HistogramMatching(Input_imgBuf, matched_imgBuf, Image_Histogram, z_Histogram, &Image_matching_Histogram[0], width, height);	//histogram matching

	DrawHistogram(Image_Histogram, 30, 700); // ������׷� �׷���
	DrawHistogram(Image_equal_Histogram, 400, 700); // ������׷� ��Ȱȭ �׷���
	DrawHistogram(Image_matching_Histogram, 770, 700);	//��Ī�� ������׷� �׷���
	DrawHistogram(z_Histogram, 1200, 700); // Ÿ�� ������׷� �׷���

	FILE* fp_outputImg = fopen("output.raw", "wb"); // ��� �����ϱ�
	fwrite(&equalized_imgBuf[0][0], sizeof(UCHAR), width * height, fp_outputImg);

	FILE* fp_matchedImg = fopen("output2.raw", "wb");	//��� ����
	fwrite(&matched_imgBuf[0][0], sizeof(UCHAR), width * height, fp_matchedImg);

	//�޸� �Ҵ� ����
	MemoryClear(Input_imgBuf);
	MemoryClear(Expect_imgBuf);
	MemoryClear(matched_imgBuf);
	MemoryClear(equalized_imgBuf);
	//���� �ݱ�
	fclose(fp_outputImg);
	fclose(fp_InputImg);
	return 0;
}