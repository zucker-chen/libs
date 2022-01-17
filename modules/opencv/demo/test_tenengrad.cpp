#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include <ctype.h>
#include <stdio.h>
#include <iostream>

using namespace cv;
using namespace std;


int main (int argc, char *argv[])
{

	Mat imgSource;
	Mat imgGray;
	Mat imgBinary;
	Mat imageSobel;
	Mat imageLUT;
	Mat lookUpTable(1, 256, CV_8U);
	float tenengrad;
	int meanBright;
	unsigned char *plut;
	
	string imgFileName("1.jpg");
	
	if (argc > 1) {
		imgFileName = argv[1];
	}
	// 图片读取
	imgSource = imread(imgFileName);
	// 灰度转换
	cvtColor(imgSource, imgGray, CV_BGR2GRAY);
	// 亮度归一化
	plut = lookUpTable.data;
	meanBright = mean(imgGray)[0];
	for (int i = 0; i < 256; i++)
	{
		int tmp = i * 128 / meanBright;
		plut[i] = tmp > 255 ? 255 : tmp;
	}
	LUT(imgGray, lookUpTable, imageLUT);
	imwrite("1g.jpg", imageLUT);
	// 二值化
	threshold(imgGray, imgBinary, 0, 255, THRESH_BINARY|THRESH_OTSU);
	imwrite("1b.jpg", imgBinary);
	// 梯度计算
	Sobel(imageLUT, imageSobel, CV_16U, 1, 1);
	// 梯度平均值
	tenengrad = mean(imageSobel)[0];
	cout << "img:" << imgFileName << ",result:" << tenengrad << endl;
	// 保存图片
	imwrite("1n.jpg", imageSobel);





	return 0;	
}
