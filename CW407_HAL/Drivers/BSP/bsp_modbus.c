/*
*********************************************************************************************************
*
*	模块名称 : modbus底层驱动程序
*	文件名称 : bsp_modbus.c
*	版    本 : V1.0
*	说    明 : Modbus驱动程序，提供收发的函数。
*
*	Copyright (C), 2014-2015, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/
/* Includes -----------------------------------------------------------------*/
#include "bsp.h"
/* Private function prototypes ----------------------------------------------*/
static void MODBUS_SendWithCRC(uint8_t *_pBuf, uint8_t _ucLen);
static void MODBUS_SendAckErr(uint8_t _ucErrCode);
static void MODBUS_SendAckOk(void);
/* Private variables --------------------------------------------------------*/
static uint8_t g_rtu_timeout = 0;
/* Global variables ---------------------------------------------------------*/
MODBUS_T g_tModbus;
/* Declaration of extern functions ------------------------------------------*/
extern void MODBUS_AnalyzeApp(void);

extern __IO uint16_t AdcValue[];//临时
/*
*********************************************************************************************************
*	函 数 名: MODBUS_SendWithCRC
*	功能说明: 发送一串数据, 自动追加2字节CRC
*	形    参: _pBuf 数据；
*			  _ucLen 数据长度（不带CRC）
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODBUS_SendWithCRC(uint8_t *_pBuf, uint8_t _ucLen)
{
   uint16_t crc;
   uint8_t buf[MODBUS_TX_SIZE];

   memcpy(buf, _pBuf, _ucLen);
   crc = CRC16_Modbus(_pBuf, _ucLen);
   buf[_ucLen++] = crc >> 8;
   buf[_ucLen++] = crc;
   RS485_SendBuf(buf, _ucLen);
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_SendAckErr
*	功能说明: 发送错误应答
*	形    参: _ucErrCode : 错误代码
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODBUS_SendAckErr(uint8_t _ucErrCode)
{
   uint8_t txbuf[3];

   txbuf[0] = g_tModbus.RxBuf[0];                  /* 485地址 */
   txbuf[1] = g_tModbus.RxBuf[1] | 0x80;               /* 异常的功能码 */
   txbuf[2] = _ucErrCode;                          /* 错误代码(01,02,03,04) */

   MODBUS_SendWithCRC(txbuf, 3);
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_SendAckOk
*	功能说明: 发送正确的应答.
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODBUS_SendAckOk(void)
{
   uint8_t txbuf[6];
   uint8_t i;

   for (i = 0; i < 6; i++)
   {
      txbuf[i] = g_tModbus.RxBuf[i];
   }
   MODBUS_SendWithCRC(txbuf, 6);
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_RxTimeOut
*	功能说明: 超过3.5个字符时间后执行本函数。 设置全局变量 g_rtu_timeout = 1; 通知主程序开始解码。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void MODBUS_RxTimeOut(void)
{
   g_rtu_timeout = 1;
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_InitVar
*	功能说明: 初始化Modbus结构变量
*   形    参: _Baud 通信波特率，改参数决定了RTU协议包间的超时时间。3.5个字符。us*            
*             _WorkMode 接收中断处理模式1. RXM_NO_CRC   RXM_MODBUS_HOST   RXM_MODBUS_DEVICE
*             _DeviceAddr  站地址,如果设置为主机,该参数为0
*
*	返 回 值: 无
*********************************************************************************************************
*/
void MODBUS_InitVar(uint32_t _Baud, uint8_t _WorkMode, uint8_t _DeviceAddr)
{
   g_rtu_timeout = 0;
   g_tModbus.RxCount = 0;

   g_tModbus.Baud = _Baud;

   g_tModbus.WorkMode = _WorkMode; /* 接收数据帧不进行CRC校验 */

   g_tModbus.DevieAddr = _DeviceAddr;
   //bsp_Set485Baud(_Baud);
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_Poll
*	功能说明: 解析数据包. 在主程序中轮流调用。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void MODBUS_Poll(void)
{
   uint16_t crc1;

   if (g_rtu_timeout == 0)
   {
      /* 没有超时，继续接收。不要清零 g_tModbus.RxCount */
      return;
   }
   g_rtu_timeout = 0;              /* 清标志 */

   /* 收到命令
       05 06 00 88 04 57 3B70 (8 字节)
           05    :  数码管屏的号站，
           06    :  指令
           00 88 :  数码管屏的显示寄存器
           04 57 :  数据,,,转换成 10 进制是 1111.高位在前,
           3B70  :  二个字节 CRC 码	从05到 57的校验
   */


   switch (g_tModbus.WorkMode)
   {
   case WKM_NO_CRC:    /* 接收数据帧不进行CRC校验. 用于ASCII协议 */
      {
         /* 将接收的数据复制到另外一个缓冲区，等待APP程序读取 */
         memcpy(g_tModbus.AppRxBuf, g_tModbus.RxBuf, g_tModbus.RxCount);
         g_tModbus.AppRxCount = g_tModbus.RxCount;
//  			bsp_PutKey(MSG_485_RX);		/* 借用按键FIFO，发送一个收到485数据帧的消息 */
      }
      break;

   case WKM_MODBUS_HOST:           /* Modbus 主机模式 */
      if (g_tModbus.RxCount < 4)
      {
         goto err_ret;
      }

      /* 计算CRC校验和 */
      crc1 = CRC16_Modbus(g_tModbus.RxBuf, g_tModbus.RxCount);
      if (crc1 != 0)
      {
         goto err_ret;
      }

      /* 站地址 (1字节） */
      g_tModbus.AppRxAddr = g_tModbus.RxBuf[0];   /* 第1字节 站号 */

      /* 将接收的数据复制到另外一个缓冲区，等待APP程序读取 */
      memcpy(g_tModbus.AppRxBuf, g_tModbus.RxBuf, g_tModbus.RxCount);
      g_tModbus.AppRxCount = g_tModbus.RxCount;
      break;

   case WKM_MODBUS_DEVICE:         /* Modbus 从机模式 */
      if (g_tModbus.RxCount < 4)
      {
         goto err_ret;
      }

      /* 计算CRC校验和 */
      crc1 = CRC16_Modbus(g_tModbus.RxBuf, g_tModbus.RxCount);
      if (crc1 != 0)
      {
         goto err_ret;
      }

      /* 站地址 (1字节） */
      g_tModbus.AppRxAddr = g_tModbus.RxBuf[0];           /* 第1字节 站号 */
      if (g_tModbus.AppRxAddr != g_tModbus.DevieAddr)     /* 判断主机发送的命令地址是否符合 */
      {
         goto err_ret;
      }

      /* 分析应用层协议 */
      MODBUS_AnalyzeApp();
      break;

   default:
      break;
   }

err_ret:
   g_tModbus.RxCount = 0;  /* 必须清零计数器，方便下次帧同步 */
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_ReciveNew
*	功能说明: 串口接收中断服务程序会调用本函数。当收到一个字节时，执行一次本函数。
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void MODBUS_ReciveNew(uint8_t _byte)
{
   /*
       3.5个字符的时间间隔，只是用在RTU模式下面，因为RTU模式没有开始符和结束符，
       两个数据包之间只能靠时间间隔来区分，Modbus定义在不同的波特率下，间隔时间是不一样的，
       所以就是3.5个字符的时间，波特率高，这个时间间隔就小，波特率低，这个时间间隔相应就大

       4800  = 7.297ms
       9600  = 3.646ms
       19200  = 1.771ms
       38400  = 0.885ms
   */
   uint32_t timeout;

   timeout = 35000000 / g_tModbus.Baud;        /* 计算超时时间，单位us */

   /* 硬件定时中断，定时精度us 定时器4用于Modbus */
//	bsp_StartHardTimer(4, timeout, (void *)MODBUS_RxTimeOut);

   htim5.Instance->CNT = 0;
//	htim5.Instance = TIM5;
//	htim5.Init.Prescaler = HAL_RCC_GetHCLKFreq()/1000000 - 1;/* ms*/
//	htim5.Init.CounterMode = TIM_COUNTERMODE_UP;
   htim5.Init.Period = timeout;
//	htim5.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
   HAL_TIM_Base_Init(&htim5);

   /* 重启定时器 500ms后在中断关闭LED,并关闭定时器 */

   HAL_TIM_Base_Start_IT(&htim5);


   if (g_tModbus.RxCount < MODBUS_RX_SIZE)
   {
      g_tModbus.RxBuf[g_tModbus.RxCount++] = _byte;
   }
}



/* Modbus 应用层解码示范。下面的代码请放在主程序做 */

/*
*********************************************************************************************************
*	函 数 名: MODBUS_01H
*	功能说明: 读取线圈状态（对应远程开关D01/D02/D03）
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODBUS_01H(void)
{
   /*
    举例：
       主机发送:
           11 从机地址
           01 功能码
           00 寄存器起始地址高字节
           13 寄存器起始地址低字节
           00 寄存器数量高字节
           25 寄存器数量低字节
           0E CRC校验高字节
           84 CRC校验低字节

       从机应答: 	1代表ON，0代表OFF。若返回的线圈数不为8的倍数，则在最后数据字节未尾使用0代替. BIT0对应第1个
           11 从机地址
           01 功能码
           05 返回字节数
           CD 数据1(线圈0013H-线圈001AH)
           6B 数据2(线圈001BH-线圈0022H)
           B2 数据3(线圈0023H-线圈002AH)
           0E 数据4(线圈0032H-线圈002BH)
           1B 数据5(线圈0037H-线圈0033H)
           45 CRC校验高字节
           E6 CRC校验低字节

       例子:
           01 01 10 01 00 03   29 0B	--- 查询D01开始的3个继电器状态
           01 01 10 03 00 01   09 0A   --- 查询D03继电器的状态
   */
   uint16_t reg;
   uint16_t num;
   uint16_t i;
   uint16_t m;
   uint8_t status[10];

   g_tModbus.RspCode = RSP_OK;

   /* 没有外部继电器，直接应答错误 */
   if (g_tModbus.RxCount != 8)
   {
      g_tModbus.RspCode = RSP_ERR_VALUE;                /* 数据值域错误 */
      return;
   }

   reg = BEBufToUint16(&g_tModbus.RxBuf[2]);             /* 寄存器号 */
   num = BEBufToUint16(&g_tModbus.RxBuf[4]);             /* 寄存器个数 */

   m = (num + 7) / 8;

   if ((reg >= REG_D01) && (num > 0) && (reg + num <= REG_DXX + 1))
   {
      for (i = 0; i < m; i++)
      {
         status[i] = 0;
      }
      for (i = 0; i < num; i++)
      {
//			if (bsp_IsLedOn(i + 1 + reg - REG_D01))		/* 读LED的状态，写入状态寄存器的每一位 */
//			{
//				status[i / 8] |= (1 << (i % 8));
//			}
      }
   }
   else
   {
      g_tModbus.RspCode = RSP_ERR_REG_ADDR;             /* 寄存器地址错误 */
   }

   status[0] = 0xaa; //调试

   if (g_tModbus.RspCode == RSP_OK)                      /* 正确应答 */
   {
      g_tModbus.TxCount = 0;
      g_tModbus.TxBuf[g_tModbus.TxCount++] = g_tModbus.RxBuf[0];
      g_tModbus.TxBuf[g_tModbus.TxCount++] = g_tModbus.RxBuf[1];
      g_tModbus.TxBuf[g_tModbus.TxCount++] = m;           /* 返回字节数 */

      for (i = 0; i < m; i++)
      {
         g_tModbus.TxBuf[g_tModbus.TxCount++] = status[i];   /* 继电器状态 */
      }
      MODBUS_SendWithCRC(g_tModbus.TxBuf, g_tModbus.TxCount);
   }
   else
   {
      MODBUS_SendAckErr(g_tModbus.RspCode);               /* 告诉主机命令错误 */
   }
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_02H
*	功能说明: 读取输入状态（对应T01～T18）
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODBUS_02H(void)
{
   /*
       主机发送:
           11 从机地址
           02 功能码
           00 寄存器地址高字节
           C4 寄存器地址低字节
           00 寄存器数量高字节
           16 寄存器数量低字节
           BA CRC校验高字节
           A9 CRC校验低字节

       从机应答:  响应各离散输入寄存器状态，分别对应数据区中的每位值，1 代表ON；0 代表OFF。
                  第一个数据字节的LSB(最低字节)为查询的寻址地址，其他输入口按顺序在该字节中由低字节
                  向高字节排列，直到填充满8位。下一个字节中的8个输入位也是从低字节到高字节排列。
                  若返回的输入位数不是8的倍数，则在最后的数据字节中的剩余位至该字节的最高位使用0填充。
           11 从机地址
           02 功能码
           03 返回字节数
           AC 数据1(00C4H-00CBH)
           DB 数据2(00CCH-00D3H)
           35 数据3(00D4H-00D9H)
           20 CRC校验高字节
           18 CRC校验低字节

       例子:
       01 02 20 01 00 08  23CC  ---- 读取T01-08的状态
       01 02 20 04 00 02  B3CA  ---- 读取T04-05的状态
       01 02 20 01 00 12  A207   ---- 读 T01-18
   */

   uint16_t reg;
   uint16_t num;
   uint16_t i;
   uint16_t m;
   uint8_t status[10];

   g_tModbus.RspCode = RSP_OK;

   if (g_tModbus.RxCount != 8)
   {
      g_tModbus.RspCode = RSP_ERR_VALUE;              /* 数据值域错误 */
      return;
   }

   reg = BEBufToUint16(&g_tModbus.RxBuf[2]);           /* 寄存器号 */
   num = BEBufToUint16(&g_tModbus.RxBuf[4]);           /* 寄存器个数 */

   m = (num + 7) / 8;
   if ((reg >= REG_T01) && (num > 0) && (reg + num <= REG_TXX + 1))
   {
      for (i = 0; i < m; i++)
      {
         status[i] = 0;
      }
      for (i = 0; i < num; i++)
      {
         if (ReadDI(reg - REG_T01 + i) == ON)
         {
            status[i / 8] |= (1 << (i % 8));
         }
      }
   }
   else
   {
      g_tModbus.RspCode = RSP_ERR_REG_ADDR;               /* 寄存器地址错误 */
   }

   if (g_tModbus.RspCode == RSP_OK)                        /* 正确应答 */
   {
      g_tModbus.TxCount = 0;
      g_tModbus.TxBuf[g_tModbus.TxCount++] = g_tModbus.RxBuf[0];
      g_tModbus.TxBuf[g_tModbus.TxCount++] = g_tModbus.RxBuf[1];
      g_tModbus.TxBuf[g_tModbus.TxCount++] = m;           /* 返回字节数 */

      for (i = 0; i < m; i++)
      {
         g_tModbus.TxBuf[g_tModbus.TxCount++] = status[i];   /* T01-02状态 */
      }
      MODBUS_SendWithCRC(g_tModbus.TxBuf, g_tModbus.TxCount);
   }
   else
   {
      MODBUS_SendAckErr(g_tModbus.RspCode);               /* 告诉主机命令错误 */
   }
}
/*
*********************************************************************************************************
*	函 数 名: MODBUS_03H
*	功能说明: 读取保持寄存器 在一个或多个保持寄存器中取得当前的二进制值
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODBUS_03H(void)
{
   /*
       从机地址为11H。保持寄存器的起始地址为006BH，结束地址为006DH。该次查询总共访问3个保持寄存器。

       主机发送:
           11 从机地址
           03 功能码
           00 寄存器地址高字节
           6B 寄存器地址低字节
           00 寄存器数量高字节
           03 寄存器数量低字节
           76 CRC高字节
           87 CRC低字节

       从机应答: 	保持寄存器的长度为2个字节。对于单个保持寄存器而言，寄存器高字节数据先被传输，
                   低字节数据后被传输。保持寄存器之间，低地址寄存器先被传输，高地址寄存器后被传输。
           11 从机地址
           03 功能码
           06 字节数
           00 数据1高字节(006BH)
           6B 数据1低字节(006BH)
           00 数据2高字节(006CH)
           13 数据2 低字节(006CH)
           00 数据3高字节(006DH)
           00 数据3低字节(006DH)
           38 CRC高字节
           B9 CRC低字节

       例子:
           01 03 30 06 00 01  6B0B      ---- 读 3006H, 触发电流
           01 03 4000 0010 51C6         ---- 读 4000H 倒数第1条浪涌记录 32字节
           01 03 4001 0010 0006         ---- 读 4001H 倒数第1条浪涌记录 32字节

           01 03 F000 0008 770C         ---- 读 F000H 倒数第1条告警记录 16字节
           01 03 F001 0008 26CC         ---- 读 F001H 倒数第2条告警记录 16字节

           01 03 7000 0020 5ED2         ---- 读 7000H 倒数第1条波形记录第1段 64字节
           01 03 7001 0020 0F12         ---- 读 7001H 倒数第1条波形记录第2段 64字节

           01 03 7040 0020 5F06         ---- 读 7040H 倒数第2条波形记录第1段 64字节
   */
   uint16_t reg;
   uint16_t num;
   uint16_t i;
   uint8_t reg_value[64];

   g_tModbus.RspCode = RSP_OK;

   if (g_tModbus.RxCount != 8)                             /* 03H命令必须是8个字节 */
   {
      g_tModbus.RspCode = RSP_ERR_VALUE;                  /* 数据值域错误 */
      goto err_ret;
   }

   reg = BEBufToUint16(&g_tModbus.RxBuf[2]);               /* 寄存器号 */
   num = BEBufToUint16(&g_tModbus.RxBuf[4]);                   /* 寄存器个数 */
   if (num > sizeof(reg_value) / 2)
   {
      g_tModbus.RspCode = RSP_ERR_VALUE;                  /* 数据值域错误 */
      goto err_ret;
   }

   for (i = 0; i < num; i++)
   {
//		if (MODBUS_ReadRegValue(reg, &reg_value[2 * i]) == 0)	/* 读出寄存器值放入reg_value */
//		{
//			g_tModbus.RspCode = RSP_ERR_REG_ADDR;				/* 寄存器地址错误 */
//			break;
//		}
      reg++;
   }

err_ret:
   if (g_tModbus.RspCode == RSP_OK)                            /* 正确应答 */
   {
      g_tModbus.TxCount = 0;
      g_tModbus.TxBuf[g_tModbus.TxCount++] = g_tModbus.RxBuf[0];
      g_tModbus.TxBuf[g_tModbus.TxCount++] = g_tModbus.RxBuf[1];
      g_tModbus.TxBuf[g_tModbus.TxCount++] = num * 2;         /* 返回字节数 */

      for (i = 0; i < num; i++)
      {
         g_tModbus.TxBuf[g_tModbus.TxCount++] = reg_value[2 * i];
         g_tModbus.TxBuf[g_tModbus.TxCount++] = reg_value[2 * i + 1];
      }
      MODBUS_SendWithCRC(g_tModbus.TxBuf, g_tModbus.TxCount); /* 发送正确应答 */
   }
   else
   {
      MODBUS_SendAckErr(g_tModbus.RspCode);                   /* 发送错误应答 */
   }
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_04H
*	功能说明: 读取输入寄存器（对应A01/A02） SMA
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODBUS_04H(void)
{
   /*
       主机发送:
           11 从机地址
           04 功能码
           00 寄存器起始地址高字节
           08 寄存器起始地址低字节
           00 寄存器个数高字节
           02 寄存器个数低字节
           F2 CRC高字节
           99 CRC低字节

       从机应答:  输入寄存器长度为2个字节。对于单个输入寄存器而言，寄存器高字节数据先被传输，
               低字节数据后被传输。输入寄存器之间，低地址寄存器先被传输，高地址寄存器后被传输。
           11 从机地址
           04 功能码
           04 字节数
           00 数据1高字节(0008H)
           0A 数据1低字节(0008H)
           00 数据2高字节(0009H)
           0B 数据2低字节(0009H)
           8B CRC高字节
           80 CRC低字节

       例子:

           01 04 2201 0006 2BB0  --- 读 2201H A01通道模拟量 开始的6个数据
           01 04 2201 0001 6A72  --- 读 2201H

   */
   uint16_t reg;
   uint16_t num;
   uint16_t i;
   uint16_t status[10];

   memset(status, 0, 10);

   g_tModbus.RspCode = RSP_OK;

   if (g_tModbus.RxCount != 8)
   {
      g_tModbus.RspCode = RSP_ERR_VALUE;  /* 数据值域错误 */
      goto err_ret;
   }

   reg = BEBufToUint16(&g_tModbus.RxBuf[2]);   /* 寄存器号 */
   num = BEBufToUint16(&g_tModbus.RxBuf[4]);   /* 寄存器个数 */

   if ((reg >= REG_A01) && (num > 0) && (reg + num <= REG_AXX + 1))
   {
      for (i = 0; i < num; i++)
      {
         switch (reg)
         {
            /* 测试参数 */
         case REG_A01:
            status[i] = AdcValue[0];
            break;
         case REG_A02:
            status[i] = AdcValue[1];
            break;
         case REG_A03:
            status[i] = AdcValue[2];
            break; 
         case REG_A04:
            status[i] = AdcValue[3];
            break;
         case REG_A05:
            status[i] = AdcValue[4];
            break;
         case REG_A06:
            status[i] = AdcValue[5];
            break;
         case REG_A07:
            status[i] = AdcValue[6];
            break;
         case REG_A08:
            status[i] = AdcValue[7];
            break;

         default:
            status[i] = 0;
            break;
         }
         reg++;
      }
   }
   else
   {
      g_tModbus.RspCode = RSP_ERR_REG_ADDR;       /* 寄存器地址错误 */
   }

err_ret:
   if (g_tModbus.RspCode == RSP_OK)        /* 正确应答 */
   {
      g_tModbus.TxCount = 0;
      g_tModbus.TxBuf[g_tModbus.TxCount++] = g_tModbus.RxBuf[0];
      g_tModbus.TxBuf[g_tModbus.TxCount++] = g_tModbus.RxBuf[1];
      g_tModbus.TxBuf[g_tModbus.TxCount++] = num * 2;         /* 返回字节数 */

      for (i = 0; i < num; i++)
      {
         g_tModbus.TxBuf[g_tModbus.TxCount++] = status[i] >> 8;
         g_tModbus.TxBuf[g_tModbus.TxCount++] = status[i] & 0xFF;
      }
      MODBUS_SendWithCRC(g_tModbus.TxBuf, g_tModbus.TxCount);
   }
   else
   {
      MODBUS_SendAckErr(g_tModbus.RspCode);   /* 告诉主机命令错误 */
   }
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_05H
*	功能说明: 强制单线圈（对应D01/D02/D03）
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODBUS_05H(void)
{
   /*
       主机发送: 写单个线圈寄存器。FF00H值请求线圈处于ON状态，0000H值请求线圈处于OFF状态
       。05H指令设置单个线圈的状态，15H指令可以设置多个线圈的状态。
           11 从机地址
           05 功能码
           00 寄存器地址高字节
           AC 寄存器地址低字节
           FF 数据1高字节
           00 数据2低字节
           4E CRC校验高字节
           8B CRC校验低字节

       从机应答:
           11 从机地址
           05 功能码
           00 寄存器地址高字节
           AC 寄存器地址低字节
           FF 寄存器1高字节
           00 寄存器1低字节
           4E CRC校验高字节
           8B CRC校验低字节

       例子:
       01 05 10 01 FF 00   D93A   -- D01打开
       01 05 10 01 00 00   98CA   -- D01关闭

       01 05 10 02 FF 00   293A   -- D02打开
       01 05 10 02 00 00   68CA   -- D02关闭

       01 05 10 03 FF 00   78FA   -- D03打开
       01 05 10 03 00 00   390A   -- D03关闭
   */
   uint16_t reg;
   uint16_t value;

   g_tModbus.RspCode = RSP_OK;

   if (g_tModbus.RxCount != 8)
   {
      g_tModbus.RspCode = RSP_ERR_VALUE;      /* 数据值域错误 */
      goto err_ret;
   }

   reg = BEBufToUint16(&g_tModbus.RxBuf[2]);   /* 寄存器号 */
   value = BEBufToUint16(&g_tModbus.RxBuf[4]); /* 数据 */

   if (value != 0 && value != 0xFF00)
   {
      g_tModbus.RspCode = RSP_ERR_VALUE;      /* 数据值域错误 */
      goto err_ret;
   }

   if (value == 0xFF00)
   {
      value = 1;
   }

   if (reg == REG_D01)
   {
      SetDO1((IO_SateTypedef)value);
   }
   else if (reg == REG_D02)
   {
      SetDO2((IO_SateTypedef)value);
   }
   else if (reg == REG_D03)
   {
      SetDO3((IO_SateTypedef)value);
   }
   else if (reg == REG_D04)
   {
      SetDO4((IO_SateTypedef)value);
   }
   else if (reg == REG_D05)
   {
      SetDO5((IO_SateTypedef)value);
   }
   else if (reg == REG_D06)
   {
      SetDO6((IO_SateTypedef)value);
   }
   else if (reg == REG_D07)
   {
      SetDO7((IO_SateTypedef)value);
   }
   else if (reg == REG_D08)
   {
      SetDO8((IO_SateTypedef)value);
   }
   else
   {
      g_tModbus.RspCode = RSP_ERR_REG_ADDR;       /* 寄存器地址错误 */
   }
err_ret:
   if (g_tModbus.RspCode == RSP_OK)                /* 正确应答 */
   {
      MODBUS_SendAckOk();
   }
   else
   {
      MODBUS_SendAckErr(g_tModbus.RspCode);       /* 告诉主机命令错误 */
   }
}

/*
*********************************************************************************************************
*	函 数 名: MODBUS_06H
*	功能说明: 写单个寄存器
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODBUS_06H(void)
{

}


/*
*********************************************************************************************************
*	函 数 名: MODBUS_10H
*	功能说明: 连续写多个寄存器.  进用于改写时钟
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
static void MODBUS_10H(void)
{

}


/*
*********************************************************************************************************
*	函 数 名: MODBUS_AnalyzeApp
*	功能说明: 分析应用层协议
*	形    参:
*		     _DispBuf  存储解析到的显示数据ASCII字符串，0结束
*	返 回 值: 无
*********************************************************************************************************
*/
void MODBUS_AnalyzeApp(void)
{
   /* Modbus从机 */
   switch (g_tModbus.RxBuf[1])         /* 第2个字节 功能码 */
   {
   case 0x01:  /* 读取线圈状态（对应远程开关D01/D02/D03） */
      MODBUS_01H();
      break;

   case 0x02:  /* 读取输入状态（对应T01～T18） */
      MODBUS_02H();
      break;

   case 0x03:  /* 读取保持寄存器 在一个或多个保持寄存器中取得当前的二进制值 */
      MODBUS_03H();
      break;

   case 0x04:  /* 读取输入寄存器（对应A01/A02） ） */
      MODBUS_04H();
      break;

   case 0x05:  /* 强制单线圈（对应D01/D02/D03） */
      MODBUS_05H();
      break;

   case 0x06:  /* 写单个寄存器 (存储在EEPROM中的参数) */
      MODBUS_06H();
      break;

   case 0x10:  /* 写多个寄存器 （改写时钟） */
      MODBUS_10H();
      break;

   default:
      g_tModbus.RspCode = RSP_ERR_CMD;
      //MODBUS_SendAckErr(g_tModbus.RspCode);	/* 告诉主机命令错误 */
      break;
   }
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
