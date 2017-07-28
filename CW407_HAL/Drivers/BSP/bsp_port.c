/****************** Copyright(c) 广州正虹科技发展有限公司 ***********************
* 
*------File Info--------------------------------------------------------------
* File Name: 文件名
* Latest modified Date: 最终修改日期（YYYY-MM-DD）
* Latest Version:       最终修订版本号
* Description:          文件的简要描述信息
* compiler:             MDK v4.74
* MCU:                  STM32F103VE
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
#include <stdint.h>                    /* data type definitions and macro*/
#include "bsp.h"
/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private constants ------------------------------------------------------- */
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/


/* Private functions --------------------------------------------------------*/

/* Declaration of extern functions ------------------------------------------*/
/* Global variables ---------------------------------------------------------*/



/** @defgroup bsp_port_functions
  * @{
  */

void LED1_On(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
}

void LED1_Off(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
}




/************** (C) COPYRIGHT 广州正虹科技发展有限公司 ********END OF FILE****/
