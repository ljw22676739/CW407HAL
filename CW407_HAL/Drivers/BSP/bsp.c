/****************** Copyright(c) 广州正虹科技发展有限公司 ***********************
* 
*------File Info--------------------------------------------------------------
* File Name: 			bsp.c
* Latest modified Date: 最终修改日期（YYYY-MM-DD）
* Latest Version:       最终修订版本号
* Description:          和硬件层相关的定义和函数
* compiler:             MDK v5.20
* MCU:                  STM32F407
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


/* Includes -----------------------------------------------------------------*/
#include "stdint.h"                    /* data type definitions and macro*/
#include "stm32f4xx_hal.h"
#include "main.h"
#include "bsp.h"

//包含相应的底层驱动,通过Config.h文件配置相应的功能


#if SPI_FLASH_EN
#include "bsp_spi_flash.c"
#endif

#if SD_CARD_EN
//#include "spi_sd_card_driver.c"
#endif

#if EEPROM_EN
#include "bsp_eeprom_24xx.c"
#endif

#if MODBUS_EN
#include "bsp_modbus.c"
#endif

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
#define COM_RS485	huart3
/* Private macro ------------------------------------------------------------*/
/* Private constants ------------------------------------------------------- */
/* Private variables --------------------------------------------------------*/

/* Private function prototypes ----------------------------------------------*/
/* Private functions --------------------------------------------------------*/

/* Forward Declaration of local functions -----------------------------------*/
/* Declaration of extern functions ------------------------------------------*/
/* Global variables ---------------------------------------------------------*/
extern const uint8_t BitMask[8] = {0x01,0x02,0x04,0x10,0x20,0x40,0x80};    //位操作掩码
extern const uint8_t NotBitMask[8] = {0xfe,0xfd,0xfb,0xf7,0xef,0xdf,0xbf,0x7f};//位操作掩码


/** @defgroup ETH_Private_Functions
  * @{
  */

/**
  * @brief  Delay some us
  * @param  cnt 1~65535
  * @retval None
  */
void bsp_DelayUS(uint16_t cnt)
{
    unsigned char i;
    while (cnt--)
    {
        i = 11;
        while (i--);
    }
}

/**
  * @brief  调试时打印调试信息
  * @param pString: 需要打印的字符串                   
  * @retval none                
  */
void DEBUG_PrintfString(char *pString)
{
//  comSendString(DEBUG_COM, pString);
    HAL_UART_Transmit(&huart1,(uint8_t*)pString,strlen(pString),0xffff);  //调试时可能会用到
//  HAL_UART_Transmit_IT(&huart1,pString,strlen(pString));  //调试时可能会用到
}

/**
  * @brief  调试时打印调试信息
  * @param  pBuf: 需要打印的数据 
  *         length 需要打印是数据长度 
  * @retval none                
  */
void DEBUG_PrintfBytes(char *pBuf, uint16_t length)
{
//  comSendBuf(DEBUG_COM, (uint8_t *)pBuf, length);
    HAL_UART_Transmit_IT(&huart1,pBuf,length);  //调试时可能会用到
}


/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/
/************** (C) COPYRIGHT 广州正虹科技发展有限公司 ********END OF FILE****/
