#include "contour.h"
#include <stdio.h>
#include <strings.h>

void contour(AXI_STREAM& video_in, AXI_STREAM& video_out, int rows, int cols,
		uint32_t& xmin, uint32_t& xmax, uint32_t& ymin, uint32_t& ymax) {
#pragma HLS DATAFLOW

#pragma HLS INTERFACE axis port=video_in
#pragma HLS INTERFACE axis port=video_out
#pragma HLS INTERFACE m_axi port=ref_mem

#pragma HLS INTERFACE s_axilite port=rows bundle=ctrl_bus
#pragma HLS INTERFACE s_axilite port=cols bundle=ctrl_bus
#pragma HLS INTERFACE ap_none port=xmin
#pragma HLS INTERFACE ap_none port=xmax
#pragma HLS INTERFACE ap_none port=ymin
#pragma HLS INTERFACE ap_none port=ymax
#pragma HLS INTERFACE s_axilite port=return bundle=ctrl_bus

	IMAGE_RGB img0(rows, cols);
	IMAGE_RGB img1(rows, cols);

	hls::AXIvideo2Mat(video_in, img0);

	for (int i = 0; i < MAX_HEIGHT; ++i) {
		for (int j = 0; j < MAX_WIDTH; ++j) {
			hls::Scalar<3, unsigned char> pixelValue = img0.read();

			if (((i == ymin || i == ymax) && (j >= xmin && j <= xmax))
					|| ((i >= ymin && i <= ymax) && (j == xmin || j == xmax))) {
				// Drawing bounding box in pure red
				img1.write(hls::Scalar<3, unsigned char>(0, 0, 0xFF));
			} else {
				img1.write(pixelValue);
			}
		}
	}
	hls::Mat2AXIvideo(img1, video_out);
}
