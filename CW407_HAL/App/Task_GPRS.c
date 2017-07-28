/*
*********************************************************************************************************
*
*	模块名称 : 电阻式触摸板驱动模块
*	文件名称 : task_gprs.c
*	版    本 : V1.0
*	说    明 : MB_HVOC的gprs任务应用程序
*	修改记录 :
*	版本号   日期        作者    说明
*   v1.0    2016-02-23 李杰文  ST固件库V3.6.1版本。
*
*	Copyright (C), 2016-2026, 广州正虹科技 www.zhenghongkeji.com
*
*********************************************************************************************************
*/
#include "bsp.h"
#include "Task_ADC.h"
#include "ff.h"
#include "protocol.h"

/*
*********************************************************************************************************
*                               ADC相关定义
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                               全局变量
*********************************************************************************************************
*/
static uint8_t ucCnt1_t1s = 0;
static uint8_t ucCnt2_t1s = 0;
static uint8_t ucCnt3_t1s = 0;

/*
*********************************************************************************************************
*                               本地函数
*********************************************************************************************************
*/


#if SD_CARD_EN
/******************************************************************************
  Function:     Write_Data
  Description:  把采集数据写入SD卡CSV文件中的最后
  Input:        char *s 需要写入的字符串
  Output:       none
  Return:       0：成功；其他：出错
  Others:       以日期为文件名
******************************************************************************/
char Write_Data(char *s)
{
    char tmp[200];
    uint8_t i;
    FRESULT g_fres;
    UINT bw;
    FIL fs;

    sprintf(tmp, "%04d%02d%02d.csv", DateCurrent.Year + 2000, DateCurrent.Month, DateCurrent.Date);

    //记录到SD卡
    g_fres = f_open(&fs, tmp, FA_OPEN_EXISTING | FA_WRITE);
    if (g_fres != 0)
    {
        i = 2;
        do
        {
            g_fres = f_open(&fs, tmp, FA_WRITE | FA_CREATE_NEW);     //新建
            if (g_fres)
            {
                CheckSDCardSpace();                                  //新建失败，检查磁盘空间，看看是否需要删掉部分早期文件
            }
        }while (g_fres && i--);

        sprintf(tmp, "Date(YYYYMMDD),Time(HHMMSS),Channel1,Channel2,Chanel3,Channel4,Channel5,Channel6,Channel7,Channel8,Temperature,Humidity,AirPressure,Concentration\r\n");
        g_fres = f_write(&fs, tmp, strlen(tmp), &bw);
        if (g_fres)
        {
            return g_fres;                                           //写入出错，返回
        }
    }
    f_lseek(&fs, fs.fsize);                                          //把文件指针移动文件最后
    g_fres = f_write(&fs, s, strlen(s), &bw);
    f_sync(&fs);
    f_close(&fs);
    return g_fres;
}
#endif

/******************************************************************************
  Function:     SaveAdValue
  Description:  把ADC采样值保存在TF卡文件中
  Input:        none
  Return:       none
  Others:       none
******************************************************************************/
void SaveAdValue(void)
{
    char string_tmp[300] = { 0 };

    sprintf(string_tmp, "%04d%02d%02d,%02d%02d%02d,0.0,0.0,0.0,0.0,%d.%01d,%d.%01d,%d.%01d,%d.%01d\r\n",\
                2000 + DateCurrent.Year, DateCurrent.Month, DateCurrent.Date,\
                TimeCurrent.Hours, TimeCurrent.Minutes, TimeCurrent.Seconds,\
                AinValue[4] / 1000, AinValue[4] % 1000 / 100, AinValue[5] / 1000, AinValue[5] % 1000 / 100,\
                AinValue[6] / 1000, AinValue[6] % 1000 / 100, AinValue[7] / 1000, AinValue[7] % 1000 / 100);
#if SD_CARD_EN
    Write_Data(string_tmp);                                          //保存到CSV文件中
#endif
}

/**
  * @brief  Event_OneMinute function 
  *         分钟事件
  * @param  None
  * @retval None
  */
static void Event_OneMinute(void)
{
    static uint16_t save_rtd_cnt = 0;
    static uint16_t srv1_cnt = 0;
    static uint16_t srv2_cnt = 0;
    static uint16_t srv3_cnt = 0;
    static uint16_t srv4_cnt = 0;
    static __IO uint8_t last_hours = 0;                                   //保存上一次分钟数

    /* 获取时间日期 */
    HAL_RTC_GetTime(&hrtc, &TimeCurrent, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &DateCurrent, RTC_FORMAT_BIN);

    /* 保存AD值 */
    if (g_tParam.u16_SaveRtdInterval)
    {
        save_rtd_cnt++;
        if (save_rtd_cnt >= g_tParam.u16_SaveRtdInterval)
        {
            save_rtd_cnt = 0;
            SaveAdValue();                                           //保存Rtd值到TF卡
        }
    }

    /* 服务器1 */
    if (g_tParam.Server[0].rtd_interval && g_tParam.Server[0].port)
    {
        srv1_cnt++;
        if (srv1_cnt >= g_tParam.Server[0].rtd_interval)
        {
            srv1_cnt = 0;
            UpLoadData(0);                                           //上传数据到服务器1
        }
    }

    /* 服务器2 */
    if (g_tParam.Server[1].rtd_interval && g_tParam.Server[1].port)
    {
        srv2_cnt++;
        if (srv2_cnt >= g_tParam.Server[1].rtd_interval)
        {
            srv2_cnt = 0;
            UpLoadData(1);                                           //上传数据到服务器2
        }
    }

    /* 服务器3 */
    if (g_tParam.Server[2].rtd_interval && g_tParam.Server[2].port)
    {
        srv3_cnt++;
        if (srv3_cnt >= g_tParam.Server[2].rtd_interval)
        {
            srv3_cnt = 0;
            UpLoadData(2);                                           //上传数据到服务器3
        }
    }

    /* 服务器4 */
    if (g_tParam.Server[3].rtd_interval && g_tParam.Server[3].port)
    {
        srv4_cnt++;
        if (srv4_cnt >= g_tParam.Server[3].rtd_interval)
        {
            srv4_cnt = 0;
            UpLoadData(3);                                           //上传数据到服务器1
        }
    }

    QR_Update();                                                     /* 更新二维码 */

    if (last_hours != TimeCurrent.Hours)                             /* 1小时事件 */
    {
        last_hours = TimeCurrent.Hours;

        /* 每天0时0分重启系统,放在这里可避免启动时是0:00则不断重启 */
        if (TimeCurrent.Hours == 0 && TimeCurrent.Minutes == 0)
        {
#if DEBUG_EN
            DEBUG_PrintfString("Is time to reset system.\r\n");
            osDelay(100);
#endif
//            __set_FAULTMASK(1);                                          // 关闭所有中端
////          __disable_fault_irq();                                       //禁止fault中断
//            NVIC_SystemReset();                                          //重启MCU

            IwdgRefreshEn = 0;
        }

        CheckSDCardSpace();
    }
}

/*
*********************************************************************************************************
*	函 数 名: DTU_HeartbeartTmrCallback
*	功能说明: UCOS软件定时器DTU_HeartbeartTmr的回调函数
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void DTU_HeartbeartTmrCallback(void *ptmr, void *callback_arg)
{
    //  uint8_t i;
    //  uint8_t temp = g_ucConnectEnable;
    //
    //  for (i=0;i<8;i++)
    //  {
    //      if (temp & 0x01)
    //      {
    //          g_ucaHeartbeatCnt[i]++;
    //      }
    //      temp >>= 1;
    //  }
}

void UpLoadTestData(void)
{
    static uint32_t cnt = 0;
    char str_temp[30];
    sprintf(str_temp, "Test data: %010d\r\n", cnt++);                //Test data: 0000002975
    ME_TCPSendString(0, str_temp, 100);
}

/*
*********************************************************************************************************
*	函 数 名: GprsTimer
*	功能说明: GPRS定时器,周期一秒,目前由秒任务调用
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void GprsTimer(void)
{
    ucCnt1_t1s++;
    ucCnt2_t1s++;
    ucCnt3_t1s++;
    ME_HeartbeartTmrCallback();
}

/*
*********************************************************************************************************
*	函 数 名: Task_GPRS
*	功能说明: GPRS任务函数
*	形    参：无
*	返 回 值: 无
    优 先 级：5
*********************************************************************************************************
*/
void Task_GPRS(void const *argument)
{
    static uint8_t last_minutes = 0;
    uint32_t i; 

    //  static osTimerId gprs_timer = NULL;    //保存定时器句柄
    //
    //  gprs_timer = osTimerCreate(osTimerDef_t(GprsTimerCallback),osTimerPeriodic,0);
    //  if (gprs_timer != NULL)
    //  {
    //      osTimerStart(gprs_timer, 1000);
    //  }

#if SD_CARD_EN

    SdCardInit();
    //  TestSdCard();
#endif

    DTU_Init();

    while (1)
    {
        /* 连接的心跳包发送 */
        ME_Heartbeart();

        /* 断线重连的检查时间间隔 */
        if (ucCnt1_t1s > 10)
        {
            ucCnt1_t1s = 0;
            ME_CheckConnection();
            ME_Reconnection();
        }

        /* 网络状态检查的时间间隔 */
        if (ucCnt2_t1s > 30)
        {
            ucCnt2_t1s = 0;
            DTU_CheckState();                                        //检查DTU是否死机
            DisplayNetState();                                       /* 检测并显示网络情况和网络信号强度 */
        }

        /* 分钟事件 */
        if (ucCnt3_t1s > 4)
        {
            ucCnt3_t1s = 0;
            //          UpLoadTestData();                                        //测试时发送数据
        }

        if (last_minutes != TimeCurrent.Minutes)
        {
            last_minutes = TimeCurrent.Minutes;
            Event_OneMinute();
        }
#if GPRS_EN
        /* 处理GPRS模块信息 */
        DTU_HandleRecData();
#endif

        /* 处理重新初始化连接的请求 */
        if (GprsInitRequest)
        {
            GprsInitRequest = 0;
            for (i=0;i<4;i++)
            {
                ME_Disconnect(1,i);
            }
            osDelay(100);
            for (i=0;i<4;i++)
            {
                if (g_tParam.Server[i].port != 0)
                {
                    ME_ConnectSever(i);
                }
            }
        }
        osDelay(100);
    }
}



/*
*********************************************************************************************************
*	函 数 名: GPIO_Configuration
*	功能说明: 配置STM32 GPIO
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/


/* ************************* 广州正虹科技 www.zhenghongkeji.com/ (END OF FILE) ************************* */
