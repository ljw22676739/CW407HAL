/****************** Copyright(c) 广州正虹科技发展有限公司 ***********************/
/* Define to prevent recursive inclusion ------------------------------------ */
#ifndef __GPRS_H__
#define __GPRS_H__

/* Includes ----------------------------------------------------------------- */
/* Private macros ----------------------------------------------------------- */
/** @addtogroup GPRS_Private_Macros
  * @{
  */
//GU900S PWRKEY
#define GPRS_PowerKeyEnable()    HAL_GPIO_WritePin(GPRS_PWRON_GPIO,GPRS_PWRON_PIN,GPIO_PIN_SET)
#define GPRS_PowerKeyDisable()   HAL_GPIO_WritePin(GPRS_PWRON_GPIO,GPRS_PWRON_PIN,GPIO_PIN_RESET)

#define IS_ME909_CONNECT_ID(ID)	((ID >= 1) && (ID <= 5))
/* Exported types ----------------------------------------------------------- */
#define GPRS_COM    COM3    /* 定义GPRS对应的串口 */

typedef enum {
    GPRS_OK = 0,                                                     //GPRS初始化成功
    NO_INIT,                                                         //GPRS未初始化
    NO_SIM_CARD,                                                     //无SIM卡
    INVALID_SIM_CARD,                                                //SIM卡无效
    REGISTER_FAIL,                                                   //注册网络失败
    NO_GPRS                                                          //无GPRS业务(可能通讯卡欠费)
}GPRS_StateTypeDef;

/* 网络类型 */
typedef enum {
    NOSERVICE,                                                       //无服务
    GSM,
    WCDMA,
    TDSCDMA,
    LTE
}NetTypedef;
/* Exported constants ------------------------------------------------------- */
/* Exported macros ---------------------------------------------------------- */
#define GPRS_RX_BUF_TMP_LEN  200    /* 用于GPRS命令处理的临时缓存区大小 */
/* Exported variables ------------------------------------------------------- */
extern NetTypedef NetType;                                           //网络类型
extern GPRS_StateTypeDef GPRS_State;                                 //gprs状态
extern uint8_t Isp;                                                  //ISP识别码(0:中国移动;1:中国联通)
extern char GPRS_RxBufTmp[];
extern volatile uint16_t GPRS_RxBufTmp_cnt;
extern uint8_t GprsSignalLevel;                                      //无线网络信号强度
extern uint8_t g_ucConnectEnable;                                    //8路连接使能标志 0:该连接被禁止;1:该连接被使能
extern char IMSI[16];                                                //保存15位IMSI号
extern uint8_t GprsInitRequest;                                      //请求重新初始化网络连接

/* Exported functions ------------------------------------------------------- */
uint8_t GU900_SendCmd(char *cmd, char *ack, uint16_t wait_time);
uint8_t ME909_Init(void);
void DTU_Init(void);
uint8_t ME_TCPSendString(uint8_t conn_id, char *str, uint16_t wait_time);
void DTU_HandleRecData(void);
uint8_t GPRS_CSQ(void);
void DTU_Startup(void);
void ME_Disconnect(uint8_t _type, uint8_t _connect_id);
void ME_Reconnection(void);
void ME_ConnectSever(uint8_t _ucServerNo);
void ME_HeartbeartTmrCallback(void);
void ME_CheckConnection(void);
uint8_t DTU_HCSQ(void);
uint8_t DTU_GetSimNum(char *s);
void ME_Heartbeart(void);
void DTU_CheckState(void);

#endif /*__GPRS_H__*/

/********* (C) COPYRIGHT 2015-2025 广州正虹科技发展有限公司 ****END OF FILE****/

