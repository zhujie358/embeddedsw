/******************************************************************************
*
* Copyright (C) 2017 Xilinx, Inc. All rights reserved.
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
* XILINX BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/
/*****************************************************************************/
/**
*
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver  Who Date     Changes
* ---- --- -------- --------------------------------------------------
* 1.00 KI  07/13/17 Initial release.
*
* </pre>
*
******************************************************************************/

//#include "LMK04906.h"
#include <stdlib.h>
#include "xspi.h"
#include "xparameters.h"

#define  LMK04906_DEVICE_ID  XPAR_SPI_0_DEVICE_ID
#define  LMK04906_DEVICE_BASEADDR  XPAR_SPI_0_BASEADDR

typedef struct {
	u32 SPI_BaseAddr;
} LMK04906_SPI_Info;

volatile int TransferInProgress;

void LMK04906_init(XSpi *SPI_LMK04906);
int  IF_LoopBack_Test();
int  LMK04906_RegWrite(XSpi *SPI_LMK04906 , u32 RegData ,u32 RegAddr);

void Soft_Reset();
void Set_Option(u32 Option);
u32  Get_Option();
u32  Get_Status();
u32  Rx_Data_Read();
void Tx_Data_Write(u32 WriteData);


void LMK04906_init(XSpi *SPI_LMK04906){
    int Status    = 0;
    u32 SPI_Option = 0;
    XSpi_Config *SPI_LMK04906_Conf;

	// SPI Setting Load
	SPI_LMK04906_Conf = (XSpi_LookupConfig(LMK04906_DEVICE_ID));

	if (SPI_LMK04906_Conf == NULL) {
		xil_printf("Error : SPI Device ConfSetting Not Found !\r\n");
        exit(-1);
	}

    // SPI Device init
    Status = XSpi_CfgInitialize(
		SPI_LMK04906, SPI_LMK04906_Conf,SPI_LMK04906_Conf->BaseAddress);
    if(Status != XST_SUCCESS){
	xil_printf("Error : SPI Device Not Found ( Status Num : %d )!\r\n",
																	Status);
	exit(-1);
    }

    //Reset
    XSpi_Reset(SPI_LMK04906);
    // Get Option
    SPI_Option = Get_Option();
    // Set Option
    SPI_Option = ((SPI_Option|0x00000066)&0xFFFFFEFE);
    Set_Option(SPI_Option);
    XSpi_WriteReg(LMK04906_DEVICE_BASEADDR,0x70, 0xFFFF);
}

int LMK04906_RegWrite(XSpi *SPI_LMK04906 , u32 RegData ,u32 RegAddr){

	const u32 WriteData = (RegData << 5)|RegAddr ;
	volatile u32 wait_cnt = 0;
    XSpi_WriteReg(LMK04906_DEVICE_BASEADDR,0x70, 0x0000);


    Tx_Data_Write(WriteData);

// set long enough timer value to wait for LMK to be ready
    while(wait_cnt++ != 2000);  ////////// for Zynq
    XSpi_WriteReg(LMK04906_DEVICE_BASEADDR,0x70, 0xFFFF);

	return XST_SUCCESS;
}

int IF_LoopBack_Test(XSpi *SPI_LMK04906) {

    u32 SPI_Option;
    int Count;

    SPI_Option = Get_Option();
    SPI_Option  |= (u32)0x00000001;
    xil_printf("SPI Set option %x\r\n",SPI_Option);
    Set_Option(SPI_Option);

    xil_printf("Internal Loop Back Mode Set \r\n");
	for (Count = 0; Count < 15; Count++) {
		// TX DATA
		Tx_Data_Write(Count);
	}
	for (Count = 0; Count < 15; Count++) {
		// RX DATA
	    if(Rx_Data_Read() != Count){
	        SPI_Option  &= (u32)0xFFFFFFFE;
	        Set_Option( SPI_Option);
		xil_printf("Internal LoopBack FAILD \r\n");
		return 1;
	    }
	}
    SPI_Option  &= (u32)0xFFFFFFFE;
    Set_Option( SPI_Option);
	xil_printf("Internal LoopBack Success \r\n");
    return XST_SUCCESS;
}

void Soft_Reset(){
	//Reset
	XSpi_WriteReg(LMK04906_DEVICE_BASEADDR,0x40, 0xa);
#if defined (__MICROBLAZE__)
	MB_Sleep(100);
#else
	// wait long enough for the LMK operations
	usleep(100000);
#endif
//	XSpi_WriteReg(LMK04906_DEVICE_BASEADDR,0x40, 0x0);
}
void Set_Option(u32 Option){
    XSpi_WriteReg(LMK04906_DEVICE_BASEADDR,0x60, Option);
}
u32 Get_Option(){
    return XSpi_ReadReg(LMK04906_DEVICE_BASEADDR,0x60);
}

u32 Get_Status(){
	return XSpi_ReadReg(LMK04906_DEVICE_BASEADDR,0x64);
}

u32 Rx_Data_Read(){
	volatile u32 retval=Get_Status();

    while(retval&0x1){
	retval=Get_Status();
    }
	return XSpi_ReadReg(LMK04906_DEVICE_BASEADDR,0x6C);

}
void Tx_Data_Write(u32 WriteData){
	while(Get_Status()&0x8);
    XSpi_WriteReg(LMK04906_DEVICE_BASEADDR,0x68, WriteData);

}