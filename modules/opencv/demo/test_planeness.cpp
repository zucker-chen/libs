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
	imwrite("1l.jpg", imageLUT);
	
	return 0;
}

// 测试卡抠图
static int cardContour(Mat &srcImg, vector<Mat> &outCards)
{
	
	// 二值化
	Mat imgBinary;
	threshold(srcImg, imgBinary, 200, 255, THRESH_BINARY|THRESH_OTSU);
	//imwrite("1b.jpg", imgBinary);
	// 开运算，先腐蚀，再膨胀，可清除一些小东西(亮的)，放大局部低亮度的区域 
	Mat element = getStructuringElement(MORPH_RECT, Size(17, 15) );
	morphologyEx(imgBinary, imgBinary, CV_MOP_OPEN, element);
	imwrite("1b.jpg", imgBinary);
	// 找最大轮廓
	vector< vector<Point> > contours;
	findContours(imgBinary, contours, RETR_EXTERNAL, CHAIN_APPROX_NONE);	// 轮廓从左下角开始逆时针存储
	cout << "contours size = " << contours.size() << endl;
	drawContours(imgBinary, contours, -1, (0, 0, 255), -1);
	imwrite("1c.jpg", imgBinary);
	if (contours.size() != 4) {
		cout << "error: contours.size() = " << contours.size() << endl;
		return -1;
	}
	
	// 抠图 + 放射变换
	vector< vector<Point> > approx_contours(contours.size());
	for (int i = 0; i < contours.size(); i++)
    {
		// 拟合计算外接四边形
		approxPolyDP(Mat(contours[i]), approx_contours[i], 40, true);
		drawContours(imgBinary, approx_contours, i, (100, 100, 100), 2, 8);
		imwrite("1c.jpg", imgBinary);
		// 放射变换前四边形顶点计算排序，approx_contours 各定点乱序，重新排序
		vector<Point2f> srcPoints(4);					// 四边形顶点
		Scalar centerPoint = mean(approx_contours[i]);	// 四边形中心点
		//cout << "centerPoint:" << centerPoint << endl;
		for (int j = 0; j < contours.size(); j++)
		{
			if (approx_contours[i][j].x < centerPoint[0] && approx_contours[i][j].y < centerPoint[1]) {	// 左上
				srcPoints[0] = approx_contours[i][j];
			}
			if (approx_contours[i][j].x > centerPoint[0] && approx_contours[i][j].y < centerPoint[1]) {	// 右上
				srcPoints[1] = approx_contours[i][j];
			}
			if (approx_contours[i][j].x < centerPoint[0] && approx_contours[i][j].y > centerPoint[1]) {	// 左下
				srcPoints[2] = approx_contours[i][j];
			}
			if (approx_contours[i][j].x > centerPoint[0] && approx_contours[i][j].y > centerPoint[1]) {	// 右下
				srcPoints[3] = approx_contours[i][j];
			}
		}
		// 透视变换
		Mat dstImg(Size(320+10, 240+10), CV_8UC1);
		vector<Point2f> dstPoints(4);
		dstPoints[0] = Point2f(0, 0);
		dstPoints[1] = Point2f(dstImg.cols, 0);
		dstPoints[2] = Point2f(0, dstImg.rows);
		dstPoints[3] = Point2f(dstImg.cols, dstImg.rows);
		Mat Trans = getPerspectiveTransform(srcPoints, dstPoints);
		//cout << "srcPoints = " << srcPoints << endl;
		warpPerspective(srcImg, dstImg, Trans, Size(dstImg.cols, dstImg.rows), CV_INTER_CUBIC);
		// 裁掉边缘，有时边缘有黑边
		dstImg = dstImg(Rect(5, 5, dstImg.cols-10, dstImg.rows-10));
		// 测试卡排序，左上方开始顺时针
		if (centerPoint[0] < srcImg.cols/2 && centerPoint[1] < srcImg.rows/2) {	// 左上
			outCards[0] = dstImg;
			cout << " 0 --> srcPoints = \n" << srcPoints << endl;
		}
		if (centerPoint[0] > srcImg.cols/2 && centerPoint[1] < srcImg.rows/2) {	// 右上
			outCards[1] = dstImg;
			cout << " 1 --> srcPoints = \n" << srcPoints << endl;
		}
		if (centerPoint[0] < srcImg.cols/2 && centerPoint[1] > srcImg.rows/2) {	// 左下
			outCards[2] = dstImg;
			cout << " 2 --> srcPoints = \n" << srcPoints << endl;
		}
		if (centerPoint[0] > srcImg.cols/2 && centerPoint[1] > srcImg.rows/2) {	// 右下
			outCards[3] = dstImg;
			cout << " 3 --> srcPoints = \n" << srcPoints << endl;
		}
    }

	imwrite("1c.jpg", imgBinary);
	imwrite("1s0.jpg", outCards[0]);
	imwrite("1s1.jpg", outCards[1]);
	imwrite("1s2.jpg", outCards[2]);
	imwrite("1s3.jpg", outCards[3]);
	
	return 0;
}

// 清晰度计算
static int imgDefinition(Mat &srcImg)
{
	Mat imageSobel;
	float tenengrad;
	
	// 亮度归一化，不同卡位置亮度不均匀
	imgLut(srcImg, srcImg);
	// 梯度计算
	Sobel(srcImg, imageSobel, CV_16U, 1, 1);
	tenengrad = 1000 * mean(imageSobel)[0];
	//cout << "result:" << tenengrad << endl;
	// 保存图片
	//imwrite("1n.jpg", imageSobel);

	return static_cast <double>(tenengrad);
	
}

int main (int argc, char *argv[])
{

	Mat imgSource;
	Mat imgGray;
	
	string imgFileName("2.jpg");
	
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
	imwrite("1g.jpg", imgGray);
	// 测试卡提取
	vector<Mat> outCards(4);
	cardContour(imgGray, outCards);

	// 清晰度计算
	vector<int> cardFV(outCards.size());
	for (int i = 0; i < outCards.size(); i++)
	{
		cardFV[i] = imgDefinition(outCards[i]);
		cout << "cardFV[" << i << "]=" << cardFV[i] << endl;
	}

	return 0;	
}


