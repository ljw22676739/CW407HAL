/*
*********************************************************************************************************
*
*	模块名称 : modbus底层驱动程序
*	文件名称 : bsp_modbus.h
*	版    本 : V1.0
*	说    明 : 头文件
*
*	Copyright (C), 2014-2015, 安富莱电子 www.armfly.com
*
*********************************************************************************************************
*/

#ifndef __BSP_MODBUS_H
#define __BSP_MODBUS_H

/* 借用KEY_FIFO 消息通知应用程序 */
#define MSG_485_RX			0xFE	/* 此代码值需要和按键代码、红外遥控代码同意编码，保持唯一性 */

/* 接收中断程序的工作模式 -- ModbusInitVar() 函数的形参 */
#define WKM_NO_CRC			0		/* 不校验CRC和数据长度。比如ASCII协议。超时后直接给应用层处理 */
#define WKM_MODBUS_HOST		1		/* 校验CRC,不校验地址。 供Modbus主机应用 */
#define WKM_MODBUS_DEVICE	2		/* 校验CRC，并校验本机地址。 供Modbus设备（从机）应用 */

#define SADDR485	1
#define SBAUD485	UART3_BAUD

/* 01H 读强制单线圈 */
/* 05H 写强制单线圈 */
#define REG_D01		100
#define REG_D02		101
#define REG_D03		102
#define REG_D04		103
#define REG_D05		104
#define REG_D06		105
#define REG_D07		106
#define REG_D08		107
#define REG_DXX 	REG_D08

/* 02H 读取输入状态 */
#define REG_T01		200
#define REG_T02		201
#define REG_T03		202
#define REG_T04		203
#define REG_T05		204
#define REG_T06		205
#define REG_T07		206
#define REG_T08		207
#define REG_TXX		REG_T08

/* 03H 读保持寄存器 */
/* 06H 写保持寄存器 */
/* 10H 写多个保存寄存器 */
#define SLAVE_REG_P01		0x0301
#define SLAVE_REG_P02		0x0302

/* 04H 读取输入寄存器(模拟信号) */
#define REG_A01		300
#define REG_A02		301
#define REG_A03		302
#define REG_A04		303
#define REG_A05		304
#define REG_A06		305
#define REG_A07		306
#define REG_A08		307
#define REG_AXX		REG_A08

/* RTU 应答代码 */
#define RSP_OK				0		/* 成功 */
#define RSP_ERR_CMD			0x01	/* 不支持的功能码 */
#define RSP_ERR_REG_ADDR	0x02	/* 寄存器地址错误 */
#define RSP_ERR_VALUE		0x03	/* 数据值域错误 */
#define RSP_ERR_WRITE		0x04	/* 写入失败 */

#define MODBUS_RX_SIZE		128
#define MODBUS_TX_SIZE      128

typedef struct
{
	uint8_t RxBuf[MODBUS_RX_SIZE];
	uint8_t RxCount;
	uint8_t RxStatus;
	uint8_t RxNewFlag;

	uint8_t AppRxBuf[MODBUS_RX_SIZE];
	uint8_t AppRxCount;
	uint8_t AppRxAddr;					/* 接收到的数据帧里的设备地址 */
	
	uint8_t  DevieAddr;				    /* 设备地址 */
	uint32_t Baud;
	
	uint8_t RspCode;

	uint8_t TxBuf[MODBUS_TX_SIZE];
	uint8_t TxCount;

	uint8_t WorkMode;	/* 接收中断的工作模式： ASCII, MODBUS主机或 MODBUS 从机 */
}MODBUS_T;

/* DI DO(线圈)的状态 */
typedef enum
{
    OFF = 0,
    ON
}IO_SateTypedef;

void MODBUS_InitVar(uint32_t _Baud, uint8_t _WorkMode, uint8_t _DeviceAddr);
void MODBUS_Poll(void);
void MODBUS_RxTimeOut(void);
void MODBUS_SendWithCRC(uint8_t *_pBuf, uint8_t _ucLen);
void MODBUS_ReciveNew(uint8_t _byte);		/* 被串口接收中断服务程序调用 */

extern MODBUS_T g_tModbus;

#endif

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
