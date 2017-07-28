/*
*********************************************************************************************************
*
*	模块名称 : 应用程序参数模块
*	文件名称 : param.h
*	版    本 : V1.0
*	说    明 : 头文件
*
*	Copyright (C), 2016-2026, 广州正虹科技发展有限公司
*
*********************************************************************************************************
*/

#ifndef __PARAM_H
#define __PARAM_H

#include <stdint.h>

#define PARAM_SAVE_TO_FLASH		/* 参数存储到CPU内部Flash */

#ifdef PARAM_SAVE_TO_FLASH
#if 0  /* 大容量*/
/* 存储参数配置信息 */
#define PARAM_START_ADDR		((int32_t)0x807f800)    /* FLASH区域的最后2K */
/* 存储参数配置信息的备份区 */
#define PARAM_BACKUP_START_ADDR ((uint32_t)0x807f000)   /* FLASH区域的最后倒数第二个2K */
#else /* 中小容量芯片 */
/* 存储参数配置信息 */
#define PARAM_START_ADDR		((uint32_t)0x803f800)   /* FLASH区域的最后2K */
/* 存储参数配置信息的备份区 */
#define PARAM_BACKUP_START_ADDR ((uint32_t)0x803f000)   /* FLASH区域的最后倒数第二个2K */
#endif

//  #define PARAM_START_ADDR		ADDR_FLASH_SECTOR_3			/* 0x0800C000 中间的16KB扇区用来存放参数 */


//#define PARAM_START_ADDR	 ADDR_FLASH_SECTOR_11		/* 0x080E0000 Flash最后128K扇区用来存放参数 */
#endif

#define  FLASH_DATA_BEGIN_FLAG      0x55555555          //FLASH 数据区起始标志
#define  FLASH_DATA_END_FLAG        0xaaaaaaaa          //FLASH 数据区结束标志

typedef struct
{
	uint8_t ip[4];         //IP地址
	uint16_t port;          //端口
}IpAddr_TypeDef; //保存和服务器相关的数据(IP,端口,上传数据间隔时间)

typedef struct
{
	uint8_t ip[4];         //IP地址
	uint16_t port;          //端口
	uint16_t rtd_interval;  //上传数据的时间间隔
	struct uip_conn *uip_conn;      //uip连接的句柄
	uint16_t gprs_id;       //GPRS的连接号
	uint16_t heartbeart_period;      //心跳周期
}Srv_TypeDef; //保存和服务器相关的数据(IP,端口,上传数据间隔时间)

/* 全局参数(尽量是4字节的倍数,能适应按半字和按字写入FLASH) */
typedef struct struct_param
{
	uint32_t Version;               /* 参数区版本控制（可用于程序升级时，决定是否对参数区进行升级） */

	/* 触摸屏校准参数 */
	char DevNO[16];                 /* 保存设备编号,目前只用15位 */
	uint32_t Pwd;                   /* 保存6位密码 */

	uint8_t u8_ST;                  /* 污染源编号 */
	uint8_t NTP_En;                 /* 0:禁能NTP对时功能;1:使能NTP对时功能 */
	uint16_t u16_SaveRtdInterval;   /* 保存AD采样值的时间间隔，单位秒 */

	Srv_TypeDef Server[4];          /* 保存服务器相关的数据(IP,端口,上传数据间隔时间等) */

	uint8_t local_ip[4];            /* 本地ip */

	char apn[32];                   /* 保存上网参数APN */

	uint32_t SystemFlag;            /* bit0:(0-禁止采集;1-使能采集).注意:bit0=0会出现采集数据和上传数据都是0 */

	uint32_t threshold;             /* 风机净化器开关量阈值uA/uV */

	uint8_t FanChannel;             /* 风机的通道 */
	uint8_t CleanChannel;           /* 净化器的通道 */
	uint16_t uc;					/* 填充 */

	uint16_t Heardbeat[8];          /* 心跳包周期/秒 */
}
PARAM_T;

extern __align(8) PARAM_T g_tParam;

uint8_t LoadParam(void);
uint8_t SaveParam(void);
uint8_t SetDefaultParam(void);

#endif /* __PARAM_H */
