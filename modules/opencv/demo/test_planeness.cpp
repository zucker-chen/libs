#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include <ctype.h>
#include <stdio.h>
#include <iostream>

using namespace cv;
using namespace std;


// 亮度归一化
static int imgLut(Mat &srcImg, Mat &outImg)
{
	Mat imageLUT = outImg;
	Mat lookUpTable(1, 256, CV_8U);
	int meanBright;
	unsigned char *plut;
	plut = lookUpTable.data;
	
	meanBright = mean(srcImg)[0];
	for (int i = 0; i < 256; i++)
	{
		int tmp = i * 128 / meanBright;
		plut[i] = tmp > 255 ? 255 : tmp;
	}
	LUT(srcImg, lookUpTable, imageLUT);
	imwrite("1g.jpg", imageLUT);
	
	return 0;
}

// 测试卡抠图
static int cardCutout(Mat &srcImg, vector<Mat> &outCards)
{
	
	// 二值化
	Mat imgBinary;
	threshold(srcImg, imgBinary, 200, 255, THRESH_BINARY|THRESH_OTSU);
	imwrite("1b.jpg", imgBinary);
	// 找最大轮廓
	vector< vector<Point> > contours;
	findContours(imgBinary, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);
	cout << "contours size = " << contours.size() << endl;
	
	drawContours(srcImg, contours, -1, (0, 0, 255), -1);
	imwrite("1c.jpg", srcImg);
	
	// 计算外接正矩形
	//Rect cardRect;
	//cardRect = boundingRect(contours[0]);
	//rectangle(srcImg, cardRect, (0, 0, 255), 2);
	//cout << "contour = " << contours[0] << "rect = " << cardRect << endl;
	//imwrite("1c.jpg", srcImg);
	// 拟合计算外接四边形
	//vector< vector<Point> > approx_contours(contours.size());
	//approxPolyDP(Mat(contours[0]), approx_contours[0], 20, true);
	//drawContours(srcImg, approx_contours, 0, (0, 0, 255), -1);
	//cout << "approx_contours = " << approx_contours[0] << endl;
	//imwrite("1c.jpg", srcImg);
	
	return 0;
}


int main (int argc, char *argv[])
{

	Mat imgSource;
	Mat imgGray;
	Mat imageSobel;
	float tenengrad;
	
	string imgFileName("2.png");
	
	if (argc > 1) {
		imgFileName = argv[1];
	}
	// 图片读取
	imgSource = imread(imgFileName);
    if(imgSource.empty()) { // check if the image has been loaded properly
        return -1;
	}
	// 灰度转换
	cvtColor(imgSource, imgGray, CV_BGR2GRAY);
	// 亮度归一化
	//imgLut(imgGray, imgGray);
	// 测试卡提取
	vector<Mat> outCards;
	cardCutout(imgGray, outCards);
	// 梯度计算
	Sobel(imgGray, imageSobel, CV_16U, 1, 1);
	// 梯度平均值
	tenengrad = mean(imageSobel)[0];
	cout << "img:" << imgFileName << ",result:" << tenengrad << endl;
	// 保存图片
	imwrite("1n.jpg", imageSobel);





	return 0;	
}
