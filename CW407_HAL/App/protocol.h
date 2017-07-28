/****************** Copyright(c) 广州正虹科技发展有限公司 ***********************/
/* Define to prevent recursive inclusion ------------------------------------ */
#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

/* Includes ----------------------------------------------------------------- */
#include "bsp.h"
/* Private macros ----------------------------------------------------------- */
/* Exported types ----------------------------------------------------------- */
/* Exported constants ------------------------------------------------------- */
//#define CONNECT_BUFFER_SIZE 1032
/* Exported macros ---------------------------------------------------------- */
/* Exported variables ------------------------------------------------------- */
//extern uint8_t  USART1_RecData[CONNECT_BUFFER_SIZE];//保存USART1接收到的数据，等待处理
//extern uint16_t USART1_RecDataCnt; //保存接收到的字节数
//extern uint8_t  USART1_RecDataOver;//1:USART1接收数据保存区溢出（暂未使用）
//
//extern uint8_t  USART1_SendBuffer[CONNECT_BUFFER_SIZE];//USART1 send buffer
//extern uint16_t USART1_SendBufferCnt;
//
//extern uint8_t  USART1_SendState;//0：未发送状态;1:正在发送状态 暂未使用
//
//extern uint16_t USART1_RecTimeOutCnt;//接收数据包时超时计时器，超过一定值则原来的数据包
//                                        //清零，重新接收
//extern uint8_t  USART1_RecFrameDone; //1:接收完一帧数据包;0:未接收到数据包

extern char g_u8_PkgBuf[];

/* Exported functions ------------------------------------------------------- */
uint32_t ConvertNum(char *str,unsigned char length);
void UpLoadData(uint8_t conn_id);
void QnReturn(uint8_t conn_id, char *PtQn);
void ExeReturn(uint8_t conn_id, char * PtQn, uint8_t ExeRtn);
void DateAnalyzer(uint8_t conn_id, const char *str);
uint8_t HJ212_CheckData(char dat[], uint16_t len);
uint8_t HJ212_DataHandle(uint8_t conn_id, char dat[]);
void QR_Update(void);

#endif /*__PROTOCOL_H__*/

/********* (C) COPYRIGHT 2015-2025 广州正虹科技发展有限公司 *******END OF FILE****/

