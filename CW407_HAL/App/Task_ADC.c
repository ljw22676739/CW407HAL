/****************** Copyright(c) 广州正虹科技发展有限公司 ***********************
* 
*------File Info--------------------------------------------------------------
* File Name: Task_ADC
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
#include "stdint.h"                    /* data type definitions and macro*/
#include "stm32f1xx_hal.h"
#include "string.h"
#include "bsp.h"
#include "cmsis_os.h"
#include "config.h"
#include "lcd.h"
#include "Task_GPRS.h"

#if ADC_EN

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
#define AIN_CHANNEL_NUM				8	/* 模拟通道数量 */
/* Private macro ------------------------------------------------------------*/
/* Private constants ------------------------------------------------------- */
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
static void Timer4Config(uint16_t _period);
/* Forward Declaration of local functions -----------------------------------*/
/* Declaration of extern functions ------------------------------------------*/
/* Global variables ---------------------------------------------------------*/
/* ADC变量 */
__IO uint16_t AdcValue[AIN_CHANNEL_NUM];                             /* ADC变量,DMA自动存入该变量 */
__IO uint16_t AdcValueFifo[AIN_CHANNEL_NUM][ADC_FIFO_SIZE];          /* ADC值FIFO缓冲区 */
__IO uint16_t AdcValueFifoCnt = 0;                                   /* 指向ADC值FIFO缓冲区位置 */
__IO int32_t AinValue[8];                                            /* 保存ADC通道采样、滤波、转换后的值,为实际值放大1000000倍,单位uV和uA*/

__IO uint8_t ChannelState[8];                                        /* 各通道开关量状态(通过阈值g_tParam.threshold判断) */

/* 模拟通道量程类别 */
const uint8_t AIN_Range[AIN_CHANNEL_NUM] =
{
    AIN1_RANGE, AIN2_RANGE, AIN3_RANGE, AIN4_RANGE, AIN5_RANGE, AIN6_RANGE, AIN7_RANGE, AIN8_RANGE
};


/** @defgroup Application_Functions
  * @{
  */
/**
  * @brief  Timer4Config function 
  *         不使用CubeMX生成的定时器初始化函数，采用此函数进行初始化
  * @param  _period 定时周期/ms(1-6000ms)
  * @retval None
  */
static void Timer4Config(uint16_t _period)
{
    htim4.Instance = TIM4;
    htim4.Init.Prescaler = HAL_RCC_GetHCLKFreq() / 10000 - 1;         /* 100us*/
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = _period*10 - 1;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
    {
        Error_Handler();
    }

    /* 添加部分 */
    //HAL_TIM_GenerateEvent(&htim5,TIM_EVENTSOURCE_UPDATE);
    __HAL_TIM_CLEAR_IT(&htim4, TIM_IT_UPDATE);                       /* 关键部分 要想在中断中关闭定时器,必须在启动定时器前要先清除标志位,暂时不知道原因 */
    HAL_TIM_Base_Start_IT(&htim4);
}

/**
  * @brief This function is called to increment  a global variable "uwTick"
  *        used as application time base.
  * @note In the default implementation, this variable is incremented each 1ms
  *       in Systick ISR.
  * @note 替代系统的函数,增加自己的内容
  * @retval None
  */
void Task_AdcInit(void)
{
    /* 配置在定时器4采样ADC值 */
    Timer4Config(ADC_SAMPLE_PERIOD_MS);
}

/**
  * @brief This function is called to sample adc vaule.
  * @note 在定时器4中断中调用此函数. 
  * @retval None
  */
void AdcSample(void)
{
    uint8_t i;
    static uint8_t cnt = 0;
    static uint16_t tmp[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };


    for (i = 4; i < 8; i++)
    {
        //取20ms周期内最大值
        if (AdcValue[i] > tmp[i])
        {
            tmp[i] = AdcValue[i];
        }
    }

    if (++cnt >= 20)
    {
        cnt = 0;

        /* 把ADC值存入FIFO中 */
        for (i = 4; i < AIN_CHANNEL_NUM; i++)
        {
//          AdcValueFifo[i][AdcValueFifoCnt] = AdcValue[i];
            AdcValueFifo[i][AdcValueFifoCnt] = tmp[i];               //把20ms周期内最大值存入
            tmp[i] = 0;
        }

        if (++AdcValueFifoCnt >= ADC_FIFO_SIZE)
        {
            AdcValueFifoCnt = 0;
        }
    }
}

/**
  * @brief  ADC值转换为量程值功能函数
  *         转换后的值为实际值放大1000000倍,单位uV和uA
  * @param  None
  * @retval None
  */
int32_t AdcConvert(uint8_t cmd, uint16_t val)
{
    int32_t rst;

    switch (cmd)
    {
    case 1:                                                          // 0~+10V 公式:Value/4095*2.5(基准电压)/运放放大系数(1/4)*1000000(放大为uV)
        rst = val * 2442;                                            //uV
        break;
    case 2:                                                          // -10V~+10V
        rst = 4882 * ((int16_t)val - 2048);
        break;
    case 3:                                                          // 0~5V
        rst = val * 1221;                                            //uV
        break;
    case 4:                                                          // -5V~+5V
        rst = 2441 * ((int16_t)val - 2048);
        break;
    case 5:                                                          // 0~+20mA
//      rst = 20000 * (uint32_t)val / 4095;
        rst = 14140 * (uint32_t)val / 4095;/* 交流有效值 */
        break;
    case 6:                                                          // -20mA~+20mA
        rst = 9765 * ((int16_t)val - 2048);
        break;
    default:
        rst = 0;
        break;
    }

    return rst;
}

/**
  * @brief  ADC sample task function 
  *         该任务每秒执行一次 
  * @param  None
  * @retval None
  */
void Task_ADC(void const *argument)
{
    static uint8_t i, j;
    static uint32_t xLastWakeTime;
    static uint32_t sum;
    static uint16_t array_temp[ADC_FIFO_SIZE];                       /* 排序临时存储区 */

#if ADC_DEBUG_EN
    static char str_tmp[200];
#endif

    /* Run the ADC calibration */
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        /* Calibration Error */
        Error_Handler();
    }

    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)AdcValue, 8);

    Task_AdcInit();

    /* 变量xLastWakeTime需要被初始化为当前心跳计数值。说明一下，这是该变量唯一一次被显式赋值。之后，
        xLastWakeTime将在函数vTaskDelayUntil()中自动更新。 */
    xLastWakeTime = xTaskGetTickCount();

#if LCD_EN
    /* LCD显示初始化 */
    LCD_SetScreen(0);                                                //切换到界面0
    LCD_Gif(0, 5, 0);                                                //停止风机运行动画
    LCD_Gif(0, 4, 0);                                                //停止净化器运行动画
#endif

    for (;;)
    {
        HAL_RTC_GetTime(&hrtc, &TimeCurrent, RTC_FORMAT_BIN);

        /************************ ADC部分 *************************/
        /** 目前仅测电流通道4-7,通道指示灯控制对应输出通道4-7 *       */
        for (i = 4; i < 8; i++)
        {
            /* 拷贝到临时存储区,并进行排序 */
            for (j = 0, sum = 0; j < ADC_FIFO_SIZE; j++)
            {
                array_temp[j] = AdcValueFifo[i][j];
            }
            bubble_sort_uint16_t(array_temp, ADC_FIFO_SIZE);

            /* 除去修剪部分,计算平均值 */
            for (j = ADC_FIFO_TRIM, sum = 0; j < (ADC_FIFO_SIZE - ADC_FIFO_TRIM);)
            {
                sum += array_temp[j++];
            }
            sum /= (ADC_FIFO_SIZE - (ADC_FIFO_TRIM << 1));

            AinValue[i] = AdcConvert(AIN_Range[i], sum);

            /* 禁止采集标志位如果为0,则采集数据为0 */
            if ((g_tParam.SystemFlag & (uint32_t)1) == 0)
            {
                 AinValue[i] = 0;
            }

            if (AinValue[i] > g_tParam.threshold)
            {
                ChannelState[i] = 1;

                /* 通道1作为GPRS联网指示灯 */
                if (i != 0)
                {
                    SetOutputBit(i, ON);
                }
            }
            else
            {
                ChannelState[i] = 0;

                /* 通道1作为GPRS联网指示灯 */
                if (i != 0)
                {
                    SetOutputBit(i, OFF);
                }
            }
        }
#if ADC_DEBUG_EN
        /* 打印ADC值 */
        sprintf(str_tmp, "Adc: %d  %d  %d  %d  %d  %d  %d  %d\r\n",
                AdcValue[0], AdcValue[1], AdcValue[2], AdcValue[3], AdcValue[4], AdcValue[5], AdcValue[6], AdcValue[7]);
        comSendString(COM1, str_tmp);

        sprintf(str_tmp, "AIN Value: %lduV  %lduV  %lduV  %lduV  %lduA  %lduA  %lduA  %lduA\r\n",
                AinValue[0], AinValue[1], AinValue[2], AinValue[3], AinValue[4], AinValue[5], AinValue[6], AinValue[7]);
        comSendString(COM1, str_tmp);
#endif

        /**************************** LCD显示部分 ********************************/
#if LCD_EN
        UpdateLcdDisplay();
#endif

        /* 延时1秒 */
        osDelayUntil(&xLastWakeTime, 1000);
        Timer_OneSecondCallback();                                   /* 秒计时单位更新 */
        GprsTimer();                                                 /* GPRS的定时器 */
        HAL_GPIO_TogglePin(LED1_GPIO, LED1_PIN);


        if (IwdgRefreshEn)
        {
            HAL_IWDG_Refresh(&hiwdg);                                    /* 喂独立看门狗 */
        }
    }
}
#endif

/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/
/************** (C) COPYRIGHT 广州正虹科技发展有限公司 ********END OF FILE****/
