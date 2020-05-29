#include "diff2img.h"
#include <stdio.h>
#include <strings.h>

void diff2img(AXI_STREAM_8& video_in, AXI_STREAM_8& video_out,
		unsigned char* ref_mem, int rows, int cols, uint32_t& xmin,
		uint32_t& xmax, uint32_t& ymin, uint32_t& ymax) {
#pragma HLS DATAFLOW

#pragma HLS INTERFACE axis port=video_in
#pragma HLS INTERFACE axis port=video_out
#pragma HLS INTERFACE m_axi port=ref_mem

#pragma HLS INTERFACE s_axilite port=ref_mem bundle=ctrl_bus
#pragma HLS INTERFACE s_axilite port=rows bundle=ctrl_bus
#pragma HLS INTERFACE s_axilite port=cols bundle=ctrl_bus
#pragma HLS INTERFACE ap_vld port=xmin
#pragma HLS INTERFACE ap_vld port=xmax
#pragma HLS INTERFACE ap_vld port=ymin
#pragma HLS INTERFACE ap_vld port=ymax
#pragma HLS INTERFACE s_axilite port=return bundle=ctrl_bus

	IMAGE_C1 img0(rows, cols);
	IMAGE_C1 img1(rows, cols);

	bool isMinSet = false;
	uint32_t xmaxTemp = 0, ymaxTemp;

	hls::AXIvideo2Mat(video_in, img0);

	for (int i = 0; i < MAX_HEIGHT; ++i) {
		unsigned char imgBuffer[MAX_WIDTH] = { 0 };
		memcpy(imgBuffer, (unsigned char *) ref_mem + (i * MAX_WIDTH),
		MAX_WIDTH * sizeof(unsigned char));
		for (int j = 0; j < MAX_WIDTH; ++j) {
			int pixelValue = (int) img0.read().val[0];
			int comparisonPixel = (int) imgBuffer[j];
			if (pixelValue > comparisonPixel) {
				img1.write(255);
				if (!isMinSet) {
					xmin = j;
					ymin = i;
					isMinSet = true;
				}
				if(j > xmaxTemp)
					xmaxTemp = j;
				ymaxTemp = i;
			} else
				img1.write(0);
		}

	}
	xmax = xmaxTemp;
	ymax = ymaxTemp;
	hls::Mat2AXIvideo(img1, video_out);

}
