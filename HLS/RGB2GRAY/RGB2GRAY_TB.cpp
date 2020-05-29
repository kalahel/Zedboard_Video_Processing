#include "RGB2GRAY.h"
#include "hls_opencv.h"
#include <opencv2/opencv.hpp>
#include <stdio.h>


using namespace cv;

int main(int argc, char** argv) {
	Mat src_rgb(MAX_HEIGHT, MAX_WIDTH, CV_8UC3);
	Mat result(MAX_HEIGHT, MAX_WIDTH, CV_8UC1);
	Mat finalResult(MAX_HEIGHT, MAX_WIDTH, CV_8UC3);
	src_rgb = imread("E:\\Fac\\SoC\\Projet\\images\\base.jpg");
	AXI_STREAM videoIn;
	AXI_STREAM_8 videoOut;
	cvMat2AXIvideo(src_rgb, videoIn);

	RGB2GRAY(videoIn, videoOut, MAX_HEIGHT, MAX_WIDTH);

	AXIvideo2cvMat(videoOut, result);
    //cvtColor(result, finalResult, CV_GRAY2RGB);


	imwrite("E:\\Fac\\SoC\\Projet\\images\\result_RGB2GRAY.jpg", result);

	return 0;
}
