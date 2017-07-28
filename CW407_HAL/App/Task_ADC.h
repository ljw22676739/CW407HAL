/****************** Copyright(c) 广州正虹科技发展有限公司 ***********************/

/* Define to prevent recursive inclusion ------------------------------------ */
#ifndef __TASK_ADC_H__
#define __TASK_ADC_H__
/* Includes ----------------------------------------------------------------- */
#include <stdint.h>
#include "stm32f1xx_hal.h"
/* Private macros ----------------------------------------------------------- */

/* Exported types and constants --------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
/* Exported constants ------------------------------------------------------- */
/* Exported macros ---------------------------------------------------------- */
typedef enum {AIN1,AIN2,AIN3,AIN4,AIN5,AIN6,AIN7,AINT8} AdcChannelTypeDef;




/* Exported variables ------------------------------------------------------- */
extern __IO int32_t  AinValue[];                                      /* 保存ADC通道采样、滤波、转换后的值,为实际值放大1000000倍,单位uV和uA*/
extern __IO uint8_t ChannelState[8];                                  /* 各通道开关量状态(通过阈值g_tParam.threshold判断) */

/* Exported functions ------------------------------------------------------- */
void Task_ADC(void const *argument);
void AdcSample(void);

#endif /*__TASK_ADC_H__*/

/************** (C) COPYRIGHT 广州正虹科技发展有限公司 ********END OF FILE****/
