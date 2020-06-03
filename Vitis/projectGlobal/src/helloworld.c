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
