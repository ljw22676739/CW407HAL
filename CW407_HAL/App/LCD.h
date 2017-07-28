/****************** Copyright(c) 广州正虹科技发展有限公司 ***********************/
/* Define to prevent recursive inclusion ------------------------------------ */
#ifndef __LCD_H__
#define __LCD_H__

/* Includes ----------------------------------------------------------------- */
/* Private macros ----------------------------------------------------------- */
/******** 配置信息 *******/
#define LCD_COM     COM1      /* 定义LCD串口 */

/* Exported types ----------------------------------------------------------- */
/* Exported constants ------------------------------------------------------- */
/* Exported macros ---------------------------------------------------------- */
/* Exported variables ------------------------------------------------------- */
extern uint8_t fan_last_state;                      //风机上一次的开关状态
extern uint8_t clean_last_state;                    //净化器上一次的开关状态
extern volatile uint8_t LCD_ScreenId;               //串口显示屏当前画面ID,用于更新显示
/* Exported functions ------------------------------------------------------- */
void LCD_Icon(uint8_t screen_id, uint8_t icon_id, uint8_t state);
void LCD_Gif(uint8_t screen_id, uint8_t control_id, uint8_t state);
void LCD_Txt(uint8_t screen_id, uint8_t icon_id, char* str);
void LCD_SetScreen(uint8_t screen_id);
void UpdateLcdDisplay(void);
void UpdateLcdTime(void);
uint8_t LCD_CmdHandle(void);
void DisplayNetState(void);
void LCD_Init(void);

#endif /*__LCD_H__*/

/********* (C) COPYRIGHT 2015-2025 广州正虹科技发展有限公司 ****END OF FILE****/

