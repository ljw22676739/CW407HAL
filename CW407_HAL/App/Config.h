/****************** Copyright(c) 广州正虹科技发展有限公司 ***********************
* 
*------File Info--------------------------------------------------------------
* File Name: Config.h
* Latest modified Date: 最终修改日期（YYYY-MM-DD）
* Latest Version:       最终修订版本号
* Description:          配置本工程项目的功能0
* compiler:             MDK v5.21
* MCU:                  STM32F429
* Oscillator Crystal Frequency:    8MHz
*-----------------------------------------------------------------------------
* Created By:    文件的创建者姓名
* Created date:  文件的创建日期（YYYY-MM-DD）
* Version:       文件创建时的版本号
* Descriptions:  文件创建时的简要描述
*
*-----------------------------------------------------------------------------
* Modified by:   修改人的姓名
* Modified date: 文件的修改日期（YYYY-MM-DD）
* Version:       文件修订的版本号
* Description:   文件修订的简要描述
*
******************************************************************************/

/* Define to prevent recursive inclusion ------------------------------------ */
#ifndef __CONFIG_H__
#define __CONFIG_H__
/* Includes ----------------------------------------------------------------- */
//#include <stdint.h>
/* Private macros ----------------------------------------------------------- */
/***************************************************************************** 
* 注意设置使能相关功能仅仅是使能APP层的功能函数,不包括硬件初始化,硬件初始化放在HAL层,目前没独立出来
*****************************************************************************/

#define SD_CARD_EN                      1   /* 1:使能SDIO驱动TF卡 */
#define SPI_FLASH_EN                    0   /* 1:使能SPI FLASH disk 注意使能后要在CubeMX配置相关IO和打开SPI1(重映射) */
#define EEPROM_EN                       0   /* 1:使能EEPROM 注意使能后要在CubeMX配置相关IO和打开SPI1(重映射) */
#define ETHERNET_EN                     0   /* 1:使能以太网功能 */
#define ADC_EN                          0   /* 1:使能ADC采集功能 */
#define MODBUS_EN                       0   /* 1:使能Modbus功能 */
#define LCD_EN                          0   /* 1:使能串口屏功能 */
#define GPRS_EN                         0   /* 使能GPRS功能 */
#define SDRAM_EN                        1   /* 1:使能SRAM功能 */

#define RELEASE_EN                      0   /* 发布程序 */

/**
  * @brief  FMC SDRAM Bank address
  */   
#define SDRAM_DEVICE_ADDR         ((uint32_t)0x68000000)
#define SDRAM_DEVICE_SIZE         ((uint32_t)0x100000)  /* SRAM device size in MBytes */

/***************************************************************************** 
* RTOS任务配置
*****************************************************************************/
/* 任务栈大小配置(单位:Byte) */
#define TASK_DEFAULT_STACK_SIZE        128  /* 默认任务的任务栈大小 */
#define TASK_ADC_STACK_SIZE            1024  /* ADC任务栈大小 */
#define TASK_GPRS_STACK_SIZE           (1*1024)  /* GPRS任务栈大小 */
#define TASK_ETHERNET_STACK_SIZE       128  /* 以太网任务栈大小 */

/* 任务优先级配置 */
#define TASK_DEFAULT_PRIORITY        osPriorityBelowNormal  /* 默认任务的任务优先级 */
#define TASK_ADC_PRIORITY            osPriorityAboveNormal  /* ADC任务优先级 */
#define TASK_GPRS_PRIORITY                osPriorityNormal  /* GPRS任务优先级 */
#define TASK_EHTERNET_PRIORITY               osPriorityLow  /* 以太网任务优先级 */

//#define TASK_DEFAULT_PRIORITY        osPriorityNormal  /* 默认任务的任务优先级 */
//#define TASK_ADC_PRIORITY            osPriorityAboveNormal  /* ADC任务优先级 */
////#define TASK_GPRS_PRIORITY                osPriorityNormal  /* GPRS任务优先级 */
//#define TASK_EHTERNET_PRIORITY               osPriorityLow  /* 以太网任务优先级 */

/***************************************************************************** 
* ADC 配置项 8个模拟通道AIN1-AIN8
*****************************************************************************/

#define ADC_SAMPLE_PERIOD_MS           1    /* ADC采样周期,单位ms */
#define ADC_FIFO_SIZE                  50   /* ADC FIFO缓存区大小(ADC_FIFO_TRIM*2 < 范围 < 65535)*/
#define ADC_FIFO_TRIM                   5   /* 前后各修剪的数量 (0~1000太大没意义)*/

/*
模拟通道AIN1-AIN8量程类别设置
0:原始ADC值(0-4095); 1, 0~+10V; 2, -10V~+10V; 3, 0~5V; 4, -5V~+5V; 5, 0~+20mA; 6, -20mA~+20mA
*/
#define AIN1_RANGE						1	/* AIN1量程类别 */
#define AIN2_RANGE						1	/* AIN量程类别 */
#define AIN3_RANGE						1	/* AIN量程类别 */
#define AIN4_RANGE						1	/* AIN量程类别 */
#define AIN5_RANGE						5	/* AIN量程类别 */
#define AIN6_RANGE						5	/* AIN量程类别 */
#define AIN7_RANGE						5	/* AIN量程类别 */
#define AIN8_RANGE						5	/* AIN量程类别 */

/***************************************************************************** 
* 网络配置  
*****************************************************************************/
#define USE_DHCP                        1 /* enable DHCP, if disabled static address is used*/

/*Static IP ADDRESS*/
#define IP_ADDR0   192
#define IP_ADDR1   168
#define IP_ADDR2   0
#define IP_ADDR3   105

/*NETMASK*/
#define NETMASK_ADDR0   255
#define NETMASK_ADDR1   255
#define NETMASK_ADDR2   255
#define NETMASK_ADDR3   0

/*Gateway Address*/
#define GW_ADDR0   192
#define GW_ADDR1   168
#define GW_ADDR2   1
#define GW_ADDR3   1

#define DEST_IP_ADDR0   192
#define DEST_IP_ADDR1   168
#define DEST_IP_ADDR2   1
#define DEST_IP_ADDR3   100

#define DEST_PORT       6000

#define TCP_CLIENT_BUFFER_ZISE      2000    /* 客户端应用程序接收缓存区大小 */
#define TCP_CLIENT_NUM              2       /* 客户端数目,用于连接远程服务器 */
/***************************************************************************** 
* 配置DEBUG功能
*****************************************************************************/
#define DEBUG_COM        COM1   /* 调试串口 */
#define DEBUG_EN            1   /* 调试功能总开关 */
#define ADC_DEBUG_EN        0   /* 使能ADC调试功能 */
#define GPRS_DEBUG_EN       1   /* 使能GPRS模块调试功能 */

#if (DEBUG_EN==0)
#define ADC_DEBUG_EN        0   /* 禁止ADC调试功能 */
#define GPRS_DEBUG_EN       0   /* 禁止GPRS模块调试功能 */
#endif


#define guiVersion 2017020901   /* 定义版本号 */
/* Exported types and constants --------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
/* Exported constants ------------------------------------------------------- */
/* Exported macros ---------------------------------------------------------- */


/* Exported variables ------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */



#endif /*__CONFIG_H__*/

/************** (C) COPYRIGHT 广州正虹科技发展有限公司 ********END OF FILE****/
