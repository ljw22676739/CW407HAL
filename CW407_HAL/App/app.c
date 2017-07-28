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
#include "stdint.h"                    /* data type definitions and macro*/
#include "stm32f4xx_hal.h"
#include "main.h"
#include "string.h"
#include "bsp.h"
#include "cmsis_os.h"
#include "ff.h"
#include "diskio.h"
#include "fatfs.h"
//#include "MyNet.h"
//#include "Task_ADC.h"
//#include "Task_GPRS.h"
//#include "spi_sd_card_driver.h"
//#include <time.h>
#include "ff.h"

#if LCD_EN
#include "lcd.h"
#endif

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/

/* Private macro ------------------------------------------------------------*/
/* Private constants ------------------------------------------------------- */
/* Private variables --------------------------------------------------------*/
/* Private function prototypes ----------------------------------------------*/
#if MODBUS_EN
static void Timer5Config(void);
#endif
/* Private functions --------------------------------------------------------*/

/* Forward Declaration of local functions -----------------------------------*/
/* Declaration of extern functions ------------------------------------------*/
/* Global variables ---------------------------------------------------------*/
#if SD_CARD_EN
FATFS   SdFatfs;                                                     /* 保存SD卡文件系统结构体信息 */
#endif

//TIM_HandleTypeDef htim7;
volatile uint32_t ulHighFrequencyTimerTicks;

uint8_t DataPackage[DATA_PACKAGE_SIZE];                              /* 用于存储一个HJT212数据包(目前以太网和GPRS共用,同一个任务中轮询处理,因此不会冲突) */

/* CMSIS OS tick */
static __IO uint32_t _uwTick;
/******** 信号量 *******/
osMutexId osMutexEthernet;                                           /* 以太网互斥信号量,暂时仅支持一个连接 */
osMutexId osMutexGprs;                                               /* GPRS接口互斥信号量 */
osMutexId osMutexUart1,osMutexUart2,osMutexUart3,osMutexUart4,osMutexUart5; /* 串口互斥信号量 */

RTC_TimeTypeDef TimeCurrent;                                         /* 保存当前时间 在秒任务中更新时间 */
RTC_DateTypeDef DateCurrent;                                         /* 保存当前日期 */

/* 时间变量 */
uint16_t TimeSecondCnt = 0;                                          /* 时间变量,秒计数器 */

/* 看门狗和任务监测的相关变量 */
uint8_t IwdgRefreshEn = 1;                                           /* 使能独立看门狗刷新功能 */

/** @defgroup Application_Functions
  * @{
  */

/**
  * @brief  Timer5Config function 
  *         不使用CubeMX生成的定时器初始化函数，采用此函数进行初始化
  * @param  None
  * @retval None
  */
#if MODBUS_EN
static void Timer5Config(void)
{
    htim5.Instance = TIM5;
    htim5.Init.Prescaler = HAL_RCC_GetHCLKFreq() / 1000000 - 1;      /* us*/
    htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim5.Init.Period = 5000 - 1;
    htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_Base_Init(&htim5) != HAL_OK)
    {
        Error_Handler();
    }

    /* 添加部分 */
    //HAL_TIM_GenerateEvent(&htim5,TIM_EVENTSOURCE_UPDATE);
    __HAL_TIM_CLEAR_IT(&htim5, TIM_IT_UPDATE);                       /* 关键部分 要想在中断中关闭定时器,必须在启动定时器前要先清除标志位,暂时不知道原因 */
    // HAL_TIM_Base_Start_IT(&htim5);
}
#endif

/**
  * @brief This function is called to increment  a global variable "uwTick"
  *        used as application time base.
  * @note In the default implementation, this variable is incremented each 1ms
  *       in Systick ISR.
  * @note 替代系统的函数,增加自己的内容
  * @retval None
  */
void HAL_IncTick(void)
{
    static uint8_t i = 0;
    _uwTick++;

    /* uip 10ms定时器 */
    if (++i >= 10)
    {
        i = 0;
        //      TimerCallback();
    }
}

/**
  * @brief Provides a tick value in millisecond.
  * @note 替代系统的函数,使用变量改为_uwTick
  * @retval tick value
  */
uint32_t HAL_GetTick(void)
{
    return _uwTick;
}

#if SD_CARD_EN
/**
  * @brief  SdCardInit function
  * @param  None
  * @retval 0:测试通过; 
  *         1:SD卡挂载失败; 
  */
void SdCardInit(void)
{
    FRESULT result;

    //  result = SD_Init();
    //  DEBUG_PrintfString("SD card init result is:%d\r\n", result);

    /* 挂载SD卡 */
    result = f_mount(&SdFatfs, (TCHAR const*)SD_Path, 1);
    if (result == RES_OK)
    {
        DEBUG_PrintfString("SD fatfs mount sucess.\r\n");
        CheckSDCardSpace();
    }
    else
    {
        DEBUG_PrintfString("SD fatfs mount fail.\r\n");
    }
}

/**
  * @brief  TestSdCard function
  * @param  None
  * @retval 0:测试通过; 
  *         1:SD卡挂载失败; 
  */
uint8_t TestSdCard(void)
{
    char result;
    FIL file;
    FATFS SdFatfs;
    uint32_t temp;
    char str_temp[200];                                              //注意堆栈溢出

    /* 挂载SD卡 */
    //  result = f_mount(&SdFatfs, (TCHAR const*)USER_Path, 1);
    //  if (result != RES_OK)
    //  {
    //      DEBUG_PrintfString("SD fatfs mount fail.\r\n");
    //      return 1;
    //  }
    //  DEBUG_PrintfString("SD fatfs mount sucess.\r\n");

    /* 尝试打开temp.txt文件,并写入abc,如果打开失败则新建一个temp.txt文件 */
    result = f_open(&file, "0:/temp.txt", FA_OPEN_EXISTING | FA_WRITE); //打开今天日期的文件

    if (result == RES_OK)
    {
        DEBUG_PrintfString("Open file\"temp.txt\" sucess.\r\n");

        /* 移动到文件最后 */
        temp = f_size(&file);
        f_lseek(&file, temp);

        result = f_write(&file, "abc1234567891551\r\n", 18, (UINT *)&temp);
        if (result == RES_OK)
        {
            sprintf(str_temp, "Write %d bytes data to file suess.\r\n", temp);
            f_sync(&file);
        }
        else
        {
            DEBUG_PrintfString("Write data to file fail.\r\n");
        }
        f_close(&file);
    }
    else
    {
        DEBUG_PrintfString("Open file\"temp.txt\" fail.\r\n");
        result = f_open(&file, "0:/temp.txt", FA_CREATE_ALWAYS | FA_WRITE); //打开今天日期的文件

        if (result == RES_OK)
        {
            DEBUG_PrintfString("Create file\"temp.txt\" sucess.\r\n");
            result = f_write(&file, "abc1234567891551\r\n", 18, (UINT *)&temp);
            if (result == RES_OK)
            {
                sprintf(str_temp, "Write %d bytes data to file suess.\r\n",temp);
                f_sync(&file);
            }
            else
            {
                DEBUG_PrintfString("Write data to file fail.\r\n");
            }
            f_close(&file);
        }
        else
        {
            DEBUG_PrintfString("Create file\"temp.txt\" fail.\r\n");
        }
    }

    /* 尝试打开text.txt文件,并把文件内容打印到串口上 */
    result = f_open(&file, "temp.txt", FA_OPEN_EXISTING | FA_READ);  //打开今天日期的文件

    if (result == RES_OK)
    {
        DEBUG_PrintfString("Open file\"temp.txt\" sucess.\r\n");

        while (f_gets(str_temp, 200, &file) != NULL)                 //获取一行数据
        {
            DEBUG_PrintfString(str_temp);
        }

        f_close(&file);
    }
    else
    {
        DEBUG_PrintfString("File\"temp.txt\" not exist.\r\n");
    }

    return 0;
}
#endif

/**
  * @brief  TestSpiFlash function
  * @param  None
  * @retval 0:测试通过; 
  *        1:SPI FLASH挂载失败 ;
  *        2:格式化失败;
*/
#if SPI_FLASH_EN
uint8_t TestSpiFlash(void)
{
    char result;
    FIL file;
    FATFS SfFatfs;
    uint32_t temp;
    char str_temp[200];

    /* 挂载SD卡 */
    result = f_mount(&SfFatfs, (TCHAR const*)USER_Path, 0);
    if (result == FR_NO_FILESYSTEM)
    {
        DEBUG_PrintfString("SPI-FLASH no file system.\r\nStart format.\r\n");
        result = f_mkfs((TCHAR const*)USER_Path, 0, 1);
        if (result != RES_OK)
        {
            DEBUG_PrintfString("SPI-FLASH disk format fail.\r\nReturn.\r\n");
            return 2;
        }
        DEBUG_PrintfString("SPI-FLASH disk format sucess.\r\n");

        return 1;
    }
    DEBUG_PrintfString("SPI-FLASH fatfs mount sucess.\r\n");

    /* 格式化磁盘 */


    /* 尝试打开temp.txt文件,并写入abc,如果打开失败则新建一个temp.txt文件 */
    result = f_open(&file, "1:/temp.txt", FA_OPEN_EXISTING | FA_WRITE); //打开今天日期的文件

    if (result == RES_OK)
    {
        DEBUG_PrintfString("Open file\"temp.txt\" sucess.\r\n");
        f_close(&file);
    }
    else
    {
        DEBUG_PrintfString("Open file\"temp.txt\" fail.\r\n");
        result = f_open(&file, "1:/temp.txt", FA_CREATE_ALWAYS | FA_WRITE); //打开今天日期的文件

        if (result == RES_OK)
        {
            DEBUG_PrintfString("Create file\"temp.txt\" sucess.\r\n");
            f_write(&file, "abc", 3, (UINT *)&temp);
            f_close(&file);
        }
        else
        {
            DEBUG_PrintfString("Create file\"temp.txt\" fail.\r\n");
        }
    }

    /* 尝试打开text.txt文件,并把文件内容打印到串口上 */
    result = f_open(&file, "1:/temp.txt", FA_OPEN_EXISTING | FA_READ); //打开今天日期的文件

    if (result == RES_OK)
    {
        DEBUG_PrintfString("Open file\"temp.txt\" sucess.\r\n");

        while (f_gets(str_temp, 200, &file) != NULL)                 //获取一行数据
        {
            DEBUG_PrintfString(str_temp);
        }

        f_close(&file);
    }
    else
    {
        DEBUG_PrintfString("File\"temp.txt\" not exist.\r\n");
    }

    return 0;
}
#endif //SPI_FLASH_EN

/**
  * @brief  TestEeprom function 
  *         目前仅仅测试驱动是否正常 
  * @param  None
  * @retval 0:测试通过; 
*/
#if EEPROM_EN
uint8_t TestEeprom(void)
{
    uint8_t i;
    uint8_t data;
    char str_tmp[5];
    char ucString[30] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30 };
    HAL_StatusTypeDef result;

    DEBUG_PrintfString("************ EEPROM test **************\r\n");

    /* 擦除 */
    for (i = 0, data = 0xff; i < 200; i++)
    {
        EEPROM_WriteData(i, &data, 1);
    }

    /* 打印EEPROM里面的内容 */
    DEBUG_PrintfString("EEPROM data:\r\n");
    for (i = 0; i < 200; i++)
    {
        EEPROM_ReadData(i, &data, 1);
        sprintf(str_tmp, "%d ", data);
        DEBUG_PrintfString(str_tmp);
    }
    DEBUG_PrintfString("\r\n");

    /* 写入测试 */
    result = EEPROM_WriteData(0, (uint8_t *)ucString, sizeof(ucString));

    sprintf(str_tmp, "result: %d", result);
    DEBUG_PrintfString(str_tmp);

    /* 打印EEPROM里面的内容 */
    DEBUG_PrintfString("EEPROM data:\r\n");
    for (i = 0; i < 200; i++)
    {
        EEPROM_ReadData(i, &data, 1);
        sprintf(str_tmp, "%d ", data);
        DEBUG_PrintfString(str_tmp);
    }
    DEBUG_PrintfString("\r\n");

    //  EEPROM_ReadData(0,ucString,sizeof(ucString));
    //  DEBUG_PrintfString("EEPROM data:\r\n");
    //  for (i=0;i<sizeof(ucString);i++)
    //  {
    //  	sprintf(str_tmp,"%d ",ucString[i]);
    //  	DEBUG_PrintfString(str_tmp);
    //  }
    //  DEBUG_PrintfString("\r\n");

    //  for (i=0;i<200;i++)
    //  {
    //      EEPROM_ReadData(i,&data,1);
    //  	if (data != i)
    //  	{
    //          DEBUG_PrintfString("EEPROM error.\r\n");
    //  	}
    //  }

    DEBUG_PrintfString("*********** EEPROM test finish ***********\r\n");

    return 0;
}
#endif //EEPROM_EN

//SDRAM内存测试
#if SDRAM_EN
void TestSdram(void)
{
    uint32_t i = 0;
    uint32_t address = 0;
    uint32_t *p;
    uint8_t result = 0;
    uint16_t *pusP = 0;
    uint8_t *pucP = 0;
    uint8_t str_tmp[50];

    DEBUG_PrintfString("Extern memory Test start.\r\n");

    /* 逐字测试 */
    DEBUG_PrintfString("32-bit accesse test...\r\n");
    DEBUG_PrintfString("Start write test data to sdram.\r\n");
    for (i = 0, p = (uint32_t *)SDRAM_DEVICE_ADDR; i < SDRAM_DEVICE_SIZE / 4; i++)
    {
        *p++ = i;
    }

    DEBUG_PrintfString("Start read test date to sdram.\r\n");

    for (i = 0, p = (uint32_t *)SDRAM_DEVICE_ADDR; i < SDRAM_DEVICE_SIZE / 4; i++)
    {
        if (*p != i)
        {
            sprintf(str_tmp, "Error:add[%d]=%08X;\r\n", i, *p);
            DEBUG_PrintfString(str_tmp);
            result = 1;
        }
        p++;
    }

    //  DEBUG_PrintfString("SDRAM test finish.\r\n");
    if (result == 0)
    {
        DEBUG_PrintfString("SDRAM 32bit test OK.\r\n");
    }
    else
    {
        DEBUG_PrintfString("SDRAM 32bit test fail.\r\n");
    }

    /* 按半字访问测试 */
    //擦除
    // for (i=0,p=(uint32_t *)SDRAM_DEVICE_ADDR; i<SDRAM_DEVICE_SIZE/4; i++)
    // {
    //    *p++ = 0xFFFFFFFF;
    // }
    DEBUG_PrintfString("16-bit accesse test...\r\n");
    DEBUG_PrintfString("Start write test data to sdram.\r\n");
    for (pusP = (uint16_t *)SDRAM_DEVICE_ADDR, address = SDRAM_DEVICE_ADDR; address < (SDRAM_DEVICE_ADDR + SDRAM_DEVICE_SIZE / 2); address += 0x10000)
    {
        for (i = 0; i <= 0xffff; i++)
        {
            *pusP++ = i;
        }
    }

    DEBUG_PrintfString("Start read test date to sdram.\r\n");

    for (pusP = (uint16_t *)SDRAM_DEVICE_ADDR, address = SDRAM_DEVICE_ADDR; address < (SDRAM_DEVICE_ADDR + SDRAM_DEVICE_SIZE / 2); address += 0x10000)
    {
        for (i = 0; i <= 0xffff; i++)
        {
            if (*pusP != i)
            {
                sprintf(str_tmp, "Error:add[%d]=%d;\r\n", i, *pusP);
                DEBUG_PrintfString(str_tmp);
                result = 1;
            }
            pusP++;
        }
    }

    //  DEBUG_PrintfString("SDRAM test finish.\r\n");
    if (result == 0)
    {
        DEBUG_PrintfString("SDRAM 16bit test OK.\r\n");
    }
    else
    {
        DEBUG_PrintfString("SDRAM 16bit test fail.\r\n");
    }


    /* 逐字节访问测试 */
    DEBUG_PrintfString("8-bit accesse test...\r\n");
    DEBUG_PrintfString("Start write test data to sdram.\r\n");
    for (pucP = (uint8_t *)SDRAM_DEVICE_ADDR, address = SDRAM_DEVICE_ADDR; address < (SDRAM_DEVICE_ADDR + SDRAM_DEVICE_SIZE); address += 0x100)
    {
        for (i = 0; i <= 0xff; i++)
        {
            *pucP++ = i;
        }
    }

    DEBUG_PrintfString("Start read test date to sdram.\r\n");

    //  for (result = 0, pucP = (uint8_t *)SDRAM_DEVICE_ADDR, address = SDRAM_DEVICE_ADDR; address < (SDRAM_DEVICE_ADDR + SDRAM_DEVICE_SIZE); address += 0x100)
    for (result = 0, pucP = (uint8_t *)SDRAM_DEVICE_ADDR, address = 0; address < 100; address++)
    {
        for (i = 0; i <= 0xff; i++)
        {
            if (*pucP != i)
            {
                printf("Error:add[%d]=%d;\r\n", address + i, *pucP);
                result = 1;
            }
            pucP++;
        }
    }

    //  DEBUG_PrintfString("SDRAM test finish.\r\n");
    if (result == 0)
    {
        DEBUG_PrintfString("SDRAM bytes access test OK.\r\n");
    }
    else
    {
        DEBUG_PrintfString("SDRAM bytes access test fail.\r\n");
    }

    /* 抽测，主要测试驱动是否正常以及测试容量 */
    // //每隔16K字节,写入一个数据,总共写入2048个数据,刚好是32M字节
    // for (i = 0; i < 32 * 1024 * 1024; i += 16 * 1024)
    // {
    //    *(volatile uint32_t *)(SDRAM_DEVICE_ADDR + i) = temp;
    //    temp++;
    // }
    // //依次读出之前写入的数据,进行校验
    // for (i = 0; i < 32 * 1024 * 1024; i += 16 * 1024)
    // {
    //    temp = *(volatile uint32_t *)(SDRAM_DEVICE_ADDR + i);
    //    if (i == 0) sval = temp;
    //    else if (temp <= sval) break; //后面读出的数据一定要比第一次读到的数据大.
    //    DEBUG_PrintfString("SDRAM Capacity:%dKB\r\n", (uint16_t)(temp - sval + 1) * 16); //打印SDRAM容量
    // }
}
#endif


/**
  * @brief  default task function
  * @param  None
  * @retval None
  */
void Task_Default(void const *argument)
{
    char dat;
    uint32_t error;

    /* USER CODE BEGIN 5 */
    DEBUG_PrintfString("System startup.\r\n");
    //  HAL_UART_Transmit(&huart1,"System startup.\r\n",17,0xffff);

#if RELEASE_EN
    FLASH_OBProgramInitTypeDef sOBInit;
    HAL_FLASHEx_OBGetConfig(&sOBInit);
    if (sOBInit.RDPLevel == OB_RDP_LEVEL_0)
    {
        DEBUG_PrintfString("Set read protected.\r\n");
        osDelay(100);
        sOBInit.RDPLevel = OB_RDP_LEVEL_1;
        sOBInit.OptionType = OPTIONBYTE_RDP;
        taskENTER_CRITICAL();
        HAL_FLASH_Unlock();
        HAL_FLASH_OB_Unlock();
        HAL_FLASHEx_OBProgram(&sOBInit);
        HAL_FLASH_OB_Lock();
        HAL_FLASH_Lock();
        taskEXIT_CRITICAL();

        DEBUG_PrintfString("Set read protected finish.\r\n");
        osDelay(100);
        __disable_fault_irq();                                       //禁止fault中断
        NVIC_SystemReset();                                          //重启MCU
    }
#endif

    /******************* 信号量初始化 ************************/
    //  osMutexEthernet = osMutexCreate(NULL);
    //  osMutexGprs = osMutexCreate(NULL);
    //  osMutexUart1 = osMutexCreate(NULL);
    //  osMutexUart2 = osMutexCreate(NULL);
    //  osMutexUart3 = osMutexCreate(NULL);
    //  osMutexUart4 = osMutexCreate(NULL);
    //  osMutexUart5 = osMutexCreate(NULL);

    //  LoadParam();                                                     /* 载入FLASH中的参数 */

    ///   static FLASH_EraseInitTypeDef EraseInitStruct;
    //
    //   __set_PRIMASK(1);       /* 关中断 */
    //
    //   /* Unlock the Flash to enable the flash control register access *************/
    //   HAL_FLASH_Unlock();
    //   /* Fill EraseInit structure*/
    //   EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
    //   EraseInitStruct.PageAddress = bsp_GetSector(PARAM_START_ADDR); //FLASH_USER_START_ADDR;
    //   EraseInitStruct.NbPages     = 1;
    //   HAL_FLASHEx_Erase(&EraseInitStruct, &error);
    //   HAL_FLASH_Lock();
    //
    //   __set_PRIMASK(0);       /* 开中断 */
    //   SetDefaultParam();
    //   SaveParam();

#if SD_CARD_EN
    //dat = SD_Init();
    //DEBUG_PrintfString("SD card init result is:%d\r\n", dat);
    SdCardInit();
    TestSdCard();
#endif

#if SPI_FLASH_EN
    TestSpiFlash();
#endif

#if EEPROM_EN
    TestEeprom();
#endif

#if SDRAM_EN
//  TestSdram();
#endif

#if ETHERNET_EN
    UIP_Config();
    osThreadDef(EthernetTask, Task_Ethernet, TASK_EHTERNET_PRIORITY, 0, TASK_ETHERNET_STACK_SIZE);
    osThreadCreate(osThread(EthernetTask), NULL);
#endif

#if LCD_EN
    /* 液晶屏初始化,其实只初始化液晶屏的串口的互斥变量,必须在ADC任务前初始化,因为该任务有使用的到液晶屏 */
    LCD_Init();
#endif

#if ADC_EN
    /* definition and creation of defaultTask */
    osThreadDef(adcTask, Task_ADC, TASK_ADC_PRIORITY, 0, TASK_ADC_STACK_SIZE);
    osThreadCreate(osThread(adcTask), NULL);
    //  xTaskCreate(Task_ADC,"Task adc",256,0,4,NULL);
#endif

    /* 初始化ModBus功能 */
#if MODBUS_EN
    MODBUS_InitVar(115200, WKM_MODBUS_DEVICE, 1);
    Timer5Config();                                                  /* 配置定时器,用于Modbus计时3.5T超时时间 */
#endif



#if GPRS_EN
    /* 创建GPRS任务 */
    osThreadDef(GprsTask, Task_GPRS, TASK_GPRS_PRIORITY, 0, TASK_GPRS_STACK_SIZE);
    if (osThreadCreate(osThread(GprsTask), NULL) == NULL)
    {
        DEBUG_PrintfString("Create GPRS task fail.\n");
    }
#endif

    /* 获取时间日期 */
    HAL_RTC_GetTime(&hrtc, &TimeCurrent, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &DateCurrent, RTC_FORMAT_BIN);

    /* Infinite loop */
    for (;;)
    {
        osDelay(100);

#if MODBUS_EN
        MODBUS_Poll();
#endif

#if ETHERNET_EN
        /* uip轮询处理函数 */
        uip_polling();
#endif

#if LCD_EN
        /* 处理液晶屏命令 */
        LCD_CmdHandle();
#endif

        HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);

        /* 分钟事件处理 */
        if (TimeSecondCnt >= 60)
        {
            TimeSecondCnt = 0;

            static uint8_t cnt1 = 0;
            if (++cnt1 >= 1)
            {
                cnt1 = 0;
                /* 打印任务信息,包括堆栈情况 */
                //              PrintTaskList();
            }
        }

    }
}

/**
  * @brief  PrintTaskList
  *         打印任务详情
  * @param  None
  * @retval None
  */
//void PrintTaskList(void)
//{
//    static char str_temp[500];
//    vTaskList(str_temp);
//    DEBUG_PrintfString("\r\n任务名\t\t状态\t优先级\t剩余栈\t任务序号\r\n");
//    DEBUG_PrintfString(str_temp);
//
//    vTaskGetRunTimeStats(str_temp);
//    DEBUG_PrintfString("\r\n任务名\t\t运行计数\t使用率\r\n");
//    DEBUG_PrintfString(str_temp);
//}

/**
  * @brief  Timer_OneSecondCallback
  *         1秒定时器调用的回调函数,此函数一般用于更新各个计时变量,目前由秒任务负责调用
  * @param  None
  * @retval None
  */
//void Timer_OneSecondCallback(void)
//{
//    TimeSecondCnt++;                                                 /* 秒计时单位加1 */
//}

/******************************************************************************
  Function:     CheckSDCardSpace
  Description:  检查SD卡空间，在剩余空间不足时，删掉最早的文件(按文件名)
  Input:        none
  Return:       none
  Others:       none
******************************************************************************/
void CheckSDCardSpace(void)
{
    FRESULT res;                                                     // FatFs function common result code--FAT
    FATFS *fs;
    DWORD fre_clust, fre_sect;
    DWORD tot_sect;
#if DEBUG_EN
    char str[200];
#endif

    /* Get volume information and free clusters of drive 1 */
    res = f_getfree("0:/", &fre_clust, &fs);
    if (res == FR_OK)
    {
        /* Get total sectors and free sectors */
        tot_sect = (fs->n_fatent - 2) * fs->csize;
        fre_sect = fre_clust * fs->csize;

#if DEBUG_EN
        /* Print the free space (assuming 512 bytes/sector) */
        sprintf(str, "%luKB total drive space.%luKB available\r\n",
                tot_sect / 2, fre_sect / 2);
        //LCD_Txt(0, 6, str);
        DEBUG_PrintfString(str);
#endif

        if (fre_sect < 20480)                                        //剩余空间不足20MB
        {
            //          DeleteFirstFile();
        }
    }
}
/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/
/************** (C) COPYRIGHT 广州正虹科技发展有限公司 ********END OF FILE****/
