/*
*********************************************************************************************************
*
*	模块名称 : cpu内部falsh操作模块(for F4)
*	文件名称 : bsp_cpu_flash.c
*	版    本 : V1.0
*	说    明 : 提供读写CPU内部Flash的函数
*	修改记录 :
*		版本号  日期        作者     说明
*		V1.0    2013-02-01 armfly  正式发布
*
*	Copyright (C), 2013-2014, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#include "bsp.h"

//#define SECTOR_MASK			0xFFFFF800  /* 大容量芯片2kB/页的掩码 */
#define SECTOR_MASK			0xFFFFFC00  /* 中小容量芯片1kB/页的掩码 */

/*Variable used for Erase procedure*/
static FLASH_EraseInitTypeDef EraseInitStruct;
/*
*********************************************************************************************************
*	函 数 名: bsp_GetSector
*	功能说明: 根据地址计算扇区首地址
*	形    参：无
*	返 回 值: 扇区首地址
*********************************************************************************************************
*/
uint32_t bsp_GetSector(uint32_t _ulWrAddr)
{
   uint32_t sector = 0;

   sector = _ulWrAddr & SECTOR_MASK;

   return sector;
}

/*
*********************************************************************************************************
*	函 数 名: bsp_ReadCpuFlash
*	功能说明: 读取CPU Flash的内容
*	形    参：_ucpDst : 目标缓冲区
*			 _ulFlashAddr : 起始地址
*			 _ulSize : 数据大小（单位是字节）
*	返 回 值: 0=成功，1=失败
*********************************************************************************************************
*/
uint8_t bsp_ReadCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpDst, uint32_t _ulSize)
{
   uint32_t i;

   /* 如果偏移地址超过芯片容量，则不改写输出缓冲区 */
   if (_ulFlashAddr + _ulSize > FLASH_BASE_ADDR + FLASH_SIZE)
   {
      return 1;
   }

   /* 长度为0时不继续操作,否则起始地址为奇地址会出错 */
   if (_ulSize == 0)
   {
      return 1;
   }

   for (i = 0; i < _ulSize; i++)
   {
      *_ucpDst++ = *(uint8_t *)_ulFlashAddr++;
   }

   return 0;
}

/*
*********************************************************************************************************
*	函 数 名: bsp_CmpCpuFlash
*	功能说明: 比较Flash指定地址的数据.
*	形    参: _ulFlashAddr : Flash地址
*			 _ucpBuf : 数据缓冲区
*			 _ulSize : 数据大小（单位是字节）
*	返 回 值:
*			FLASH_IS_EQU		0   Flash内容和待写入的数据相等，不需要擦除和写操作
*			FLASH_REQ_WRITE		1	Flash不需要擦除，直接写
*			FLASH_REQ_ERASE		2	Flash需要先擦除,再写
*			FLASH_PARAM_ERR		3	函数参数错误
*********************************************************************************************************
*/
uint8_t bsp_CmpCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpBuf, uint32_t _ulSize)
{
   uint32_t i;
   uint8_t ucIsEqu;    /* 相等标志 */
   uint8_t ucByte;

   /* 如果偏移地址超过芯片容量，则不改写输出缓冲区 */
   if (_ulFlashAddr + _ulSize > FLASH_BASE_ADDR + FLASH_SIZE)
   {
      return FLASH_PARAM_ERR;     /*　函数参数错误　*/
   }

   /* 长度为0时返回正确 */
   if (_ulSize == 0)
   {
      return FLASH_IS_EQU;        /* Flash内容和待写入的数据相等 */
   }

   ucIsEqu = 1;            /* 先假设所有字节和待写入的数据相等，如果遇到任何一个不相等，则设置为 0 */
   for (i = 0; i < _ulSize; i++)
   {
      ucByte = *(uint8_t *)_ulFlashAddr;

      if (ucByte != *_ucpBuf)
      {
         if (ucByte != 0xFF)
         {
            return FLASH_REQ_ERASE;     /* 需要擦除后再写 */
         }
         else
         {
            ucIsEqu = 0;    /* 不相等，需要写 */
         }
      }

      _ulFlashAddr++;
      _ucpBuf++;
   }

   if (ucIsEqu == 1)
   {
      return FLASH_IS_EQU;    /* Flash内容和待写入的数据相等，不需要擦除和写操作 */
   }
   else
   {
      return FLASH_REQ_WRITE; /* Flash不需要擦除，直接写 */
   }
}

/*
*********************************************************************************************************
*	函 数 名: bsp_WriteCpuFlash
*	功能说明: 写参数数据到CPU 内部Flash,自动把参数区前后防错的标志写入FLASH。
*	形    参: _ulFlashAddr : Flash地址,尽量使用扇区的首地址
*			 _ucpSrc : 数据缓冲区
*			 _ulSize : 数据大小（单位是字节）
*	返 回 值: 0-成功，1-数据长度或地址溢出，2-写Flash出错(估计Flash寿命到,或者其他写入出错)
*********************************************************************************************************
*/
uint8_t bsp_WriteCpuFlash(uint32_t _ulFlashAddr, uint8_t *_ucpSrc, uint32_t _ulSize)
{
   uint32_t i;
   uint8_t ucRet;
   uint16_t usTemp;
   uint32_t ulFlashAddrBackup = _ulFlashAddr;
   uint32_t PAGEError = 0;

   /* 如果偏移地址超过芯片容量，则不改写输出缓冲区 */
   if (_ulFlashAddr + _ulSize > FLASH_BASE_ADDR + FLASH_SIZE)
   {
      return 1;
   }

   /* 长度为0 时不继续操作  */
   if (_ulSize == 0)
   {
      return 0;
   }

   /* 长度为奇数时不继续操作,保证半字对齐,因为是按照半字方式写入 */
   if ((_ulSize % 2) != 0)
   {
      return 1;
   }

   ucRet = bsp_CmpCpuFlash(_ulFlashAddr + 4, _ucpSrc, _ulSize);

   if (ucRet == FLASH_IS_EQU)
   {
      return 0;
   }

   __set_PRIMASK(1);       /* 关中断 */

   /* Unlock the Flash to enable the flash control register access *************/
   HAL_FLASH_Unlock();

   /* 需要擦除 */
// if (ucRet == FLASH_REQ_ERASE)
   {
      /* Fill EraseInit structure*/
      EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
      EraseInitStruct.PageAddress = bsp_GetSector(_ulFlashAddr); //FLASH_USER_START_ADDR;
      i= _ulFlashAddr - EraseInitStruct.PageAddress + _ulSize;
      EraseInitStruct.NbPages = i / FLASH_PAGE_SIZE;
      if (i%FLASH_PAGE_SIZE)
      {
          EraseInitStruct.NbPages++;
      }

      if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
      {
         goto PROGRAM_ERROR;
      }
   }

   /* 写入起始标志 */
   if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, _ulFlashAddr, FLASH_DATA_BEGIN_FLAG) != HAL_OK)
   {
      goto PROGRAM_ERROR;
   }
   _ulFlashAddr += 4;

   /* 按半字模式编程（为提高效率，可以按字编程，一次写入4字节） */
   for (i = 0; i < _ulSize / 2; i++)
   {
      usTemp = _ucpSrc[2 * i];
      usTemp |= (_ucpSrc[2 * i + 1] << 8);
      if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, _ulFlashAddr, usTemp) != HAL_OK)
      {
         goto PROGRAM_ERROR;
      }

      _ulFlashAddr += 2;
   }

   /* 校验 */
   if (*(uint32_t *)ulFlashAddrBackup != FLASH_DATA_BEGIN_FLAG)
   {
      goto PROGRAM_ERROR;
   }

   ucRet = bsp_CmpCpuFlash(ulFlashAddrBackup + 4, _ucpSrc, _ulSize);

   if (ucRet != FLASH_IS_EQU)
   {
      goto PROGRAM_ERROR;
   }

   /* 写入结束标志 */
   if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, _ulFlashAddr, FLASH_DATA_END_FLAG) != HAL_OK)
   {
      goto PROGRAM_ERROR;
   }

   /* Flash 加锁，禁止写Flash控制寄存器 */
   /* Lock the Flash to disable the flash control register access (recommended
    to protect the FLASH memory against possible unwanted operation) *********/
   HAL_FLASH_Lock();

   __set_PRIMASK(0);       /* 开中断 */


   return 0;
PROGRAM_ERROR:
   /* Flash 加锁，禁止写Flash控制寄存器 */
   /* Lock the Flash to disable the flash control register access (recommended
      to protect the FLASH memory against possible unwanted operation) *********/
   HAL_FLASH_Lock();
   __set_PRIMASK(0);       /* 开中断 */
   return 2;
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
