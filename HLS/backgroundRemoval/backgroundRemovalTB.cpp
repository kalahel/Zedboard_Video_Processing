#include "backgroundRemoval.h"
#include "hls_opencv.h"
#include <opencv2/opencv.hpp>
#include <stdio.h>

using namespace cv;

int main(int argc, char** argv) {
	Mat src_rgb(MAX_HEIGHT, MAX_WIDTH, CV_8UC1);
	Mat blured(MAX_HEIGHT, MAX_WIDTH, CV_8UC1);
	Mat moved(MAX_HEIGHT, MAX_WIDTH, CV_8UC1);
	Mat result(MAX_HEIGHT, MAX_WIDTH, CV_8UC1);

	src_rgb = imread("E:\\Fac\\SoC\\Projet\\images\\baseGray.jpg", 0);
	moved = imread("E:\\Fac\\SoC\\Projet\\images\\baseGrayMoved2.jpg", 0);

	GaussianBlur(src_rgb, blured, Size(5, 5), 0);

	unsigned char * bluredArray = blured.data;


	AXI_STREAM_8 videoIn, videoOut;
	cvMat2AXIvideo(moved, videoIn);

	backgroundRemoval(videoIn, videoOut, bluredArray, MAX_HEIGHT, MAX_WIDTH, 30);

	AXIvideo2cvMat(videoOut, result);

	imwrite("E:\\Fac\\SoC\\Projet\\images\\backgroundRemoval36.jpg", result);

	return 0;
}
