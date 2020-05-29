#include "contour.h"
#include "hls_opencv.h"
#include <opencv2/opencv.hpp>
#include <stdio.h>

using namespace cv;

int main(int argc, char** argv) {
	Mat src_rgb(MAX_HEIGHT, MAX_WIDTH, CV_8UC3);
	Mat result(MAX_HEIGHT, MAX_WIDTH, CV_8UC3);

	src_rgb = imread("E:\\Fac\\SoC\\Projet\\images\\baseColorMoved.jpg");

	AXI_STREAM videoIn, videoOut;
	cvMat2AXIvideo(src_rgb, videoIn);

	uint32_t xmin = 880;
	uint32_t xmax = 1223;
	uint32_t ymin = 408;
	uint32_t ymax = 894;

	contour(videoIn, videoOut, MAX_HEIGHT, MAX_WIDTH, xmin, xmax, ymin, ymax);

	AXIvideo2cvMat(videoOut, result);

	imwrite("E:\\Fac\\SoC\\Projet\\images\\contour.jpg", result);

	return 0;
}
