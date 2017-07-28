/******************* Copyright(c) 广州正虹科技发展有限公司 *******************
* 
*------File Info--------------------------------------------------------------
* File Name:            gprs.c
*   					ME909s-821驱动
*   					ME909的连接号为1-5,但为了统一编号,改为0-4
* Latest modified Date: 
* Latest Version:       
* Description:          gprs通讯
* compiler:             MDK V5.22
* MCU:                  STM32F103VE
* Oscillator Crystal Frequency:    8MHz
*-----------------------------------------------------------------------------
* Created By:    李杰文
* Created date:  2015-07-27
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
#include <stdlib.h>
#include "protocol.h"

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
#define LINK_MAX	4								/* 设置最大的连接数量 */
/* Private macro ------------------------------------------------------------*/
#define CONNECT_BUFFER_SIZE 1055                    /* 接收GPRS各链接的数据缓存区的大小 */
/* Private variables --------------------------------------------------------*/
char Connect_RecBuf[CONNECT_BUFFER_SIZE];                            //链路0接收信息的buffer
uint16_t Connect0_RecBufCnt = 0;

//char g_u8_PkgBuf[UART1_TX_BUF_LEN];
/* Private function prototypes ----------------------------------------------*/
static uint8_t DTU_PowerOn(void);
static void DTU_PowerOff(void);
/* Private functions --------------------------------------------------------*/

/* Forward Declaration of local functions -----------------------------------*/
/* Declaration of extern functions ------------------------------------------*/
/* Global variables ---------------------------------------------------------*/
//GPRS控制串口的另一个缓存区,用于GPRS命令的应答检查,可以在发送命令前清空
char GPRS_RxBufTmp[GPRS_RX_BUF_TMP_LEN];
volatile uint16_t GPRS_RxBufTmp_cnt;

NetTypedef NetType = NOSERVICE;                                      //网络类型
GPRS_StateTypeDef GPRS_State = NO_INIT;                              //gprs状态
uint8_t Isp;                                                         //ISP识别码(0:中国移动;1:中国联通;2:中国电信;3:其他)
char IMSI[16] = { 0 };                                               //保存15位IMSI号
uint8_t g_ucConnectEnable = 0x00;                                    //8路连接使能标志 0:该连接被禁止;1:该连接被使能
uint8_t g_ucConnectState = 0x00;                                     //8路连接的状态,bit0~bit7对于0~7路连接 0:连接服务器; 1:未连接服务器
uint8_t g_ucReconnectCnt = 0x00;                                     //重连次数,多次连接后则重启GPRS模块.//暂时没使用
uint8_t g_ucaHeartbeatCnt[LINK_MAX] = { 0 };                         //各连接的心跳包时间计数,为0则发送心跳包
uint8_t GprsSignalLevel = 0;                                         //无线网络信号强度
uint8_t GprsInitRequest = 0;                                         //请求重新初始化网络连接
/*---------------------------------------------------------------------------*/
/*                          Private functions                                */
/*---------------------------------------------------------------------------*/

/*
*********************************************************************************************************
*	函 数 名: DTU_CleanRxBufTmp
*	功能说明: 清除GPRS信息临时接收区
*   形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void DTU_CleanRxBufTmp(void)
{
    //  uint16_t i;
    //  for (i = 0; i < GPRS_RX_BUF_TMP_LEN; i++)
    //  {
    //      GPRS_RxBufTmp[i++] = 0;
    //  }
    memset(GPRS_RxBufTmp, '\0', GPRS_RX_BUF_TMP_LEN);
    GPRS_RxBufTmp_cnt = 0;
}

/*
*********************************************************************************************************
*	函 数 名: ME_SendAtCmdNoResponse
*	功能说明: 向GSM模块发送AT命令。 本函数自动在AT字符串口增加<CR>字符
*	形    参: _Str : AT命令字符串，不包括末尾的回车<CR>. 以字符0结束
*	返 回 值: 无
*********************************************************************************************************
*/
static void ME_SendAtCmdNoResponse(char *_Cmd)
{
    comSendBuf(GPRS_COM, (uint8_t *)_Cmd, strlen(_Cmd));
    comSendBuf(GPRS_COM, "\r", 1);
}

/*
*********************************************************************************************************
*	函 数 名: ME_Connect
*	功能说明: 查询当前网络信号强度
*   形    参: uint8_t _ucConnect 连接号 0-3 自动转换为ME909的1-5
*            uint8_t _ucType  连接类型 0:TCP; 1:UDP; 2:FTP; 3:FTPS
*            uint8_t _ucIp[4] 保存ip的数组
*            uint16_t _uiPort 连接端口
*	返 回 值: 0: 连接成功; 1:连接已存在; 2:连接失败
*********************************************************************************************************
*/
static uint8_t ME_Connect(uint8_t _ucConnect, uint8_t _ucType, uint8_t _ucIp[4], uint16_t _uiPort)
{
    char str_tmp[45];
    uint16_t i;

    /* Check the parameters */
    assert_param(IS_CONNECT_ID(_ucConnect));

    g_ucConnectEnable |= (uint8_t)(1 << _ucConnect);
    g_ucaHeartbeatCnt[_ucConnect] = 0;                               //心跳包计时清零

    {
        switch (_ucType)
        {
        case 0:                                                      //AT^IPOPEN=2,"TCP","120.197.159.424",9895
            sprintf(str_tmp, "AT^IPOPEN=%1d,\"TCP\",\"%d.%d.%d.%d\",%d", _ucConnect + 1, _ucIp[0], _ucIp[1], _ucIp[2], _ucIp[3], _uiPort);
            break;
        case 1:
            sprintf(str_tmp, "AT^IPOPEN=%1d,\"UDP\",\"%d.%d.%d.%d\",%d", _ucConnect + 1, _ucIp[0], _ucIp[1], _ucIp[2], _ucIp[3], _uiPort);
            break;
        default:
            break;
        }

        DTU_CleanRxBufTmp();                                         //清除GPRS信息接收的临时存储区
        ME_SendAtCmdNoResponse(str_tmp);
        i = 100;
        do
        {
            if (strstr(GPRS_RxBufTmp, "OK") != NULL)
            {
#if GPRS_DEBUG_EN
                DEBUG_PrintfString(GPRS_RxBufTmp);
#endif
                return 0;                                            //连接成功
            }
            else if (strstr(GPRS_RxBufTmp, "ERROR") != NULL)
            {
                if (strstr(GPRS_RxBufTmp, "+CME ERROR: 1003") != NULL)
                {
#if GPRS_DEBUG_EN
                    DEBUG_PrintfString(GPRS_RxBufTmp);
#endif
                    return 1;                                        //连接已存在
                }
                else if (strstr(GPRS_RxBufTmp, "+CME ERROR: 1012") != NULL) /* 网络断开 */
                {
                    ME909_Init();
                    return 2;
                }
                break;
            }
            osDelay(10);
        }while (i--);
    }
#if GPRS_DEBUG_EN
    DEBUG_PrintfString(GPRS_RxBufTmp);
#endif
    return 2;                                                        //连接失败
}

/* *****************************************************************************
  Function:     ME_SendAtCmd
  Description:  发送AT命令给通信模块
  Input:        uint8_t *cmd 命令字符串
                uint8_t *ack 需要应答的字符串内容
                uint16_t wait_time 允许等待的时间n*10ms
  Return:       0: 命令发送成功，应答成功
                1: 等待应答超时
                2: 返回ERROR
  Others:       
***************************************************************************** */
static uint8_t ME_SendAtCmd(char *cmd, char *ack, uint16_t wait_time)
{
    /* 需要等待结果则清除临时区 */
    if (wait_time)
    {
        DTU_CleanRxBufTmp();                                         //清除GPRS信息接收的临时存储区
    }

    ME_SendAtCmdNoResponse(cmd);

    while (wait_time--)
    {
        osDelay(10);
        if (strstr(GPRS_RxBufTmp, ack) != NULL)
        {
#if  GPRS_DEBUG_EN
            DEBUG_PrintfString(GPRS_RxBufTmp);
#endif
            return 0;                                                //发送成功
        }
        else if (strstr(GPRS_RxBufTmp, "ERROR") != NULL)
        {
            return 2;
        }
    }
#if  GPRS_DEBUG_EN
    DEBUG_PrintfString(GPRS_RxBufTmp);
#endif
    return 1;                                                        //超时,返回
}

/*
*********************************************************************************************************
*	函 数 名: ME_ErrorHandle
*	功能说明: 处理DTU返回的错误信息
*   形    参: str 错误信息的消息字符串
*             conn_id 连接号 
*	返 回 值: 无
*********************************************************************************************************
*/
static void ME_ErrorHandle(char *str, uint8_t conn_id)
{
    if (strstr(GPRS_RxBufTmp, "+CME ERROR: 1002") != NULL)           /* 处理1002错误(连接断开) */
    {
        ME_ConnectSever(conn_id);
    }
    else if (strstr(GPRS_RxBufTmp, "+CME ERROR: 1012") != NULL)      /* 处理1012错误(网络断开) */
    {
        DTU_Init();
    }
    else
    {}
}
/*
*********************************************************************************************************
*	函 数 名: DTU_PowerOff
*	功能说明: 控制DTU模块关机(ME909无硬件关机功能)
*	形    参: 无
*	返 回 值: 无
********************
*************************************************************************************
*/
static void DTU_PowerOff(void)
{
    ME_SendAtCmd("AT^MSO", "OK", 100);
}

/*---------------------------------------------------------------------------*/
/*                            function code                                  */
/*---------------------------------------------------------------------------*/
//在非透传模式下发生一串字符
/******************************************************************************
  Function:     ME_TCPSendString
  Description:  通过DTU响应的链路发送一个字符串
  Input:        uint8_t conn_id 链路ID号 0-3 自动转换为ME909的1-5
                char *str       字符串指针
                uint16_t wait_time  等待的时间,单位1ms
  Return:       0: 命令发送成功，应答成功
                1: 发送失败
                2: 发送超时
  Others:       
******************************************************************************/
uint8_t ME_TCPSendString(uint8_t conn_id, char *str, uint16_t wait_time)
{
    assert_param(IS_CONNECT_ID(conn_id));
    char str_tmp[25];
    uint16_t length = strlen(str);
    uint8_t result;
//    char *ptr;
    uint8_t i;

    //   ME_SendAtCmd("AT^IPSEND=1,\"abc\"","OK",100);
    for (i = 0; i < 2; i++)
    {
        sprintf(str_tmp, "AT^IPSENDEX=%1d,2,%d", conn_id + 1, length); //采用模式2发送
        result = ME_SendAtCmd(str_tmp, "OK", 50);

        if (result == 0)
        {
            DTU_CleanRxBufTmp();
            comSendBuf(GPRS_COM, (uint8_t *)str, length);

            while (wait_time--)
            {
                if (strstr(GPRS_RxBufTmp, "^IPSENDEX:") != NULL)
                {
#if GPRS_DEBUG_EN
                    DEBUG_PrintfString("Send data to connect :");
                    printf("%d OK : ", conn_id);
                    DEBUG_PrintfString(str);
                    DEBUG_PrintfBytes("\r\n", 3);
                    DEBUG_PrintfString(GPRS_RxBufTmp);
#endif
                    return 0;
                }
                else if (strstr(GPRS_RxBufTmp, "ERROR") != NULL)
                {

#if GPRS_DEBUG_EN
                    DEBUG_PrintfString("Send data to connect :");
                    printf("%d fail:", conn_id);
                    DEBUG_PrintfString(str);
                    DEBUG_PrintfBytes("\r\n", 3);
                    DEBUG_PrintfString(GPRS_RxBufTmp);
#endif
                    ME_ErrorHandle(GPRS_RxBufTmp, conn_id);
                    break;
                }
                osDelay(1);
            }                                                        //while
            break;
        }
        else
        {
            ME_ErrorHandle(GPRS_RxBufTmp, conn_id);
        }
    }
    return 1;                                                        /* 发送失败 */
}

/******************************************************************************
  Function:     ME909_Init
  Description:  初始化DTU
  Input:        none
  Return:       GPRS状态
  Others:       
******************************************************************************/
GPRS_StateTypeDef ME909_Init(void)
{
    char str_tmp[50];
    uint8_t i, res;
    char *p;

    GPRS_State = NO_INIT;

    i = 120;
    while (ME_SendAtCmd("AT", "OK", 10) && i--)
    {
        osDelay(1000);
    }

    /* 关闭部分主动上报提示功能 */
    ME_SendAtCmd("AT^CURC=0", "OK", 10);

    for (i = 0; i < 20; i++)
    {
        res = ME_SendAtCmd("AT+CIMI", "OK", 100);                    //读取IMSI号
        if (res == 0)
        {
            if ((p = strstr(GPRS_RxBufTmp, "46001")) != NULL)        /* 联通卡 */
            {
                Isp = 1;
            }
            else if ((p = strstr(GPRS_RxBufTmp, "46003")) != NULL)   /* 电信卡 */
            {
                Isp = 2;
            }
            else if ((p = strstr(GPRS_RxBufTmp, "46005")) != NULL)   /* 电信卡 */
            {
                Isp = 2;
            }
            else                                                     /* 移动卡 */
            {
                Isp = 0;
            }
            strncpy(IMSI, p, 15);
            break;
        }
        osDelay(1000);
    }
    if (i >= 20)
    {
        GPRS_State = NO_SIM_CARD;
        return GPRS_State;                                           // 无SIM卡
    }

    i = 60;
    do
    {
        res = ME_SendAtCmd("AT+CPIN?", "READY", 200);                //查询SIM卡是否正常
        if (res == 0)
        {
            break;
        }
        osDelay(1000);
    }while (res && (i--));

    if (res && (i == 0))
    {
#if  GPRS_DEBUG_EN
        DEBUG_PrintfString("SIM card fail.\n");
#endif
        GPRS_State = INVALID_SIM_CARD;
        return GPRS_State;                                           //SIM卡初始化失败
    }

    i = 10;
    do
    {
        res = ME_SendAtCmd("AT^IPINIT?", "^IPINIT: 1,", 300);        //查询GPRS业务
        if (res == 0)
        {
            break;
        }
        else if (res == 2)
        {
            continue;
        }

        /* 初始化网络参数 */
        sprintf(str_tmp,"AT^IPINIT=\"%s\"",g_tParam.apn);
        ME_SendAtCmd(str_tmp, "OK", 300);              //手动附着GPRS业务
        //ME_SendAtCmd("AT^IPINIT=\"3gnet\"", "OK", 300);              //手动附着GPRS业务
        osDelay(1000);
    }while (i--);

    if (res && (i == 0))
    {
#if  GPRS_DEBUG_EN
        DEBUG_PrintfString("no GPRS.\n");
#endif
        GPRS_State = NO_GPRS;
        return GPRS_State;                                           //无GPRS业务
    }

    //链接1
    if (g_tParam.Server[0].port != 0)
    {
        res = ME_Connect(0, 0, g_tParam.Server[0].ip, g_tParam.Server[0].port);

#if GPRS_DEBUG_EN
        if (res == 0)
        {
            printf("connect0 %d.%d.%d.%d,%d, connect ok.\n", g_tParam.Server[0].ip[0], g_tParam.Server[0].ip[1], g_tParam.Server[0].ip[2], g_tParam.Server[0].ip[3], g_tParam.Server[0].port);
        }
        else if (res == 1)
        {
            printf("connect0 %d.%d.%d.%d,%d, connect exit.\n", g_tParam.Server[0].ip[0], g_tParam.Server[0].ip[1], g_tParam.Server[0].ip[2], g_tParam.Server[0].ip[3], g_tParam.Server[0].port);
        }
        else
        {
            printf("connect0 %d.%d.%d.%d,%d, connect fail.\n", g_tParam.Server[0].ip[0], g_tParam.Server[0].ip[1], g_tParam.Server[0].ip[2], g_tParam.Server[0].ip[3], g_tParam.Server[0].port);
        }
#endif
    }

    //链接2
    if (g_tParam.Server[1].port != 0)
    {
        res = ME_Connect(1, 0, g_tParam.Server[1].ip, g_tParam.Server[1].port);
#if GPRS_DEBUG_EN
        if (res == 0)
        {
            printf("connect1 %d.%d.%d.%d,%d, connect ok.\n", g_tParam.Server[1].ip[0], g_tParam.Server[1].ip[1], g_tParam.Server[1].ip[2], g_tParam.Server[1].ip[3], g_tParam.Server[1].port);
        }
        else if (res == 1)
        {
            printf("connect1 %d.%d.%d.%d,%d, connect exit.\n", g_tParam.Server[1].ip[0], g_tParam.Server[1].ip[1], g_tParam.Server[1].ip[2], g_tParam.Server[1].ip[3], g_tParam.Server[1].port);
        }
#endif
    }

    //链接3
    if (g_tParam.Server[2].port != 0)
    {
        res = ME_Connect(2, 0, g_tParam.Server[2].ip, g_tParam.Server[2].port);
#if GPRS_DEBUG_EN
        if (res == 0)
        {
            printf("connect2 %d.%d.%d.%d,%d, connect ok.\n", g_tParam.Server[2].ip[0], g_tParam.Server[2].ip[1], g_tParam.Server[2].ip[2], g_tParam.Server[2].ip[3], g_tParam.Server[2].port);
        }
        else if (res == 1)
        {
            printf("connect2 %d.%d.%d.%d,%d, connect exit.\n", g_tParam.Server[2].ip[0], g_tParam.Server[2].ip[1], g_tParam.Server[2].ip[2], g_tParam.Server[2].ip[3], g_tParam.Server[2].port);
        }
#endif
    }

    //链接4
    if (g_tParam.Server[3].port != 0)
    {
        res = ME_Connect(3, 0, g_tParam.Server[3].ip, g_tParam.Server[3].port);
#if GPRS_DEBUG_EN
        if (res == 0)
        {
            printf("connect3 %d.%d.%d.%d,%d, connect ok.\n", g_tParam.Server[3].ip[0], g_tParam.Server[3].ip[1], g_tParam.Server[3].ip[2], g_tParam.Server[3].ip[3], g_tParam.Server[3].port);
        }
        else if (res == 1)
        {
            printf("connect3 %d.%d.%d.%d,%d, connect exit.\n", g_tParam.Server[3].ip[0], g_tParam.Server[3].ip[1], g_tParam.Server[3].ip[2], g_tParam.Server[3].ip[3], g_tParam.Server[3].port);
        }
#endif

    }

    comClearRxFifo(GPRS_COM);                                        /* 清空接收缓存区 */
#if  GPRS_DEBUG_EN
    DEBUG_PrintfString("ME init success.\n");
#endif
    GPRS_State = GPRS_OK;                                            //网络初始化成功

    return GPRS_State;
}

/******************************************************************************
  Function:     DTU_Init
  Description:  初始化GPRS模块
  Input:        none
  Return:       none
  Others:       尝试初始化GPRS模块,最大3次
******************************************************************************/
void DTU_Init(void)
{
    uint8_t i, res;

    LCD_Txt(0, 6, "正在连接网络...");
    for (i = 0; i < 3; i++)
    {
#if GPRS_DEBUG_EN
			printf("DTUD start time:%d.\n", i);
#endif
        if (DTU_PowerOn() != 0)
        {
            continue;
        }
        res = ME909_Init();
#if GPRS_DEBUG_EN
        printf("DTUD respond %d.\n", res);
#endif
        DisplayNetState();
        if (GPRS_State == GPRS_OK)
        {
            break;
        }
        //      if (i > 1)
        {
            //         ME_SendAtCmd("AT+MSO", "", 0);                           //由于开机端通过10K电阻接地,关机命令变成重启DTU模块
            osDelay(2000);
            LCD_Txt(0, 6, "重新连接...");
        }
    }
}

/*
*********************************************************************************************************
*	函 数 名: ME_ReceiveOneLineInfo
*	功能说明: 从DTU模块 GPRS串口缓存中获取一行数据,以0x0A为结束标志,或者超时结束
*   形    参: char *_p_cBuf:指向保存数据的存储地址;
*            uint16_t _usBufSize:存储区的长度/字节;必须大于1个字节
*            uint16_t _usTimeOut:等待的时间/毫秒
*	返 回 值: 0:无数据或者超时; >0:数据长度
*********************************************************************************************************
*/
static uint16_t ME_ReceiveOneLineInfo(char *_p_cBuf, uint16_t _usBufSize, uint16_t _usTimeOut)
{
    uint8_t ucData;
    uint16_t cnt = 0;

    /* 存储区长度过小 */
    if (_usBufSize <= 1)
    {
        return 0;
    }

    while (_usTimeOut && --_usBufSize)
    {
        if (comGetChar(GPRS_COM, &ucData))
        {
#if GPRS_DEBUG_EN
            DEBUG_PrintfBytes((char *)&ucData, 1);                   /* 将接收到数据打印到调试串口1 */
#endif
            *_p_cBuf++ = ucData;                                     /* 保存接收到的数据 */
            cnt++;

            /* 接收到结束符'\n',退出 */
            if (ucData == '\n')
            {
                break;
            }
        }
        else
        {
            osDelay(1);
            _usTimeOut--;
        }
    }                                                                //while

    *_p_cBuf = '\0';

    return cnt;
}

/******************************************************************************
  Function:     DTU_HandleRecData
  Description:  处理GPRS模块接收返回的数据
  Input:        none
  Return:       0: 无异常
                1: 无数据或数据出错
  Others:       目前只支持接收服务器返回的信息,每次都会把接收缓存区中的数据处理完,
                如果未满一帧的会存入本函数的数据帧临时存储区
                不考虑TCP数据通讯出错的情况,TCP通讯会把通讯异常的数据包丢弃
******************************************************************************/
void DTU_HandleRecData(void)
{
    //char buf[15];                                                    //数据帧临时存储区
    //uint16_t buf_cnt = 0;                                            //临时存储区数据计数器
    //uint8_t _plus_state = 0;                                         //1:收到字符+;2:收到IPD
    uint8_t res;
    //int16_t num;

    //uint16_t i, j;
    char *p;
    uint8_t connect_id;                                              //保存接收到的数据的连接号
    uint16_t length;                                                 //保存接收到的数据段的长度

    while (ME_ReceiveOneLineInfo(Connect_RecBuf, CONNECT_BUFFER_SIZE, 100))
    {
        if ((p = strstr(Connect_RecBuf, "^IPDATA:")) != NULL)        /* 处理接收到的服务器信息 */
        {
            /* 获取连接号 */
            p += 9;
            if (*p < '0' || *p > '9')
            {
                continue;
            }
            connect_id = *p - '0';

            /* 获取数据段长度 */
            p += 2;
            length = atoi(p);

            if (length == 0 || length > (1024+12))
            {
                continue;
            }

            /* 指向数据段 */
            p = strchr(p, ',');
            p++;


#if GPRS_DEBUG_EN
            printf("connect%d receive: ", connect_id);
            DEBUG_PrintfBytes(p, length);
#endif

            res = HJ212_CheckData(p, length);                        //对数据进行HJ212协议检查
            if (res == 0)
            {
                HJ212_DataHandle(connect_id - 1, p);
            }
        }
        else if (strstr(Connect_RecBuf, "^IPSTATE:") != NULL)        /* 处理网络状态变化通知消息 */
        {
            ME_CheckConnection();
            ME_Reconnection();
        }
    }                                                                //while
}

/******************************************************************************
  Function:     GPRS_CSQ
  Description:  查询GPRS信号强度
  Input:        none
  Return:       GPRS模块的信号强度0-31
  Others:       none
******************************************************************************/
uint8_t GPRS_CSQ(void)
{
    char *p;
    uint8_t i;
    char str_tmp[3];

    ME_SendAtCmd("AT+CSQ", "+CSQ:", 50);
    p = strstr(GPRS_RxBufTmp, "+CSQ:");
    p += 6;
    i = 0;
    while (*p >= '0' && *p <= '9' && i < 2)
    {
        str_tmp[i++] = *p++;
    }
    str_tmp[i] = '\0';
    i = atoi(str_tmp);
    return i;
}

/******************************************************************************
  Function:     DTU_HCSQ
  Description:  查询DTU信号强度
  Input:        none
  Return:       信号强度0-255
  Others:       none
******************************************************************************/
uint8_t DTU_HCSQ(void)
{
    char *p;
    uint8_t i;

    i = ME_SendAtCmd("AT^HCSQ?", "^HCSQ:", 50);
    if (i != 0)
    {
        return 255;                                                  /* 出错,信号强度未知 */
    }

    /* 判断网络类型 */
    if ((p = strstr(GPRS_RxBufTmp, "LTE")) != NULL)
    {
        NetType = LTE;
    }
    else if ((p = strstr(GPRS_RxBufTmp, "TD-SCDMA")) != NULL)
    {
        NetType = TDSCDMA;
    }
    else if ((p = strstr(GPRS_RxBufTmp, "WCDMA")) != NULL)
    {
        NetType = WCDMA;
    }
    else if ((p = strstr(GPRS_RxBufTmp, "GSM")) != NULL)
    {
        NetType = TDSCDMA;
    }
    else
    {
        NetType = NOSERVICE;
        return 255;                                                  /* 未知网络,信号强度未知 */
    }

    //^HCSQ: "LTE",63,61,186,34
    /* 如果是LTE,则以rsrp为信号强度 */
    if (NetType == LTE)
    {
        p = strstr(p, ",");
        if (p == NULL)
        {
            return 255;                                              /* 未知错误 */
        }
        p++;
    }

    p = strstr(p, ",");
    if (p == NULL)
    {
        return 255;                                                  /* 未知错误 */
    }
    p++;
    i = atoi(p);
    return i;
}

/*
*********************************************************************************************************
*	函 数 名: DTU_PowerOn
*	功能说明: 模块上电. 函数内部先判断是否已经开机，如果已开机则直接返回1
*	形    参: 无
*	返 回 值: 0:表示上电成功; 1: 表示异常
*********************************************************************************************************
*/
static uint8_t DTU_PowerOn(void)
{
    uint8_t i;
    uint8_t res;

    /* 判断是否开机 */
    for (i = 0; i < 3; i++)
    {
        res = ME_SendAtCmd("AT", "OK", 100);
        if (res == 0)
        {
            return 0;
        }
    }

#if GPRS_DEBUG_EN
    DEBUG_PrintfString("Restart ME909.\n");
#endif

    /* 通过拉低 RESIN 引脚100ms复位模块 */
    GPRS_PowerKeyEnable();
    osDelay(200);
    GPRS_PowerKeyDisable();

    /* 开始同步波特率: 主机发送AT，只到接收到正确的OK 
        模块开机后建议延迟 2 至 3 秒后再发送同步字符，用户可发送“ AT” (大写、小写均可)来和模块
      同步波特率，当主机收到模块返回“ OK”，			
    */
    osDelay(2000);
    for (i = 0; i < 200; i++)
    {
        if (ME_SendAtCmd("AT", "OK", 100) == 0)
        {
            return 0;
        }
    }

    return 1;
}

/*
*********************************************************************************************************
*	函 数 名: DTU_CheckState
*	功能说明: 检测DTU状态,如果DTU无回应,视为死机,重启模块电源
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void DTU_CheckState(void)
{
    uint8_t i;

    for (i = 3; i > 0; i--)
    {
        printf("check me909 state.\n");
        if (DTU_PowerOn() != 0)
        {
            /* 开机失败,延时10秒再尝试 */
            osDelay(10000);
        }
        else
        {
            break;                                                   //开机成功,结束
        }
    }
}

/******************************************************************************
  Function:     DTU_Startup
  Description:  启动GPRS模块,如果GPRS已经启动，则先关闭1分钟后重新上电
  Input:        none
  Return:       none
  Others:       none
******************************************************************************/
void DTU_Startup(void)
{
    DTU_PowerOff();
    LCD_Txt(0, 6, "正关闭GPRS模块...");
    osDelay(30000);                                                  //等待30秒
    DTU_Init();
}

/*
*********************************************************************************************************
*	函 数 名: ME_Disconnect
*	功能说明: 断开某个连接
*   形    参: _type 0:慢关; 1:快关  (ME909不使用)
*            _connect_id 连接号0-3
*	返 回 值: 无
*********************************************************************************************************
*/
void ME_Disconnect(uint8_t _type, uint8_t _connect_id)
{
    char str_tmp[15];

    g_ucConnectState &= ~(uint8_t)(1 << _connect_id);
    g_ucConnectEnable &= ~(uint8_t)(1 << _connect_id);
    sprintf(str_tmp, "AT^IPCLOSE=%d", _connect_id + 1);
    ME_SendAtCmd(str_tmp, "OK", 100);
}

/*
*********************************************************************************************************
*	函 数 名: ME_Reconnection
*	功能说明: 恢复断开的连接
*   形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void ME_Reconnection(void)
{
    uint8_t temp;
    uint8_t i;

    /* 有连接断开 */
    if (g_ucConnectEnable != g_ucConnectState)
    {
        temp = g_ucConnectEnable & (uint8_t)(~g_ucConnectState);     //获得断开的连接号
        for (i = 0; i < 8; i++)
        {
            if (temp & 0x01)
            {
                ME_ConnectSever(i);
            }
            temp >>= 1;
        }

        temp = g_ucConnectState & (uint8_t)(~g_ucConnectEnable);     /* 获取需要关闭的连接号 */
        for (i = 0; i < 8; i++)
        {
            if (temp & 0x01)
            {
                ME_Disconnect(1, i);
            }
            temp >>= 1;
        }

    }
}

/*
*********************************************************************************************************
*	函 数 名: ME_ConnectSever
*	功能说明: 连接到N#服务器
*   形    参: uint8_t _ucServerNo: 连接到0#~3#服务器，服务器地址保存在全局参数结构体g_tParam中           
*	返 回 值: 无
*********************************************************************************************************
*/
void ME_ConnectSever(uint8_t _ucServerNo)
{
    /* Check the parameters */
    switch (_ucServerNo)
    {
    case 0:
        ME_Connect(0, 0, g_tParam.Server[0].ip, g_tParam.Server[0].port); //连接1#服务器
        break;
    case 1:
        ME_Connect(1, 0, g_tParam.Server[1].ip, g_tParam.Server[1].port); //连接2#服务器
        break;
    case 2:
        ME_Connect(2, 0, g_tParam.Server[2].ip, g_tParam.Server[2].port); //连接3#服务器
        break;
    case 3:
        ME_Connect(3, 0, g_tParam.Server[3].ip, g_tParam.Server[3].port); //连接4#服务器
        break;
    case 4:
        //      ME_Connect(4, 0, g_tParam.Server[3].ip, g_tParam.Server[3].port); //连接5#服务器
        break;
    default:
        break;
    }
}

/*
*********************************************************************************************************
*	函 数 名: ME_HeartbeartTmrCallback
*	功能说明: 软件定时器每秒调用一次此函数,更新各连接心跳计时
*	形    参：无
*	返 回 值: 无
*********************************************************************************************************
*/
void ME_HeartbeartTmrCallback(void)
{
    uint8_t i;
    uint8_t temp = g_ucConnectEnable;

    for (i = 0; i < LINK_MAX; i++)
    {
        if (temp & 0x01)
        {
            g_ucaHeartbeatCnt[i]++;
        }
        temp >>= 1;
    }
}

/*
*********************************************************************************************************
*	函 数 名: ME_Heartbeart
*	功能说明: 心跳包功能函数
*   形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void ME_Heartbeart(void)
{
    uint8_t i;
    uint8_t temp = g_ucConnectEnable;

    for (i = 0; i < 8; i++)
    {
        if (temp & 0x01)
        {
            /* 时间到,发心跳包 */
//          printf("connect%d cnt: %d\r\n", i, g_ucaHeartbeatCnt[i]);
            if (g_ucaHeartbeatCnt[i] >= g_tParam.Server[i].heartbeart_period)
            {
                g_ucaHeartbeatCnt[i] = 0;
                DEBUG_PrintfString("Send heartbeat data.\r\n");
                ME_TCPSendString(i, "##0000FFFF\r\n", 0);            //在此修改心跳包格式
            }
        }
        temp >>= 1;
    }
}

/*
*********************************************************************************************************
*	函 数 名: ME_CheckConnection
*	功能说明: 检查连接状态,把断开的连接重新连接
*   形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void ME_CheckConnection(void)
{
    uint8_t i;
    uint8_t mask = 0x01;
    char *ptr;
    static uint8_t ucOver = 0;

    ME_SendAtCmd("AT^IPCLOSE?", "^IPCLOSE:", 300);                   //^IPCLOSE: 0,0,0,0,0
    ptr = strstr(GPRS_RxBufTmp, "^IPCLOSE:");
    if (ptr != NULL)
    {
        /*  检查各路连接状态 */
        for (ptr += 10, i = 0; i < LINK_MAX; i++)
        {
            if (*ptr == '0')
            {
                g_ucConnectState &= (uint8_t)(~mask);                /* 连接断开 */
            }
            else
            {
                g_ucConnectState |= mask;                            /* 连接已连上 */
            }
            ptr += 2;
            mask <<= 1;
        }

        /* 有使能连接,但全部断线 */
        if (g_ucConnectState == 0 && g_ucConnectEnable != 0)
        {
            ucOver++;

            /* 全部网络断开,断开10秒*30=5分钟都未能连接,重启通信模块 */
            if (ucOver > 30)
            {
                ucOver = 0;
                DTU_Startup();                                       /*重启通信模块 */
            }
        }
        else
        {
            ucOver = 0;
        }
    }
}

/*
*********************************************************************************************************
*	函 数 名: DTU_GetSimNum
*	功能说明: 获取DTU SIM卡号码
*   形    参: s 指向存储13位号码的地址,空间需14字节
*	返 回 值: 无
*********************************************************************************************************
*/
uint8_t DTU_GetSimNum(char *s)
{
    char *p;
    if (ME_SendAtCmd("AT+CNUM", "OK", 100) == 0)                     /* 读取SIM卡号码 */
    {
        if ((p = strstr(GPRS_RxBufTmp, "+CNUM:")) != NULL)           /* 联通卡 */
        {
            p += 6; 
            p = strchr(p, '+');
            p++;
            strncpy(s, p, 13);                                       //+CNUM: "","+8618620747949",145
            return 0;                                                /* 获取成功*/
        }
    }
    return 1;                                                        /* 获取失败 */
}

/************** (C) COPYRIGHT 广州正虹科技发展有限公司 ********END OF FILE****/

