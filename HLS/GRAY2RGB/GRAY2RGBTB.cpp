#include "GRAY2RGB.h"
#include "hls_opencv.h"
#include <opencv2/opencv.hpp>
#include <stdio.h>


using namespace cv;

int main(int argc, char** argv) {
	Mat src_rgb(MAX_HEIGHT, MAX_WIDTH, CV_8UC1);
	Mat result(MAX_HEIGHT, MAX_WIDTH, CV_8UC3);
	src_rgb = imread("E:\\Fac\\SoC\\Projet\\images\\result_RGB2GRAY.jpg",0);
	AXI_STREAM_8 videoIn;
	AXI_STREAM videoOut;
	cvMat2AXIvideo(src_rgb, videoIn);

	GRAY2RGB(videoIn, videoOut, MAX_HEIGHT, MAX_WIDTH);

	AXIvideo2cvMat(videoOut, result);


	imwrite("E:\\Fac\\SoC\\Projet\\images\\result_GRAY2RGB.jpg", result);

	return 0;
}
