/*****************************************************************************
 *
 * Filename:
 * ---------
 *   sensor.c
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   Source code of Sensor driver
 *
 *
 * Author:
 * -------
 *   Anyuan Huang (MTK70663)
 *
 *============================================================================
 *             HISTORY
 * Below this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 * $Revision:$
 * $Modtime:$
 * $Log:$
 *
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <asm/io.h>
#include <asm/system.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"
#include "kd_camera_feature.h"

#include "hi253yuv_Sensor.h"
#include "hi253yuv_Camera_Sensor_para.h"
#include "hi253yuv_CameraCustomized.h"

#define HI253YUV_DEBUG
#ifdef HI253YUV_DEBUG
#define SENSORDB printk
#else
#define SENSORDB(x,...)
#endif

struct
{
  kal_bool    NightMode;
  kal_uint8   ZoomFactor; /* Zoom Index */
  kal_uint16  Banding;
  kal_uint32  PvShutter;
  kal_uint32  PvDummyPixels;
  kal_uint32  PvDummyLines;
  kal_uint32  CapDummyPixels;
  kal_uint32  CapDummyLines;
  kal_uint32  PvOpClk;
  kal_uint32  CapOpClk;

  /* Video frame rate 300 means 30.0fps. Unit Multiple 10. */
  kal_uint32  MaxFrameRate;
  kal_uint32  MiniFrameRate;
  /* Sensor Register backup. */
  kal_uint8   VDOCTL2; /* P0.0x11. */
  kal_uint8   ISPCTL3; /* P10.0x12. */
  kal_uint8   AECTL1;  /* P20.0x10. */
  kal_uint8   AWBCTL1; /* P22.0x10. */
} HI253Status;

static DEFINE_SPINLOCK(hi253_drv_lock);


extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

#define Sleep(ms) mdelay(ms)

kal_uint16 HI253WriteCmosSensor(kal_uint32 Addr, kal_uint32 Para)
{
  char pSendCmd[2] = {(char)(Addr & 0xFF) ,(char)(Para & 0xFF)};

  iWriteRegI2C(pSendCmd , 2,HI253_WRITE_ID);
}

kal_uint16 HI253ReadCmosSensor(kal_uint32 Addr)
{
  char pGetByte=0;
  char pSendCmd = (char)(Addr & 0xFF);

  iReadRegI2C(&pSendCmd , 1, &pGetByte,1,HI253_WRITE_ID);

  return pGetByte;
}

void HI253SetPage(kal_uint8 Page)
{
  HI253WriteCmosSensor(0x03, Page);
}

void HI253InitSetting(void)
{
  /*[SENSOR_INITIALIZATION]
    DISP_DATE = "2010-11-24 "
    DISP_WIDTH = 800
    DISP_HEIGHT = 600
    DISP_FORMAT = YUV422
    DISP_DATAORDER = YUYV
    MCLK = 26.00
    PLL = 2.00
    BEGIN */

   HI253WriteCmosSensor(0x01, 0xf9); // Sleep ON
  HI253WriteCmosSensor(0x08, 0x0f); // Hi-Z ON
  HI253WriteCmosSensor(0x01, 0xf8); // Sleep OFF

  HI253SetPage(0x00); // Dummy 750us START
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00); // Dummy 750us END

  HI253WriteCmosSensor(0x0e, 0x00); // PLL 

  HI253SetPage(0x00); // Dummy 750us START
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00); // Dummy 750us END

  HI253WriteCmosSensor(0x0e, 0x00); // PLL OFF
  HI253WriteCmosSensor(0x01, 0xf1); // Sleep ON
  HI253WriteCmosSensor(0x08, 0x00); // Hi-Z OFF
  HI253WriteCmosSensor(0x01, 0xf3);
  HI253WriteCmosSensor(0x01, 0xf1);

// PAGE 20
HI253SetPage(0x20); //page 20
 HI253WriteCmosSensor(0x10, 0x1c); //ae off

// PAGE 22
HI253SetPage(0x22); //page 22
 HI253WriteCmosSensor(0x10, 0x69); //awb off

HI253SetPage(0x00);
  HI253WriteCmosSensor(0x10, 0x10); // Sub1/2_Preview2 Mode_H binning
  HI253WriteCmosSensor(0x11, 0x90); // Windowing ON, 1Frame Skip
  HI253WriteCmosSensor(0x12, 0x04); // 00:Rinsing edge 04:fall edge
  HI253WriteCmosSensor(0x0b, 0xaa); // ESD Check Register
  HI253WriteCmosSensor(0x0c, 0xaa); // ESD Check Register
  HI253WriteCmosSensor(0x0d, 0xaa); // ESD Check Register
  HI253WriteCmosSensor(0x20, 0x00); // WINROWH
  HI253WriteCmosSensor(0x21, 0x04); // WINROWL
  HI253WriteCmosSensor(0x22, 0x00); // WINCOLH
  HI253WriteCmosSensor(0x23, 0x07); // WINCOLL
  HI253WriteCmosSensor(0x24, 0x04); // WINHGTH
  HI253WriteCmosSensor(0x25, 0xb0); // WINHGTL
  HI253WriteCmosSensor(0x26, 0x06); // WINWIDH
  HI253WriteCmosSensor(0x27, 0x40); // WINWIDL

  HI253WriteCmosSensor(0x40, 0x01); // HBLANKH 424
  HI253WriteCmosSensor(0x41, 0x68); // HBLANKL
  HI253WriteCmosSensor(0x42, 0x00); // VSYNCH 62
  HI253WriteCmosSensor(0x43, 0x14); // VSYNCL

  HI253WriteCmosSensor(0x45, 0x04); // VSCTL1
  HI253WriteCmosSensor(0x46, 0x18); // VSCTL2
  HI253WriteCmosSensor(0x47, 0xd8); // VSCTL3

  HI253WriteCmosSensor(0xe1, 0x0f);

  //BLC 
  HI253WriteCmosSensor(0x80, 0x2e); // BLCCTL
  HI253WriteCmosSensor(0x81, 0x7e);
  HI253WriteCmosSensor(0x82, 0x90);
  HI253WriteCmosSensor(0x83, 0x00);
  HI253WriteCmosSensor(0x84, 0x0c);
  HI253WriteCmosSensor(0x85, 0x00);
  HI253WriteCmosSensor(0x90, 0x0C); // BLCTIMETH ON
  HI253WriteCmosSensor(0x91, 0x0C); // BLCTIMETH OFF
  HI253WriteCmosSensor(0x92, 0x78); // BLCAGTH ON
  HI253WriteCmosSensor(0x93, 0x70); // BLCAGTH OFF
  HI253WriteCmosSensor(0x94, 0x75); // BLCDGTH ON
  HI253WriteCmosSensor(0x95, 0x70); // BLCDGTH OFF
  HI253WriteCmosSensor(0x96, 0xdc);
  HI253WriteCmosSensor(0x97, 0xfe);
 HI253WriteCmosSensor(0x98, 0x38);

  //OutDoor BLC
  HI253WriteCmosSensor(0x99,0x42);// B
  HI253WriteCmosSensor(0x9a,0x42);// Gb
  HI253WriteCmosSensor(0x9b,0x42);// R
  HI253WriteCmosSensor(0x9c,0x42);// Gr

  //Dark BLC 
  HI253WriteCmosSensor(0xa0, 0x00); // BLCOFS DB
  HI253WriteCmosSensor(0xa2, 0x00); // BLCOFS DGb
  HI253WriteCmosSensor(0xa4, 0x00); // BLCOFS DR
  HI253WriteCmosSensor(0xa6, 0x00); // BLCOFS DGr

  //Normal BLC
  HI253WriteCmosSensor(0xa8, 0x43);
  HI253WriteCmosSensor(0xaa, 0x43);
  HI253WriteCmosSensor(0xac, 0x43);
  HI253WriteCmosSensor(0xae, 0x43);

/////// PAGE 2 START ///////
HI253SetPage(0x02); //Page 02
 HI253WriteCmosSensor(0x10, 0x00); //Mode_test
 HI253WriteCmosSensor(0x11, 0x00); //Mode_dead_test
 HI253WriteCmosSensor(0x12, 0x03); //pwr_ctl_ctl1
 HI253WriteCmosSensor(0x13, 0x03); //Mode_ana_test
 HI253WriteCmosSensor(0x14, 0x00); //mode_memory
 HI253WriteCmosSensor(0x16, 0x00); //dcdc_ctl1
 HI253WriteCmosSensor(0x17, 0x8c); //dcdc_ctl2
 HI253WriteCmosSensor(0x18, 0x4C); //analog_func1
 HI253WriteCmosSensor(0x19, 0x00); //analog_func2
 HI253WriteCmosSensor(0x1a, 0x39); //analog_func3
 HI253WriteCmosSensor(0x1b, 0x00); //analog_func4
 HI253WriteCmosSensor(0x1c, 0x09); //dcdc_ctl3
 HI253WriteCmosSensor(0x1d, 0x40); //dcdc_ctl4
 HI253WriteCmosSensor(0x1e, 0x30); //analog_func7
 HI253WriteCmosSensor(0x1f, 0x10); //analog_func8
 HI253WriteCmosSensor(0x20, 0x77); //pixel bias
 HI253WriteCmosSensor(0x21, 0xde); //adc,asp bias
 HI253WriteCmosSensor(0x22, 0xa7); //main,bus bias
 HI253WriteCmosSensor(0x23, 0x30); //clamp
 HI253WriteCmosSensor(0x24, 0x4a);
 HI253WriteCmosSensor(0x25, 0x10);
 HI253WriteCmosSensor(0x27, 0x3c);
 HI253WriteCmosSensor(0x28, 0x00);
 HI253WriteCmosSensor(0x29, 0x0c);
 HI253WriteCmosSensor(0x2a, 0x80);
 HI253WriteCmosSensor(0x2b, 0x80);
 HI253WriteCmosSensor(0x2c, 0x02);
 HI253WriteCmosSensor(0x2d, 0xa0);
 HI253WriteCmosSensor(0x2e, 0x11);
 HI253WriteCmosSensor(0x2f, 0xa1);
 HI253WriteCmosSensor(0x30, 0x05); //swap_ctl
 HI253WriteCmosSensor(0x31, 0x99);
 HI253WriteCmosSensor(0x32, 0x00);
 HI253WriteCmosSensor(0x33, 0x00);
 HI253WriteCmosSensor(0x34, 0x22);
 HI253WriteCmosSensor(0x38, 0x88);
 HI253WriteCmosSensor(0x39, 0x88);
 HI253WriteCmosSensor(0x50, 0x20);
 HI253WriteCmosSensor(0x51, 0x00);
 HI253WriteCmosSensor(0x52, 0x01);
 HI253WriteCmosSensor(0x53, 0xc1);
 HI253WriteCmosSensor(0x54, 0x10);
 HI253WriteCmosSensor(0x55, 0x1c);
 HI253WriteCmosSensor(0x56, 0x11);
 HI253WriteCmosSensor(0x58, 0x10);
 HI253WriteCmosSensor(0x59, 0x0e);
 HI253WriteCmosSensor(0x5d, 0xa2);
 HI253WriteCmosSensor(0x5e, 0x5a);
 HI253WriteCmosSensor(0x60, 0x87);
 HI253WriteCmosSensor(0x61, 0x99);
 HI253WriteCmosSensor(0x62, 0x88);
 HI253WriteCmosSensor(0x63, 0x97);
 HI253WriteCmosSensor(0x64, 0x88);
 HI253WriteCmosSensor(0x65, 0x97);
 HI253WriteCmosSensor(0x67, 0x0c);
 HI253WriteCmosSensor(0x68, 0x0c);
 HI253WriteCmosSensor(0x69, 0x0c);
 HI253WriteCmosSensor(0x6a, 0xb4);
 HI253WriteCmosSensor(0x6b, 0xc4);
 HI253WriteCmosSensor(0x6c, 0xb5);
 HI253WriteCmosSensor(0x6d, 0xc2);
 HI253WriteCmosSensor(0x6e, 0xb5);
 HI253WriteCmosSensor(0x6f, 0xc0);
 HI253WriteCmosSensor(0x70, 0xb6);
 HI253WriteCmosSensor(0x71, 0xb8);
 HI253WriteCmosSensor(0x72, 0x89);
 HI253WriteCmosSensor(0x73, 0x96);
 HI253WriteCmosSensor(0x74, 0x89);
 HI253WriteCmosSensor(0x75, 0x96);
 HI253WriteCmosSensor(0x76, 0x89);
 HI253WriteCmosSensor(0x77, 0x96);
 HI253WriteCmosSensor(0x7c, 0x85);
 HI253WriteCmosSensor(0x7d, 0xaf);
 HI253WriteCmosSensor(0x80, 0x01);
 HI253WriteCmosSensor(0x81, 0x7f);
 HI253WriteCmosSensor(0x82, 0x13); //rx_on1_read
 HI253WriteCmosSensor(0x83, 0x24);
 HI253WriteCmosSensor(0x84, 0x7D);
 HI253WriteCmosSensor(0x85, 0x81);
 HI253WriteCmosSensor(0x86, 0x7D);
 HI253WriteCmosSensor(0x87, 0x81);
 HI253WriteCmosSensor(0x88, 0xab);
 HI253WriteCmosSensor(0x89, 0xbc);
 HI253WriteCmosSensor(0x8a, 0xac);
 HI253WriteCmosSensor(0x8b, 0xba);
 HI253WriteCmosSensor(0x8c, 0xad);
 HI253WriteCmosSensor(0x8d, 0xb8);
 HI253WriteCmosSensor(0x8e, 0xae);
 HI253WriteCmosSensor(0x8f, 0xb2);
 HI253WriteCmosSensor(0x90, 0xb3);
 HI253WriteCmosSensor(0x91, 0xb7);
 HI253WriteCmosSensor(0x92, 0x48);
 HI253WriteCmosSensor(0x93, 0x54);
 HI253WriteCmosSensor(0x94, 0x7D);
 HI253WriteCmosSensor(0x95, 0x81);
 HI253WriteCmosSensor(0x96, 0x7D);
 HI253WriteCmosSensor(0x97, 0x81);
 HI253WriteCmosSensor(0xa0, 0x02);
 HI253WriteCmosSensor(0xa1, 0x7B);
 HI253WriteCmosSensor(0xa2, 0x02);
 HI253WriteCmosSensor(0xa3, 0x7B);
 HI253WriteCmosSensor(0xa4, 0x7B);
 HI253WriteCmosSensor(0xa5, 0x02);
 HI253WriteCmosSensor(0xa6, 0x7B);
 HI253WriteCmosSensor(0xa7, 0x02);
 HI253WriteCmosSensor(0xa8, 0x85);
 HI253WriteCmosSensor(0xa9, 0x8C);
 HI253WriteCmosSensor(0xaa, 0x85);
 HI253WriteCmosSensor(0xab, 0x8C);
 HI253WriteCmosSensor(0xac, 0x10); //Rx_pwr_off1_read
 HI253WriteCmosSensor(0xad, 0x16); //Rx_pwr_on1_read
 HI253WriteCmosSensor(0xae, 0x10); //Rx_pwr_off2_read
 HI253WriteCmosSensor(0xaf, 0x16); //Rx_pwr_on1_read
 HI253WriteCmosSensor(0xb0, 0x99);
 HI253WriteCmosSensor(0xb1, 0xA3);
 HI253WriteCmosSensor(0xb2, 0xA4);
 HI253WriteCmosSensor(0xb3, 0xAE);
 HI253WriteCmosSensor(0xb4, 0x9B);
 HI253WriteCmosSensor(0xb5, 0xA2);
 HI253WriteCmosSensor(0xb6, 0xA6);
 HI253WriteCmosSensor(0xb7, 0xAC);
 HI253WriteCmosSensor(0xb8, 0x9B);
 HI253WriteCmosSensor(0xb9, 0x9F);
 HI253WriteCmosSensor(0xba, 0xA6);
 HI253WriteCmosSensor(0xbb, 0xAA);
 HI253WriteCmosSensor(0xbc, 0x9B);
 HI253WriteCmosSensor(0xbd, 0x9F);
 HI253WriteCmosSensor(0xbe, 0xA6);
 HI253WriteCmosSensor(0xbf, 0xaa);
 HI253WriteCmosSensor(0xc4, 0x2c);
 HI253WriteCmosSensor(0xc5, 0x43);
 HI253WriteCmosSensor(0xc6, 0x63);
 HI253WriteCmosSensor(0xc7, 0x79);
 HI253WriteCmosSensor(0xc8, 0x2d);
 HI253WriteCmosSensor(0xc9, 0x42);
 HI253WriteCmosSensor(0xca, 0x2d);
 HI253WriteCmosSensor(0xcb, 0x42);
 HI253WriteCmosSensor(0xcc, 0x64);
 HI253WriteCmosSensor(0xcd, 0x78);
 HI253WriteCmosSensor(0xce, 0x64);
 HI253WriteCmosSensor(0xcf, 0x78);
 HI253WriteCmosSensor(0xd0, 0x0a);
 HI253WriteCmosSensor(0xd1, 0x09);
 HI253WriteCmosSensor(0xd2, 0x20);
 HI253WriteCmosSensor(0xd3, 0x00);

 HI253WriteCmosSensor(0xd4, 0x0c);
 HI253WriteCmosSensor(0xd5, 0x0c);
 HI253WriteCmosSensor(0xd6, 0x78);
 HI253WriteCmosSensor(0xd7, 0x70);

 HI253WriteCmosSensor(0xe0, 0xc4);
 HI253WriteCmosSensor(0xe1, 0xc4);
 HI253WriteCmosSensor(0xe2, 0xc4);
 HI253WriteCmosSensor(0xe3, 0xc4);
 HI253WriteCmosSensor(0xe4, 0x00);
 HI253WriteCmosSensor(0xe8, 0x80);
 HI253WriteCmosSensor(0xe9, 0x40);
 HI253WriteCmosSensor(0xea, 0x7f);
 HI253WriteCmosSensor(0xf0, 0x01); //sram1_cfg
 HI253WriteCmosSensor(0xf1, 0x01); //sram2_cfg
 HI253WriteCmosSensor(0xf2, 0x01); //sram3_cfg
 HI253WriteCmosSensor(0xf3, 0x01); //sram4_cfg
 HI253WriteCmosSensor(0xf4, 0x01); //sram5_cfg
///////////

  HI253SetPage(0x03);
  HI253WriteCmosSensor(0x10, 0x10);

  HI253SetPage(0x10); 
  HI253WriteCmosSensor(0x10, 0x03); // YUYV
  HI253WriteCmosSensor(0x12, 0x30); // ISPCTL3
 HI253WriteCmosSensor(0x13, 0x0a); // contrast on
 HI253WriteCmosSensor(0x20, 0x00);
 HI253WriteCmosSensor(0x40, 0x90);
 HI253WriteCmosSensor(0x41, 0x00);
 HI253WriteCmosSensor(0x48, 0x88);// 84
 HI253WriteCmosSensor(0x50, 0x90);
 HI253WriteCmosSensor(0x30, 0x00);
 HI253WriteCmosSensor(0x31, 0x00);
 HI253WriteCmosSensor(0x32, 0x00);
 HI253WriteCmosSensor(0x33, 0x00);
 HI253WriteCmosSensor(0x34, 0x30);
 HI253WriteCmosSensor(0x35, 0x00);
 HI253WriteCmosSensor(0x36, 0x00);
 HI253WriteCmosSensor(0x38, 0x00);
 HI253WriteCmosSensor(0x3e, 0x58);
 HI253WriteCmosSensor(0x3f, 0x00);

//Saturation
 HI253WriteCmosSensor(0x60, 0x67);
 HI253WriteCmosSensor(0x61, 0x80);// 74
 HI253WriteCmosSensor(0x62, 0x80);// 76
 HI253WriteCmosSensor(0x63, 0x50);
 HI253WriteCmosSensor(0x64, 0x41);
 HI253WriteCmosSensor(0x66, 0x33);
 HI253WriteCmosSensor(0x67, 0x00);

 HI253WriteCmosSensor(0x6a, 0x90); 
 HI253WriteCmosSensor(0x6b, 0x80); 
 HI253WriteCmosSensor(0x6c, 0x80); 
 HI253WriteCmosSensor(0x6d, 0xa0); 

 HI253WriteCmosSensor(0x76, 0x01); 
 HI253WriteCmosSensor(0x74, 0x66);
 HI253WriteCmosSensor(0x79, 0x06);

  HI253SetPage(0x11);
  HI253WriteCmosSensor(0x10, 0x3f); // DLPFCTL1
  HI253WriteCmosSensor(0x11, 0x40);
  HI253WriteCmosSensor(0x12, 0xba);
  HI253WriteCmosSensor(0x13, 0xcb);
  HI253WriteCmosSensor(0x26, 0x20); // LPFAGTHL
  HI253WriteCmosSensor(0x27, 0x22); // LPFAGTHH
  HI253WriteCmosSensor(0x28, 0x0f); // LPFOUTTHL
  HI253WriteCmosSensor(0x29, 0x10); // LPFOUTTHH
  HI253WriteCmosSensor(0x2b, 0x30); // LPFYMEANTHL
  HI253WriteCmosSensor(0x2c, 0x32); // LPFYMEANTHH

  //Out2 D-LPF th
  HI253WriteCmosSensor(0x30, 0x70); // OUT2YBOUNDH
  HI253WriteCmosSensor(0x31, 0x10); // OUT2YBOUNDL
  HI253WriteCmosSensor(0x32, 0x65); // OUT2RATIO
  HI253WriteCmosSensor(0x33, 0x09); // OUT2THH
  HI253WriteCmosSensor(0x34, 0x06); // OUT2THM
  HI253WriteCmosSensor(0x35, 0x04); // OUT2THL

  //Out1 D-LPF th
  HI253WriteCmosSensor(0x36, 0x70); // OUT1YBOUNDH
  HI253WriteCmosSensor(0x37, 0x18); // OUT1YBOUNDL
  HI253WriteCmosSensor(0x38, 0x65); // OUT1RATIO
  HI253WriteCmosSensor(0x39, 0x09); // OUT1THH
  HI253WriteCmosSensor(0x3a, 0x06); // OUT1THM
  HI253WriteCmosSensor(0x3b, 0x04); // OUT1THL

  //Indoor D-LPF th
  HI253WriteCmosSensor(0x3c, 0x80); // INYBOUNDH
  HI253WriteCmosSensor(0x3d, 0x18); // INYBOUNDL
  HI253WriteCmosSensor(0x3e, 0x80); // INRATIO
  HI253WriteCmosSensor(0x3f, 0x0c); // INTHH
  HI253WriteCmosSensor(0x40, 0x09); // INTHM
  HI253WriteCmosSensor(0x41, 0x06); // INTHL

  HI253WriteCmosSensor(0x42, 0x80); // DARK1YBOUNDH
  HI253WriteCmosSensor(0x43, 0x18); // DARK1YBOUNDL
  HI253WriteCmosSensor(0x44, 0x80); // DARK1RATIO
  HI253WriteCmosSensor(0x45, 0x12); // DARK1THH
  HI253WriteCmosSensor(0x46, 0x10); // DARK1THM
  HI253WriteCmosSensor(0x47, 0x10); // DARK1THL
  HI253WriteCmosSensor(0x48, 0x90); // DARK2YBOUNDH
  HI253WriteCmosSensor(0x49, 0x40); // DARK2YBOUNDL
  HI253WriteCmosSensor(0x4a, 0x80); // DARK2RATIO
  HI253WriteCmosSensor(0x4b, 0x13); // DARK2THH
  HI253WriteCmosSensor(0x4c, 0x10); // DARK2THM
  HI253WriteCmosSensor(0x4d, 0x11); // DARK2THL
  HI253WriteCmosSensor(0x4e, 0x80); // DARK3YBOUNDH
  HI253WriteCmosSensor(0x4f, 0x30); // DARK3YBOUNDL
  HI253WriteCmosSensor(0x50, 0x80); // DARK3RATIO
  HI253WriteCmosSensor(0x51, 0x13); // DARK3THH
  HI253WriteCmosSensor(0x52, 0x10); // DARK3THM
  HI253WriteCmosSensor(0x53, 0x13); // DARK3THL
  HI253WriteCmosSensor(0x54, 0x11);
  HI253WriteCmosSensor(0x55, 0x17);
  HI253WriteCmosSensor(0x56, 0x20);
  HI253WriteCmosSensor(0x57, 0x20);
  HI253WriteCmosSensor(0x58, 0x20);
  HI253WriteCmosSensor(0x59, 0x30);
  HI253WriteCmosSensor(0x5a, 0x18);
  HI253WriteCmosSensor(0x5b, 0x00);
  HI253WriteCmosSensor(0x5c, 0x00);
  HI253WriteCmosSensor(0x60, 0x3f);
  HI253WriteCmosSensor(0x62, 0x50);
  HI253WriteCmosSensor(0x70, 0x06);

HI253SetPage(0x12);
 HI253WriteCmosSensor(0x20, 0x0f);
 HI253WriteCmosSensor(0x21, 0x0f);

  HI253WriteCmosSensor(0x25, 0x30);
  HI253WriteCmosSensor(0x28, 0x00); // Out1 BP t
  HI253WriteCmosSensor(0x29, 0x00); // Out2 BP t
  HI253WriteCmosSensor(0x2a, 0x00);
  HI253WriteCmosSensor(0x30, 0x50);
  HI253WriteCmosSensor(0x31, 0x18); // YCUNI1TH
  HI253WriteCmosSensor(0x32, 0x32); // YCUNI2TH
 HI253WriteCmosSensor(0x33, 0x40);
 HI253WriteCmosSensor(0x34, 0x50);
 HI253WriteCmosSensor(0x35, 0x70);
 HI253WriteCmosSensor(0x36, 0xa0);

  //Out2 th
  HI253WriteCmosSensor(0x40, 0xa0); // YCOUT2THH
  HI253WriteCmosSensor(0x41, 0x40); // YCOUT2THL
  HI253WriteCmosSensor(0x42, 0xa0); // YCOUT2STDH
  HI253WriteCmosSensor(0x43, 0x90); // YCOUT2STDM
  HI253WriteCmosSensor(0x44, 0x90); // YCOUT2STDL
  HI253WriteCmosSensor(0x45, 0x80); // YCOUT2RAT

  //Out1 th 
  HI253WriteCmosSensor(0x46, 0xb0); // YCOUT1THH
  HI253WriteCmosSensor(0x47, 0x55); // YCOUT1THL
  HI253WriteCmosSensor(0x48, 0xa0); // YCOUT1STDH
  HI253WriteCmosSensor(0x49, 0x90); // YCOUT1STDM
  HI253WriteCmosSensor(0x4a, 0x90); // YCOUT1STDL
  HI253WriteCmosSensor(0x4b, 0x80); // YCOUT1RAT

  //In door th
  HI253WriteCmosSensor(0x4c, 0xb0); // YCINTHH
  HI253WriteCmosSensor(0x4d, 0x40); // YCINTHL
  HI253WriteCmosSensor(0x4e, 0x90); // YCINSTDH
  HI253WriteCmosSensor(0x4f, 0x90); // YCINSTDM
  HI253WriteCmosSensor(0x50, 0xe6); // YCINSTDL
  HI253WriteCmosSensor(0x51, 0x80); // YCINRAT

  //Dark1 th
  HI253WriteCmosSensor(0x52, 0xb0); // YCDARK1THH
  HI253WriteCmosSensor(0x53, 0x60); // YCDARK1THL
  HI253WriteCmosSensor(0x54, 0xc0); // YCDARK1STDH
  HI253WriteCmosSensor(0x55, 0xc0); // YCDARK1STDM
  HI253WriteCmosSensor(0x56, 0xc0); // YCDARK1STDL
  HI253WriteCmosSensor(0x57, 0x80); // YCDARK1RAT

  //Dark2 th
  HI253WriteCmosSensor(0x58, 0x90); // YCDARK2THH
  HI253WriteCmosSensor(0x59, 0x40); // YCDARK2THL
  HI253WriteCmosSensor(0x5a, 0xd0); // YCDARK2STDH
  HI253WriteCmosSensor(0x5b, 0xd0); // YCDARK2STDM
  HI253WriteCmosSensor(0x5c, 0xe0); // YCDARK2STDL
  HI253WriteCmosSensor(0x5d, 0x80); // YCDARK2RAT

//Dark3 th
 HI253WriteCmosSensor(0x5e, 0x70);//88
 HI253WriteCmosSensor(0x5f, 0x40);
 HI253WriteCmosSensor(0x60, 0xe0);
 HI253WriteCmosSensor(0x61, 0xe0);
 HI253WriteCmosSensor(0x62, 0xe0);
 HI253WriteCmosSensor(0x63, 0xb4);//80

  HI253WriteCmosSensor(0x70, 0x15);
  HI253WriteCmosSensor(0x71, 0x01); //Don't Touch register

  HI253WriteCmosSensor(0x72, 0x18);
  HI253WriteCmosSensor(0x73, 0x01); //Don't Touch register

 HI253WriteCmosSensor(0x74, 0x25);
 HI253WriteCmosSensor(0x75, 0x15);

 HI253WriteCmosSensor(0x80, 0x20);
 HI253WriteCmosSensor(0x81, 0x40);
 HI253WriteCmosSensor(0x82, 0x65);
 HI253WriteCmosSensor(0x85, 0x1a);
 HI253WriteCmosSensor(0x88, 0x00);
 HI253WriteCmosSensor(0x89, 0x00);
 HI253WriteCmosSensor(0x90, 0x5d); //For capture
 HI253WriteCmosSensor(0x91, 0x88);
 HI253WriteCmosSensor(0x98, 0x7d);
 HI253WriteCmosSensor(0x99, 0x28);
 HI253WriteCmosSensor(0x9A, 0x14);
 HI253WriteCmosSensor(0x9B, 0xc8);
 HI253WriteCmosSensor(0x9C, 0x02);
 HI253WriteCmosSensor(0x9D, 0x1e);
 HI253WriteCmosSensor(0x9E, 0x28);
 HI253WriteCmosSensor(0x9F, 0x07);
 HI253WriteCmosSensor(0xA0, 0x32);
 HI253WriteCmosSensor(0xA4, 0x04);
 HI253WriteCmosSensor(0xA5, 0x0e);
 HI253WriteCmosSensor(0xA6, 0x0c);
 HI253WriteCmosSensor(0xA7, 0x04);
 HI253WriteCmosSensor(0xA8, 0x3c);

 HI253WriteCmosSensor(0xAA, 0x14);
 HI253WriteCmosSensor(0xAB, 0x11);
 HI253WriteCmosSensor(0xAC, 0x0f);
 HI253WriteCmosSensor(0xAD, 0x16);
 HI253WriteCmosSensor(0xAE, 0x15);
 HI253WriteCmosSensor(0xAF, 0x14);
 HI253WriteCmosSensor(0xB1, 0xaa);
 HI253WriteCmosSensor(0xB2, 0x96);
 HI253WriteCmosSensor(0xB3, 0x28);
//Dont Touch register
 HI253WriteCmosSensor(0xB8, 0x78);
 HI253WriteCmosSensor(0xB9, 0xa0);
 HI253WriteCmosSensor(0xBA, 0xb4);
 HI253WriteCmosSensor(0xBB, 0x14);
 HI253WriteCmosSensor(0xBC, 0x14);
 HI253WriteCmosSensor(0xBD, 0x00);//14 \u53bb\u767d\u70b9
 HI253WriteCmosSensor(0xBE, 0x64);
 HI253WriteCmosSensor(0xBF, 0x64);
 HI253WriteCmosSensor(0xC0, 0x64);
 HI253WriteCmosSensor(0xC1, 0x64);
 HI253WriteCmosSensor(0xC2, 0x04);
 HI253WriteCmosSensor(0xC3, 0x03);
 HI253WriteCmosSensor(0xC4, 0x0c);
 HI253WriteCmosSensor(0xC5, 0x30);
 HI253WriteCmosSensor(0xC6, 0x2a);
  //Dont Touch register
  HI253WriteCmosSensor(0xD0, 0x0c);
  HI253WriteCmosSensor(0xD1, 0x80);
  HI253WriteCmosSensor(0xD2, 0x67);
  HI253WriteCmosSensor(0xD3, 0x00);
  HI253WriteCmosSensor(0xD4, 0x00);
  HI253WriteCmosSensor(0xD5, 0x02);
  HI253WriteCmosSensor(0xD6, 0xff);
  HI253WriteCmosSensor(0xD7, 0x18);
//End
 HI253WriteCmosSensor(0x3b, 0x06);
 HI253WriteCmosSensor(0x3c, 0x06);

 HI253WriteCmosSensor(0xc5, 0x00);//55->48
 HI253WriteCmosSensor(0xc6, 0x00);//48->40

/////// PAGE 13 START ///////
HI253SetPage(0x13);
//Edge
 HI253WriteCmosSensor(0x10, 0xcb);
 HI253WriteCmosSensor(0x11, 0x7b);
 HI253WriteCmosSensor(0x12, 0x07);
 HI253WriteCmosSensor(0x14, 0x00);

 HI253WriteCmosSensor(0x20, 0x15);
 HI253WriteCmosSensor(0x21, 0x13);
 HI253WriteCmosSensor(0x22, 0x33);
 HI253WriteCmosSensor(0x23, 0x05);
 HI253WriteCmosSensor(0x24, 0x12);//09

 HI253WriteCmosSensor(0x26, 0x18);
 HI253WriteCmosSensor(0x27, 0x30);
 HI253WriteCmosSensor(0x29, 0x12);
 HI253WriteCmosSensor(0x2a, 0x50);

//Low clip th
 HI253WriteCmosSensor(0x2b, 0x01); //Out2 02
 HI253WriteCmosSensor(0x2c, 0x01); //Out1 02
 HI253WriteCmosSensor(0x25, 0x06);
 HI253WriteCmosSensor(0x2d, 0x0c);
 HI253WriteCmosSensor(0x2e, 0x12);
 HI253WriteCmosSensor(0x2f, 0x12);

//Out2 Edge
 HI253WriteCmosSensor(0x50, 0x18);
 HI253WriteCmosSensor(0x51, 0x1c);
 HI253WriteCmosSensor(0x52, 0x1b);
 HI253WriteCmosSensor(0x53, 0x15);
 HI253WriteCmosSensor(0x54, 0x18);
 HI253WriteCmosSensor(0x55, 0x15);

//Out1 Edge  
 HI253WriteCmosSensor(0x56, 0x18);
 HI253WriteCmosSensor(0x57, 0x1c);////1C
 HI253WriteCmosSensor(0x58, 0x1b);
 HI253WriteCmosSensor(0x59, 0x15);
 HI253WriteCmosSensor(0x5a, 0x18);//18
 HI253WriteCmosSensor(0x5b, 0x15);

//Indoor Edge
 HI253WriteCmosSensor(0x5c, 0x0b);
 HI253WriteCmosSensor(0x5d, 0x10);//0C
 HI253WriteCmosSensor(0x5e, 0x0a);
 HI253WriteCmosSensor(0x5f, 0x08);
 HI253WriteCmosSensor(0x60, 0x19);//09
 HI253WriteCmosSensor(0x61, 0x08);

//Dark1 Edge
 HI253WriteCmosSensor(0x62, 0x08);
 HI253WriteCmosSensor(0x63, 0x18);//08
 HI253WriteCmosSensor(0x64, 0x08);
 HI253WriteCmosSensor(0x65, 0x06);
 HI253WriteCmosSensor(0x66, 0x16);//16
 HI253WriteCmosSensor(0x67, 0x06);

//Dark2 Edge
 HI253WriteCmosSensor(0x68, 0x07);
 HI253WriteCmosSensor(0x69, 0x07);
 HI253WriteCmosSensor(0x6a, 0x07);
 HI253WriteCmosSensor(0x6b, 0x05);
 HI253WriteCmosSensor(0x6c, 0x05);
 HI253WriteCmosSensor(0x6d, 0x05);

//Dark3 Edge
 HI253WriteCmosSensor(0x6e, 0x07);
 HI253WriteCmosSensor(0x6f, 0x07);
 HI253WriteCmosSensor(0x70, 0x07);
 HI253WriteCmosSensor(0x71, 0x05);
 HI253WriteCmosSensor(0x72, 0x05);
 HI253WriteCmosSensor(0x73, 0x05);

//2DY
 HI253WriteCmosSensor(0x80, 0xfd);
 HI253WriteCmosSensor(0x81, 0x1f);
 HI253WriteCmosSensor(0x82, 0x05);
 HI253WriteCmosSensor(0x83, 0x31);

 HI253WriteCmosSensor(0x90, 0x15);
 HI253WriteCmosSensor(0x91, 0x15);
 HI253WriteCmosSensor(0x92, 0x33);
 HI253WriteCmosSensor(0x93, 0x30);
 HI253WriteCmosSensor(0x94, 0x03);
 HI253WriteCmosSensor(0x95, 0x14);
 HI253WriteCmosSensor(0x97, 0x30);
 HI253WriteCmosSensor(0x99, 0x30);

 HI253WriteCmosSensor(0xa0, 0x01);
 HI253WriteCmosSensor(0xa1, 0x02);
 HI253WriteCmosSensor(0xa2, 0x01);
 HI253WriteCmosSensor(0xa3, 0x02);
 HI253WriteCmosSensor(0xa4, 0x05);
 HI253WriteCmosSensor(0xa5, 0x05);
 HI253WriteCmosSensor(0xa6, 0x07);
 HI253WriteCmosSensor(0xa7, 0x08);
 HI253WriteCmosSensor(0xa8, 0x07);
 HI253WriteCmosSensor(0xa9, 0x08);
 HI253WriteCmosSensor(0xaa, 0x07);
 HI253WriteCmosSensor(0xab, 0x08);

//Out2 
 HI253WriteCmosSensor(0xb0, 0x22);
 HI253WriteCmosSensor(0xb1, 0x2a);
 HI253WriteCmosSensor(0xb2, 0x28);
 HI253WriteCmosSensor(0xb3, 0x22);
 HI253WriteCmosSensor(0xb4, 0x2a);
 HI253WriteCmosSensor(0xb5, 0x28);

//Out1 
 HI253WriteCmosSensor(0xb6, 0x22);
 HI253WriteCmosSensor(0xb7, 0x2a);
 HI253WriteCmosSensor(0xb8, 0x28);
 HI253WriteCmosSensor(0xb9, 0x22);
 HI253WriteCmosSensor(0xba, 0x2a);
 HI253WriteCmosSensor(0xbb, 0x28);

//Indoor 
 HI253WriteCmosSensor(0xbc, 0x25);
 HI253WriteCmosSensor(0xbd, 0x2a);
 HI253WriteCmosSensor(0xbe, 0x27);
 HI253WriteCmosSensor(0xbf, 0x25);
 HI253WriteCmosSensor(0xc0, 0x2a);
 HI253WriteCmosSensor(0xc1, 0x27);

//Dark1
 HI253WriteCmosSensor(0xc2, 0x1e);
 HI253WriteCmosSensor(0xc3, 0x24);
 HI253WriteCmosSensor(0xc4, 0x20);
 HI253WriteCmosSensor(0xc5, 0x1e);
 HI253WriteCmosSensor(0xc6, 0x24);
 HI253WriteCmosSensor(0xc7, 0x20);

//Dark2
 HI253WriteCmosSensor(0xc8, 0x18);
 HI253WriteCmosSensor(0xc9, 0x20);
 HI253WriteCmosSensor(0xca, 0x1e);
 HI253WriteCmosSensor(0xcb, 0x18);
 HI253WriteCmosSensor(0xcc, 0x20);
 HI253WriteCmosSensor(0xcd, 0x1e);

//Dark3 
 HI253WriteCmosSensor(0xce, 0x18);
 HI253WriteCmosSensor(0xcf, 0x20);
 HI253WriteCmosSensor(0xd0, 0x1e);
 HI253WriteCmosSensor(0xd1, 0x18);
 HI253WriteCmosSensor(0xd2, 0x20);
 HI253WriteCmosSensor(0xd3, 0x1e);


/////// PAGE 14 START ///////
HI253SetPage(0x14);
 HI253WriteCmosSensor(0x10, 0x11);
 HI253WriteCmosSensor(0x14, 0x80); // GX
 HI253WriteCmosSensor(0x15, 0x80); // GY
 HI253WriteCmosSensor(0x16, 0x80); // RX
 HI253WriteCmosSensor(0x17, 0x80); // RY
 HI253WriteCmosSensor(0x18, 0x80); // BX
 HI253WriteCmosSensor(0x19, 0x80); // BY
 HI253WriteCmosSensor(0x20, 0x60); //X 60 //a0
 HI253WriteCmosSensor(0x21, 0x80); //Y
 HI253WriteCmosSensor(0x22, 0x70);
 HI253WriteCmosSensor(0x23, 0x80);
 HI253WriteCmosSensor(0x24, 0x80);
 HI253WriteCmosSensor(0x30, 0xc8);
 HI253WriteCmosSensor(0x31, 0x2b);
 HI253WriteCmosSensor(0x32, 0x00);
 HI253WriteCmosSensor(0x33, 0x00);
 HI253WriteCmosSensor(0x34, 0x90);
 HI253WriteCmosSensor(0x40, 0x45);
 HI253WriteCmosSensor(0x50, 0x2d);
 HI253WriteCmosSensor(0x60, 0x28);
 HI253WriteCmosSensor(0x70, 0x2d);

  HI253SetPage(0x15); 
  HI253WriteCmosSensor(0x10, 0x0f); // CMCCTL
  HI253WriteCmosSensor(0x14, 0x52); // CMCOFSGH
  HI253WriteCmosSensor(0x15, 0x42); // CMCOFSGM
  HI253WriteCmosSensor(0x16, 0x32); // CMCOFSGL
  HI253WriteCmosSensor(0x17, 0x2f); // CMCSIGN

  //CMC 
  HI253WriteCmosSensor(0x30, 0x8f); // CMC11
  HI253WriteCmosSensor(0x31, 0x59); // CMC12
  HI253WriteCmosSensor(0x32, 0x0a); // CMC13
  HI253WriteCmosSensor(0x33, 0x15); // CMC21
  HI253WriteCmosSensor(0x34, 0x5b); // CMC22
  HI253WriteCmosSensor(0x35, 0x06); // CMC23
  HI253WriteCmosSensor(0x36, 0x07); // CMC31
  HI253WriteCmosSensor(0x37, 0x40); // CMC32
  HI253WriteCmosSensor(0x38, 0x86); // CMC33

  //CMC OFS 
  HI253WriteCmosSensor(0x40, 0x95); // CMCOFSL11
  HI253WriteCmosSensor(0x41, 0x1f); // CMCOFSL12
  HI253WriteCmosSensor(0x42, 0x8a); // CMCOFSL13
  HI253WriteCmosSensor(0x43, 0x86); // CMCOFSL21
  HI253WriteCmosSensor(0x44, 0x0a); // CMCOFSL22
  HI253WriteCmosSensor(0x45, 0x84); // CMCOFSL23
  HI253WriteCmosSensor(0x46, 0x87); // CMCOFSL31
  HI253WriteCmosSensor(0x47, 0x9b); // CMCOFSL32
  HI253WriteCmosSensor(0x48, 0x23); // CMCOFSL33

  //CMC POFS
  HI253WriteCmosSensor(0x50, 0x8c); // CMCOFSH11
  HI253WriteCmosSensor(0x51, 0x0c); // CMCOFSH12
  HI253WriteCmosSensor(0x52, 0x00); // CMCOFSH13
  HI253WriteCmosSensor(0x53, 0x07); // CMCOFSH21
  HI253WriteCmosSensor(0x54, 0x17); // CMCOFSH22
  HI253WriteCmosSensor(0x55, 0x9d); // CMCOFSH23
  HI253WriteCmosSensor(0x56, 0x00); // CMCOFSH31
  HI253WriteCmosSensor(0x57, 0x0b); // CMCOFSH32
  HI253WriteCmosSensor(0x58, 0x89); // CMCOFSH33

  HI253WriteCmosSensor(0x80, 0x03);
  HI253WriteCmosSensor(0x85, 0x40);
  HI253WriteCmosSensor(0x87, 0x02);
  HI253WriteCmosSensor(0x88, 0x00);
  HI253WriteCmosSensor(0x89, 0x00);
  HI253WriteCmosSensor(0x8a, 0x00);

/////// PAGE 16 START ///////
HI253SetPage(0x16);
 HI253WriteCmosSensor(0x10, 0x31);//GMA_CTL
 HI253WriteCmosSensor(0x18, 0x37);//AG_ON
 HI253WriteCmosSensor(0x19, 0x36);//AG_OFF
 HI253WriteCmosSensor(0x1A, 0x0e);//TIME_ON
 HI253WriteCmosSensor(0x1B, 0x01);//TIME_OFF
 HI253WriteCmosSensor(0x1C, 0xdc);//OUT_ON
 HI253WriteCmosSensor(0x1D, 0xfe);//OUT_OFF
//GMA
 HI253WriteCmosSensor(0x30, 0x00);
 HI253WriteCmosSensor(0x31, 0x08);
 HI253WriteCmosSensor(0x32, 0x19);
 HI253WriteCmosSensor(0x33, 0x32);
 HI253WriteCmosSensor(0x34, 0x55);
 HI253WriteCmosSensor(0x35, 0x73);
 HI253WriteCmosSensor(0x36, 0x86);
 HI253WriteCmosSensor(0x37, 0x96);
 HI253WriteCmosSensor(0x38, 0xa6);
 HI253WriteCmosSensor(0x39, 0xb6);
 HI253WriteCmosSensor(0x3a, 0xc5);
 HI253WriteCmosSensor(0x3b, 0xd0);
 HI253WriteCmosSensor(0x3c, 0xd8);
 HI253WriteCmosSensor(0x3d, 0xe0);
 HI253WriteCmosSensor(0x3e, 0xe8);
 HI253WriteCmosSensor(0x3f, 0xf0);
 HI253WriteCmosSensor(0x40, 0xf7);
 HI253WriteCmosSensor(0x41, 0xfe);
 HI253WriteCmosSensor(0x42, 0xff);
//RGMA
 HI253WriteCmosSensor(0x50, 0x00);
 HI253WriteCmosSensor(0x51, 0x0d);
 HI253WriteCmosSensor(0x52, 0x16);
 HI253WriteCmosSensor(0x53, 0x27);
 HI253WriteCmosSensor(0x54, 0x49);
 HI253WriteCmosSensor(0x55, 0x69);
 HI253WriteCmosSensor(0x56, 0x85);
 HI253WriteCmosSensor(0x57, 0x9e);
 HI253WriteCmosSensor(0x58, 0xb3);
 HI253WriteCmosSensor(0x59, 0xc4);
 HI253WriteCmosSensor(0x5a, 0xd2);
 HI253WriteCmosSensor(0x5b, 0xdd);
 HI253WriteCmosSensor(0x5c, 0xe7);
 HI253WriteCmosSensor(0x5d, 0xee);
 HI253WriteCmosSensor(0x5e, 0xf3);
 HI253WriteCmosSensor(0x5f, 0xf8);
 HI253WriteCmosSensor(0x60, 0xfb);
 HI253WriteCmosSensor(0x61, 0xfe);
 HI253WriteCmosSensor(0x62, 0xff);
//BGMA
 HI253WriteCmosSensor(0x70, 0x00);
 HI253WriteCmosSensor(0x71, 0x0a);
 HI253WriteCmosSensor(0x72, 0x19);
 HI253WriteCmosSensor(0x73, 0x31);
 HI253WriteCmosSensor(0x74, 0x55);
 HI253WriteCmosSensor(0x75, 0x6e);
 HI253WriteCmosSensor(0x76, 0x83);
 HI253WriteCmosSensor(0x77, 0x96);
 HI253WriteCmosSensor(0x78, 0xa6);
 HI253WriteCmosSensor(0x79, 0xb5);
 HI253WriteCmosSensor(0x7a, 0xc2);
 HI253WriteCmosSensor(0x7b, 0xcd);
 HI253WriteCmosSensor(0x7c, 0xd7);
 HI253WriteCmosSensor(0x7d, 0xe0);
 HI253WriteCmosSensor(0x7e, 0xe8);
 HI253WriteCmosSensor(0x7f, 0xf0);
 HI253WriteCmosSensor(0x80, 0xf6);
 HI253WriteCmosSensor(0x81, 0xfc);
 HI253WriteCmosSensor(0x82, 0xff);

/////// PAGE 17 START ///////
HI253SetPage(0x17);
 HI253WriteCmosSensor(0x10, 0xf7);

/////// PAGE 20 START ///////
HI253SetPage(0x20);
 HI253WriteCmosSensor(0x11, 0x1c);
 HI253WriteCmosSensor(0x18, 0x30);
 HI253WriteCmosSensor(0x1a, 0x08);
 HI253WriteCmosSensor(0x20, 0x01); //05_lowtemp Y Mean off
 HI253WriteCmosSensor(0x21, 0x30);
 HI253WriteCmosSensor(0x22, 0x10);
 HI253WriteCmosSensor(0x23, 0x00);
 HI253WriteCmosSensor(0x24, 0x00); //Uniform Scene Off
 HI253WriteCmosSensor(0x28, 0xe7);
 HI253WriteCmosSensor(0x29, 0x0d); //20100305 ad->0d
 HI253WriteCmosSensor(0x2a, 0xff);
 HI253WriteCmosSensor(0x2b, 0x34); //f4->Adaptive off
 HI253WriteCmosSensor(0x2c, 0xc3);
 HI253WriteCmosSensor(0x2d, 0xcf);  //fe->AE Speed option
 HI253WriteCmosSensor(0x2e, 0x33);
 HI253WriteCmosSensor(0x30, 0x78); //f8
 HI253WriteCmosSensor(0x32, 0x03);
 HI253WriteCmosSensor(0x33, 0x2e);
 HI253WriteCmosSensor(0x34, 0x30);
 HI253WriteCmosSensor(0x35, 0xd4);
 HI253WriteCmosSensor(0x36, 0xfe);
 HI253WriteCmosSensor(0x37, 0x32);
 HI253WriteCmosSensor(0x38, 0x04);
 HI253WriteCmosSensor(0x39, 0x22); //AE_escapeC10
 HI253WriteCmosSensor(0x3a, 0xde); //AE_escapeC11
 HI253WriteCmosSensor(0x3b, 0x22); //AE_escapeC1
 HI253WriteCmosSensor(0x3c, 0xde); //AE_escapeC2
 HI253WriteCmosSensor(0x50, 0x45);
 HI253WriteCmosSensor(0x51, 0x88);
 HI253WriteCmosSensor(0x56, 0x03);
 HI253WriteCmosSensor(0x57, 0xf7);
 HI253WriteCmosSensor(0x58, 0x14);
 HI253WriteCmosSensor(0x59, 0x88);
 HI253WriteCmosSensor(0x5a, 0x04);

//New Weight For Samsung
 HI253WriteCmosSensor(0x60, 0x5f);
 HI253WriteCmosSensor(0x61, 0xf5);
 HI253WriteCmosSensor(0x62, 0x6a);
 HI253WriteCmosSensor(0x63, 0xa9);
 HI253WriteCmosSensor(0x64, 0x6f);
 HI253WriteCmosSensor(0x65, 0xf9);
 HI253WriteCmosSensor(0x66, 0x6f);//eb
 HI253WriteCmosSensor(0x67, 0xf9);//eb
 HI253WriteCmosSensor(0x68, 0x6f);//eb
 HI253WriteCmosSensor(0x69, 0xf9);//eb
 HI253WriteCmosSensor(0x6a, 0x6a);
 HI253WriteCmosSensor(0x6b, 0xa9);
 HI253WriteCmosSensor(0x6c, 0x6a);
 HI253WriteCmosSensor(0x6d, 0xa9);
 HI253WriteCmosSensor(0x6e, 0x55);
 HI253WriteCmosSensor(0x6f, 0x55);
// HI253WriteCmosSensor(0x70, 0x42); 
 HI253WriteCmosSensor(0x70, 0x48); 
 HI253WriteCmosSensor(0x71, 0xBb);

// haunting control
 HI253WriteCmosSensor(0x76, 0x43);
 HI253WriteCmosSensor(0x77, 0xe2); //04 //f2
 HI253WriteCmosSensor(0x78, 0x23); //Yth1
 HI253WriteCmosSensor(0x79, 0x46); //Yth2 //46
 HI253WriteCmosSensor(0x7a, 0x23); //23
 HI253WriteCmosSensor(0x7b, 0x22); //22
 HI253WriteCmosSensor(0x7d, 0x23);
 
  HI253WriteCmosSensor(0x83, 0x01); //EXP Normal 33.33 fps 
  HI253WriteCmosSensor(0x84, 0x7c); 
  HI253WriteCmosSensor(0x85, 0xDC); 
  HI253WriteCmosSensor(0x86, 0x01); //EXPMin 10416.67 fps
  HI253WriteCmosSensor(0x87, 0xF4); 
  HI253WriteCmosSensor(0x88, 0x05); //EXP Max 10.00 fps 
  HI253WriteCmosSensor(0x89, 0xf3); 
  HI253WriteCmosSensor(0x8a, 0x70); 
  HI253WriteCmosSensor(0x8B, 0x7e); //EXP100 
  HI253WriteCmosSensor(0x8C, 0xF4); 
  HI253WriteCmosSensor(0x8D, 0x69); //EXP120 
  HI253WriteCmosSensor(0x8E, 0x78); 
  HI253WriteCmosSensor(0x9c, 0x0F); //EXP Limit 1736.11 fps 
  HI253WriteCmosSensor(0x9d, 0xA0); 
  HI253WriteCmosSensor(0x9e, 0x01); //EXP Unit 
  HI253WriteCmosSensor(0x9f, 0xF4); 

 HI253WriteCmosSensor(0xb0, 0x18);
 HI253WriteCmosSensor(0xb1, 0x14); //ADC 400->560
 HI253WriteCmosSensor(0xb2, 0xA0); //d0
 HI253WriteCmosSensor(0xb3, 0x18);
 HI253WriteCmosSensor(0xb4, 0x1a);
 HI253WriteCmosSensor(0xb5, 0x44);
 HI253WriteCmosSensor(0xb6, 0x2f);
 HI253WriteCmosSensor(0xb7, 0x28);
 HI253WriteCmosSensor(0xb8, 0x25);
 HI253WriteCmosSensor(0xb9, 0x22);
 HI253WriteCmosSensor(0xba, 0x21);
 HI253WriteCmosSensor(0xbb, 0x20);
 HI253WriteCmosSensor(0xbc, 0x1f);
 HI253WriteCmosSensor(0xbd, 0x1f);
 HI253WriteCmosSensor(0xc0, 0x14);
 HI253WriteCmosSensor(0xc1, 0x1f);
 HI253WriteCmosSensor(0xc2, 0x1f);
 HI253WriteCmosSensor(0xc3, 0x18); //2b
 HI253WriteCmosSensor(0xc4, 0x10); //08
 HI253WriteCmosSensor(0xc8, 0x80);
 HI253WriteCmosSensor(0xc9, 0x40);

/////// PAGE 22 START ///////
HI253SetPage(0x22);
 HI253WriteCmosSensor(0x10, 0xfd);
 HI253WriteCmosSensor(0x11, 0x2e);
 HI253WriteCmosSensor(0x19, 0x01); // Low On //
 HI253WriteCmosSensor(0x20, 0x30);
 HI253WriteCmosSensor(0x21, 0x80);
 HI253WriteCmosSensor(0x23, 0x08);
 HI253WriteCmosSensor(0x24, 0x01);

 HI253WriteCmosSensor(0x30, 0x80); // ULVL
 HI253WriteCmosSensor(0x31, 0x80); // VLVL
 HI253WriteCmosSensor(0x38, 0x11); // UVTH1
 HI253WriteCmosSensor(0x39, 0x34); // UVTH2
 HI253WriteCmosSensor(0x40, 0xf7); // YRANGE

 HI253WriteCmosSensor(0x41, 0x55); //44
 HI253WriteCmosSensor(0x42, 0x33); //43
 HI253WriteCmosSensor(0x43, 0xf6);
 HI253WriteCmosSensor(0x44, 0x55); //44
 HI253WriteCmosSensor(0x45, 0x44); //33
 HI253WriteCmosSensor(0x46, 0x00);

 HI253WriteCmosSensor(0x50, 0xb2);
 HI253WriteCmosSensor(0x51, 0x81);
 HI253WriteCmosSensor(0x52, 0x98);

 HI253WriteCmosSensor(0x80, 0x38); // RGAIN
 HI253WriteCmosSensor(0x81, 0x20); // GGAIN
 HI253WriteCmosSensor(0x82, 0x3e);
 HI253WriteCmosSensor(0x83, 0x53);
 HI253WriteCmosSensor(0x84, 0x16);//1E
 HI253WriteCmosSensor(0x85, 0x5A);//52
 HI253WriteCmosSensor(0x86, 0x25);
 HI253WriteCmosSensor(0x87, 0x40);
 HI253WriteCmosSensor(0x88, 0x30);
 HI253WriteCmosSensor(0x89, 0x3f);
 HI253WriteCmosSensor(0x8a, 0x28);
 HI253WriteCmosSensor(0x8b, 0x40);
 HI253WriteCmosSensor(0x8c, 0x33);//39-32
 HI253WriteCmosSensor(0x8d, 0x39);
 HI253WriteCmosSensor(0x8e, 0x30);
 HI253WriteCmosSensor(0x8f, 0x53);
 HI253WriteCmosSensor(0x90, 0x52);
 HI253WriteCmosSensor(0x91, 0x50);
 HI253WriteCmosSensor(0x92, 0x4c);
 HI253WriteCmosSensor(0x93, 0x43);
 HI253WriteCmosSensor(0x94, 0x37);
 HI253WriteCmosSensor(0x95, 0x2f);//2a
 HI253WriteCmosSensor(0x96, 0x26);//24
 HI253WriteCmosSensor(0x97, 0x21);//20
 HI253WriteCmosSensor(0x98, 0x1e);
 HI253WriteCmosSensor(0x99, 0x1f);
 HI253WriteCmosSensor(0x9a, 0x20);
 HI253WriteCmosSensor(0x9b, 0x88);
 HI253WriteCmosSensor(0x9c, 0x88);
 HI253WriteCmosSensor(0x9d, 0x48);
 HI253WriteCmosSensor(0x9e, 0x38);
 HI253WriteCmosSensor(0x9f, 0x30);
 HI253WriteCmosSensor(0xa0, 0x60);
 HI253WriteCmosSensor(0xa1, 0x34);
 HI253WriteCmosSensor(0xa2, 0x6f);
 HI253WriteCmosSensor(0xa3, 0xff);
 HI253WriteCmosSensor(0xa4, 0x14); //1500fps
 HI253WriteCmosSensor(0xa5, 0x2c); // 700fps
 HI253WriteCmosSensor(0xa6, 0xcf);
 HI253WriteCmosSensor(0xad, 0x40);
 HI253WriteCmosSensor(0xae, 0x4a);
 HI253WriteCmosSensor(0xaf, 0x28);  // low temp Rgain
 HI253WriteCmosSensor(0xb0, 0x26);  // low temp Rgain
 HI253WriteCmosSensor(0xb1, 0x00); //0x20 -> 0x00 0405 modify
 HI253WriteCmosSensor(0xb8, 0xa0); //a2: b-2, R+2  //b4 B-3, R+4 lowtemp
 HI253WriteCmosSensor(0xb9, 0x00);

  HI253SetPage(0x24); 
  HI253WriteCmosSensor(0x10, 0x01); // AFCTL1
  HI253WriteCmosSensor(0x18, 0x06);
  HI253WriteCmosSensor(0x30, 0x06);
  HI253WriteCmosSensor(0x31, 0x90);
  HI253WriteCmosSensor(0x32, 0x25);
  HI253WriteCmosSensor(0x33, 0xa2);
  HI253WriteCmosSensor(0x34, 0x26);
  HI253WriteCmosSensor(0x35, 0x58);
  HI253WriteCmosSensor(0x36, 0x60);
  HI253WriteCmosSensor(0x37, 0x00);
  HI253WriteCmosSensor(0x38, 0x50);
  HI253WriteCmosSensor(0x39, 0x00);

  HI253SetPage(0x20); 
  HI253WriteCmosSensor(0x10, 0x9c); // AE ON 50Hz
  HI253SetPage(0x22); 
  HI253WriteCmosSensor(0x10, 0xe9); // AWB ON

  HI253SetPage(0x00); 
  HI253WriteCmosSensor(0x0e, 0x03); // PLL
  HI253WriteCmosSensor(0x0e, 0x73); // PLL ON x2

  HI253SetPage(0x00); // Dummy 750us START
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00);
  HI253SetPage(0x00); // Dummy 750us END

  HI253SetPage(0x00); 
  HI253WriteCmosSensor(0x01, 0xf8); // Sleep OFF
  
  /*[END]*/
}

void HI253InitPara(void)
{
	spin_lock(&hi253_drv_lock);
  HI253Status.NightMode = KAL_FALSE;
  HI253Status.ZoomFactor = 0;
  HI253Status.Banding = AE_FLICKER_MODE_50HZ;
  HI253Status.PvShutter = 0x5e800;
  HI253Status.MaxFrameRate = HI253_MAX_FPS;
  HI253Status.MiniFrameRate = HI253_FPS(10);
  HI253Status.PvDummyPixels = 408;
  HI253Status.PvDummyLines = 130;
  HI253Status.CapDummyPixels = 408;
  HI253Status.CapDummyLines = 130; /* 10 FPS, 104 for 9.6 FPS*/
  HI253Status.PvOpClk = 26;
  HI253Status.CapOpClk = 26;
  HI253Status.VDOCTL2 = 0x90;
  HI253Status.ISPCTL3 = 0x30;
  HI253Status.AECTL1 = 0x9c;
  HI253Status.AWBCTL1 = 0xe9;
  	spin_unlock(&hi253_drv_lock);
}

/*************************************************************************
* FUNCTION
*  HI253SetMirror
*
* DESCRIPTION
*  This function mirror, flip or mirror & flip the sensor output image.
*
*  IMPORTANT NOTICE: For some sensor, it need re-set the output order Y1CbY2Cr after
*  mirror or flip.
*
* PARAMETERS
*  1. kal_uint16 : horizontal mirror or vertical flip direction.
*
* RETURNS
*  None
*
*************************************************************************/
static void HI253SetMirror(kal_uint16 ImageMirror)
{
	spin_lock(&hi253_drv_lock);
  HI253Status.VDOCTL2 &= 0xfc;
  spin_unlock(&hi253_drv_lock);
  switch (ImageMirror)
  {
    case IMAGE_H_MIRROR:
		spin_lock(&hi253_drv_lock);
      HI253Status.VDOCTL2 |= 0x01;
	  spin_unlock(&hi253_drv_lock);
      break;
    case IMAGE_V_MIRROR:
		spin_lock(&hi253_drv_lock);
      HI253Status.VDOCTL2 |= 0x02;
	  spin_unlock(&hi253_drv_lock);
      break;
    case IMAGE_HV_MIRROR:
		spin_lock(&hi253_drv_lock);
      HI253Status.VDOCTL2 |= 0x00;
	  spin_unlock(&hi253_drv_lock);
      break;
    case IMAGE_NORMAL:
    default:
		spin_lock(&hi253_drv_lock);
      HI253Status.VDOCTL2 |= 0x03;
	  spin_unlock(&hi253_drv_lock);
  }
  HI253SetPage(0x00);
  HI253WriteCmosSensor(0x11,HI253Status.VDOCTL2);
}

static void HI253SetAeMode(kal_bool AeEnable)
{
  SENSORDB("[HI253]HI253SetAeMode AeEnable:%d;\n",AeEnable);

  if (AeEnable == KAL_TRUE)
  {
  	spin_lock(&hi253_drv_lock);
    HI253Status.AECTL1 |= 0x80;
	spin_unlock(&hi253_drv_lock);
  }
  else
  {
  	spin_lock(&hi253_drv_lock);
    HI253Status.AECTL1 &= (~0x80);
	spin_unlock(&hi253_drv_lock);
  }
  HI253SetPage(0x20);
  HI253WriteCmosSensor(0x10,HI253Status.AECTL1);
}


static void HI253SetAwbMode(kal_bool AwbEnable)
{
  SENSORDB("[HI253]HI253SetAwbMode AwbEnable:%d;\n",AwbEnable);
  if (AwbEnable == KAL_TRUE)
  {
  	spin_lock(&hi253_drv_lock);
    HI253Status.AWBCTL1 |= 0x80;
	spin_unlock(&hi253_drv_lock);
  }
  else
  {
  	spin_lock(&hi253_drv_lock);
    HI253Status.AWBCTL1 &= (~0x80);
	spin_unlock(&hi253_drv_lock);
  }
  HI253SetPage(0x22);
  HI253WriteCmosSensor(0x10,HI253Status.AWBCTL1);
}

/*************************************************************************
* FUNCTION
* HI253NightMode
*
* DESCRIPTION
* This function night mode of HI253.
*
* PARAMETERS
* none
*
* RETURNS
* None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void HI253NightMode(kal_bool Enable)
{
  kal_uint32 EXPMAX, EXPTIME, BLC_TIME_TH_ONOFF;
  kal_uint32 LineLength,BandingValue;
  SENSORDB("[HI253]HI253NightMode Enable:%d;\n",Enable);
  /* Night mode only for camera preview */
  if (HI253Status.MaxFrameRate == HI253Status.MiniFrameRate)  return ;
  spin_lock(&hi253_drv_lock);
  HI253Status.MiniFrameRate = Enable ? HI253_FPS(5) : HI253_FPS(10);
  spin_unlock(&hi253_drv_lock);
  LineLength = HI253_PV_PERIOD_PIXEL_NUMS + HI253Status.PvDummyPixels;
  BandingValue = (HI253Status.Banding == AE_FLICKER_MODE_50HZ) ? 100 : 120;

  EXPTIME = (HI253Status.PvOpClk * 1000000 / LineLength / BandingValue) * BandingValue * LineLength * HI253_FRAME_RATE_UNIT / 8 / HI253Status.MaxFrameRate;
  EXPMAX = (HI253Status.PvOpClk * 1000000 / LineLength / BandingValue) * BandingValue * LineLength * HI253_FRAME_RATE_UNIT / 8 / HI253Status.MiniFrameRate;
  BLC_TIME_TH_ONOFF =  BandingValue * HI253_FRAME_RATE_UNIT / HI253Status.MiniFrameRate;

  SENSORDB("[HI253]LineLenght:%d,BandingValue:%d;MiniFrameRaet:%d\n",LineLength,BandingValue,HI253Status.MiniFrameRate);
  SENSORDB("[HI253]EXPTIME:%d; EXPMAX:%d; BLC_TIME_TH_ONOFF:%d;\n",EXPTIME,EXPMAX,BLC_TIME_TH_ONOFF);
  SENSORDB("[HI253]VDOCTL2:%x; AECTL1:%x; \n",HI253Status.VDOCTL2,HI253Status.AECTL1);

  HI253SetPage(0x00);
  HI253WriteCmosSensor(0x01, 0xf9); // Sleep ON
  spin_lock(&hi253_drv_lock);
  HI253Status.VDOCTL2 &= 0xfb;
  spin_unlock(&hi253_drv_lock);
  HI253WriteCmosSensor(0x11,HI253Status.VDOCTL2);  // Fixed frame rate OFF
  HI253WriteCmosSensor(0x90, 0x0c); // BLC_TIME_TH_ON
  HI253WriteCmosSensor(0x91, 0x0c); // BLC_TIME_TH_OFF
  HI253WriteCmosSensor(0x92, 0x78); // BLC_AG_TH_ON
  HI253WriteCmosSensor(0x93, 0x70); // BLC_AG_TH_OFF
  HI253SetPage(0x02);
  HI253WriteCmosSensor(0xd4, 0x0c); // DCDC_TIME_TH_ON
  HI253WriteCmosSensor(0xd5, 0x0c); // DCDC_TIME_TH_OFF
  HI253WriteCmosSensor(0xd6, 0x78); // DCDC_AG_TH_ON
  HI253WriteCmosSensor(0xd7, 0x70); // DCDC_AG_TH_OFF
  HI253SetPage(0x20);
  spin_lock(&hi253_drv_lock);
  HI253Status.AECTL1 &= (~0x80);
  spin_unlock(&hi253_drv_lock);
  HI253WriteCmosSensor(0x10,HI253Status.AECTL1);// AE ON BIT 7
  HI253WriteCmosSensor(0x18, 0x38); // AE Reset ON
  HI253WriteCmosSensor(0x11, 0x1c); // 0x35 for fixed frame rate
  HI253WriteCmosSensor(0x2a, 0xf0); // 0x35 for fixed frame rate
  HI253WriteCmosSensor(0x2b, 0x34); // 0x35 for fixed frame rate, 0x34 for dynamic frame rate
  //HI253WriteCmosSensor(0x83, 0x01); //EXP Normal 33.33 fps
  //HI253WriteCmosSensor(0x84, 0x7c);
  //HI253WriteCmosSensor(0x85, 0xdc);
  HI253WriteCmosSensor(0x88, 0x05); //EXP Max 8.33 fps
  HI253WriteCmosSensor(0x89, 0xf3);
  HI253WriteCmosSensor(0x8a, 0x70);
  HI253WriteCmosSensor(0x01, 0xf8); // Sleep OFF
  spin_lock(&hi253_drv_lock);
  HI253Status.AECTL1 |= 0x80;
  spin_unlock(&hi253_drv_lock);
  HI253WriteCmosSensor(0x10,HI253Status.AECTL1);// AE ON BIT 7
  HI253WriteCmosSensor(0x18, 0x30); // AE Reset OFF
} /* HI253NightMode */


/*************************************************************************
* FUNCTION
* HI253Open
*
* DESCRIPTION
* this function initialize the registers of CMOS sensor
*
* PARAMETERS
* none
*
* RETURNS
*  none
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HI253Open(void)
{
  kal_uint16 SensorId = 0;
  //1 software reset sensor and wait (to sensor)
  HI253SetPage(0x00);
  HI253WriteCmosSensor(0x01,0xf1);
  HI253WriteCmosSensor(0x01,0xf3);
  HI253WriteCmosSensor(0x01,0xf1);

  SensorId = HI253ReadCmosSensor(0x04);
  Sleep(3);
  SENSORDB("[HI253]HI253Open: Sensor ID %x\n",SensorId);

  if(SensorId != HI253_SENSOR_ID)
  {
    return ERROR_SENSOR_CONNECT_FAIL;
  }
  HI253InitSetting();
  HI253InitPara();
  return ERROR_NONE;

}
/* HI253Open() */

/*************************************************************************
* FUNCTION
*   HI253GetSensorID
*
* DESCRIPTION
*   This function get the sensor ID
*
* PARAMETERS
*   *sensorID : return the sensor ID
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HI253GetSensorID(UINT32 *sensorID)
{
	//1 software reset sensor and wait (to sensor)
	HI253SetPage(0x00);
	HI253WriteCmosSensor(0x01,0xf1);
	HI253WriteCmosSensor(0x01,0xf3);
	HI253WriteCmosSensor(0x01,0xf1);

	*sensorID = HI253ReadCmosSensor(0x04);
	Sleep(3);
	SENSORDB("[HI253]HI253GetSensorID: Sensor ID %x\n",*sensorID);

	if(*sensorID != HI253_SENSOR_ID)
	{
        *sensorID = 0xFFFFFFFF;
	  return ERROR_SENSOR_CONNECT_FAIL;
	}
	return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
* HI253Close
*
* DESCRIPTION
* This function is to turn off sensor module power.
*
* PARAMETERS
* None
*
* RETURNS
* None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HI253Close(void)
{
  return ERROR_NONE;
} /* HI253Close() */

/*************************************************************************
* FUNCTION
* HI253Preview
*
* DESCRIPTION
* This function start the sensor preview.
*
* PARAMETERS
* *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
* None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 HI253Preview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
  kal_uint32 LineLength, EXP100, EXP120, EXPMIN, EXPUNIT;

  SENSORDB("\n\n\n\n\n\n");
  SENSORDB("[HI253]HI253Preview\n");
  /* For change max frame rate only need modify HI253Status.MaxFrameRate */
  spin_lock(&hi253_drv_lock);
  HI253Status.MaxFrameRate = HI253_MAX_FPS;
  spin_unlock(&hi253_drv_lock);
//	HI253SetMirror(IMAGE_NORMAL);

	spin_lock(&hi253_drv_lock);
  HI253Status.PvDummyPixels = 424;
  spin_unlock(&hi253_drv_lock);
  LineLength = HI253_PV_PERIOD_PIXEL_NUMS + HI253Status.PvDummyPixels;
  spin_lock(&hi253_drv_lock);
  HI253Status.MiniFrameRate = HI253_FPS(10);
  HI253Status.PvDummyLines = HI253Status.PvOpClk * 1000000 * HI253_FRAME_RATE_UNIT / LineLength / HI253Status.MaxFrameRate -  HI253_PV_PERIOD_LINE_NUMS;
  spin_unlock(&hi253_drv_lock);

  HI253SetPage(0x00);
	HI253WriteCmosSensor(0x10, 0x10);
	HI253WriteCmosSensor(0x11, 0x90);
	HI253WriteCmosSensor(0x12, 0x04);
	HI253WriteCmosSensor(0x20, 0x00); // WINROWH
	HI253WriteCmosSensor(0x21, 0x04); // WINROWL
	HI253WriteCmosSensor(0x22, 0x00); // WINCOLH
	HI253WriteCmosSensor(0x23, 0x07); // WINCOLL

	HI253WriteCmosSensor(0x3f, 0x00);
	HI253WriteCmosSensor(0x40, 0x01); //Hblank 360
	HI253WriteCmosSensor(0x41, 0x68);
	HI253WriteCmosSensor(0x42, 0x00); //Vblank 18
	HI253WriteCmosSensor(0x43, 0x12);
       HI253WriteCmosSensor(0x3f, 0x02);

  HI253SetPage(0x12);
  HI253WriteCmosSensor(0x20, 0x0f);
  HI253WriteCmosSensor(0x21, 0x0f);
  HI253WriteCmosSensor(0x90, 0x5d);
  HI253SetPage(0x13);
  HI253WriteCmosSensor(0x80, 0xfd);

  EXP100 = (HI253Status.PvOpClk * 1000000 / LineLength) * LineLength / 100 / 8;
  EXP120 = (HI253Status.PvOpClk * 1000000 / LineLength) * LineLength / 120 / 8;
  EXPMIN = EXPUNIT = LineLength / 4;

  SENSORDB("[HI253]DummyPixel:%d DummyLine:%d; LineLenght:%d,Plck:%d\n",HI253Status.PvDummyPixels,HI253Status.PvDummyLines,LineLength,HI253Status.PvOpClk);
  SENSORDB("[HI253]EXP100:%d EXP120:%d;\n",EXP100,EXP120);

	HI253SetPage(0x20);
  HI253WriteCmosSensor(0x86, 0x01); //EXPMin 10416.67 fps
  HI253WriteCmosSensor(0x87, 0xF4); 

  HI253WriteCmosSensor(0x8B, 0x7e); //EXP100 
  HI253WriteCmosSensor(0x8C, 0xF4); 
  HI253WriteCmosSensor(0x8D, 0x69); //EXP120 
  HI253WriteCmosSensor(0x8E, 0x78); 
  HI253WriteCmosSensor(0x9c, 0x0F); //EXP Limit 1736.11 fps 
  HI253WriteCmosSensor(0x9d, 0xA0); 
  HI253WriteCmosSensor(0x9e, 0x01); //EXP Unit 
  HI253WriteCmosSensor(0x9f, 0xF4); 

	HI253SetAeMode(KAL_TRUE);
	HI253SetAwbMode(KAL_TRUE);
	HI253SetMirror(IMAGE_HV_MIRROR);
	return ERROR_NONE;
}/* HI253Preview() */

UINT32 HI253Capture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
  kal_uint32 LineLength, EXP100, EXP120, EXPMIN, EXPUNIT, CapShutter;
  kal_uint8 ClockDivider;
  kal_uint32 temp;
  SENSORDB("\n\n\n\n\n\n");
  SENSORDB("[HI253]HI253Capture!!!!!!!!!!!!!\n");
  SENSORDB("[HI253]Image Target Width: %d; Height: %d\n",image_window->ImageTargetWidth, image_window->ImageTargetHeight);
  if ((image_window->ImageTargetWidth<=HI253_PV_WIDTH)&&
      (image_window->ImageTargetHeight<=HI253_PV_HEIGHT))
    return ERROR_NONE;    /* Less than PV Mode */

  HI253WriteCmosSensor(0x01, 0xf9); // Sleep ON
  HI253SetAeMode(KAL_FALSE);
  HI253SetAwbMode(KAL_FALSE);

  HI253SetPage(0x20);
  temp=(HI253ReadCmosSensor(0x80) << 16)|(HI253ReadCmosSensor(0x81) << 8)|HI253ReadCmosSensor(0x82);
  spin_lock(&hi253_drv_lock);
  HI253Status.PvShutter = temp;
  spin_unlock(&hi253_drv_lock);

  // 1600*1200
  HI253SetPage(0x00);
  HI253WriteCmosSensor(0x10,0x00);
 HI253WriteCmosSensor(0x11, 0x90);
  HI253WriteCmosSensor(0x3f,0x00);
  HI253SetPage(0x12);
  HI253WriteCmosSensor(0x20, 0x0f);
  HI253WriteCmosSensor(0x21, 0x0f);
  HI253WriteCmosSensor(0x90, 0x5d);
  HI253SetPage(0x13);
  HI253WriteCmosSensor(0x80, 0xfd);
  /*capture 1600*1200 start x, y*/
  HI253SetPage(0x00);
  HI253WriteCmosSensor(0x20, 0x00); // WINROWH
  HI253WriteCmosSensor(0x21, 0x0f); // WINROWL
  HI253WriteCmosSensor(0x22, 0x00); // WINCOLH
  HI253WriteCmosSensor(0x23, 0x19); // WINCOLL
  spin_lock(&hi253_drv_lock);
  HI253Status.CapDummyPixels = 360;
  HI253Status.CapDummyLines = 52; /* 10 FPS, 104 for 9.6 FPS*/
  spin_unlock(&hi253_drv_lock);
  LineLength = HI253_FULL_PERIOD_PIXEL_NUMS + HI253Status.CapDummyPixels;
  spin_lock(&hi253_drv_lock);
  HI253Status.CapOpClk = 26;
  spin_unlock(&hi253_drv_lock);
  EXP100 = (HI253Status.CapOpClk * 1000000 / LineLength) * LineLength / 100 / 8;
  EXP120 = (HI253Status.CapOpClk * 1000000 / LineLength) * LineLength / 120 / 8;
  EXPMIN = EXPUNIT = LineLength / 4;

  HI253SetPage(0x20);

  
  HI253WriteCmosSensor(0x86, 0x01); //EXPMin 10416.67 fps
  HI253WriteCmosSensor(0x87, 0xF4); 

  HI253WriteCmosSensor(0x8B, 0x7e); //EXP100 
  HI253WriteCmosSensor(0x8C, 0xF4); 
  HI253WriteCmosSensor(0x8D, 0x69); //EXP120 
  HI253WriteCmosSensor(0x8E, 0x78); 
  HI253WriteCmosSensor(0x9c, 0x0F); //EXP Limit 1736.11 fps 
  HI253WriteCmosSensor(0x9d, 0xA0); 
  HI253WriteCmosSensor(0x9e, 0x01); //EXP Unit 
  HI253WriteCmosSensor(0x9f, 0xF4); 

	HI253SetPage(0x00);
	HI253WriteCmosSensor(0x40, 0x01); //Hblank 360
	HI253WriteCmosSensor(0x41, 0x68);
	HI253WriteCmosSensor(0x42, 0x00); //Vblank 18
	HI253WriteCmosSensor(0x43, 0x14);

  if(HI253Status.ZoomFactor > 8)
  {
    ClockDivider = 1; //Op CLock 13M
  }
  else
  {
    ClockDivider = 0; //OpCLock 26M
  }
  SENSORDB("[HI253]ClockDivider: %d \n",ClockDivider);
  HI253WriteCmosSensor(0x12, 0x04|ClockDivider);
  CapShutter = HI253Status.PvShutter >> ClockDivider;
  if(CapShutter<1)      CapShutter = 1;

	HI253SetPage(0x20);
	//HI253WriteCmosSensor(0x83, (CapShutter >> 16) & 0xFF);
	//HI253WriteCmosSensor(0x84, (CapShutter >> 8) & 0xFF);
	//HI253WriteCmosSensor(0x85, CapShutter & 0xFF);
	HI253WriteCmosSensor(0x01, 0xf8); // Sleep OFF
	return ERROR_NONE;
} /* HI253Capture() */

UINT32 HI253GetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
  pSensorResolution->SensorFullWidth = HI253_FULL_WIDTH;
  pSensorResolution->SensorFullHeight = HI253_FULL_HEIGHT;
  pSensorResolution->SensorPreviewWidth = HI253_PV_WIDTH;
  pSensorResolution->SensorPreviewHeight = HI253_PV_HEIGHT;

  pSensorResolution->SensorVideoWidth=HI253_PV_WIDTH;
  pSensorResolution->SensorVideoHeight=HI253_PV_HEIGHT;
  return ERROR_NONE;
} /* HI253GetResolution() */

UINT32 HI253GetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                    MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
  pSensorInfo->SensorPreviewResolutionX=HI253_PV_WIDTH;
  pSensorInfo->SensorPreviewResolutionY=HI253_PV_HEIGHT;
  pSensorInfo->SensorFullResolutionX=HI253_FULL_WIDTH;
  pSensorInfo->SensorFullResolutionY=HI253_FULL_HEIGHT;

  pSensorInfo->SensorCameraPreviewFrameRate=30;
  pSensorInfo->SensorVideoFrameRate=30;
  pSensorInfo->SensorStillCaptureFrameRate=10;
  pSensorInfo->SensorWebCamCaptureFrameRate=15;
  pSensorInfo->SensorResetActiveHigh=FALSE;
  pSensorInfo->SensorResetDelayCount=1;
  pSensorInfo->SensorOutputDataFormat=SENSOR_OUTPUT_FORMAT_UYVY;//SENSOR_OUTPUT_FORMAT_YUYV; // back for 16 SENSOR_OUTPUT_FORMAT_UYVY;
  pSensorInfo->SensorClockPolarity=SENSOR_CLOCK_POLARITY_LOW;
  pSensorInfo->SensorClockFallingPolarity=SENSOR_CLOCK_POLARITY_LOW;
  pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
  pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
  pSensorInfo->SensorInterruptDelayLines = 1;
  pSensorInfo->SensroInterfaceType=SENSOR_INTERFACE_TYPE_PARALLEL;


  pSensorInfo->CaptureDelayFrame = 1;
  pSensorInfo->PreviewDelayFrame = 1;
  pSensorInfo->VideoDelayFrame = 2;
  pSensorInfo->SensorMasterClockSwitch = 0;
  pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_8MA;

  switch (ScenarioId)
  {
    case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
    case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    default:
      pSensorInfo->SensorClockFreq=26;
      pSensorInfo->SensorClockDividCount=3;
      pSensorInfo->SensorClockRisingCount=0;
      pSensorInfo->SensorClockFallingCount=2;
      pSensorInfo->SensorPixelClockCount=3;
      pSensorInfo->SensorDataLatchCount=2;
      pSensorInfo->SensorGrabStartX = HI253_GRAB_START_X;
      pSensorInfo->SensorGrabStartY = HI253_GRAB_START_Y;
      break;
  }
  return ERROR_NONE;
} /* HI253GetInfo() */


UINT32 HI253Control(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
  switch (ScenarioId)
  {
  case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
  case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
    HI253Preview(pImageWindow, pSensorConfigData);
    break;
  case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
    HI253Capture(pImageWindow, pSensorConfigData);
    break;
  default:
    break;
  }
  return TRUE;
} /* HI253Control() */


BOOL HI253SetWb(UINT16 Para)
{
  SENSORDB("[HI253]HI253SetWb Para:%d;\n",Para);
  switch (Para)
  {
    case AWB_MODE_OFF:
      HI253SetAwbMode(KAL_FALSE);
      break;
    case AWB_MODE_AUTO:
      HI253SetAwbMode(KAL_TRUE);
      HI253WriteCmosSensor(0x03, 0x22);
      HI253WriteCmosSensor(0x11, 0x2e);
      HI253WriteCmosSensor(0x83, 0x5e);
      HI253WriteCmosSensor(0x84, 0x1e);
      HI253WriteCmosSensor(0x85, 0x5e);
      HI253WriteCmosSensor(0x86, 0x25);
      break;
    case AWB_MODE_CLOUDY_DAYLIGHT: //cloudy
      HI253SetAwbMode(KAL_FALSE);
      HI253WriteCmosSensor(0x03, 0x22);
      HI253WriteCmosSensor(0x11, 0x28);
      HI253WriteCmosSensor(0x80, 0x49);
      HI253WriteCmosSensor(0x81, 0x20);
      HI253WriteCmosSensor(0x82, 0x24);
      HI253WriteCmosSensor(0x83, 0x50);
      HI253WriteCmosSensor(0x84, 0x45);
      HI253WriteCmosSensor(0x85, 0x24);
      HI253WriteCmosSensor(0x86, 0x1e);
      break;
    case AWB_MODE_DAYLIGHT: //sunny
      HI253SetAwbMode(KAL_FALSE);
      HI253WriteCmosSensor(0x03, 0x22);
      HI253WriteCmosSensor(0x11, 0x28);
      HI253WriteCmosSensor(0x80, 0x45);
      HI253WriteCmosSensor(0x81, 0x20);
      HI253WriteCmosSensor(0x82, 0x27);
      HI253WriteCmosSensor(0x83, 0x44);
      HI253WriteCmosSensor(0x84, 0x3f);
      HI253WriteCmosSensor(0x85, 0x29);
      HI253WriteCmosSensor(0x86, 0x23);
      break;
    case AWB_MODE_INCANDESCENT: //office
      HI253SetAwbMode(KAL_FALSE);
	HI253WriteCmosSensor(0x03, 0x22);
      HI253WriteCmosSensor(0x11, 0x28);
      HI253WriteCmosSensor(0x80, 0x33);
      HI253WriteCmosSensor(0x81, 0x20);
      HI253WriteCmosSensor(0x82, 0x3d);
      HI253WriteCmosSensor(0x83, 0x2e);
      HI253WriteCmosSensor(0x84, 0x24);
      HI253WriteCmosSensor(0x85, 0x43);
      HI253WriteCmosSensor(0x86, 0x3d);
      break;
    case AWB_MODE_TUNGSTEN: //home
      HI253SetAwbMode(KAL_FALSE);
	HI253WriteCmosSensor(0x03, 0x22);
      HI253WriteCmosSensor(0x11, 0x28);
      HI253WriteCmosSensor(0x80, 0x25);
      HI253WriteCmosSensor(0x81, 0x20);
      HI253WriteCmosSensor(0x82, 0x44);
      HI253WriteCmosSensor(0x83, 0x22);
      HI253WriteCmosSensor(0x84, 0x1e);
      HI253WriteCmosSensor(0x85, 0x50);
      HI253WriteCmosSensor(0x86, 0x45);
      break;
    case AWB_MODE_FLUORESCENT:
      HI253SetAwbMode(KAL_FALSE);
	HI253WriteCmosSensor(0x03, 0x22);
      HI253WriteCmosSensor(0x11, 0x28);
      HI253WriteCmosSensor(0x80, 0x45);
      HI253WriteCmosSensor(0x81, 0x20);
      HI253WriteCmosSensor(0x82, 0x2f);
      HI253WriteCmosSensor(0x83, 0x38);
      HI253WriteCmosSensor(0x84, 0x32);
      HI253WriteCmosSensor(0x85, 0x39);
      HI253WriteCmosSensor(0x86, 0x33);
    default:
      return KAL_FALSE;
  }
  return KAL_TRUE;
} /* HI253SetWb */

BOOL HI253SetEffect(UINT16 Para)
{
  SENSORDB("[HI253]HI253SetEffect Para:%d;\n",Para);
  switch (Para)
  {
    case MEFFECT_OFF:
      HI253SetPage(0x10);
      HI253WriteCmosSensor(0x11, 0x03);
      HI253WriteCmosSensor(0x12, 0x30);
      HI253WriteCmosSensor(0x13, 0x0a);
      break;
    case MEFFECT_SEPIA:
      HI253SetPage(0x10);
      HI253WriteCmosSensor(0x11, 0x03);
      HI253WriteCmosSensor(0x12, 0x23);
      HI253WriteCmosSensor(0x13, 0x00);
      HI253WriteCmosSensor(0x44, 0x70);
      HI253WriteCmosSensor(0x45, 0x98);
      HI253WriteCmosSensor(0x47, 0x7f);
      HI253WriteCmosSensor(0x03, 0x13);
      HI253WriteCmosSensor(0x20, 0x07);
      HI253WriteCmosSensor(0x21, 0x07);
      break;
    case MEFFECT_NEGATIVE://----datasheet
      HI253SetPage(0x10);
      HI253WriteCmosSensor(0x11, 0x03);
      HI253WriteCmosSensor(0x12, 0x28);
      HI253WriteCmosSensor(0x13, 0x00);
      HI253WriteCmosSensor(0x44, 0x80);
      HI253WriteCmosSensor(0x45, 0x80);
      HI253WriteCmosSensor(0x47, 0x7f);
      HI253WriteCmosSensor(0x03, 0x13);
      HI253WriteCmosSensor(0x20, 0x07);
      HI253WriteCmosSensor(0x21, 0x07);
      break;
    case MEFFECT_SEPIAGREEN://----datasheet aqua
      HI253SetPage(0x10);
      HI253WriteCmosSensor(0x11, 0x03);
      HI253WriteCmosSensor(0x12, 0x23);
      HI253WriteCmosSensor(0x13, 0x00);
      HI253WriteCmosSensor(0x44, 0x80);
      HI253WriteCmosSensor(0x45, 0x04);
      HI253WriteCmosSensor(0x47, 0x7f);
      HI253WriteCmosSensor(0x03, 0x13);
      HI253WriteCmosSensor(0x20, 0x07);
      HI253WriteCmosSensor(0x21, 0x07);
      break;
    case MEFFECT_SEPIABLUE:
      HI253SetPage(0x10);
      HI253WriteCmosSensor(0x11, 0x03);
      HI253WriteCmosSensor(0x12, 0x23);
      HI253WriteCmosSensor(0x13, 0x00);
      HI253WriteCmosSensor(0x44, 0xb0);
      HI253WriteCmosSensor(0x45, 0x40);
      HI253WriteCmosSensor(0x47, 0x7f);
      HI253WriteCmosSensor(0x03, 0x13);
      HI253WriteCmosSensor(0x20, 0x07);
      HI253WriteCmosSensor(0x21, 0x07);
      break;
    case MEFFECT_MONO: //----datasheet black & white
      HI253SetPage(0x10);
      HI253WriteCmosSensor(0x11, 0x03);
      HI253WriteCmosSensor(0x12, 0x23);
      HI253WriteCmosSensor(0x13, 0x00);
      HI253WriteCmosSensor(0x44, 0x80);
      HI253WriteCmosSensor(0x45, 0x80);
      HI253WriteCmosSensor(0x47, 0x7f);
      HI253WriteCmosSensor(0x03, 0x13);
      HI253WriteCmosSensor(0x20, 0x07);
      HI253WriteCmosSensor(0x21, 0x07);
      break;
    default:
      return KAL_FALSE;
  }
  return KAL_TRUE;

} /* HI253SetEffect */

BOOL HI253SetBanding(UINT16 Para)
{
  SENSORDB("[HI253]HI253SetBanding Para:%d;\n",Para);
  spin_lock(&hi253_drv_lock);
  HI253Status.Banding = Para;
  spin_unlock(&hi253_drv_lock);
  if (HI253Status.Banding == AE_FLICKER_MODE_50HZ)
  	{
  	spin_lock(&hi253_drv_lock);
    HI253Status.AECTL1 |= 0x10;
	spin_unlock(&hi253_drv_lock);
  	}
  else
  	{
  	spin_lock(&hi253_drv_lock);
    HI253Status.AECTL1 &= (~0x10);
	spin_unlock(&hi253_drv_lock);
  	}

  HI253SetPage(0x20);
  HI253WriteCmosSensor(0x10,HI253Status.AECTL1);
  return TRUE;
} /* HI253SetBanding */

BOOL HI253SetExposure(UINT16 Para)
{
  SENSORDB("[HI253]HI253SetExposure Para:%d;\n",Para);
  HI253SetPage(0x10);
  spin_lock(&hi253_drv_lock);
  HI253Status.ISPCTL3 |= 0x10;
  spin_unlock(&hi253_drv_lock);
  HI253WriteCmosSensor(0x12,HI253Status.ISPCTL3);//make sure the Yoffset control is opened.

  switch (Para)
  {
    case AE_EV_COMP_n13:              /* EV -2 */
      HI253WriteCmosSensor(0x40,0xe0);
      break;
    case AE_EV_COMP_n10:              /* EV -1.5 */
      HI253WriteCmosSensor(0x40,0xc8);
      break;
    case AE_EV_COMP_n07:              /* EV -1 */
      HI253WriteCmosSensor(0x40,0xb0);
      break;
    case AE_EV_COMP_n03:              /* EV -0.5 */
      HI253WriteCmosSensor(0x40,0x98);
      break;
    case AE_EV_COMP_00:                /* EV 0 */\
      HI253WriteCmosSensor(0x40,0x88);
      break;
    case AE_EV_COMP_03:              /* EV +0.5 */
      HI253WriteCmosSensor(0x40,0x18);
      break;
    case AE_EV_COMP_07:              /* EV +1 */
      HI253WriteCmosSensor(0x40,0x30);
      break;
    case AE_EV_COMP_10:              /* EV +1.5 */
      HI253WriteCmosSensor(0x40,0x48);
      break;
    case AE_EV_COMP_13:              /* EV +2 */
      HI253WriteCmosSensor(0x40,0x60);
      break;
    default:
      return KAL_FALSE;
  }
  return KAL_TRUE;
} /* HI253SetExposure */

UINT32 HI253YUVSensorSetting(FEATURE_ID Cmd, UINT32 Para)
{
  switch (Cmd) {
    case FID_SCENE_MODE:
      if (Para == SCENE_MODE_OFF)
      {
        HI253NightMode(KAL_FALSE);
      }
      else if (Para == SCENE_MODE_NIGHTSCENE)
      {
        HI253NightMode(KAL_TRUE);
      }
      break;
    case FID_AWB_MODE:
      HI253SetWb(Para);
      break;
    case FID_COLOR_EFFECT:
      HI253SetEffect(Para);
      break;
    case FID_AE_EV:
      HI253SetExposure(Para);
      break;
    case FID_AE_FLICKER:
      HI253SetBanding(Para);
      break;
    case FID_AE_SCENE_MODE:
      if (Para == AE_MODE_OFF)
      {
        HI253SetAeMode(KAL_FALSE);
      }
      else
      {
        HI253SetAeMode(KAL_TRUE);
      }
      break;
    case FID_ZOOM_FACTOR:
      SENSORDB("[HI253]ZoomFactor :%d;\n",Para);
	  spin_lock(&hi253_drv_lock);
      HI253Status.ZoomFactor = Para;
	  spin_unlock(&hi253_drv_lock);
      break;
    default:
      break;
  }
  return TRUE;
}   /* HI253YUVSensorSetting */

UINT32 HI253YUVSetVideoMode(UINT16 FrameRate)
{
  return;
  kal_uint32 EXPFIX, BLC_TIME_TH_ONOFF;
  kal_uint32 LineLength,BandingValue;

  SENSORDB("[HI253]HI253YUVSetVideoMode FrameRate:%d;\n",FrameRate);
  if (FrameRate * HI253_FRAME_RATE_UNIT > HI253_MAX_FPS)
    return -1;
  spin_lock(&hi253_drv_lock);
  HI253Status.MaxFrameRate = HI253Status.MiniFrameRate = FrameRate * HI253_FRAME_RATE_UNIT;
  spin_unlock(&hi253_drv_lock);
  LineLength = HI253_PV_PERIOD_PIXEL_NUMS + HI253Status.PvDummyPixels;
  BandingValue = (HI253Status.Banding == AE_FLICKER_MODE_50HZ) ? 100 : 120;

  EXPFIX = (HI253Status.PvOpClk * 1000000 / LineLength / BandingValue) * BandingValue * LineLength * HI253_FRAME_RATE_UNIT / 8 / HI253Status.MiniFrameRate;

  BLC_TIME_TH_ONOFF =  BandingValue * HI253_FRAME_RATE_UNIT / HI253Status.MiniFrameRate;

  SENSORDB("[HI253]LineLenght:%d,BandingValue:%d\n",LineLength,BandingValue);
  SENSORDB("[HI253]EXPFIX:%d BLC_TIME_TH_ONOFF:%d\n;",EXPFIX,BLC_TIME_TH_ONOFF);

  HI253SetPage(0x00);
  HI253WriteCmosSensor(0x01, 0xf9); // Sleep ON
  spin_lock(&hi253_drv_lock);
  HI253Status.VDOCTL2 |= 0x04;
  spin_unlock(&hi253_drv_lock);
  HI253WriteCmosSensor(0x11,HI253Status.VDOCTL2);  // Fixed frame rate OFF
  HI253WriteCmosSensor(0x90, 0x0c); // BLC_TIME_TH_ON
  HI253WriteCmosSensor(0x91, 0x0c); // BLC_TIME_TH_OFF
  HI253WriteCmosSensor(0x92, 0x78); // BLC_AG_TH_ON
  HI253WriteCmosSensor(0x93, 0x70); // BLC_AG_TH_OFF
  HI253WriteCmosSensor(0x03, 0x02); // Page 2
  HI253WriteCmosSensor(0xd4, 0x0c); // DCDC_TIME_TH_ON
  HI253WriteCmosSensor(0xd5, 0x0c); // DCDC_TIME_TH_OFF
  HI253WriteCmosSensor(0xd6, 0x78); // DCDC_AG_TH_ON
  HI253WriteCmosSensor(0xd7, 0x70); // DCDC_AG_TH_OFF

  HI253SetPage(0x20);
  spin_lock(&hi253_drv_lock);
  HI253Status.AECTL1 &= (~0x80);
  spin_unlock(&hi253_drv_lock);
  HI253WriteCmosSensor(0x10,HI253Status.AECTL1);// AE ON BIT 7
  HI253WriteCmosSensor(0x18, 0x38); // AE Reset ON
  HI253WriteCmosSensor(0x11, 0x00); // 0x35 for fixed frame rate
  HI253WriteCmosSensor(0x2a, 0x03); // 0x35 for fixed frame rate
  HI253WriteCmosSensor(0x2b, 0x35); // 0x35 for fixed frame rate, 0x34 for dynamic frame rate
 // HI253WriteCmosSensor(0x83, 0x01); //EXP Normal 33.33 fps
 // HI253WriteCmosSensor(0x84, 0x7c);
 // HI253WriteCmosSensor(0x85, 0xdc);
  HI253WriteCmosSensor(0x88, 0x05); //EXP Max 8.33 fps
  HI253WriteCmosSensor(0x89, 0xf3);
  HI253WriteCmosSensor(0x8a, 0x70);
  HI253WriteCmosSensor(0x91, 0x06); //EXP Fix 8.00 fps
  HI253WriteCmosSensor(0x92, 0x32);
  HI253WriteCmosSensor(0x93, 0xea);
  HI253WriteCmosSensor(0x01, 0xf8); // Sleep OFF
  spin_lock(&hi253_drv_lock);
  HI253Status.AECTL1 |= 0x80;
  spin_unlock(&hi253_drv_lock);
  HI253WriteCmosSensor(0x10,HI253Status.AECTL1);// AE ON BIT 7
  HI253WriteCmosSensor(0x18, 0x30); // AE Reset OFF
  HI253SetMirror(IMAGE_HV_MIRROR);

  
  return TRUE;
}

UINT32 HI253FeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
                        UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
  UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
  UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
  UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
  UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
  MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
  MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;

  switch (FeatureId)
  {
    case SENSOR_FEATURE_GET_RESOLUTION:
      *pFeatureReturnPara16++=HI253_FULL_WIDTH;
      *pFeatureReturnPara16=HI253_FULL_HEIGHT;
      *pFeatureParaLen=4;
      break;
    case SENSOR_FEATURE_GET_PERIOD:
      *pFeatureReturnPara16++=HI253_PV_PERIOD_PIXEL_NUMS+HI253Status.PvDummyPixels;
      *pFeatureReturnPara16=HI253_PV_PERIOD_LINE_NUMS+HI253Status.PvDummyLines;
      *pFeatureParaLen=4;
      break;
    case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
      *pFeatureReturnPara32 = HI253Status.PvOpClk*2;
      *pFeatureParaLen=4;
      break;
    case SENSOR_FEATURE_SET_ESHUTTER:
      break;
    case SENSOR_FEATURE_SET_NIGHTMODE:
      HI253NightMode((BOOL) *pFeatureData16);
      break;
    case SENSOR_FEATURE_SET_GAIN:
      case SENSOR_FEATURE_SET_FLASHLIGHT:
      break;
    case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
      break;
    case SENSOR_FEATURE_SET_REGISTER:
      HI253WriteCmosSensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
      break;
    case SENSOR_FEATURE_GET_REGISTER:
      pSensorRegData->RegData = HI253ReadCmosSensor(pSensorRegData->RegAddr);
      break;
    case SENSOR_FEATURE_GET_CONFIG_PARA:
      *pFeatureParaLen=sizeof(MSDK_SENSOR_CONFIG_STRUCT);
      break;
    case SENSOR_FEATURE_SET_CCT_REGISTER:
    case SENSOR_FEATURE_GET_CCT_REGISTER:
    case SENSOR_FEATURE_SET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_ENG_REGISTER:
    case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
    case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
    case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
    case SENSOR_FEATURE_GET_GROUP_INFO:
    case SENSOR_FEATURE_GET_ITEM_INFO:
    case SENSOR_FEATURE_SET_ITEM_INFO:
    case SENSOR_FEATURE_GET_ENG_INFO:
      break;
    case SENSOR_FEATURE_GET_GROUP_COUNT:
      *pFeatureReturnPara32++=0;
      *pFeatureParaLen=4;
      break;
    case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
      // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
      // if EEPROM does not exist in camera module.
      *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
      *pFeatureParaLen=4;
      break;
    case SENSOR_FEATURE_SET_YUV_CMD:
      HI253YUVSensorSetting((FEATURE_ID)*pFeatureData32, *(pFeatureData32+1));
      break;
    case SENSOR_FEATURE_SET_VIDEO_MODE:
      HI253YUVSetVideoMode(*pFeatureData16);
      break;
  case SENSOR_FEATURE_CHECK_SENSOR_ID:
	  HI253GetSensorID(pFeatureReturnPara32);
	  break;
    default:
      break;
  }
  return ERROR_NONE;
} /* HI253FeatureControl() */



UINT32 HI253_YUV_SensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
  static SENSOR_FUNCTION_STRUCT SensorFuncHI253=
  {
    HI253Open,
    HI253GetInfo,
    HI253GetResolution,
    HI253FeatureControl,
    HI253Control,
    HI253Close
  };

  /* To Do : Check Sensor status here */
  if (pfFunc!=NULL)
    *pfFunc=&SensorFuncHI253;

  return ERROR_NONE;
} /* SensorInit() */
