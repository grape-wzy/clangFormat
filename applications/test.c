/*******************************************************************************
* file    kprintf.c
* author  mackgim
* version 1.0.0
* date    
* brief   Ö÷º¯Êý
*******************************************************************************/

#include "test.h"
#include "user_drivers.h"

static uint32_t SpeedStartFlag = 0;
static uint32_t TestSpeedStartTime = 0;
static uint32_t TestSpeedStopTime = 0;
__TESTSPEED_DATA_TypeDef TestSpeedBuff = {0};

static uint32_t TestSpeedError = 0;
static uint32_t TestSpeedLoss = 0;

static uint32_t TestSpeedPreviousID = 0;
static uint32_t TestSpeedCurrentID = 0;
static uint32_t TestSpeedReceivedFrame = 0;
__TESTSPEED_STATUS_TypeDef TestSpeedStatus = {0} ;


static uint8_t WhatTypeData = 0; //1 SPEED£¬ 2 AUDIO

//COM_FRAME PCFrame ;

void testspeed_ble_upload_start(void)
{
  
  if (WhatTypeData == 0)
  {
    WhatTypeData = 1;
  }
  else
  {
    return;
  }
  
  TestSpeedStartTime = Clock_Time();
  TestSpeedStopTime = 0;
  TestSpeedError = 0;
  TestSpeedLoss = 0;
  TestSpeedPreviousID = 0;
  TestSpeedCurrentID = 0;
  TestSpeedReceivedFrame = 0;
  memset((uint8_t *)&TestSpeedStatus,0,sizeof(TestSpeedStatus));
  memset((uint8_t *)&TestSpeedBuff,0,sizeof(TestSpeedBuff));
  SpeedStartFlag = 1;
  kprintf("[TestSpeed]:Start\r\n");
}
void testspeed_ble_upload_stop(void)
{
  uint32_t lapse;
  uint64_t datasize;
  
  if (WhatTypeData == 1)
  {
    WhatTypeData = 0;
  }
  else
  {
    return;
  }  
  
  TestSpeedStopTime = Clock_Time();
  lapse =  TestSpeedStopTime - TestSpeedStartTime;
  datasize = (uint64_t)TestSpeedReceivedFrame*sizeof(TestSpeedBuff)*8*1000;
  TestSpeedStatus.Speed = (uint32_t)((uint64_t)(datasize)/lapse);
  TestSpeedStatus.PackageLoss = TestSpeedLoss;
  TestSpeedStatus.PackageCurrentID = TestSpeedCurrentID;
  TestSpeedStatus.PackageError = TestSpeedError;
  kprintf("[TestSpeed]: Current ID = %"PRIu32", Frame amount = %"PRIu32" , datasize = %"PRIu32" Byte, Time = %"PRIu32"\r\n",TestSpeedBuff.ID,TestSpeedReceivedFrame,TestSpeedReceivedFrame * 20,lapse);
  kprintf("[TestSpeed]: Speed = %"PRIu32"kbps, Loss = %"PRIu32" package, ID = %"PRIu32" , ERROR = %"PRIu32"\r\n", TestSpeedStatus.Speed,TestSpeedStatus.PackageLoss,TestSpeedStatus.PackageCurrentID,TestSpeedStatus.PackageError);
  SpeedStartFlag = 0;
  kprintf("[TestSpeed]:Stop\r\n");
  
  TestSpeedStartTime = Clock_Time();
  TestSpeedStopTime = 0;
  TestSpeedError = 0;
  TestSpeedLoss = 0;
  TestSpeedPreviousID = 0;
  TestSpeedCurrentID = 0;
  TestSpeedReceivedFrame = 0;  
  memset((uint8_t *)&TestSpeedBuff,0,sizeof(TestSpeedBuff));
  
}
uint8_t testspeed_ble_upload_process(uint8_t *buff,uint16_t size)
{   
  
  if (WhatTypeData != 1)
  {
    return 0;
  }
  
  if (SpeedStartFlag == 1)
  {
    if (size == 20)
    {
      memcpy((uint8_t *)&TestSpeedBuff,buff,size);
      TestSpeedCurrentID = TestSpeedBuff.ID;
      if ((TestSpeedCurrentID - TestSpeedPreviousID) != 1)
      {
        TestSpeedLoss += (TestSpeedCurrentID - TestSpeedPreviousID);
        kprintf("[TestSpeed]: Loss - Current ID = %"PRIu32", Previous ID = %"PRIu32"\r\n",TestSpeedCurrentID,TestSpeedPreviousID);
      }
      if ((TestSpeedBuff.buff[0] != 0x12345678)||(TestSpeedBuff.buff[1] != 0x9abcdef0)||(TestSpeedBuff.buff[2] != 0x02468ace)||(TestSpeedBuff.buff[3] != 0x13579bdf))
      {
        TestSpeedError++;
        kprintf("[TestSpeed]: False - buff[0] = 0x%"PRIx32", buff[1] = 0x%"PRIx32", buff[2] = 0x%"PRIx32", buff[3] = 0x%"PRIx32",\r\n", TestSpeedBuff.buff[0],TestSpeedBuff.buff[1],TestSpeedBuff.buff[2],TestSpeedBuff.buff[3]);
      }
      TestSpeedPreviousID = TestSpeedCurrentID;
      TestSpeedReceivedFrame ++ ;
    }
    else
    {
      TestSpeedError++;
      kprintf("[TestSpeed]: False - size is not 20\r\n");
    }
    return 1;
  }
  else
  {
    return 0;
  }
}

uint8_t testspeed_ble_upload_readstatus(uint8_t *buff)
{
  
  memcpy(buff,(uint8_t *)&TestSpeedStatus,sizeof(__TESTSPEED_STATUS_TypeDef));
  return sizeof(__TESTSPEED_STATUS_TypeDef);
}














