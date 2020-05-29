
/*******************************************************************************/
#ifndef SOBEL_H_
#define SOBEL_H_
#include "hls_video.h"

#define MAX_HEIGHT	1080
#define MAX_WIDTH	1920

// typedef video library core structures
typedef hls::stream<ap_axiu<24,1,1,1> >               AXI_STREAM;
typedef hls::stream<ap_axiu<8,1,1,1> >               AXI_STREAM_8;

typedef hls::Mat<MAX_HEIGHT, MAX_WIDTH, HLS_8UC1>     	IMAGE_C1;
typedef hls::Mat<MAX_HEIGHT, MAX_WIDTH, HLS_8UC3>     	IMAGE_RGB;

void RGB2GRAY(AXI_STREAM& video_in, AXI_STREAM_8& video_out, int rows, int cols);

#endif
