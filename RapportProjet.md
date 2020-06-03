# Projet SoC

Mathieu Hannoun

## Modules HLS

### Test Bench et .h

Tout les test bench suivront plus ou moins le même code de base :

```cpp
#include "RGB2GRAY.h"
#include "hls_opencv.h"
#include <opencv2/opencv.hpp>
#include <stdio.h>

using namespace cv;

int main(int argc, char** argv) {
	Mat src_rgb(MAX_HEIGHT, MAX_WIDTH, CV_8UC3);
	Mat result(MAX_HEIGHT, MAX_WIDTH, CV_8UC3);
	src_rgb = imread("E:\\Fac\\SoC\\Projet\\images\\base.jpg");
	AXI_STREAM videoIn, videoOut;
	cvMat2AXIvideo(src_rgb, videoIn);
	
    // Insert function to test here
	RGB2GRAY(videoIn, videoOut, MAX_HEIGHT, MAX_WIDTH);
    
	AXIvideo2cvMat(videoOut, result);

	imwrite("E:\\Fac\\SoC\\Projet\\images\\result_RGB2GRAY.jpg", result);

	return 0;
}

```

Il en va de même pour les .h, qui eux suivent cette structure :

```cpp
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


```



### RGB2GRAY

#### CPP

```cpp
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
```

#### TB

```cpp
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
	AXI_STREAM videoIn, videoOut;
	cvMat2AXIvideo(src_rgb, videoIn);

	RGB2GRAY(videoIn, videoOut, MAX_HEIGHT, MAX_WIDTH);

	AXIvideo2cvMat(videoOut, result);

	imwrite("E:\\Fac\\SoC\\Projet\\images\\result_RGB2GRAY.jpg", result);

	return 0;
}

```



#### Résultat

On passe de cette image :

![base](https://i.ibb.co/HYyKmKT/base.jpg)

A ceci :

![result](https://i.ibb.co/NtrNTLQ/result-RGB2-GRAY.jpg)

#### Synthèse

![synthese](https://i.ibb.co/FwZg4yT/Capture.png)

### GRAY2RGB

#### CPP

```cpp
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
```

#### TestBench

```cpp
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
```



#### Synthèse

![syntese](https://i.ibb.co/BCMfXm4/Capture.png)

### Background removal simple

Ceci traite de la suppression du fond sans procéder à une érosion/dilatation et moyennage.

#### CPP

Les lignes de l'image de référence sont copiées d'un bloc pour optimiser le burst de l'axi master (à voir si cela n'est pas trop volumineux pour un unique transfert).

```cpp
#include "backgroundRemoval.h"
#include <stdio.h>
#include <strings.h>

void backgroundRemoval(AXI_STREAM_8& video_in, AXI_STREAM_8& video_out,
		unsigned char* ref_mem, int rows, int cols, uint32_t thresh) {
#pragma HLS DATAFLOW

#pragma HLS INTERFACE axis port=video_in
#pragma HLS INTERFACE axis port=video_out
#pragma HLS INTERFACE m_axi port=ref_mem
#pragma HLS INTERFACE s_axilite port=rows bundle=ctrl_bus
#pragma HLS INTERFACE s_axilite port=cols bundle=ctrl_bus
#pragma HLS INTERFACE s_axilite port=thresh bundle=ctrl_bus
#pragma HLS INTERFACE s_axilite port=return bundle=ctrl_bus

	IMAGE_C1 img0(rows, cols);
	IMAGE_C1 img1(rows, cols);

	hls::AXIvideo2Mat(video_in, img0);
	for (int i = 0; i < MAX_HEIGHT; ++i) {
		unsigned char imgBuffer[MAX_WIDTH] = { 0 };
		memcpy(imgBuffer, (unsigned char *) ref_mem + (i * MAX_WIDTH),
		MAX_WIDTH * sizeof(unsigned char));
		for (int j = 0; j < MAX_WIDTH; ++j) {
			int pixelValue = (int) img0.read().val[0];
			int comparisonPixel = (int) imgBuffer[j];
			if (((pixelValue - comparisonPixel) * (pixelValue - comparisonPixel))
					> (int) thresh)
				img1.write(255);
			else
				img1.write(0);
		}

	}
	hls::Mat2AXIvideo(img1, video_out);

}
```

### TB

```cpp
#include "backgroundRemoval.h"
#include "hls_opencv.h"
#include <opencv2/opencv.hpp>
#include <stdio.h>

using namespace cv;

int main(int argc, char** argv) {
	Mat src_rgb(MAX_HEIGHT, MAX_WIDTH, CV_8UC1);
	Mat moved(MAX_HEIGHT, MAX_WIDTH, CV_8UC1);
	Mat result(MAX_HEIGHT, MAX_WIDTH, CV_8UC1);

	src_rgb = imread("E:\\Fac\\SoC\\Projet\\images\\baseGray.jpg", 0);
	moved = imread("E:\\Fac\\SoC\\Projet\\images\\baseGrayMoved.jpg", 0);

	unsigned char * movedArray = moved.data;

	AXI_STREAM_8 videoIn, videoOut;
	cvMat2AXIvideo(src_rgb, videoIn);

	backgroundRemoval(videoIn, videoOut, movedArray, MAX_HEIGHT, MAX_WIDTH, 30);

	AXIvideo2cvMat(videoOut, result);

	imwrite("E:\\Fac\\SoC\\Projet\\images\\backgroundRemoval.jpg", result);

	return 0;
}
```

#### Synthèse

![synth](https://i.ibb.co/7J3yDbL/Capture.png)

#### Résultat

Tout les résultats suivant sont obtenus avec `treshold = 30`.

Image d'origine :

![origin](https://i.ibb.co/gzF29xN/baseGray.jpg)

Image avec deux éléments décalés :

![moved](https://i.ibb.co/fMpn2xC/base-Gray-Moved.jpg)

Résultat :

![result](https://i.ibb.co/K62kfRW/background-Removal.jpg)

### BackgroundRemoval complet - Filtrage final

Ceci est une reprise de l'ip précédente en y ajoutant une opération de dilatation/érosion puis un filtre gaussien.

### CPP

```cpp
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
	IMAGE_C1 img1(rows, cols);
	IMAGE_C1 imgEroded(rows, cols);
	IMAGE_C1 imgDilated(rows, cols);
	IMAGE_C1 imgBlured(rows, cols);



	hls::AXIvideo2Mat(video_in, img0);
	for (int i = 0; i < MAX_HEIGHT; ++i) {
		unsigned char imgBuffer[MAX_WIDTH] = { 0 };
		memcpy(imgBuffer, (unsigned char *) ref_mem + (i * MAX_WIDTH),
		MAX_WIDTH * sizeof(unsigned char));
		for (int j = 0; j < MAX_WIDTH; ++j) {
			int pixelValue = (int) img0.read().val[0];
			int comparisonPixel = (int) imgBuffer[j];
			if (((pixelValue - comparisonPixel) * (pixelValue - comparisonPixel))
					> (int) thresh)
				img1.write(255);
			else
				img1.write(0);
		}

	}
 
	hls::Dilate(img1, imgDilated);
	hls::Erode(imgDilated, imgEroded);
	hls::GaussianBlur<5,5>(imgEroded, imgBlured);
	hls::Mat2AXIvideo(imgBlured, video_out);
}
```

#### Résultat

En prenant les même image que précédemment voici le résultat obtenu :

![results](https://i.ibb.co/X2298sX/background-Removal.jpg)

### BackgroundRemoval complet - Filtrage initial

Le  coeur de l'ip précédente est reprise mais cette fois en ajoutant un filtrage gaussien à l'image courante, l'image de référence étant stocké avec un filtre déjà appliqué pour réduire la charge de calcul. La différence des deux images est faites puis on applique ensuite une fonction de dilatation et d'érosion.

#### CPP

```cpp
#include "backgroundRemoval.h"
#include <stdio.h>
#include <strings.h>

void backgroundRemoval(AXI_STREAM_8& video_in, AXI_STREAM_8& video_out,
		unsigned char* ref_mem, int rows, int cols, uint32_t thresh) {
#pragma HLS DATAFLOW

#pragma HLS INTERFACE axis port=video_in
#pragma HLS INTERFACE axis port=video_out
#pragma HLS INTERFACE m_axi port=ref_mem depth=2073600 max_read_burst_length=1080 num_write_outstanding=1080*4

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

	for (int i = 0; i < MAX_HEIGHT; ++i) {
		unsigned char imgBuffer[MAX_WIDTH] = { 0 };
		memcpy(imgBuffer, (unsigned char *) ref_mem + (i * MAX_WIDTH),
		MAX_WIDTH * sizeof(unsigned char));
		for (int j = 0; j < MAX_WIDTH; ++j) {
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
```



#### TB

```cpp
#include "backgroundRemoval.h"
#include "hls_opencv.h"
#include <opencv2/opencv.hpp>
#include <stdio.h>

using namespace cv;

int main(int argc, char** argv) {
	Mat src_rgb(MAX_HEIGHT, MAX_WIDTH, CV_8UC1);
	Mat blured(MAX_HEIGHT, MAX_WIDTH, CV_8UC1);
	Mat moved(MAX_HEIGHT, MAX_WIDTH, CV_8UC1);
	Mat result(MAX_HEIGHT, MAX_WIDTH, CV_8UC1);

	src_rgb = imread("E:\\Fac\\SoC\\Projet\\images\\baseGray.jpg", 0);
	moved = imread("E:\\Fac\\SoC\\Projet\\images\\baseGrayMoved.jpg", 0);

	GaussianBlur(src_rgb, blured, Size(5, 5), 0);

	unsigned char * bluredArray = blured.data;


	AXI_STREAM_8 videoIn, videoOut;
	cvMat2AXIvideo(moved, videoIn);

	backgroundRemoval(videoIn, videoOut, bluredArray, MAX_HEIGHT, MAX_WIDTH, 30);

	AXIvideo2cvMat(videoOut, result);

	imwrite("E:\\Fac\\SoC\\Projet\\images\\backgroundRemoval2.jpg", result);

	return 0;
}
```

#### Résultat

![result](https://i.ibb.co/yXXpJbb/background-Removal2.jpg)

Le résultat semble moins englobant que celui précédent.

### Diff2img

#### CPP

```cpp
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
```

J'évite de faire la soustraction simple entre les deux images pour ne pas avoir de valeurs de pixel négatives.

### TB

```cpp
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

	diff2img(videoIn, videoOut, srcArray, MAX_HEIGHT, MAX_WIDTH, xmin, xmax,
			ymin, ymax);
	AXIvideo2cvMat(videoOut, result);

	imwrite("E:\\Fac\\SoC\\Projet\\images\\diff2img.jpg", result);
	printf("Min : [%d:%d]\tMax : [%d:%d]\n", xmin, ymin, xmax, ymax);

	return 0;
}
```

#### Synthèse

![synth](https://i.ibb.co/kHbZHyz/Capture.png)

#### Résultats

J'utilise ces deux images préalablement isolé du fond en utilisant l'ip précédente. Entre les deux images, le personnage à été légèrement décalé vers la droite.

![img1](https://i.ibb.co/yXXpJbb/background-Removal2.jpg)

![img2](https://i.ibb.co/8bSgvP4/background-Removal3.jpg)

Ce qui donne :

![res](https://i.ibb.co/RzjkBys/diff2img.jpg)

Même si du bruit subsiste on note clairement que la différence majoritaire est bien une bande vers la droite.

Voici les résultats donnée pour les positions [x,y] :

`Min : [880:408]	Max : [1223:894]`

Voici ce qu'on obtient en traçant un rectangle rouge à ces coordonnées :

![resbouding](https://i.ibb.co/pz2MTxb/diff2img-Bounding.jpg)

L'objet qui à bougé à donc été correctement détecté.

### Contour

Ip traçant un rectangle rouge en utilisant les coordonnées basé donné  par 'l'ip précédente.

#### CPP

```cpp
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
				// Drawing bounding box in pure red (it seems to be in BGR order)
				img1.write(hls::Scalar<3, unsigned char>(0, 0, 0xFF));
			} else {
				img1.write(pixelValue);
			}
		}
	}
	hls::Mat2AXIvideo(img1, video_out);
}
```

#### TB

```cpp
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
```

#### Synthèse

![synth](https://i.ibb.co/N6s5bT7/Capture.png)

#### Résultat

En ce basant sur les résultats de l'ip précédente voici le résultat de cette ip :

![results](https://i.ibb.co/Y0Nzrzy/contour.jpg)

La bounding box est bien là on elle devrait être, le mouvement vers la droite à bien été pris en compte.

## Vivado / Vitis

### Test AXI-Stream

Pour se familiariser avec le module DMA, j'ai réaliser un montage avec une simple FIFO.

#### Vivado

![vivado](https://i.ibb.co/MkNwDSC/Capture.png)

#### Vitis

```cpp
/******************************************************************************
 *
 * Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * Use of the Software is limited solely to applications:
 * (a) running on a Xilinx device, or
 * (b) that interact with a Xilinx device through a bus or interconnect.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Xilinx shall not be used
 * in advertising or otherwise to promote the sale, use or other dealings in
 * this Software without prior written authorization from Xilinx.
 *
 ******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xaxidma.h"

#define ROWS 9
#define COLS 9
#define SIZE_ARR (COLS * ROWS)

unsigned char background[SIZE_ARR];
unsigned char img[SIZE_ARR];
unsigned char resultBuffer[SIZE_ARR];
int main() {
	init_platform();
	Xil_DCacheDisable();

	for (int i = 0; i < SIZE_ARR; ++i) {
		img[i] = (unsigned char)i;
	}
	for (int i = 0; i < SIZE_ARR; ++i) {
		resultBuffer[i] = 5u;
	}
	printf("\n__Fifo__\n\r");

	// DMA setup
	XAxiDma dma;
	XAxiDma_Config * dmaDevice = XAxiDma_LookupConfig(XPAR_AXI_DMA_0_DEVICE_ID);
	int status = XAxiDma_CfgInitialize(&dma, dmaDevice);
	if (status != 0)
		printf("ERROR\n");
	XAxiDma_IntrDisable(&dma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(&dma, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
	// flush buffers cache
	Xil_DCacheFlushRange( img, SIZE_ARR * sizeof(unsigned char));
	Xil_DCacheFlushRange( resultBuffer, SIZE_ARR * sizeof(unsigned char));

	printf("Sending data\n");

	status = XAxiDma_SimpleTransfer(&dma,  img, SIZE_ARR * sizeof(unsigned char),
	XAXIDMA_DMA_TO_DEVICE);
	if (status != 0)
		printf("ERROR TRANSFERT PS->PL FAILED\n");

	printf("Getting data\n");
	status = XAxiDma_SimpleTransfer(&dma, resultBuffer, SIZE_ARR * sizeof(unsigned char),
	XAXIDMA_DEVICE_TO_DMA);
	if (status != 0)
		printf("ERROR TRANSFERT PL->PS FAILED\n");
	Xil_DCacheInvalidateRange(resultBuffer, SIZE_ARR * sizeof(unsigned char));
	// Reading data
	for (int i = 0; i < SIZE_ARR; i++) {
		if(i % (COLS) == 0 && i !=0)
			printf("\n");
		printf("%u, ", resultBuffer[i]);
	}

	cleanup_platform();
	return 0;
}
```

#### Résultats

```
__Fifo__
Sending data
Getting data
0, 1, 2, 3, 4, 5, 6, 7, 8,
9, 10, 11, 12, 13, 14, 15, 16, 17,
18, 19, 20, 21, 22, 23, 24, 25, 26,
27, 28, 29, 30, 31, 32, 33, 34, 35,
36, 37, 38, 39, 40, 41, 42, 43, 44,
45, 46, 47, 48, 49, 50, 51, 52, 53,
54, 55, 56, 57, 58, 59, 60, 61, 62,
63, 64, 65, 66, 67, 68, 69, 70, 71,
72, 73, 74, 75, 76, 77, 78, 79, 80,
```

Le transfert s'est effectué correctement.

### BackgroundRemoval

Voici une architecture simple permettant d'utiliser une dma configurer pour des mots de 8 bits pour tester le module  `BackgroundRemoval`.

