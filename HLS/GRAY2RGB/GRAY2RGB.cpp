#include "GRAY2RGB.h"
#include <stdio.h>

void GRAY2RGB(AXI_STREAM_8& video_in, AXI_STREAM& video_out, int rows, int cols) {
#pragma HLS DATAFLOW

#pragma HLS INTERFACE axis port=video_in
#pragma HLS INTERFACE axis port=video_out
#pragma HLS INTERFACE s_axilite port=rows bundle=ctrl_bus
#pragma HLS INTERFACE s_axilite port=cols bundle=ctrl_bus
#pragma HLS INTERFACE s_axilite port=return bundle=ctrl_bus

	IMAGE_C1 img0(rows, cols);
	IMAGE_RGB img1(rows, cols);

	hls::AXIvideo2Mat(video_in, img0);
	hls::CvtColor<HLS_GRAY2RGB>(img0, img1);
	hls::Mat2AXIvideo(img1, video_out);

}
