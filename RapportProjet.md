# Projet SoC

Mathieu Hannoun

https://github.com/kalahel/Zedboard_Video_Processing

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

#### Vivado

![vivado](https://i.ibb.co/12Wy4jH/Capture.png)

#### Vitis

```cpp
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xbackgroundremoval.h"
#include "xaxidma.h"

#define ROWS 1080
#define COLS 1920
#define SIZE_ARR (COLS * ROWS)

unsigned char background[SIZE_ARR];
unsigned char img[SIZE_ARR];
unsigned char resultBuffer[SIZE_ARR];
int main() {
	init_platform();
	Xil_DCacheDisable();
	for (int i = 0; i < SIZE_ARR; ++i) {
		background[i] = 0;
	}
	for (int i = 0; i < SIZE_ARR; ++i) {
		img[i] = (unsigned char)100u;
	}
	for (int i = 0; i < SIZE_ARR; ++i) {
		resultBuffer[i] = 5u;
	}
	printf("\n__Background Removal__\n\r");

	// Background remover setup
	XBackgroundremoval backIp;
	XBackgroundremoval_Config * backConfig = XBackgroundremoval_LookupConfig(
	XPAR_XBACKGROUNDREMOVAL_0_DEVICE_ID);
	int status = XBackgroundremoval_CfgInitialize(&backIp, backConfig);
	if (status != 0)
		printf("ERROR\n");
	status = XBackgroundremoval_Initialize(&backIp, backConfig->DeviceId);
	if (status != 0)
		printf("ERROR\n");

	XBackgroundremoval_Start(&backIp);
	XBackgroundremoval_EnableAutoRestart(&backIp);
	XBackgroundremoval_Set_ref_mem(&backIp, background);
	XBackgroundremoval_Set_cols(&backIp, COLS);
	XBackgroundremoval_Set_rows(&backIp, ROWS);
	XBackgroundremoval_Set_thresh(&backIp, 30);


	// DMA setup
	XAxiDma dma;
	XAxiDma_Config * dmaDevice = XAxiDma_LookupConfig(XPAR_AXI_DMA_0_DEVICE_ID);
	status = XAxiDma_CfgInitialize(&dma, dmaDevice);
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

Malheureusement même si tout les transferts renvoient `XST_SUCCESS` signalant le bon déroulement de l'échange, le résultat est incorrect, aucune valeur de  `resultBuffer` n'est remplacée.

Même après une validation par C/RTL co-simulation validant le fonctionnement individuel de l'ip, je ne réussis pas à la faire fonctionner au sein de la partie PS.

![valid](https://i.ibb.co/0p5jHnN/Capture.png)

### Architecture complète

L'architecture suivante comprends tout les éléments du projet ainsi que l'entièreté de la chaine de traitement, elle n'utilise pas de composants soumis à des licences.

#### Vivado

![vi1](https://i.ibb.co/KsRrPnt/Capture2.png)

En orange se trouve les deux VDMA, à gauche la première en lecture écriture qui envoi les images couleurs sur 24 bits, image passant par un `axi_broadcaster` pour diffuser le flux aux deux partie de la chaine de traitement et récupérant l'image avec le rectangle rouge à la sortie de l'IP contour. A droite une VDMA en écriture seul recevant le flux inutile issu de la partie détection de contours.

En violet la partie détection de contour composé de `RGB2GRAY` => `backgroundRemoval` => `diff2img` => `GRAY2RGB`, le flux vidéo à la sortie de ce traitement n'est pas primordial mais est tout de même récupéré par la deuxième DMA pour s'assurer de la bonne consommation des données.

![purple](https://i.ibb.co/QpRZfBZ/Capture.png)

En vert les IP `diff2img` donnant les signaux permettant de tracer le rectangle rouge à l'IP `contour`, les signaux attestant de la validité de ceux-ci ne sont pas connectés, on pourrait imaginer ajouté un port à `contour` pour les tester.

![green](https://i.ibb.co/1TrCnGv/Capture.png)

#### Vitis

Le code Vitis incorpore tous les éléments nécessaire au fonctionnement des IP présentes dans le design, se charge de l'envoi et de la réception du flux.

Malheureusement OpenCV n'étant pas disponible sous Vitis, il est impossible de tester réellement le composant bien qu'il compile sans problème. 

```cpp
#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xbackgroundremoval.h"
#include "xaxivdma.h"
#include "xgray2rgb.h"
#include "xrgb2gray.h"
#include "xdiff2img.h"
#include "xcontour.h"

#define ROWS 1080
#define COLS 1920
#define SIZE_ARR (COLS * ROWS)

unsigned char background[SIZE_ARR];
unsigned char diffImg[SIZE_ARR];
unsigned char img[SIZE_ARR];
unsigned char resultBuffer[SIZE_ARR];

int main() {
	init_platform();
	Xil_DCacheDisable();
	for (int i = 0; i < SIZE_ARR; ++i) {
		background[i] = 0;
		diffImg[i] = 0;
	}
	for (int i = 0; i < SIZE_ARR; ++i) {
		img[i] = (unsigned char) 100u;
	}
	for (int i = 0; i < SIZE_ARR; ++i) {
		resultBuffer[i] = 5u;
	}
	printf("\n__Projet SoC__\n\r");

	// Background remover setup
	XBackgroundremoval backIp;
	XBackgroundremoval_Config * backConfig = XBackgroundremoval_LookupConfig(
	XPAR_XBACKGROUNDREMOVAL_0_DEVICE_ID);
	int status = XBackgroundremoval_CfgInitialize(&backIp, backConfig);
	if (status != 0)
		printf("ERROR\n");
	status = XBackgroundremoval_Initialize(&backIp, backConfig->DeviceId);
	if (status != 0)
		printf("ERROR\n");
	XBackgroundremoval_Set_ref_mem(&backIp, background);
	XBackgroundremoval_Set_cols(&backIp, COLS);
	XBackgroundremoval_Set_rows(&backIp, ROWS);
	XBackgroundremoval_Set_thresh(&backIp, 30);
	XBackgroundremoval_Start(&backIp);
	XBackgroundremoval_EnableAutoRestart(&backIp);

	// GRAY2RGB
	XGray2rgb gray2rgbIp;
	XGray2rgb_Config * gray2rgbConfig = XGray2rgb_LookupConfig(
			XPAR_XGRAY2RGB_0_DEVICE_ID);
	status = XGray2rgb_CfgInitialize(&gray2rgbIp, gray2rgbConfig);
	if (status != 0)
		printf("ERROR\n");
	status = XGray2rgb_Initialize(&gray2rgbIp, gray2rgbConfig->DeviceId);
	if (status != 0)
		printf("ERROR\n");
	XGray2rgb_Set_cols(&gray2rgbIp, COLS);
	XGray2rgb_Set_rows(&gray2rgbIp, ROWS);
	XGray2rgb_EnableAutoRestart(&gray2rgbIp);
	XGray2rgb_Start(&gray2rgbIp);

	// RGB2GRAY
	XRgb2gray rgb2grayIp;
	XRgb2gray_Config * rgb2grayConfig = XRgb2gray_LookupConfig(
			XPAR_RGB2GRAY_0_DEVICE_ID);
	status = XRgb2gray_CfgInitialize(&rgb2grayIp, rgb2grayConfig);
	if (status != 0)
		printf("ERROR\n");
	status = XRgb2gray_Initialize(&rgb2grayIp, rgb2grayConfig->DeviceId);
	if (status != 0)
		printf("ERROR\n");
	XRgb2gray_Set_cols(&rgb2grayIp, COLS);
	XRgb2gray_Set_rows(&rgb2grayIp, ROWS);
	XRgb2gray_EnableAutoRestart(&rgb2grayIp);
	XRgb2gray_Start(&rgb2grayIp);

	// diff2img
	XDiff2img diff2img;
	XDiff2img_Config * diff2imgConfig = XDiff2img_LookupConfig(
			XPAR_DIFF2IMG_0_DEVICE_ID);
	status = XDiff2img_CfgInitialize(&diff2img, diff2imgConfig);
	if (status != 0)
		printf("ERROR\n");
	status = XDiff2img_Initialize(&diff2img, diff2imgConfig->DeviceId);
	if (status != 0)
		printf("ERROR\n");
	XDiff2img_Set_cols(&diff2img, COLS);
	XDiff2img_Set_rows(&diff2img, ROWS);
	XDiff2img_Set_ref_mem(&diff2img, diffImg);
	XDiff2img_EnableAutoRestart(&diff2img);
	XDiff2img_Start(&diff2img);

	// contour
	XContour contourIp;
	XContour_Config * contourConfig = XContour_LookupConfig(
			XPAR_XCONTOUR_0_DEVICE_ID);
	status = XContour_CfgInitialize(&contourIp, contourConfig);
	if (status != 0)
		printf("ERROR\n");
	status = XContour_Initialize(&contourIp, contourConfig->DeviceId);
	if (status != 0)
		printf("ERROR\n");
	XContour_Set_cols(&contourIp, COLS);
	XContour_Set_rows(&contourIp, ROWS);
	XContour_EnableAutoRestart(&contourIp);
	XContour_Start(&contourIp);

	// VDMA 0
	XAxiVdma vdmaIp;
	XAxiVdma_DmaSetup vdmaSetUp;
	UINTPTR dma0Addr;
	XAxiVdma_Config * vdmaConfig = XAxiVdma_LookupConfig(
			XPAR_AXI_VDMA_0_DEVICE_ID);
	status = XAxiVdma_CfgInitialize(&vdmaIp, vdmaConfig, dma0Addr);
	if (status != 0)
		printf("ERROR\n");

	XAxiVdma_IntrDisable(&vdmaIp, XAXIVDMA_S2MM_IRQ_ERR_ALL_MASK,
			XAXIVDMA_READ);
	XAxiVdma_IntrDisable(&vdmaIp, XAXIVDMA_S2MM_IRQ_ERR_ALL_MASK,
			XAXIVDMA_WRITE);

	// VDMA 1 - Write only
	XAxiVdma vdma1Ip;
	UINTPTR dma1Addr;
	XAxiVdma_Config * vdma1Config = XAxiVdma_LookupConfig(
			XPAR_AXI_VDMA_1_DEVICE_ID);
	status = XAxiVdma_CfgInitialize(&vdma1Ip, vdma1Config, dma1Addr);
	if (status != 0)
		printf("ERROR\n");
	XAxiVdma_IntrDisable(&vdma1Ip, XAXIVDMA_S2MM_IRQ_ERR_ALL_MASK,
			XAXIVDMA_WRITE);

	// Transferts
	printf("Sending data\n");
	status = XAxiVdma_DmaConfig(&vdmaIp, XAXIVDMA_READ, &vdmaSetUp);
	if (status != 0)
		printf("ERROR\n");
	status = XAxiVdma_StartReadFrame(&vdmaIp, &vdmaSetUp);
	if (status != 0)
		printf("ERROR\n");

	printf("Reading data\n");
	status = XAxiVdma_DmaConfig(&vdmaIp, XAXIVDMA_WRITE, &vdmaSetUp);
	if (status != 0)
		status = XAxiVdma_StartWriteFrame(&vdmaIp, &vdmaSetUp);

	// ADD here code to display img

	cleanup_platform();
	return 0;
}

```

