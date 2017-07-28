/*
*********************************************************************************************************
*
*	模块名称 : 串行EEPROM 24xx驱动模块
*	文件名称 : bsp_eeprom_24xx.c
*	版    本 : V1.0
*	说    明 : 实现24xx系列EEPROM的读写操作。写操作采用页写模式提高写入效率。
*
*	修改记录 :
*		版本号  日期        作者     说明
*		V1.0    2013-02-01 armfly  正式发布
*
*	Copyright (C), 2013-2014, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

/*
    应用说明：访问串行EEPROM前，请先调用一次 bsp_InitI2C()函数配置好I2C相关的GPIO.
*/

#include "bsp.h"
#include "stm32f4xx_hal.h"

/*
*********************************************************************************************************
*	函 数 名: EEPROM_WriteData
*	功能说明: 往EERPOM写入数据
*	形    参:  无
*	返 回 值: 0 表示正常， other 表示不正常
*********************************************************************************************************
*/
HAL_StatusTypeDef EEPROM_WriteData(uint16_t _usMemAddress, uint8_t *_pucData, uint16_t _usSize)
{
   HAL_StatusTypeDef sta;

   while (_usSize--)
   {
      sta = HAL_I2C_Mem_Write(&hi2c1, EE_DEV_ADDR, _usMemAddress++, EE_ADD_SIZE, _pucData++, 1, 0xffff);
      if (sta != HAL_OK)
          return sta;
      osDelay(4);//不同器件延时可能不一样
   }
   return sta;
}

/*
*********************************************************************************************************
*	函 数 名: EEPROM_WriteData
*	功能说明: 往EERPOM写入数据
*	形    参:  无
*	返 回 值: 0 表示正常， other 表示不正常
*********************************************************************************************************
*/
HAL_StatusTypeDef EEPROM_ReadData(uint16_t _usMemAddress, uint8_t *_pucData, uint16_t _usSize)
{
   HAL_StatusTypeDef sta;
   while (_usSize--)
   {
      sta = HAL_I2C_Mem_Read(&hi2c1, EE_DEV_ADDR, _usMemAddress++, EE_ADD_SIZE, _pucData++, 1, 0xffffffff);
      if (sta != HAL_OK)
          return sta;
      osDelay(4);//不同器件延时可能不一样
   }
   return sta;
}



/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
