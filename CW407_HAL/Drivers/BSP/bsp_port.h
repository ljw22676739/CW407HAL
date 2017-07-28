/****************** Copyright(c) 广州正虹科技发展有限公司 ***********************/

/* Define to prevent recursive inclusion ------------------------------------ */
#ifndef __BSP_PORT_H__
#define __BSP_PORT_H__
/* Includes ----------------------------------------------------------------- */
#include "bsp_modbus.h"
#include <stdint.h>
/* Private macros ----------------------------------------------------------- */
/*******************************************************************************
* 板上IO口定义 
*******************************************************************************/ 
/* 运行灯 */
#define LED1_GPIO   GPIOB
#define LED1_PIN    GPIO_PIN_2

/* GPRS电源控制脚PWRON */
#define GPRS_PWRON_GPIO   GPIOD
#define GPRS_PWRON_PIN    GPIO_PIN_11
/* 开关量输入端 */
#define DIN1_GPIO   GPIOD
#define DIN1_PIN    GPIO_PIN_14
#define DIN2_GPIO   GPIOD
#define DIN2_PIN    GPIO_PIN_15
#define DIN3_GPIO   GPIOB
#define DIN3_PIN    GPIO_PIN_14
#define DIN4_GPIO   GPIOB
#define DIN4_PIN    GPIO_PIN_15
#define DIN5_GPIO   GPIOD
#define DIN5_PIN    GPIO_PIN_7
#define DIN6_GPIO   GPIOE
#define DIN6_PIN    GPIO_PIN_4
#define DIN7_GPIO   GPIOC
#define DIN7_PIN    GPIO_PIN_8
#define DIN8_GPIO   GPIOC
#define DIN8_PIN    GPIO_PIN_9

/* 开关量输出端 */
#define DOUT1_GPIO  GPIOE
#define DOUT1_PIN   GPIO_PIN_8
#define DOUT2_GPIO  GPIOE
#define DOUT2_PIN   GPIO_PIN_9
#define DOUT3_GPIO  GPIOE
#define DOUT3_PIN   GPIO_PIN_10
#define DOUT4_GPIO  GPIOE
#define DOUT4_PIN   GPIO_PIN_11
#define DOUT5_GPIO  GPIOE
#define DOUT5_PIN   GPIO_PIN_12
#define DOUT6_GPIO  GPIOE
#define DOUT6_PIN   GPIO_PIN_13
#define DOUT7_GPIO  GPIOE
#define DOUT7_PIN   GPIO_PIN_14
#define DOUT8_GPIO  GPIOE
#define DOUT8_PIN   GPIO_PIN_15
/* Exported types and constants --------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
/* Exported constants ------------------------------------------------------- */
/* Exported macros ---------------------------------------------------------- */

/* Exported variables ------------------------------------------------------- */
/* Exported functions ------------------------------------------------------- */
void SetOutputBit(uint8_t _channel, IO_SateTypedef _state);
void SetOutput(uint32_t _state);
uint32_t ReadOutput(void);
uint32_t ReadInput(void);
IO_SateTypedef ReadDI(uint16_t _index);

#endif /*__BSP_PORT_H__*/

/************** (C) COPYRIGHT 广州正虹科技发展有限公司 ********END OF FILE****/
