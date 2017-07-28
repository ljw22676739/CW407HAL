/******************* Copyright(c) 广州正虹科技发展有限公司 *******************
* 
*------File Info--------------------------------------------------------------
* File Name:            LCD.c
* Latest modified Date: 
* Latest Version:       
* Description:          串口液晶显示屏DC80480B070_03相关函数
* compiler:             MDK V4.73
* MCU:                  STM32F103VE
* Oscillator Crystal Frequency:    8MHz
*-----------------------------------------------------------------------------
* Created By:    李杰文
* Created date:  2015-08-26
* Version:       v0.1
* Descriptions:  
*
*-----------------------------------------------------------------------------
* Modified by:   修改人的姓名
* Modified date: 文件的修改日期（YYYY-MM-DD）
* Version:       文件修订的版本号
* Description:   文件修订的简要描述
*
******************************************************************************/

/* Includes -----------------------------------------------------------------*/
#include "bsp.h"
#include "app.h"
#include "Task_ADC.h"
#include <stdlib.h>

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
#define LCD_SET_PSW (123456)                      //串口显示屏进入设置界面的密码(不能超过9个数字)
/* Private macro ------------------------------------------------------------*/
const uint8_t LCD_CMD_BUF_MAX = 100;
/* Private variables --------------------------------------------------------*/
static char Lcd_CmdBuf[LCD_CMD_BUF_MAX];        //串口显示屏命令保存区
static uint8_t Lcd_CmdBufCnt = 0;               //串口显示屏命令保存区的数据数量
volatile uint8_t LCD_ScreenId = 0;              //串口显示屏当前画面ID,用于更新显示
uint8_t fan_last_state = 0;                     //风机上一次的开关状态
uint8_t clean_last_state = 0;                   //净化器上一次的开关状态
static uint8_t Lcd_ScreenSwitch = 0;            //界面切换标志位 0:界面没切换; 1:界面已切换


static osMutexId osMutexLcd;            /* LCD接口互斥信号量 */

/* Private function prototypes ----------------------------------------------*/
static void LCD_SendBuf(uint8_t *_ucaBuf, uint16_t _usLen);
/* Declaration of extern functions ------------------------------------------*/
/* Global variables ---------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                          Private functions                                */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                            function code                                  */
/*---------------------------------------------------------------------------*/

/******************************************************************************
  Function:     LCD_Icon
  Description:  设置LCD图标状态
  Input:        uint8_t screen_id,  屏幕ID号0~255
                uint8_t icon_id,    图标ID号0~255
                uint8_t state       指定状态0~255
  Return:       none
  Others:       none
******************************************************************************/
void LCD_Icon(uint8_t screen_id, uint8_t icon_id, uint8_t state)
{
    char buf[12] = { 0xEE, 0xB1, 0x23, 0, 0, 0, 0, 0, 0xFF, 0xFC, 0xFF, 0xFF };
    buf[4] = screen_id;
    buf[6] = icon_id;
    buf[7] = state;
    LCD_SendBuf((uint8_t*)buf, 12);
}

/******************************************************************************
  Function:     LCD_Gif
  Description:  LCD动画控件控制,目前仅支持控制动画播放和停止
  Input:        uint8_t screen_id,  屏幕ID号0~255
                uint8_t control_id, 控件ID号0~255
                uint8_t state       动画状态0:停止; 1:播放
  Return:       none
  Others:       none
******************************************************************************/
void LCD_Gif(uint8_t screen_id, uint8_t control_id, uint8_t state)
{
    char buf[11] = { 0xEE, 0xB1, 0x21, 0, 0, 0, 0, 0xFF, 0xFC, 0xFF, 0xFF };
    if (state == 0)
    {
        buf[2] = 0x21;
    }
    else
    {
        buf[2] = 0x20;
    }

    buf[4] = screen_id;
    buf[6] = control_id;

    LCD_SendBuf((uint8_t *)buf, 11);
}

/******************************************************************************
  Function:     LCD_Txt
  Description:  更新文本控件显示内容
  Input:        uint8_t screen_id,  屏幕ID号0~255
                uint8_t control_id, 控件ID号0~255
                uint8_t str         显示字符串
  Return:       none
  Others:       none
******************************************************************************/
void LCD_Txt(uint8_t screen_id, uint8_t icon_id, char *str)
{
//  char buf[12] = { 0xEE, 0xB1, 0x10, 0, 0, 0, 0 };//应该不需要12那么长
    char buf[7] = { 0xEE, 0xB1, 0x10, 0, 0, 0, 0 };
    const char buf1[] = { 0xFF, 0xFC, 0xFF, 0xFF };
    buf[4] = screen_id;
    buf[6] = icon_id;
  LCD_SendBuf( (uint8_t*)buf, 7);
  LCD_SendBuf((uint8_t*)str,strlen(str));
  LCD_SendBuf( (uint8_t*)buf1, 4);

//    osMutexWait(osMutexLcd, 60000);
//    comSendBuf(LCD_COM, buf, 7);
//    comSendString(LCD_COM, str);
//    comSendBuf(LCD_COM, (uint8_t *)buf1, 4);
//    osMutexRelease(osMutexLcd);
}

/******************************************************************************
  Function:     LCD_SetScreen
  Description:  设置串口显示屏切换到所需的画面
  Input:        none
  Return:       none
  Others:       none
******************************************************************************/
void LCD_SetScreen(uint8_t screen_id)
{
    char buf[9] = { 0xEE, 0xB1, 0x00, 0, 0, 0xFF, 0xFC, 0xFF, 0xFF };

    buf[4] = screen_id;
    LCD_ScreenId = screen_id;
    Lcd_ScreenSwitch = 1;                   //界面切换,置位标志位

    LCD_SendBuf((uint8_t*)buf, 9);
}

/******************************************************************************
  Function:     UpdateLcdDisplay
  Description:  更新串口显示屏的显示内容
  Input:        none
  Return:       none
  Others:       none
******************************************************************************/
void UpdateLcdDisplay(void)
{
    static uint8_t fan_last_state   = 0;                    //风机上一次的开关状态
    static uint8_t clean_last_state = 0;                    //净化器上一次的开关状态
    uint8_t i;
    char string_tmp[20];

    switch (LCD_ScreenId)
    {
    case 0: //更新串口显示屏相应画面的显示信息
        UpdateLcdTime();    //更新显示屏的时钟
        if (fan_last_state != ChannelState[g_tParam.FanChannel])
        {
            fan_last_state = ChannelState[g_tParam.FanChannel];
            if (ChannelState[g_tParam.FanChannel] == 1)
            {
                LCD_Icon(0, 7, 1);                             //显示风机运行图标
                LCD_Gif(0, 5, 1);                              //播放风机运行动画
            }
            else
            {
                LCD_Icon(0, 7, 0);                             //显示风机停止运行图标
                LCD_Gif(0, 5, 0);                              //停止风机运行动画
            }
        }

        if (clean_last_state != ChannelState[g_tParam.CleanChannel])
        {
            clean_last_state = ChannelState[g_tParam.CleanChannel];
            if (ChannelState[g_tParam.CleanChannel] == 1)
            {
                LCD_Icon(0, 8, 1);                             //显示净化器运行图标
                LCD_Gif(0, 4, 1);                              //播放净化器运行动画
            }
            else
            {
                LCD_Icon(0, 8, 0);                             //显示净化器停止运行图标
                LCD_Gif(0, 4, 0);                              //停止净化器运行动画
            }
        }
        break;
    case 2:
#if ADC_EN
        /* 前4个通道为检测电压,显示为0 */
        for (i = 0; i < 4; i++)
        {
            LCD_Txt(2, i + 3, "0.0");
        }

        for (i = 4; i < 8; i++)
        {
//          sprintf(string_tmp, "%d.%03d", AinValue[i] / 1000, AinValue[i] % 1000);//保留小数点3位,mA
            sprintf(string_tmp, "%d.%01d", AinValue[i] / 1000, AinValue[i] % 1000/100);//保留小数点1位
            LCD_Txt(2, i + 3, string_tmp);
        }
#endif
        break;
    case 3:
        if (Lcd_ScreenSwitch)
        {
            Lcd_ScreenSwitch = 0;

            LCD_Txt(3, 2, g_tParam.DevNO); //显示设备号
            sprintf(string_tmp, "%d.%d.%d.%d", g_tParam.Server[0].ip[0], g_tParam.Server[0].ip[1], g_tParam.Server[0].ip[2], g_tParam.Server[0].ip[3]);
            LCD_Txt(3, 3, string_tmp);                          //显示服务器1的IP
            sprintf(string_tmp, "%d", g_tParam.Server[0].port);
            LCD_Txt(3, 4, string_tmp);                          //显示服务器1的端口
            sprintf(string_tmp, "%d", g_tParam.u16_SaveRtdInterval);
            LCD_Txt(3, 6, string_tmp);                          //保存数据的时间间隔

            sprintf(string_tmp, "%04d%02d%02d%", DateCurrent.Year + 2000, DateCurrent.Month, DateCurrent.Date);
            LCD_Txt(3, 7, string_tmp);                          //显示日期

            sprintf(string_tmp, "%02d%02d%02d%", TimeCurrent.Hours, TimeCurrent.Minutes, TimeCurrent.Seconds);
            LCD_Txt(3, 8, string_tmp);                          //显示时间

            LCD_Txt(3, 5, g_tParam.apn);                        //显示APN
            
            /* 显示网络商 */
            switch (Isp)
            {
            case 0:
                LCD_Txt(3, 15, "中国移动");
                break;
            case 1:
                LCD_Txt(3, 15, "中国联通");
                break;
            case 2:
                LCD_Txt(3, 15, "中国联通");
                break;
            default:
                break;
            }
            
            sprintf(string_tmp, "%d", g_tParam.CleanChannel);
            LCD_Txt(3, 9, string_tmp);                          //显示净化器的通道值
            sprintf(string_tmp, "%d", g_tParam.FanChannel);
            LCD_Txt(3, 10, string_tmp);                         //显示风机的通道值
        }
        break;
    default:
        break;
    }
}

/******************************************************************************
  Function:     UpdateLcdTime
  Description:  更新串口显示屏的时钟
  Input:        none
  Return:       none
  Others:       none
******************************************************************************/
void UpdateLcdTime(void)
{
    static int last_sec = 0;            //保存上一次秒的读数,用于检测RTC是否停止
    static uint8_t cnt = 0;             //用于检测RTC是否停止

    char string_tmp[30];//应该只需要20B

    HAL_RTC_GetTime(&hrtc, &TimeCurrent, RTC_FORMAT_BIN);
    if (last_sec == TimeCurrent.Seconds)
    {
        if (++cnt > 5)
        {
            //重新配置RTC
//          RTC_Configuration();
//          //配置完成后，向后备寄存器中写特殊字符0xA5A5
//          BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
//          Time_SetCalendarTime(time_now);

//          HAL_RTC_Init(&hrtc);
            cnt = 0;
        }
    }
    else
    {
        cnt = 0; //后来添加的,原来的有风险
    }
    last_sec = TimeCurrent.Seconds;

    sprintf(string_tmp, "%4d-%02d-%02d %02d:%02d:%02d", DateCurrent.Year + 2000, DateCurrent.Month,\
                DateCurrent.Date, TimeCurrent.Hours, TimeCurrent.Minutes, TimeCurrent.Seconds);
    
    LCD_Txt(0, 10, string_tmp);                             //更新时钟显示
}

/******************************************************************************
  Function:     GetLcdCmd
  Description:  从串口缓冲区中获取一帧串口液晶显示屏命令,存入Lcd_CmdBuf[]
  Input:        none
  Return:       0:      获取失败
                1~255:  获取成功返回命令长度
  Others:       none
******************************************************************************/
uint8_t GetLcdCmd(void)
{
    static uint8_t Lcd_CmdState = 0;                //串口显示屏命令帧接收状态
    uint8_t _data = 0;

    while (comGetChar(LCD_COM, &_data))
    {
        //取一个数据
//      comGetChar(LCD_COM, &_data);

        if (Lcd_CmdBufCnt == 0 && _data != 0xEE) //帧头出错，跳过
            continue;

        //缓存区溢出,缓存区清除,退出
        if (Lcd_CmdBufCnt >= LCD_CMD_BUF_MAX)
        {
            Lcd_CmdBufCnt = 0;
            Lcd_CmdState = 0;
            return 0;
        }
        Lcd_CmdBuf[Lcd_CmdBufCnt++] = _data;

        //判断帧尾
        if (_data == 0xFF)
        {
            switch (Lcd_CmdState)
            {
            case 2:
                Lcd_CmdState = 3;
                break; //FF FC FF ?? (最后一个字节不对)
            case 3:
                Lcd_CmdState = 4;
                break; //FF FC FF FF 正确的帧尾
            default:
                Lcd_CmdState = 1;
                break; //FF ?? ?? ??(最后三个字节不对)
            }
        }
        else if (_data == 0xFC)
        {
            switch (Lcd_CmdState)
            {
            case 1:
                Lcd_CmdState = 2;
                break; //FF FC ?? ??(最后二个字节不对)
            case 3:
                Lcd_CmdState = 2;
                break; //FF FC FF FC 正确的帧尾
            default:
                Lcd_CmdState = 0;
                break; //?? ?? ?? ??(全部字节不对)
            }
        }
        else
            Lcd_CmdState = 0;

        //得到完整的帧尾
        if (Lcd_CmdState == 4)
        {
            Lcd_CmdState = 0;
            Lcd_CmdBufCnt = 0;
            return 1;
        }
    }

    return 0; //没有形成完整的一帧
}

/******************************************************************************
  Function:     LCD_CmdHandle
  Description:  处理串口显示屏的返回的命令
  Input:        none
  Return:       none
  Others:       none
******************************************************************************/
uint8_t LCD_CmdHandle(void)
{
    static uint32_t password = 0;                   //保存访问设置界面的密码
    uint16_t response_id;                            //高8位是画面id,低8位是控件id
    char *p;
    char str_tmp[10];
    uint8_t i;
    uint16_t j;

    if (GetLcdCmd() == 0)                              //未接收到串口显示屏命令帧
    {
        return 0;
    }

    //处理命令帧
    response_id = (uint16_t)(Lcd_CmdBuf[4] << 8);     //获取画面id
    response_id |= (uint8_t)Lcd_CmdBuf[6];          //获取控件id

    if (Lcd_CmdBuf[7] == 0x11)                      //返回文本框信息
    {
        switch (response_id)
        {
        case 0x0101:                                //密码文本框
            password = atol(&Lcd_CmdBuf[8]);
            break;
        case 0x0302:                                //输入设备号
            p = &Lcd_CmdBuf[8];
            if (strlen(p) > 16)                      //检测输入字符数是否正确
            {
                LCD_Txt(3, 17, "设备号过长，不能大于16个字符！");
            }
            else
            {
                strcpy(g_tParam.DevNO, p);
                SaveParam();
                LCD_Txt(3, 17, "设备号设置成功");
            }
            break;
        case 0x0303:                                //输入服务器1地址
            p = &Lcd_CmdBuf[8];
            for (j = 0; j < 4; j++)
            {
                i = 0;
                while (*p >= '0' && *p <= '9' && i < 3)
                {
                    str_tmp[i++] = *p++;
                }
                str_tmp[i] = '\0';
                g_tParam.Server[0].ip[j] = atoi(str_tmp);
                p++;                                //跳过字符'.'
            }
            SaveParam();
            LCD_Txt(3, 17, "服务器IP地址设置成功");
            GprsInitRequest = 1;                    //请求重新初始化GPRS
            break;
        case 0x0304:                                //输入服务1端口号
            p = &Lcd_CmdBuf[8];
            i = 0;
            while (*p >= '0' && *p <= '9' && i < 5)
            {
                str_tmp[i++] = *p++;
            }
            str_tmp[i] = '\0';
            g_tParam.Server[0].port = atoi(str_tmp);
            SaveParam();
            LCD_Txt(3, 17, "端口号设置成功");
            GprsInitRequest = 1;                    //请求重新初始化GPRS
            break;
        case 0x0305:                                //输入APN
            p = &Lcd_CmdBuf[8];
            if (strlen(p) > 31)                       //检测输入字符数是否正确
            {
                LCD_Txt(3, 17, "APN长度限制31个字符");
            }
            else
            {
                i = 0;
                while (*p > 0 && *p <= 128 && i < 31)
                {
                    g_tParam.apn[i++] = *p++;
                }
                g_tParam.apn[i] = '\0';
                SaveParam();
                LCD_Txt(3, 17, "APN设置成功");
                GprsInitRequest = 1;                    //请求重新初始化GPRS
            }
            break;
        case 0x0306:                                    //输入时间间隔(和服务器1上传数据的时间间隔相同)
            p = &Lcd_CmdBuf[8];
            if (strlen(p) > 5)                          //检测输入字符数是否正确
            {
                LCD_Txt(3, 17, "输入时间过大");
            }
            else
            {
                i = 0;
                while (*p >= '0' && *p <= '9' && i < 5)
                {
                    str_tmp[i++] = *p++;
                }
                str_tmp[i] = '\0';
                g_tParam.Server[0].rtd_interval = g_tParam.u16_SaveRtdInterval = atoi(str_tmp);
                SaveParam();
                LCD_Txt(3, 17, "时间间隔设置成功");
            }
            break;
        case 0x0307:                                //输入日期
            p = &Lcd_CmdBuf[8];
            if (strlen(p) == 8)                      //检测输入字符数是否正确
            {
                str_tmp[0] = *p++;
                str_tmp[1] = *p++;
                str_tmp[2] = *p++;
                str_tmp[3] = *p++;
                str_tmp[4] = '\0';
                j = atoi(str_tmp);
                if (j < 2000)
                {
                    LCD_Txt(3, 17, "日期年份错误！");
                    break;
                }
                DateCurrent.Year = j - 2000;
                str_tmp[0] = *p++;
                str_tmp[1] = *p++;
                str_tmp[2] = '\0';
                j = atoi(str_tmp);
                if (j > 13)
                {
                    LCD_Txt(3, 17, "日期月份错误！");
                    break;
                }
                DateCurrent.Month = j;
                str_tmp[0] = *p++;
                str_tmp[1] = *p++;
                str_tmp[2] = '\0';
                j = atoi(str_tmp);
                if (j > 31)
                {
                    LCD_Txt(3, 17, "日期错误！");
                    break;
                }
                DateCurrent.Date = j;

                HAL_RTC_SetDate(&hrtc, &DateCurrent, RTC_FORMAT_BIN);
                RTC_BackupDate(&hrtc,&DateCurrent);
                LCD_Txt(3, 17, "日期设置成功");
            }
            else
            {
                LCD_Txt(3, 17, "时间格式错误！");
            }
            break;
        case 0x0308:                                //输入时间
            p = &Lcd_CmdBuf[8];
            if (strlen(p) == 6)                      //检测输入字符数是否正确
            {
                str_tmp[0] = *p++;
                str_tmp[1] = *p++;
                str_tmp[2] = '\0';
                j = atoi(str_tmp);
                if (j > 23)
                {
                    LCD_Txt(3, 17, "小时错误！");
                    break;
                }
                TimeCurrent.Hours = j;
                str_tmp[0] = *p++;
                str_tmp[1] = *p++;
                str_tmp[2] = '\0';
                j = atoi(str_tmp);
                if (j > 59)
                {
                    LCD_Txt(3, 17, "分钟错误！");
                    break;
                }
                TimeCurrent.Minutes = j;
                str_tmp[0] = *p++;
                str_tmp[1] = *p++;
                str_tmp[2] = '\0';
                j = atoi(str_tmp);
                if (j > 59)
                {
                    LCD_Txt(3, 17, "秒错误！");
                    break;
                }
                TimeCurrent.Seconds = j;
                HAL_RTC_SetTime(&hrtc, &TimeCurrent, RTC_FORMAT_BIN);
                LCD_Txt(3, 17, "时间设置成功");
            }
            else
            {
                LCD_Txt(3, 17, "时间格式错误！");
            }
            break;
        case 0x0309:                                //输入净化器通道
            g_tParam.CleanChannel = atoi(&Lcd_CmdBuf[8]);
            SaveParam();
            LCD_Txt(3, 17, "净化器通道设置成功");
            break;
        case 0x030a:                                //输入风机通道
            g_tParam.FanChannel = atoi(&Lcd_CmdBuf[8]);
            SaveParam();
            LCD_Txt(3, 17, "风机通道设置成功");
            break;
        default:
            break;
        }
    }
    else if (Lcd_CmdBuf[7] == 0x10)                 //返回的是按键控件信息
    {
        switch (response_id)
        {
        case 0x0001:                                //画面0的【设置键】
            LCD_SetScreen(1);                       //切换到密码输入界面
            break;
        case 0x0102:                                //密码确认键
            if (password == LCD_SET_PSW)
            {
                LCD_SetScreen(2);                   //进入设置界面
                password = 0;
            }
            break;
        case 0x0103:                                //密码取消键
            password = 0;
            LCD_SetScreen(0);                       //切换到界面0
            break;
        case 0x0201:                                //画面2的【返回主页键】
            LCD_SetScreen(0);                       //切换到0界面
            break;
        case 0x020F:                                //画面2的【下一页】
            LCD_SetScreen(3);                       //切换到界面3
            LCD_Txt(3, 17, "");
            break;
        case 0x030E:                                //画面3的【上一页】
            LCD_SetScreen(2);                       //切换到界面2
            break;
        case 0x0310:                                //画面3的【返回主页键】
            LCD_SetScreen(0);                       //切换到界面0
            break;
        case 0x0402:                                //画面4的【返回主页键】
            LCD_SetScreen(0);                       //切换到界面0
            break;
        default:
            break;
        }
    }
    return 1;
}

/******************************************************************************
  Function:     DisplayNetState
  Description:  在LCD上显示网络状态
  Input:        none
  Return:       none
  Others:       none
******************************************************************************/
void DisplayNetState(void)
{
    char str_tmp[50];

    if (GPRS_State == GPRS_OK)
    {
        GprsSignalLevel = DTU_HCSQ();                            //查询信号强度
        printf("Net type is %d, level %d.\r\n", NetType, GprsSignalLevel);
        if (GprsSignalLevel < 255)
        {
            sprintf(str_tmp, "已连接网络   信号强度:%2d", (uint8_t)(GprsSignalLevel / 10));
            SetOutputBit(0, ON);
        }
        else
        {
            sprintf(str_tmp, "网络已断开");
            SetOutputBit(0, OFF);
        }
        LCD_Txt(0, 6, str_tmp);
    }
    else if (GPRS_State == NO_SIM_CARD)
    {
        LCD_Txt(0, 6, "网络连接失败: 无SIM卡");
    }
    else
    {
        LCD_Txt(0, 6, "网络连接失败");
    }
}


/******************************************************************************
  Function:     LCD_SendBuf
  Description:  发送信息给串口屏
  Input:        none
  Return:       none
  Others:       none
******************************************************************************/
static void LCD_SendBuf(uint8_t *_ucaBuf, uint16_t _usLen)
{
    osMutexWait(osMutexLcd, 0);
    comSendBuf(LCD_COM, _ucaBuf, _usLen);
    osMutexRelease(osMutexLcd);
}

/******************************************************************************
  Function:     LCD_Init
  Description:  串口屏初始化
  Input:        none
  Return:       none
  Others:       none
******************************************************************************/
void LCD_Init(void)
{
    osMutexLcd = osMutexCreate(NULL);
}
/************** (C) COPYRIGHT 广州正虹科技发展有限公司 ********END OF FILE****/

