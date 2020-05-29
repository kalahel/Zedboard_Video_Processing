#include "backgroundRemoval.h"
#include <stdio.h>
#include <strings.h>

void backgroundRemoval(AXI_STREAM_8& video_in, AXI_STREAM_8& video_out,
		unsigned char* ref_mem, int rows, int cols, uint32_t thresh) {
#pragma HLS DATAFLOW

#pragma HLS INTERFACE axis port=video_in
#pragma HLS INTERFACE axis port=video_out
#pragma HLS INTERFACE m_axi port=ref_mem

#pragma HLS INTERFACE s_axilite port=ref_mem bundle=ctrl_bus
#pragma HLS INTERFACE s_axilite port=rows bundle=ctrl_bus
#pragma HLS INTERFACE s_axilite port=cols bundle=ctrl_bus
#pragma HLS INTERFACE s_axilite port=thresh bundle=ctrl_bus
#pragma HLS INTERFACE s_axilite port=return bundle=ctrl_bus

	IMAGE_C1 img0(rows, cols);
	IMAGE_C1 imgBlured(rows, cols);
	IMAGE_C1 img1(rows, cols);
	IMAGE_C1 imgEroded(rows, cols);
	IMAGE_C1 imgDilated(rows, cols);



	hls::AXIvideo2Mat(video_in, img0);
	hls::GaussianBlur<5,5>(img0, imgBlured);

	for (int i = 0; i < rows; ++i) {
		unsigned char imgBuffer[MAX_WIDTH] = { 0 };
		memcpy(imgBuffer, (unsigned char *) ref_mem + (i * cols),
				cols * sizeof(unsigned char));
		for (int j = 0; j < cols; ++j) {
			int pixelValue = (int) imgBlured.read().val[0];
			int comparisonPixel = (int) imgBuffer[j];
			if (((pixelValue - comparisonPixel) * (pixelValue - imgBuffer[j]))
					> (int) thresh)
				img1.write(255);
			else
				img1.write(0);
		}

	}
	hls::Dilate(img1, imgDilated);
	hls::Erode(imgDilated, imgEroded);
	hls::Mat2AXIvideo(imgEroded, video_out);

}

