/****************** Copyright(c) 广州正虹科技发展有限公司 ***********************/

/* Define to prevent recursive inclusion ------------------------------------ */
#ifndef __BSP_H__
#define __BSP_H__
/* Includes ----------------------------------------------------------------- */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

#include "config.h"

/* 通过取消注释或者添加注释的方式控制是否包含底层驱动模块 */
#include "main.h"
#include "app.h"
#include "bsp_uart_fifo.h"
#include "bsp_user_lib.h"
#include "bsp_port.h"
#include "bsp_cpu_flash.h"
#include "param.h"

#if SPI_FLASH_EN
#include "bsp_spi_flash.h"
#endif

#if EEPROM_EN
#include "bsp_eeprom_24xx.h"
#endif

#if LCD_EN
#include "lcd.h"
#endif

#if MODBUS_EN
#include "bsp_modbus.h"
#endif

#if GPRS_EN
#include "gprs.h"
#endif

#if SD_CARD_EN

#endif

/* Private macros ----------------------------------------------------------- */

/* Exported types and constants --------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
/* Exported constants ------------------------------------------------------- */
extern const uint8_t BitMask[8];    //位操作掩码
extern const uint8_t NotBitMask[8];//位操作掩码
/* Exported macros ---------------------------------------------------------- */

//位带操作,实现51类似的GPIO控制功能
//具体实现思想,参考<<CM3权威指南>>第五章(87页~92页).
//IO口操作宏定义
#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2)) 
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr)) 
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum)) 

 
//IO口操作,只对单一的IO口!
//确保n的值小于16!
//#define PAout(n)   BIT_ADDR(GPIOA_ODR_Addr,n)  //输出 

#define  USE_FreeRTOS      1

#if USE_FreeRTOS == 1
	#include "FreeRTOS.h"
	#include "task.h"
	#define DISABLE_INT()    taskENTER_CRITICAL()
	#define ENABLE_INT()     taskEXIT_CRITICAL()
#else
	/* 开关全局中断的宏 */
	#define ENABLE_INT()	__set_PRIMASK(0)	/* 使能全局中断 */
	#define DISABLE_INT()	__set_PRIMASK(1)	/* 禁止全局中断 */
#endif

/* Exported variables ------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */
void bsp_DelayUS(uint16_t cnt);
void DEBUG_PrintfString(char *pString);
void DEBUG_PrintfBytes(char *pBuf, uint16_t length);


#endif /*__BSP_H__*/

/************** (C) COPYRIGHT 广州正虹科技发展有限公司 ********END OF FILE****/
