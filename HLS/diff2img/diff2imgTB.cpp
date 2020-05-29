#include "diff2img.h"
#include "hls_opencv.h"
#include <opencv2/opencv.hpp>
#include <stdio.h>

using namespace cv;

int main(int argc, char** argv) {
	Mat src_rgb(MAX_HEIGHT, MAX_WIDTH, CV_8UC1);
	Mat moved(MAX_HEIGHT, MAX_WIDTH, CV_8UC1);
	Mat result(MAX_HEIGHT, MAX_WIDTH, CV_8UC1);

	src_rgb = imread("E:\\Fac\\SoC\\Projet\\images\\backgroundRemoval2.jpg", 0);
	moved = imread("E:\\Fac\\SoC\\Projet\\images\\backgroundRemoval3.jpg", 0);

	unsigned char * srcArray = src_rgb.data;
	uint32_t xmin, ymin, xmax, ymax;

	AXI_STREAM_8 videoIn, videoOut;
	cvMat2AXIvideo(moved, videoIn);

	//backgroundRemoval(videoIn, videoOut, movedArray, MAX_HEIGHT, MAX_WIDTH, 30);

	diff2img(videoIn, videoOut, srcArray, MAX_HEIGHT, MAX_WIDTH, xmin, xmax,
			ymin, ymax);
	AXIvideo2cvMat(videoOut, result);

	imwrite("E:\\Fac\\SoC\\Projet\\images\\diff2img.jpg", result);
	printf("Min : [%d:%d]\tMax : [%d:%d]\n", xmin, ymin, xmax, ymax);

	return 0;
}
