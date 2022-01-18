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
	
	drawContours(imgBinary, contours, -1, (0, 0, 255), -1);
	imwrite("1c.jpg", imgBinary);
	if (contours.size() != 4) {
		cout << "error: contours.size() = " << contours.size() << endl;
		return -1;
	}
	
	vector< vector<Point> > approx_contours(contours.size());
	for (int i = 0; i < contours.size(); i++)
    {
		// 拟合计算外接四边形
		approxPolyDP(Mat(contours[i]), approx_contours[i], 40, true);
		//drawContours(imgBinary, approx_contours, i, (0, 0, 255), -1);
		//imwrite("1c.jpg", imgBinary);
		// 透视变换
		Mat dstImg(Size(320, 240), CV_8UC1);
		vector<Point2f> srcPoints;
		srcPoints.push_back(approx_contours[i][0]);	// 逆时针
		srcPoints.push_back(approx_contours[i][3]);
		srcPoints.push_back(approx_contours[i][1]);
		srcPoints.push_back(approx_contours[i][2]);
		vector<Point2f> dstPoints;	// = { Point2f(0, 0), Point2f(320, 0), Point2f(0, 240), Point2f(320, 240) };
		dstPoints.push_back(Point2f(0, 0));
		dstPoints.push_back(Point2f(320, 0));
		dstPoints.push_back(Point2f(0, 240));
		dstPoints.push_back(Point2f(320, 240));
		Mat Trans = getPerspectiveTransform(srcPoints, dstPoints);
		cout << "srcPoints = " << srcPoints << endl;
		//cout << "dstPoints = " << dstPoints << endl;
		warpPerspective(srcImg, dstImg, Trans, Size(dstImg.cols, dstImg.rows), CV_INTER_CUBIC);
		outCards.push_back(dstImg);
    }
	drawContours(imgBinary, approx_contours, -1, (100, 100, 100), 2, 8);
	imwrite("1c.jpg", imgBinary);
	imwrite("1c.jpg", imgBinary);
	imwrite("1s0.jpg", outCards[0]);
	imwrite("1s1.jpg", outCards[1]);
	imwrite("1s2.jpg", outCards[2]);
	imwrite("1s3.jpg", outCards[3]);

	
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



	// 找角点
	//vector<Point2f> corners;
	//goodFeaturesToTrack(imgBinary, corners, 12, 0.01, 10, Mat(), 3,false, 0.04);
	//cout << "corners = " << corners << endl;
	//for (int i = 0; i < corners.size(); i++)
    //{
    //    circle(imgBinary, corners[i], 9, (100, 100, 100), -1, 8, 0);
    //}
	//imwrite("1r.jpg", imgBinary);
	
	// 计算外接正矩形
	//Rect cardRect;
	//cardRect = boundingRect(contours[0]);
	//rectangle(srcImg, cardRect, (0, 0, 255), 2);
	//cout << "contour = " << contours[0] << "rect = " << cardRect << endl;
	//imwrite("1c.jpg", srcImg);
