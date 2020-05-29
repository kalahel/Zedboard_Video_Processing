#include "RGB2GRAY.h"
#include <stdio.h>

void RGB2GRAY(AXI_STREAM& video_in, AXI_STREAM_8& video_out, int rows, int cols) {
#pragma HLS DATAFLOW

#pragma HLS INTERFACE axis port=video_in
#pragma HLS INTERFACE axis port=video_out
#pragma HLS INTERFACE s_axilite port=rows bundle=ctrl_bus
#pragma HLS INTERFACE s_axilite port=cols bundle=ctrl_bus
#pragma HLS INTERFACE s_axilite port=return bundle=ctrl_bus

	IMAGE_RGB img0(rows, cols);
	IMAGE_C1 img1(rows, cols);

	hls::AXIvideo2Mat(video_in, img0);
	hls::CvtColor<HLS_RGB2GRAY>(img0, img1);
	hls::Mat2AXIvideo(img1, video_out);

}
