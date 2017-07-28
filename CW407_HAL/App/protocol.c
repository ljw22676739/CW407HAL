/****************** Copyright(c) 广州正虹科技发展有限公司 ***********************
* 
*------File Info--------------------------------------------------------------
* File Name:            Protocol.c
* Latest modified Date: 
* Latest Version:       
* Description:          实现212协议
* compiler:             MDK V4.73
* MCU:                  STM32F103VE
* Oscillator Crystal Frequency:    18.432MHz
*-----------------------------------------------------------------------------
* Created By:    李杰文
* Created date:  2015-07-15
* Version:       v0.1
* Descriptions:  实现212协议
*
*-----------------------------------------------------------------------------
* Modified by:   修改人的姓名
* Modified date: 文件的修改日期（YYYY-MM-DD）
* Version:       文件修订的版本号
* Description:   文件修订的简要描述
*
******************************************************************************/

/* Includes -----------------------------------------------------------------*/
#include "app.h"
#include "bsp.h"
#include "MyNet.h"
#include "uip.h"
#include "tcp_app.h"
#include "Task_ADC.h"
#include "ff.h"
#include <stdlib.h>

/* Private typedef ----------------------------------------------------------*/
/* Private define -----------------------------------------------------------*/
/* Private macro ------------------------------------------------------------*/
/* Private variables --------------------------------------------------------*/
/* HJ212 CRC表格 */
static const uint16_t CRC212_Tab[256] = {
    0x0000, 0xc0c1, 0xc181, 0x0140, 0xc301, 0x03c0, 0x0280, 0xc241,
    0xc601, 0x06c0, 0x0780, 0xc741, 0x0500, 0xc5c1, 0xc481, 0x0440,
    0xcc01, 0x0cc0, 0x0d80, 0xcd41, 0x0f00, 0xcfc1, 0xce81, 0x0e40,
    0x0a00, 0xcac1, 0xcb81, 0x0b40, 0xc901, 0x09c0, 0x0880, 0xc841,
    0xd801, 0x18c0, 0x1980, 0xd941, 0x1b00, 0xdbc1, 0xda81, 0x1a40,
    0x1e00, 0xdec1, 0xdf81, 0x1f40, 0xdd01, 0x1dc0, 0x1c80, 0xdc41,
    0x1400, 0xd4c1, 0xd581, 0x1540, 0xd701, 0x17c0, 0x1680, 0xd641,
    0xd201, 0x12c0, 0x1380, 0xd341, 0x1100, 0xd1c1, 0xd081, 0x1040,
    0xf001, 0x30c0, 0x3180, 0xf141, 0x3300, 0xf3c1, 0xf281, 0x3240,
    0x3600, 0xf6c1, 0xf781, 0x3740, 0xf501, 0x35c0, 0x3480, 0xf441,
    0x3c00, 0xfcc1, 0xfd81, 0x3d40, 0xff01, 0x3fc0, 0x3e80, 0xfe41,
    0xfa01, 0x3ac0, 0x3b80, 0xfb41, 0x3900, 0xf9c1, 0xf881, 0x3840,
    0x2800, 0xe8c1, 0xe981, 0x2940, 0xeb01, 0x2bc0, 0x2a80, 0xea41,
    0xee01, 0x2ec0, 0x2f80, 0xef41, 0x2d00, 0xedc1, 0xec81, 0x2c40,
    0xe401, 0x24c0, 0x2580, 0xe541, 0x2700, 0xe7c1, 0xe681, 0x2640,
    0x2200, 0xe2c1, 0xe381, 0x2340, 0xe101, 0x21c0, 0x2080, 0xe041,
    0xa001, 0x60c0, 0x6180, 0xa141, 0x6300, 0xa3c1, 0xa281, 0x6240,
    0x6600, 0xa6c1, 0xa781, 0x6740, 0xa501, 0x65c0, 0x6480, 0xa441,
    0x6c00, 0xacc1, 0xad81, 0x6d40, 0xaf01, 0x6fc0, 0x6e80, 0xae41,
    0xaa01, 0x6ac0, 0x6b80, 0xab41, 0x6900, 0xa9c1, 0xa881, 0x6840,
    0x7800, 0xb8c1, 0xb981, 0x7940, 0xbb01, 0x7bc0, 0x7a80, 0xba41,
    0xbe01, 0x7ec0, 0x7f80, 0xbf41, 0x7d00, 0xbdc1, 0xbc81, 0x7c40,
    0xb401, 0x74c0, 0x7580, 0xb541, 0x7700, 0xb7c1, 0xb681, 0x7640,
    0x7200, 0xb2c1, 0xb381, 0x7340, 0xb101, 0x71c0, 0x7080, 0xb041,
    0x5000, 0x90c1, 0x9181, 0x5140, 0x9301, 0x53c0, 0x5280, 0x9241,
    0x9601, 0x56c0, 0x5780, 0x9741, 0x5500, 0x95c1, 0x9481, 0x5440,
    0x9c01, 0x5cc0, 0x5d80, 0x9d41, 0x5f00, 0x9fc1, 0x9e81, 0x5e40,
    0x5a00, 0x9ac1, 0x9b81, 0x5b40, 0x9901, 0x59c0, 0x5880, 0x9841,
    0x8801, 0x48c0, 0x4980, 0x8941, 0x4b00, 0x8bc1, 0x8a81, 0x4a40,
    0x4e00, 0x8ec1, 0x8f81, 0x4f40, 0x8d01, 0x4dc0, 0x4c80, 0x8c41,
    0x4400, 0x84c1, 0x8581, 0x4540, 0x8701, 0x47c0, 0x4680, 0x8641,
    0x8201, 0x42c0, 0x4380, 0x8341, 0x4100, 0x81c1, 0x8081, 0x4040
};
/* Private function prototypes ----------------------------------------------*/
static uint8_t SendHjt212Package(uint8_t conn_id);
static uint16_t cal_crc(char *ptr, int len);

/* Declaration of extern functions ------------------------------------------*/
/* Global variables ---------------------------------------------------------*/
static char g_u8_PkgBuf[1052];

/*---------------------------------------------------------------------------*/
/*                          Private functions                                */
/*---------------------------------------------------------------------------*/
/******************************************************************************
  Function:     SendHjt212Package
  Description:  预先把数据段放到g_u8_PkgBuf[6]开始的空间,此函数自动组合成HJT212的数据包并通过DTU发送
  Input:        uint8_t conn_id 链路ID号 0-3 自动转换为ME909的1-5
  Return:       0: 命令发送成功，应答成功
                1: 发送失败
                2: 发送超时
  Others:       
******************************************************************************/
static uint8_t SendHjt212Package(uint8_t conn_id)
{
    uint16_t j, k;
    char str_tmp[7];

    /* 计算数据段长度 */
    k = strlen((char *)(&g_u8_PkgBuf[6]));

    /* 包头和数据段长度 */
    sprintf(str_tmp, "##%04d", k);
    memcpy(g_u8_PkgBuf, str_tmp, 6);

    /* 计算CRC */
    j = cal_crc((char *)(&g_u8_PkgBuf[6]), k);
    sprintf(str_tmp, "%04X\r\n", j);                                 //CRC
    strcat(g_u8_PkgBuf, str_tmp);

    //		TcpClinet_Send(TcpClientTcb[conn_id].connect, g_u8_PkgBuf, k+12,12000);
    return ME_TCPSendString(conn_id, g_u8_PkgBuf, 1000);
}

/******************************************************************************
  Function:     cal_crc
  Description:  根据212协议，计算CRC
  Input:        unsigned char *ptr 指向需要计算CRC的数组地址
                int len 需要计算的字节数
  Return:       CRC(Hex)
  Others:       
******************************************************************************/
/*
uint16_t cal_crc(char *ptr, int len)
{
   unsigned int crc;
   int i;
   unsigned int hib;
   crc = 0xffff;

   while (len-- != 0)
   {
      hib = crc >> 8;
      crc = (hib ^ *ptr) & 0x00ff;

      for (i = 0; i < 8; i++)
      {
         if (crc & 0x0001)
         {
            crc >>= 1;
            crc = crc ^ 0xa001;
         }
         else
         {
            crc >>= 1;
         }
      }
      ptr++;
   }
   return (crc);
}
*/
static uint16_t cal_crc(char *ptr, int len)
{
    unsigned int crc = 0xffff;
    unsigned char crc_H8;

    while (len--)
    {
        crc_H8 = (unsigned char)(crc >> 8);
        crc >>= 8;
        crc = CRC212_Tab[crc_H8 ^ *ptr];
        ptr++;
    }
    return crc;
}

/*---------------------------------------------------------------------------*/
/*                            function code                                  */
/*---------------------------------------------------------------------------*/


/******************************************************************************
  Function:     ConvertNum
  Description:  把表示无符号16进制数据的字符串转换为unsigned int
  Input:        char *str 指向字符串的指针
                unsigned char length 字符数
  Return:       unsigned int 转换后的16进制数
  Others:       
******************************************************************************/
uint32_t ConvertNum(char *str, unsigned char length)
{
    uint8_t i;
    uint32_t sum;

    sum = 0;
    for (i = 0; i < length; i++)
    {
        if (*str >= '0' && *str <= '9')
        {
            sum *= 16;
            sum += *str - '0';
            str++;
        }
        else if (*str >= 'a' && *str <= 'f')
        {
            sum *= 16;
            sum += *str - 'a' + 10;
            str++;
        }
        else if (*str >= 'A' && *str <= 'F')
        {
            sum *= 16;
            sum += *str - 'A' + 10;
            str++;
        }
    }
    return sum;
}

/******************************************************************************
  Function:     UpLoadData
  Description:  上传污物实时数据到服务器
  Input:        uint8_t conn_id 上传数据所用的链路ID号 0-3(目前仅支持4个链接)
  Return:       none
  Others:       none
******************************************************************************/
void UpLoadData(uint8_t conn_id)
{
    sprintf((char *)(&g_u8_PkgBuf[6]), "ST=%02d;CN=2011;PW=%6d;MN=%s;Flag=0;CP=&&DataTime=%04d%02d%02d%02d%02d%02d;"
            "SB1-Status=0,SB1-Rtd=0.0,SB1-Flag=N;SB2-Status=0,SB2-Rtd=0.0,SB2-Flag=N;"
            "SB3-Status=0,SB3-Rtd=0.0,SB3-Flag=N;SB4-Status=0,SB4-Rtd=0.0,SB4-Flag=N;"
            "SB5-Status=0,SB5-Rtd=%d.%01d,SB5-Flag=N;SB6-Status=0,SB6-Rtd=%d.%01d,SB6-Flag=N;"
            "SB7-Status=0,SB7-Rtd=%d.%01d,SB7-Flag=N;SB8-Status=0,SB8-Rtd=%d.%01d,SB8-Flag=N&&",\
                g_tParam.u8_ST, g_tParam.Pwd, g_tParam.DevNO, 2000 + DateCurrent.Year, DateCurrent.Month, DateCurrent.Date,\
                TimeCurrent.Hours, TimeCurrent.Minutes, TimeCurrent.Seconds,
            AinValue[4] / 1000, AinValue[4] % 1000 / 100, AinValue[5] / 1000, AinValue[5] % 1000 / 100,\
                AinValue[6] / 1000, AinValue[6] % 1000 / 100, AinValue[7] / 1000, AinValue[7] % 1000 / 100);

    SendHjt212Package(conn_id);
}


/******************************************************************************
  Function:     QnReturn
  Description:  212协议应答函数
  Input:        uint8_t conn_id 链接ID号
                char * PtQn 指向QN号的字符串
  Return:       none
  Others:       none
******************************************************************************/
void QnReturn(uint8_t conn_id, char *PtQn)
{
    /* 数据段内容 */
    sprintf((char *)(&g_u8_PkgBuf[6]), "ST=%02d;CN=9011;PW=%6d;MN=%s;Flag=0;CP=&&QN=%s;QnRtn=1&&",\
                g_tParam.u8_ST, g_tParam.Pwd, g_tParam.DevNO, PtQn);

    SendHjt212Package(conn_id);
}

/******************************************************************************
  Function:     ExeReturn
  Description:  212协议应答函数
  Input:        uint8_t conn_id 链接ID号
                char * PtQn 指向QN号的字符串
                uint8_t ExeRtn 1:执行成功;0:执行失败;100:没有数据
  Return:       none
  Others:       none
******************************************************************************/
void ExeReturn(uint8_t conn_id, char *PtQn, uint8_t ExeRtn)
{
    /* 数据段内容 */
    sprintf((char *)(&g_u8_PkgBuf[6]), "ST=%02d;CN=9012;PW=%6d;MN=%s;Flag=0;CP=&&QN=%s;ExeRtn=%d&&",
            g_tParam.u8_ST, g_tParam.Pwd, g_tParam.DevNO, PtQn, ExeRtn);

    SendHjt212Package(conn_id);
}

/******************************************************************************
  Function:     DateAnalyzer
  Description:  解析CSV文件中的一行数据字符,转换为上传数据格式并且上传
  Input:        uint8_t conn_id 链接ID号
                const char* str 要解析的字符串
  Return:       none
  Others:       none
******************************************************************************/
void DateAnalyzer(uint8_t conn_id, const char *str)
{
    char str_tmp[20];
    uint8_t i;
    //  uint16_t j, k;
    //  uint8_t res;

    sprintf((char *)(&g_u8_PkgBuf[6]), "ST=%02d;CN=2011;PW=%6d;MN=%s;Flag=0;CP=&&DataTime=",\
                g_tParam.u8_ST, g_tParam.Pwd, g_tParam.DevNO);

    strncat((char *)(&g_u8_PkgBuf[6]), str, 8);                      //拷贝日期
    str += 9;
    strncat((char *)(&g_u8_PkgBuf[6]), str, 6);                      //拷贝时间
    str += 7;
    strcat((char *)(&g_u8_PkgBuf[6]), ";SB1-Status=0,SB1-Rtd=");
    i = 0;
    while (((*str >= '0' && *str <= '9') || *str == '+' || *str == '-' || *str == '.') && i < 20)
    {
        str_tmp[i++] = *str++;
    }
    str_tmp[i] = '\0';
    strcat((char *)(&g_u8_PkgBuf[6]), str_tmp);                      //SB1-Rtd

    strcat((char *)(&g_u8_PkgBuf[6]), ",SB1-Flag=N;SB2-Status=0,SB2-Rtd=");
    str++;                                                           //跳过','
    i = 0;
    while (((*str >= '0' && *str <= '9') || *str == '+' || *str == '-' || *str == '.') && i < 20)
    {
        str_tmp[i++] = *str++;
    }
    str_tmp[i] = '\0';
    strcat((char *)(&g_u8_PkgBuf[6]), str_tmp);                      //SB2-Rtd

    strcat((char *)(&g_u8_PkgBuf[6]), ",SB2-Flag=N;SB3-Status=0,SB3-Rtd=");
    str++;                                                           //跳过','
    i = 0;
    while (((*str >= '0' && *str <= '9') || *str == '+' || *str == '-' || *str == '.') && i < 20)
    {
        str_tmp[i++] = *str++;
    }
    str_tmp[i] = '\0';
    strcat((char *)(&g_u8_PkgBuf[6]), str_tmp);                      //SB3-Rtd

    strcat((char *)(&g_u8_PkgBuf[6]), ",SB3-Flag=N;SB4-Status=0,SB4-Rtd=");
    str++;                                                           //跳过','
    i = 0;
    while (((*str >= '0' && *str <= '9') || *str == '+' || *str == '-' || *str == '.') && i < 20)
    {
        str_tmp[i++] = *str++;
    }
    str_tmp[i] = '\0';
    strcat((char *)(&g_u8_PkgBuf[6]), str_tmp);                      //SB4-Rtd

    strcat((char *)(&g_u8_PkgBuf[6]), ",SB4-Flag=N;SB5-Status=0,SB5-Rtd=");
    str++;                                                           //跳过','
    i = 0;
    while (((*str >= '0' && *str <= '9') || *str == '+' || *str == '-' || *str == '.') && i < 20)
    {
        str_tmp[i++] = *str++;
    }
    str_tmp[i] = '\0';
    strcat((char *)(&g_u8_PkgBuf[6]), str_tmp);                      //SB5-Rtd

    strcat((char *)(&g_u8_PkgBuf[6]), ",SB5-Flag=N;SB6-Status=0,SB6-Rtd=");
    str++;                                                           //跳过','
    i = 0;
    while (((*str >= '0' && *str <= '9') || *str == '+' || *str == '-' || *str == '.') && i < 20)
    {
        str_tmp[i++] = *str++;
    }
    str_tmp[i] = '\0';
    strcat((char *)(&g_u8_PkgBuf[6]), str_tmp);                      //SB6-Rtd

    strcat((char *)(&g_u8_PkgBuf[6]), ",SB6-Flag=N;SB7-Status=0,SB7-Rtd=");
    str++;                                                           //跳过','
    i = 0;
    while (((*str >= '0' && *str <= '9') || *str == '+' || *str == '-' || *str == '.') && i < 20)
    {
        str_tmp[i++] = *str++;
    }
    str_tmp[i] = '\0';
    strcat((char *)(&g_u8_PkgBuf[6]), str_tmp);                      //SB7-Rtd

    strcat((char *)(&g_u8_PkgBuf[6]), ",SB7-Flag=N;SB8-Status=0,SB8-Rtd=");
    str++;                                                           //跳过','
    i = 0;
    while (((*str >= '0' && *str <= '9') || *str == '+' || *str == '-' || *str == '.') && i < 20)
    {
        str_tmp[i++] = *str++;
    }
    str_tmp[i] = '\0';
    strcat((char *)(&g_u8_PkgBuf[6]), str_tmp);                      //SB8-Rtd

    strcat((char *)(&g_u8_PkgBuf[6]), ",SB8-Flag=N&&");

    SendHjt212Package(conn_id);
}

/*************************************************
  Function:     HJ212_CheckData
  Description:  按照HJ212协议检查数据,但不进行解析
                注：数据段无包头包尾的特殊字符。数据长度段必须为0~9的字符
                   包头包尾必须无误，数据包长度必须无误
  Input:        char dat        要检查的数据数组
                uint16_t len    数据长度
  Output:       none
  Return:       0: 解析成功,数据符合HJ212协议
                1: 数据长度不符合
                2: 数据出错
                3: crc错误
                4: 包头包尾错误
  Others:       
*************************************************/
uint8_t HJ212_CheckData(char dat[], uint16_t len)
{
    uint8_t i;
    uint16_t num, crc;
    char tmp[5];
    char *p;

    //在前面必须要先检查一下数据包是否小于最小长度
    if (len < 12)
    {
#if GPRS_DEBUG_EN
        DEBUG_PrintfString("HJ212_CheckData: len error.\n");
#endif
        return 1;                                                    //数据段长度不合法
    }
    //接收包头
    if (dat[0] != '#' && dat[1] != '#')                              //包头
    {
#if GPRS_DEBUG_EN
        DEBUG_PrintfString("HJ212_CheckData: pack head error.\n");
#endif
        return 4;                                                    //包头出错
    }

    for (i = 0, p = &dat[2]; i < 4; i++)                             //计算数据包总长度
    {
        if (*p >= '0' && *p <= '9')                                  //合法字符
        {
            tmp[i] = *p++;
        }
        else
        {
#if GPRS_DEBUG_EN
            DEBUG_PrintfString("HJ212_CheckData: char error1.\n");
#endif
            return 2;                                                //数据出错
        }
    }
    tmp[4] = 0;
    num = atoi(tmp);
    if (num >= 1024)
    {
#if GPRS_DEBUG_EN
        DEBUG_PrintfString("HJ212_CheckData: length over.\n");
#endif
        return 1;                                                    //数据段长度不合法
    }

    if (num + 12 != len)
    {
#if GPRS_DEBUG_EN
        DEBUG_PrintfString("HJ212_CheckData: length error.\n");
#endif
        return 1;                                                    //数据段长度不合法
    }

    //校验CRC
    for (i = 0, p = &dat[num + 6]; i < 4; i++)                       //获取报文中的crc值
    {
        if ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')) //合法字符
        {
            tmp[i] = *p++;
        }
        else
        {
#if GPRS_DEBUG_EN
            DEBUG_PrintfString("HJ212_CheckData: char error2.\n");
#endif
            return 2;                                                //数据出错
        }
    }
    tmp[4] = '\0';
    crc = ConvertNum(tmp, 4);                                        //获取报文中的CRC

    if (cal_crc(&dat[6], num) != crc)                                //计算报文的CRC
    {
#if GPRS_DEBUG_EN
        DEBUG_PrintfString("HJ212_CheckData: CRC error.\n");
#endif
        return 3;                                                    //CRC error
    }

    //检查包尾
    if (dat[len - 2] != '\r' || dat[len - 1] != '\n')                //包头
    {
#if GPRS_DEBUG_EN
        DEBUG_PrintfString("HJ212_CheckData: pack tail error.\n");
#endif
        return 4;                                                    //包尾出错
    }
    return 0;
}

/*************************************************
  Function:     HJ212_DataHandle
  Description:  对HJ212协议的数据进行解析和处理
  Input:        uint8_t conn_id 链接ID号
                char dat[]      需要处理的数据数组
  Return:       0:正常
                1:缓冲区数据溢出
                2:MN出错
                3:PW出错
                4:无合法命令CN
                5:无QN号
  Others:       目前没有校对密码和ST
*************************************************/
uint8_t HJ212_DataHandle(uint8_t conn_id, char *dat)
{
    uint16_t i, j;
    uint32_t x;
    char *p;
    char str_tmp[21];
    char str1[21], str2[200];
    char str_search[21];
    char qn_tmp[18];
    uint16_t cmd;
    uint8_t ExeRtn = 0;
    uint32_t begin_time, end_time;
    uint32_t begin_day;
    uint32_t end_day;
    FIL fs;


    //校对设备编号MN
    p = strstr(dat, "MN=");
    if (p != NULL)
    {
        p += 3;
        i = 0;
        while (((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) && i < 15)
        {
            str_tmp[i++] = *p++;
        }
        str_tmp[i] = '\0';
    }
    if (strcmp(str_tmp, g_tParam.DevNO) != 0)                        //判断是否本机MN码.
    {
        /* 检查是不是广播号 */
        while (i > 0)
        {
            if (str_tmp[i--] != 0)
            {
#if GPRS_DEBUG_EN
                DEBUG_PrintfString("MN error.\n");
#endif
                return 1;                                            //MN错误，返回错误代码
            }
        }
    }

    //校对设备PW
    p = strstr(dat, "PW=");
    if (p != NULL)
    {
        p += 3;
        i = 0;
        while ((*p >= '0' && *p <= '9') && i < 9)
        {
            str_tmp[i++] = *p++;
        }
        str_tmp[i] = '\0';
    }
    x = atoi(str_tmp);

    if (x != g_tParam.Pwd)                                           //判断PW是否正确
    {
#if GPRS_DEBUG_EN
        DEBUG_PrintfString("PW error.\n");
#endif
        return 3;                                                    //PW错误，返回错误代码
    }

    //====== 处理接收到的命令============================================
    //获取命令CN
    p = strstr(dat, "CN=");
    if (p == NULL)
    {
        return 4;                                                    //无合法命令
    }
    p += 3;
    str_tmp[0] = *p++;
    str_tmp[1] = *p++;
    str_tmp[2] = *p++;
    str_tmp[3] = *p;
    str_tmp[4] = '\0';
    cmd = atoi(str_tmp);

    //获取QN
    p = strstr(dat, "QN=");
    if (p == NULL)
    {
        return 5;                                                    //无QN
    }
    p += 3;
    i = 0;
    while (*p >= '0' && *p <= '9' && i < 17)
    {
        qn_tmp[i++] = *p++;
    }
    qn_tmp[i] = '\0';

    QnReturn(conn_id, qn_tmp);                                       //回应请求


    /* 命令的处理 */
    switch (cmd)
    {
    case 1007:                                                       //重启现场机
        ExeReturn(conn_id, qn_tmp, 1);                               //返回执行结果
        /* 重启MCU */
        //      __disable_fault_irq();                                       //禁止fault中断
        __set_FAULTMASK(1);                                          // 关闭所有中端
        NVIC_SystemReset();                                          //重启MCU
        break;
    case 1008:                                                       /* 提取系统参数 */
        /* 数据段内容 */
        sprintf((char *)(&g_u8_PkgBuf[6]), "ST=%02d;CN=1008;PW=%6d;MN=%s;Flag=0;CP=&&QN=%s;SaveRtdInterval=%d;"
                "Srv1-IP=%d.%d.%d.%d;Srv1-Port=%d;Srv1-RtdInterval=%d;"
                "Srv2-IP=%d.%d.%d.%d;Srv2-Port=%d;Srv2-RtdInterval=%d;"
                "Srv3-IP=%d.%d.%d.%d;Srv3-Port=%d;Srv3-RtdInterval=%d;"
                "Srv4-IP=%d.%d.%d.%d;Srv4-Port=%d;Srv4-RtdInterval=%d;"
                "System-Flag=%08X;NetSignal=%d;FanChannel=%d;"
                "CleanChannel=%d;Version=%d&&",\
                    g_tParam.u8_ST, g_tParam.Pwd, g_tParam.DevNO, qn_tmp, g_tParam.u16_SaveRtdInterval,\
                    g_tParam.Server[0].ip[0], g_tParam.Server[0].ip[1], g_tParam.Server[0].ip[2],\
                    g_tParam.Server[0].ip[3], g_tParam.Server[0].port, g_tParam.Server[0].rtd_interval,\
                    g_tParam.Server[1].ip[0], g_tParam.Server[1].ip[1], g_tParam.Server[1].ip[2],\
                    g_tParam.Server[1].ip[3], g_tParam.Server[1].port, g_tParam.Server[1].rtd_interval,\
                    g_tParam.Server[2].ip[0], g_tParam.Server[2].ip[1], g_tParam.Server[2].ip[2],\
                    g_tParam.Server[2].ip[3], g_tParam.Server[2].port, g_tParam.Server[2].rtd_interval,\
                    g_tParam.Server[3].ip[0], g_tParam.Server[3].ip[1], g_tParam.Server[3].ip[2],\
                    g_tParam.Server[3].ip[3], g_tParam.Server[3].port, g_tParam.Server[3].rtd_interval,\
                    g_tParam.SystemFlag, GprsSignalLevel, g_tParam.FanChannel, g_tParam.CleanChannel, g_tParam.Version);
        SendHjt212Package(conn_id);
        ExeReturn(conn_id, qn_tmp, 1);                               /* 返回执行结果 */
        break;
    case 1009:                                                       /* 设置系统参数 */
        ExeRtn = 0;
        p = strstr(dat, "CP=&&");                                    //只取指令参数CP字段
        dat = p;

        p = strstr(dat, "MN=");
        if (p != NULL)
        {
            p += 3;

            /* MN号为等于小于15个字母或数字 */
            i = 0;
            while (((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) && i < 15)
            {
                str_tmp[i++] = *p++;
            }
            str_tmp[i] = '\0';
            strcpy(g_tParam.DevNO, str_tmp);
        }

        p = strstr(dat, "ST=");
        if (p != NULL)
        {
            p += 3;
            i = 0;
            while (*p >= '0' && *p <= '9' && i < 2)
            {
                str_tmp[i++] = *p++;
            }
            str_tmp[i] = '\0';
            g_tParam.u8_ST = atoi(str_tmp);
        }

        p = strstr(dat, "PW=");
        if (p != NULL)
        {
            p += 3;
            i = 0;
            while (*p >= '0' && *p <= '9' && i < 6)
            {
                str_tmp[i++] = *p++;
            }
            str_tmp[i] = '\0';
            g_tParam.Pwd = atoi(str_tmp);
        }

        p = strstr(dat, "SaveRtdInterval=");
        if (p != NULL)
        {
            p += 16;
            i = 0;
            while (*p >= '0' && *p <= '9' && i < 5)
            {
                str_tmp[i++] = *p++;
            }
            str_tmp[i] = '\0';
            g_tParam.u16_SaveRtdInterval = atoi(str_tmp);
        }

        // 服务器1
        p = strstr(dat, "Srv1-IP=");
        if (p != NULL)
        {
            GprsInitRequest = 1;                                     //请求重新初始化GPRS网络连接
            p += 8;
            for (j = 0; j < 4; j++)
            {
                i = 0;
                while (*p >= '0' && *p <= '9' && i < 3)
                {
                    str_tmp[i++] = *p++;
                }
                str_tmp[i] = '\0';
                g_tParam.Server[0].ip[j] = atoi(str_tmp);
                p++;
            }
        }
        p = strstr(dat, "Srv1-Port=");
        if (p != NULL)
        {
            GprsInitRequest = 1;                                     //请求重新初始化GPRS网络连接
            p += 10;
            i = 0;
            while (*p >= '0' && *p <= '9' && i < 5)
            {
                str_tmp[i++] = *p++;
            }
            str_tmp[i] = '\0';

            g_tParam.Server[0].port = atoi(str_tmp);
            if (g_tParam.Server[0].port == 0)
            {
                g_ucConnectEnable &= 0xfe;
            }
            else
            {
                g_ucConnectEnable |= 0x01;
            }
        }
        p = strstr(dat, "Srv1-RtdInterval=");
        if (p != NULL)
        {
            p += 17;
            i = 0;
            while (*p >= '0' && *p <= '9' && i < 5)
            {
                str_tmp[i++] = *p++;
            }
            str_tmp[i] = '\0';
            g_tParam.Server[0].rtd_interval = atoi(str_tmp);
        }

        // 服务器2
        p = strstr(dat, "Srv2-IP=");
        if (p != NULL)
        {
            GprsInitRequest = 1;                                     //请求重新初始化GPRS网络连接
            p += 8;
            for (j = 0; j < 4; j++)
            {
                i = 0;
                while (*p >= '0' && *p <= '9' && i < 3)
                {
                    str_tmp[i++] = *p++;
                }
                str_tmp[i] = '\0';
                g_tParam.Server[1].ip[j] = atoi(str_tmp);
                p++;
            }
        }
        p = strstr(dat, "Srv2-Port=");
        if (p != NULL)
        {
            GprsInitRequest = 1;                                     //请求重新初始化GPRS网络连接
            p += 10;
            i = 0;
            while (*p >= '0' && *p <= '9' && i < 5)
            {
                str_tmp[i++] = *p++;
            }
            str_tmp[i] = '\0';
            g_tParam.Server[1].port = atoi(str_tmp);
            if (g_tParam.Server[1].port == 0)
            {
                g_ucConnectEnable &= 0xfd;
            }
            else
            {
                g_ucConnectEnable |= 0x02;
            }
        }
        p = strstr(dat, "Srv2-RtdInterval=");
        if (p != NULL)
        {
            p += 17;
            i = 0;
            while (*p >= '0' && *p <= '9' && i < 5)
            {
                str_tmp[i++] = *p++;
            }
            str_tmp[i] = '\0';
            g_tParam.Server[1].rtd_interval = atoi(str_tmp);
        }

        // 服务器3
        p = strstr(dat, "Srv3-IP=");
        if (p != NULL)
        {
            GprsInitRequest = 1;                                     //请求重新初始化GPRS网络连接
            p += 8;
            for (j = 0; j < 4; j++)
            {
                i = 0;
                while (*p >= '0' && *p <= '9' && i < 3)
                {
                    str_tmp[i++] = *p++;
                }
                str_tmp[i] = '\0';
                g_tParam.Server[2].ip[j] = atoi(str_tmp);
                p++;
            }
        }
        p = strstr(dat, "Srv3-Port=");
        if (p != NULL)
        {
            GprsInitRequest = 1;                                     //请求重新初始化GPRS网络连接
            p += 10;
            i = 0;
            while (*p >= '0' && *p <= '9' && i < 5)
            {
                str_tmp[i++] = *p++;
            }
            str_tmp[i] = '\0';
            g_tParam.Server[2].port = atoi(str_tmp);
            if (g_tParam.Server[2].port == 0)
            {
                g_ucConnectEnable &= 0xfb;
            }
            else
            {
                g_ucConnectEnable |= 0x04;
            }
        }
        p = strstr(dat, "Srv3-RtdInterval=");
        if (p != NULL)
        {
            p += 17;
            i = 0;
            while (*p >= '0' && *p <= '9' && i < 5)
            {
                str_tmp[i++] = *p++;
            }
            str_tmp[i] = '\0';
            g_tParam.Server[2].rtd_interval = atoi(str_tmp);
        }

        // 服务器4
        p = strstr(dat, "Srv4-IP=");
        if (p != NULL)
        {
            GprsInitRequest = 1;                                     //请求重新初始化GPRS网络连接
            p += 8;
            for (j = 0; j < 4; j++)
            {
                i = 0;
                while (*p >= '0' && *p <= '9' && i < 3)
                {
                    str_tmp[i++] = *p++;
                }
                str_tmp[i] = '\0';
                g_tParam.Server[3].ip[j] = atoi(str_tmp);
                p++;
            }
        }
        p = strstr(dat, "Srv4-Port=");
        if (p != NULL)
        {
            GprsInitRequest = 1;                                     //请求重新初始化GPRS网络连接
            p += 10;
            i = 0;
            while (*p >= '0' && *p <= '9' && i < 5)
            {
                str_tmp[i++] = *p++;
            }
            str_tmp[i] = '\0';
            g_tParam.Server[3].port = atoi(str_tmp);
            if (g_tParam.Server[3].port == 0)
            {
                g_ucConnectEnable &= 0xf7;
            }
            else
            {
                g_ucConnectEnable |= 0x08;
            }
        }
        p = strstr(dat, "Srv4-RtdInterval=");
        if (p != NULL)
        {
            p += 17;
            i = 0;
            while (*p >= '0' && *p <= '9' && i < 5)
            {
                str_tmp[i++] = *p++;
            }
            str_tmp[i] = '\0';
            g_tParam.Server[3].rtd_interval = atoi(str_tmp);
        }

        p = strstr(dat, "NTP-Enable=");
        if (p != NULL)
        {
            p += 11;
            i = 0;
            while (*p >= '0' && *p <= '9' && i < 1)
            {
                str_tmp[i++] = *p++;
            }
            str_tmp[i] = '\0';
            g_tParam.NTP_En = atoi(str_tmp);
        }

        p = strstr(dat, "System-Flag=");
        if (p != NULL)
        {
            p += 12;
            i = 0;
            while (((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F')) && i < 8)
            {
                str_tmp[i++] = *p++;
            }
            str_tmp[i] = '\0';
            g_tParam.SystemFlag = ConvertNum(str_tmp, i);
        }

        if (SaveParam() == 0)
        {
            ExeRtn = 1;
        }

        ExeReturn(conn_id, qn_tmp, ExeRtn);                          //返回执行结果

        break;
    case 1011:                                                       /* 提取现场机时间 */
        /* 获取时间日期 */
        HAL_RTC_GetTime(&hrtc, &TimeCurrent, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &DateCurrent, RTC_FORMAT_BIN);
        sprintf(g_u8_PkgBuf, "ST=%02d;CN=1011;PW=%6d;MN=%s;Flag=0;CP=&&QN=%s;SystemTime="
                "%04d%02d%02d%02d%02d%02d&&",\
                    g_tParam.u8_ST, g_tParam.Pwd, g_tParam.DevNO, qn_tmp,\
                    2000 + DateCurrent.Year, DateCurrent.Month, DateCurrent.Date,\
                    TimeCurrent.Hours, TimeCurrent.Minutes, TimeCurrent.Seconds);
        SendHjt212Package(conn_id);
        ExeReturn(conn_id, qn_tmp, 1);                               /* 返回执行结果 */
        break;
    case 1012:                                                       /* 执行设置时间 */
        ExeRtn = 0;
        p = strstr(dat, "SystemTime=");
        if (p != NULL)
        {
            ExeRtn = 1;
            p += 11;
            str_tmp[0] = *p++;
            str_tmp[1] = *p++;
            str_tmp[2] = *p++;
            str_tmp[3] = *p++;
            str_tmp[4] = '\0';
            j = atoi(str_tmp);
            DateCurrent.Year = j % 100;
            str_tmp[0] = *p++;
            str_tmp[1] = *p++;
            str_tmp[2] = '\0';
            j = atoi(str_tmp);
            DateCurrent.Month = j;
            str_tmp[0] = *p++;
            str_tmp[1] = *p++;
            str_tmp[2] = '\0';
            j = atoi(str_tmp);
            DateCurrent.Date = j;
            str_tmp[0] = *p++;
            str_tmp[1] = *p++;
            str_tmp[2] = '\0';
            j = atoi(str_tmp);
            TimeCurrent.Hours = j;
            str_tmp[0] = *p++;
            str_tmp[1] = *p++;
            str_tmp[2] = '\0';
            j = atoi(str_tmp);
            TimeCurrent.Minutes = j;
            str_tmp[0] = *p++;
            str_tmp[1] = *p++;
            str_tmp[2] = '\0';
            j = atoi(str_tmp);
            TimeCurrent.Seconds = j;

            /* RTC死机怎么处理？*/
            //RTC_Config();
            HAL_RTC_SetDate(&hrtc, &DateCurrent, RTC_FORMAT_BIN);
            RTC_BackupDate(&hrtc, &DateCurrent);
            HAL_RTC_SetTime(&hrtc, &TimeCurrent, RTC_FORMAT_BIN);
        }
        ExeReturn(conn_id, qn_tmp, ExeRtn);                          /* 返回执行结果 */
        break;
    case 1102:                                                       /* 获取SIM卡的IMSI号 */
        sprintf(g_u8_PkgBuf, "ST=%02d;CN=1102;PW=%6d;MN=%s;Flag=0;CP=&&QN=%s;IMSI="
                "%s&&",\
                    g_tParam.u8_ST, g_tParam.Pwd, g_tParam.DevNO, qn_tmp, IMSI);
        SendHjt212Package(conn_id);
        ExeReturn(conn_id, qn_tmp, 1);                               /* 返回执行结果 */
        break;
    case 1103:                                                       /* 获取SIM卡的号码 */
        DTU_GetSimNum(str_tmp);
        sprintf(g_u8_PkgBuf, "ST=%02d;CN=1103;PW=%6d;MN=%s;Flag=0;CP=&&QN=%s;SimNum="
                "%s&&",\
                    g_tParam.u8_ST, g_tParam.Pwd, g_tParam.DevNO, qn_tmp, str_tmp);
        SendHjt212Package(conn_id);
        ExeReturn(conn_id, qn_tmp, 1);                               /* 返回执行结果 */
        break;
    case 3015:                                                       /* 提取输入端状态 */
        sprintf(g_u8_PkgBuf, "ST=%02d;CN=3015;PW=%6d;MN=%s;Flag=0;CP=&&QN=%s;InputData=%08x&&",\
                    g_tParam.u8_ST, g_tParam.Pwd, g_tParam.DevNO, qn_tmp, ReadInput());
        SendHjt212Package(conn_id);
        ExeReturn(conn_id, qn_tmp, 1);                               /* 返回执行结果 */
        break;

    case 3016:                                                       //提取输出端口状态
        sprintf(g_u8_PkgBuf, "ST=%02d;CN=3016;PW=%6d;MN=%s;Flag=0;CP=&&QN=%s;OutputData=%08x&&",\
                    g_tParam.u8_ST, g_tParam.Pwd, g_tParam.DevNO, qn_tmp, ReadOutput());
        SendHjt212Package(conn_id);
        ExeReturn(conn_id, qn_tmp, 1);                               /* 返回执行结果 */
        break;
    case 3017:                                                       //设置输出端口状态
        ExeRtn = 0;
        p = strstr(dat, "OutputData=");
        if (p != NULL)
        {
            p += 11;

            for (i = 0; i < 8;)
            {
                if ((*p >= '0' && *p <= '9') || (*p >= 'A' && *p <= 'F') || (*p >= 'a' && *p <= 'f'))
                {
                    str_tmp[i++] = *p++;
                }
                else
                {
                    break;
                }
            }
            str_tmp[i] = '\0';
            if (i > 0)
            {
                begin_time = ConvertNum(str_tmp, i) & 0x000000ff;    /* begin_time只做临时变量,目前仅能控制前4个输出通道 */
                SetOutput(begin_time);                               /* 执行设置输出 */
            }
            ExeRtn = 1;
        }
        ExeReturn(conn_id, qn_tmp, ExeRtn);                          //返回执行结果
        break;
    case 3020:                                                       //取历史分钟数据
        p = strstr(dat, "BeginTime=");
        if (p != NULL)
        {
            p += 10;
            for (i = 0; i < 8;)                                      //取年月日
            {
                if (*p >= '0' && *p <= '9')                          //检查字符是否合法
                {
                    str_tmp[i++] = *p++;
                }
                else                                                 //数字字符非法,退出
                {
                    break;
                }
            }
            str_tmp[i] = '\0';
            begin_day = atoi(str_tmp);                               //起始日期

            for (i = 0; i < 4;)                                      //取时分
            {
                if (*p >= '0' && *p <= '9')                          //检查字符是否合法
                {
                    str_tmp[i++] = *p++;
                }
                else                                                 //数字字符非法,退出
                {
                    break;
                }
            }
            str_tmp[i] = '\0';
            begin_time = atoi(str_tmp);                              //起始时分
        }

        p = strstr(dat, "EndTime=");
        if (p != NULL)
        {
            p += 8;
            for (i = 0; i < 8;)                                      //取年月日
            {
                if (*p >= '0' && *p <= '9')                          //检查字符是否合法
                {
                    str_tmp[i++] = *p++;
                }
                else                                                 //数字字符非法,退出
                {
                    break;
                }
            }
            str_tmp[i] = '\0';
            end_day = atoi(str_tmp);                                 //结束日期

            for (i = 0; i < 4;)                                      //取时分
            {
                if (*p >= '0' && *p <= '9')                          //检查字符是否合法
                {
                    str_tmp[i++] = *p++;
                }
                else                                                 //数字字符非法,退出
                {
                    break;
                }
            }
            str_tmp[i] = '\0';
            end_time = atoi(str_tmp);                                //结束时分
        }
        //读取数据
        do
        {
            sprintf(str_search, "%08d,%04d", begin_day, begin_time); //把日期时间转换为字符串

            sprintf(str1, "%d.CSV", begin_day);
            if (f_open(&fs, str1, FA_READ) == FR_OK)                 //按照年月日信息打开相应的文件
            {
                do
                {
                    p = f_gets(str2, 200, &fs);                      //获取一行数据
                    if (strstr(str2, str_search) != NULL)
                    {
#if GPRS_DEBUG_EN
                        DEBUG_PrintfString(str2);
#endif
                        //解析数据,上传数据
                        DateAnalyzer(conn_id, str2);
                    }
                } while (p != NULL);
            }
            begin_time++;
        }while (begin_time <= end_time);
        f_close(&fs);
        break;
    default:
        break;
    }

    return 0;
}

/******************************************************************************
  Function:     QR_Update
  Description:  计算并更新二维码显示
  Input:        none
  Return:       none
  Others:       none
******************************************************************************/
void QR_Update(void)
{
    uint64_t sum;
    uint32_t time_code;
    char str_tmp[25];
    uint64_t mn_temp;

    /* 计算更新动态验证码(二维码) */
    mn_temp = atoll(g_tParam.DevNO);                                 /* 字符型MN号转为数字值 */
    time_code = DateCurrent.Month * 100000 + DateCurrent.Date * 1000 + TimeCurrent.Hours * 10 + TimeCurrent.Minutes / 10;
    time_code = time_code * 2 + 1073564;
    time_code %= 10000000;                                           /* 只保留7位时间码,但实际并不会超过7位 */
    sum = (uint64_t)time_code * 10000000;
    sum = sum + time_code + mn_temp;
    sprintf(str_tmp, "%07d%llu", time_code, sum);
    LCD_Txt(0, 17, str_tmp);
}

/************** (C) COPYRIGHT 广州正虹科技发展有限公司 ********END OF FILE****/

