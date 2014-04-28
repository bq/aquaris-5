#include "camera_custom_nvram.h"
#include "camera_custom_types.h"

#include "camera_custom_AEPlinetable.h"
#include "camera_custom_nvram.h"

#include <cutils/xlog.h>
#include "flash_feature.h"
#include "flash_param.h"
#include "flash_tuning_custom.h"
#include <kd_camera_feature.h>
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

int cust_getFlashModeStyle(int sensorType, int flashMode)
{
    XLOGD("cust_getFlashModeStyle");
	//LIB3A_FLASH_MODE_AUTO        =  0,
    //LIB3A_FLASH_MODE_SLOWSYNC    =  0, //NOW DO NOT SUPPORT SLOW SYNC, TEMPERALLY THE SAME WITH AUTO
    //LIB3A_FLASH_MODE_FORCE_ON    =  1,
    //LIB3A_FLASH_MODE_FORCE_OFF   =  2,
    //LIB3A_FLASH_MODE_REDEYE      =  3,
    //LIB3A_FLASH_MODE_FORCE_TORCH =  4,
    //flash style
    /*
  e_FLASH_STYLE_OFF_OFF =10, //Preview
  e_FLASH_STYLE_OFF_AUTO,
  e_FLASH_STYLE_OFF_ON,
  //e_FLASH_STYLE_OFF_TORCH,
  //e_FLASH_STYLE_ON_OFF,
  //e_FLASH_STYLE_ON_AUTO,
  e_FLASH_STYLE_ON_ON,
  e_FLASH_STYLE_ON_TORCH,
  */

   	if(sensorType==(int)DUAL_CAMERA_MAIN_SENSOR)
   	{
   		if(flashMode==LIB3A_FLASH_MODE_AUTO)
   		{
   			return e_FLASH_STYLE_OFF_AUTO;
   		}
   		else if(flashMode==LIB3A_FLASH_MODE_FORCE_ON)
   		{
   			return e_FLASH_STYLE_OFF_ON;
   		}
   		else if(flashMode==LIB3A_FLASH_MODE_FORCE_OFF)
   		{
   			return e_FLASH_STYLE_OFF_OFF;
   		}
   		else if(flashMode==LIB3A_FLASH_MODE_REDEYE)
   		{
   			return e_FLASH_STYLE_OFF_AUTO;
   		}
   		else if(flashMode==LIB3A_FLASH_MODE_FORCE_TORCH)
   		{
   			return e_FLASH_STYLE_ON_TORCH;
   		}

   	}
   	else
   	{
   		if(flashMode==LIB3A_FLASH_MODE_AUTO)
   		{
   			return e_FLASH_STYLE_OFF_AUTO;
   		}
   		else if(flashMode==LIB3A_FLASH_MODE_FORCE_ON)
   		{
   			return e_FLASH_STYLE_OFF_ON;
   		}
   		else if(flashMode==LIB3A_FLASH_MODE_FORCE_OFF)
   		{
   			return e_FLASH_STYLE_OFF_OFF;
   		}
   		else if(flashMode==LIB3A_FLASH_MODE_REDEYE)
   		{
   			return e_FLASH_STYLE_OFF_AUTO;
   		}
   		else if(flashMode==LIB3A_FLASH_MODE_FORCE_TORCH)
   		{
   			return e_FLASH_STYLE_ON_TORCH;
   		}
   	}
   	return e_FLASH_STYLE_OFF_AUTO;

}
