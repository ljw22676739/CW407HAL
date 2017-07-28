/****************** Copyright(c) 广州正虹科技发展有限公司 ***********************
*  本文件为SPI操作SD卡的底层驱动文件
*  包括SPI模块及相关IO的初始化，SPI读写SD卡（写指令，读数据等）
*******************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"                        // STM32F10x Library DefinitioN
#include "spi_sd_card_driver.h"
#include "ff.h"
#include "diskio.h"
#include "bsp.h"
#include <stdint.h>
#include <stdio.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#if SD_DEBUG_EN==0
#define printf(...)
#endif
/* Private variables ---------------------------------------------------------*/
SD_TypeDef SD_Type = SD_TYPE_MMC; //默认为MMC卡

/* Global variables ---------------------------------------------------------*/
const unsigned char *SdTypeTab[] =
{
   "SD_TYPE_NO_CARD",
   "SD_TYPE_MMC",
   "SD_TYPE_V1",
   "SD_TYPE_V2",
   "SD_TYPE_V2HC"
};

/* Declaration of extern functions ------------------------------------------*/
extern void Error_Handler(void);
/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/



/* ************************** 需要移植的接口函数 ****************************** */
/*******************************************************************************
* Function Name  : SD_SPI_Configuration
* Description    : SPI模块初始化，【包括相关IO口的初始化】
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void SD_SPI_Configuration(void)
{
   return;
}


/*******************************************************************************
* Function Name  : SPI_SetSpeed
* Description    : SPI设置速度为高速
* Input          : uint8_t SpeedSet 
*                  如果速度设置输入0，则低速模式，非0则高速模式
*                  SPI_SPEED_HIGH   1
*                  SPI_SPEED_LOW    0
* Output         : None
* Return         : None
*******************************************************************************/
static void SPI_SetSpeed(unsigned char SpeedSet)
{
   hspi1.Instance = SPI1;
   hspi1.Init.Mode = SPI_MODE_MASTER;
   hspi1.Init.Direction = SPI_DIRECTION_2LINES;
   hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
   hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
   hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
   hspi1.Init.NSS = SPI_NSS_SOFT;
// hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
   hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
   hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
   hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
   hspi1.Init.CRCPolynomial = 10;

   //如果速度设置输入0，则低速模式，非0则高速模式
   if (SpeedSet == SPI_SPEED_LOW)
   {
      hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
   }
   else
   {
      hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
   }

   if (HAL_SPI_Init(&hspi1) != HAL_OK)
   {
      Error_Handler();
   }

   return;
}

/*******************************************************************************
* Function Name  : SPI_ReadWriteByte
* Description    : SPI读写一个字节（发送完成后返回本次通讯读取的数据）
* Input          : uint8_t TxData 待发送的数
* Output         : None
* Return         : uint8_t RxData 收到的数
*******************************************************************************/
static unsigned char SPI_ReadWriteByte(unsigned char TxData)
{
   unsigned char RxData = 0;

   HAL_SPI_TransmitReceive(&hspi1,&TxData,&RxData,1,0xffff);

   return RxData;
}

/* ************************ 需要移植的接口函数结束 **************************** */


/*******************************************************************************
* Function Name  : SD_WaitReady
* Description    : 等待SD卡Ready
* Input          : None
* Output         : None
* Return         : uint8_t 
*                   0： 成功
*                   other：失败
*******************************************************************************/
unsigned char SD_WaitReady(void)
{
   unsigned char r1;
   unsigned short retry;
   retry = 0;
   do
   {
      r1 = SPI_ReadWriteByte(0xFF);
      if (retry == 0xfffe)
      {
         return 1;
      }
   }
   while (r1 != 0xFF);

   return 0;
}

/*******************************************************************************
* Function Name  : SD_SendCommand
* Description    : 向SD卡发送一个命令
* Input          : uint8_t cmd   命令 
*                  uint32_t arg  命令参数
*                  uint8_t crc   crc校验值
* Output         : None
* Return         : uint8_t r1 SD卡返回的响应
*******************************************************************************/
unsigned char SD_SendCommand(unsigned char cmd, unsigned long arg, unsigned char crc)
{
   unsigned char r1;
   unsigned char Retry = 0;

   SD_CS_DISABLE();
   //发送8位，给点时间SD卡
   SPI_ReadWriteByte(0xff);
   //片选端置低，选中SD卡
   SD_CS_ENABLE();

   //发送
   SPI_ReadWriteByte(cmd | 0x40);                         //分别写入命令
   SPI_ReadWriteByte(arg >> 24);
   SPI_ReadWriteByte(arg >> 16);
   SPI_ReadWriteByte(arg >> 8);
   SPI_ReadWriteByte(arg);
   SPI_ReadWriteByte(crc);

   //等待响应，或超时退出
   while ((r1 = SPI_ReadWriteByte(0xFF)) == 0xFF)
   {
      Retry++;
      if (Retry > 200)
      {
         break;
      }
   }

   //关闭片选
   SD_CS_DISABLE();
   //在总线上额外增加8个时钟，让SD卡完成剩下的工作
   SPI_ReadWriteByte(0xFF);

   //返回状态值
   return r1;
}


/*******************************************************************************
* Function Name  : SD_SendCommand_NoDeassert
* Description    : 向SD卡发送一个命令(结束是不失能片选，还有后续数据传来）
* Input          : uint8_t cmd   命令 
*                  uint32_t arg  命令参数
*                  uint8_t crc   crc校验值
* Output         : None
* Return         : uint8_t r1 SD卡返回的响应
*******************************************************************************/
unsigned char SD_SendCommand_NoDeassert(unsigned char cmd, unsigned long arg, unsigned char crc)
{
   unsigned char r1;
   unsigned char Retry = 0;
   //????????
   SPI_ReadWriteByte(0xff); //
                            //片选端置低，选中SD卡
   SD_CS_ENABLE();
   //发送
   SPI_ReadWriteByte(cmd | 0x40);                         //分别写入命令
   SPI_ReadWriteByte(arg >> 24);
   SPI_ReadWriteByte(arg >> 16);
   SPI_ReadWriteByte(arg >> 8);
   SPI_ReadWriteByte(arg);
   SPI_ReadWriteByte(crc);
   //等待响应，或超时退出
   while ((r1 = SPI_ReadWriteByte(0xFF)) == 0xFF)
   {
      Retry++;
      if (Retry > 200)
      {
         break;
      }
   }
   //返回响应值
   return r1;
}


/*******************************************************************************
* Function Name  : SD_Init
* Description    : 初始化SD卡
* Input          : None
* Output         : None
* Return         : uint8_t 
*                  0：NO_ERR
*                  1：TIME_OUT
*                  2：NO_CARD
*                  3：unknown card（无法识别卡）
*                  99：NO_CARD
*******************************************************************************/
uint8_t SD_Init(void)
{
   unsigned short i;      // 用来循环计数
   unsigned char r1;      // 存放SD卡的返回值
   unsigned short retry;  // 用来进行超时计数
   unsigned char buff[6];

   SD_Type = SD_TYPE_NO_CARD;  //默认是无卡，等待初始化成功

   SD_SPI_Configuration();
   SPI_SetSpeed(SPI_SPEED_LOW);

   //如果没有检测到卡插入，直接退出，返回错误标志
   if (!SD_DET())
   {
      //return 99;
      return STA_NODISK;  //  FatFS错误标志：没有插入磁盘
   }

   //SD卡上电
   SD_PWR_ON();
   // 纯延时，等待SD卡上电完成
   for(i=0;i<0xf00;i++);

   //先产生>74个脉冲，让SD卡自己初始化完成
   for (i = 0; i < 10; i++)
   {
      SPI_ReadWriteByte(0xFF);
   }

   //-----------------SD卡复位到idle开始-----------------
   //循环连续发送CMD0，直到SD卡返回0x01,进入IDLE状态
   //超时则直接退出
   retry = 0;
   do
   {
      //发送CMD0，让SD卡进入IDLE状态
      r1 = SD_SendCommand(CMD0, 0, 0x95);
      retry++;
   }
   while ((r1 != 0x01) && (retry < 200));
   //跳出循环后，检查原因：初始化成功？or 重试超时？
   if (retry == 200)
   {
      return 1;   //超时返回1
   }
   //-----------------SD卡复位到idle结束-----------------


   //获取卡片的SD版本信息
   r1 = SD_SendCommand_NoDeassert(CMD8, 0x1aa, 0x87);

   //下面是V2.0卡的初始化
   //其中需要读取OCR数据，判断是SD2.0还是SD2.0HC卡
   if (r1 == 0x01)
   {
#if SD_DEBUG_EN>0
      printf("CMD8 respond 1.\r\n");
#endif

      //V2.0的卡，CMD8命令后会传回4字节的数据，要跳过再结束本命令
      buff[0] = SPI_ReadWriteByte(0xFF);  //should be 0x00
      buff[1] = SPI_ReadWriteByte(0xFF);  //should be 0x00
      buff[2] = SPI_ReadWriteByte(0xFF);  //should be 0x01
      buff[3] = SPI_ReadWriteByte(0xFF);  //should be 0xAA

      SD_CS_DISABLE();
      //the next 8 clocks
      SPI_ReadWriteByte(0xFF);

      //判断该卡是否支持2.7V-3.6V的电压范围
      if (buff[2] == 0x01 && buff[3] == 0xAA)
      {
#if SD_DEBUG_EN>0
         printf("SD card support 2.7~3.6V\r\n");
#endif
         //支持电压范围，可以操作
         retry = 0;
         //发卡初始化指令CMD55+ACMD41
         do
         {
            r1 = SD_SendCommand(CMD55, 0, 0);
            if (r1 != 0x01)
            {
               return 3;   //如果命令没有返回正确应答，直接退出，返回无法识别卡
#if SD_DEBUG_EN>0
               printf("CMD55 no response.\r\n");
#endif
            }
#if SD_DEBUG_EN>0
            printf("CMD55 respond: %d\r\n");
#endif
            r1 = SD_SendCommand(ACMD41, 0x40000000, 0); //因为是V2版本卡，参数中HCS设置1，说明主机支持V2及以上的卡。
            if (retry > 200)
            {
               return 1;  //超时返回
#if SD_DEBUG_EN>0
               printf("ACMD41 response time out.\r\n");
#endif
            }
#if SD_DEBUG_EN>0
            printf("ACMD41 respond: %d\r\n", r1);
#endif
         }
         while (r1 != 0); //等待直到CMD41返回0，表示SD卡初始化完成。卡不再是空闲状态。

         //初始化指令发送完成，接下来获取OCR信息

         //-----------鉴别SD2.0卡版本开始-----------
         r1 = SD_SendCommand_NoDeassert(CMD58, 0, 0);
         if (r1 != 0x00)
         {
            return 3;  //如果命令没有返回正确应答，直接退出，返回无法识别卡
#if SD_DEBUG_EN>0
            printf("CMD58 no response.\r\n");
#endif
         }
         //读OCR指令发出后，紧接着是4字节的OCR信息
         buff[0] = SPI_ReadWriteByte(0xFF);
         buff[1] = SPI_ReadWriteByte(0xFF);
         buff[2] = SPI_ReadWriteByte(0xFF);
         buff[3] = SPI_ReadWriteByte(0xFF);

         //OCR接收完成，片选置高
         SD_CS_DISABLE();
         SPI_ReadWriteByte(0xFF);

         //检查接收到的OCR中的bit30位（CCS），确定其为SD2.0还是SDHC
         //如果CCS=1：SDHC   CCS=0：SD2.0
         if (buff[0] & 0x40)    //检查CCS
         {
            SD_Type = SD_TYPE_V2HC;
#if SD_DEBUG_EN>0
            printf("This is a SDHC card.\r\n");
#endif
         }
         else
         {
            SD_Type = SD_TYPE_V2;
#if SD_DEBUG_EN>0
            printf("This is a SD V2 card.\r\n");
#endif
         }
         //-----------鉴别SD2.0卡版本结束-----------
      }

   }
   else if (r1 == 0x05) //如果卡片版本信息是v1.0版本的，即r1=0x05，则进行以下初始化
   {
#if SD_DEBUG_EN>0
      printf("CMD8 respond: %d\r\n", r1);
#endif
      //设置卡类型为SDV1.0，如果后面检测到为MMC卡，再修改为MMC
      SD_Type = SD_TYPE_V1;

      //如果是V1.0卡，CMD8指令后没有后续数据
      //片选置高，结束本次命令
      SD_CS_DISABLE();
      //多发8个CLK，让SD结束后续操作
      SPI_ReadWriteByte(0xFF);

      //-----------------SD卡、MMC卡初始化开始-----------------

      //发卡初始化指令CMD55+ACMD41
      // 如果有应答，说明是SD卡，且初始化完成
      // 没有回应，说明是MMC卡，额外进行相应初始化
      retry = 0;
      do
      {
         //先发CMD55，应返回0x01；否则出错
         r1 = SD_SendCommand(CMD55, 0, 0);
         if (r1 != 0x01)
         {
            return 3;   //如果命令没有返回正确应答，直接退出，返回无法识别卡
#if SD_DEBUG_EN>0
            printf("CMD55 no response.\r\n");
#endif
         }
         //得到正确响应后，发ACMD41，应得到返回值0x00，否则重试200次
         r1 = SD_SendCommand(ACMD41, 0, 0); //因为是V1x版本卡，参数中HCS设置0，说明主机支持V1x的卡。
         retry++;
      }
      while ((r1 != 0x00) && (retry < 400)); //有文档只需要ACMD返回0或者1皆可
                                             // 判断是超时还是得到正确回应
                                             // 若有回应：是SD卡；没有回应：是MMC卡

      //----------MMC卡额外初始化操作开始------------
      if (retry == 400)
      {
         retry = 0;
         //发送MMC卡初始化命令（没有测试）
         do
         {
            r1 = SD_SendCommand(1, 0, 0);
            retry++;
         }
         while ((r1 != 0x00) && (retry < 400));
         if (retry == 400)
         {
            return 1;   //MMC卡初始化超时
         }
         //写入卡类型
         SD_Type = SD_TYPE_MMC;
#if SD_DEBUG_EN>0
         printf("This is a MMC card.\r\n");
#endif
      }
      //----------MMC卡额外初始化操作结束------------

      //设置SPI为高速模式
//        SPI_SetSpeed(1);

//        SPI_ReadWriteByte(0xFF);

//************************************** 大容量的卡是否需要设置扇区大小？**********************************************
//禁止CRC校验（默认关闭CRC，所以不需要设置）
      /*
      r1 = SD_SendCommand(CMD59, 0, 0x01);
      if(r1 != 0x00)
      {
          return r1;  //命令错误，返回r1
      }
      */
      //设置Sector Size
      r1 = SD_SendCommand(CMD16, 512, 0xff);
      if (r1 != 0x00)
      {
         return 3;   //如果命令没有返回正确应答，直接退出，返回无法识别卡
      }
//************************************************************************************
//-----------------SD卡、MMC卡初始化结束-----------------

   } //SD卡为V1.0版本的初始化结束

   r1 = SD_SendCommand(CMD16, 512, 0xff);
   if (r1 != 0x00)
   {
      return 3;   //如果命令没有返回正确应答，直接退出，返回无法识别卡
   }

   //设置SPI为高速模式
   SPI_SetSpeed(1);
#if SD_DEBUG_EN>0
   printf("SD card initialize successfully.\r\nSPI set high speed.\r\n");
#endif
   return 0;  //初始化成功
}

/*******************************************************************************
* Function Name  : SD_ReceiveData
* Description    : 从SD卡中读回指定长度的数据，放置在给定位置
* Input          : uint8_t *data(存放读回数据的内存>len)
*                  uint16_t len(数据长度）
*                  uint8_t release(传输完成后是否释放总线CS置高 0：不释放 1：释放）
* Output         : None
* Return         : uint8_t 
*                  0：NO_ERR
*                  other：错误信息
*******************************************************************************/
unsigned char SD_ReceiveData(unsigned char *data, unsigned short len, unsigned char release)
{
   unsigned short retry;
   unsigned char r1;

   // 启动一次传输
   SD_CS_ENABLE();
   //等待SD卡发回数据起始令牌0xFE
   retry = 0;
   do
   {
      r1 = SPI_ReadWriteByte(0xFF);
      retry++;
      if (retry > 2000)  //2000次等待后没有应答，退出报错
      {
         SD_CS_DISABLE();
         return 1;
      }
   }
   while (r1 != 0xFE);
   //开始接收数据
   while (len--)
   {
      *data = SPI_ReadWriteByte(0xFF);
      data++;
   }
   //下面是2个伪CRC（dummy CRC）
   SPI_ReadWriteByte(0xFF);
   SPI_ReadWriteByte(0xFF);
   //按需释放总线，将CS置高
   if (release == RELEASE)
   {
      //传输结束
      SD_CS_DISABLE();
      SPI_ReadWriteByte(0xFF);
   }

   return 0;
}



/*******************************************************************************
* Function Name  : SD_GetCID
* Description    : 获取SD卡的CID信息，包括制造商信息
* Input          : uint8_t *cid_data(存放CID的内存，至少16Byte）
* Output         : None
* Return         : uint8_t 
*                  0：NO_ERR
*                  1：TIME_OUT
*                  other：错误信息
*******************************************************************************/
unsigned char SD_GetCID(unsigned char *cid_data)
{
   unsigned char r1;

   //发CMD10命令，读CID
   r1 = SD_SendCommand(CMD10, 0, 0xFF);
   if (r1 != 0x00)
   {
      return r1;  //没返回正确应答，则退出，报错
   }
   //接收16个字节的数据
   SD_ReceiveData(cid_data, 16, RELEASE);

   return 0;
}


/*******************************************************************************
* Function Name  : SD_GetCSD
* Description    : 获取SD卡的CSD信息，包括容量和速度信息
* Input          : uint8_t *cid_data(存放CID的内存，至少16Byte）
* Output         : None
* Return         : uint8_t 
*                  0：NO_ERR
*                  1：TIME_OUT
*                  other：错误信息
*******************************************************************************/
unsigned char SD_GetCSD(unsigned char *csd_data)
{
   unsigned char r1;

   //发CMD9命令，读CSD
   r1 = SD_SendCommand(CMD9, 0, 0xFF);
   if (r1 != 0x00)
   {
      return r1;  //没返回正确应答，则退出，报错
   }
   //接收16个字节的数据
   SD_ReceiveData(csd_data, 16, RELEASE);

   return 0;
}


/*******************************************************************************
* Function Name  : SD_GetCapacity
* Description    : 获取SD卡的容量,返回的是扇区数量(512B/扇区)
* Input          : None
* Output         : None
* Return         : uint32_t capacity 
*                   0： 取容量出错 
*******************************************************************************/
unsigned long SD_GetCapacity(void)
{
   unsigned char csd[16];
   unsigned long Capacity;
//    unsigned char r1;
//    unsigned short i;
//    unsigned short temp;

   //取CSD信息，如果期间出错，返回0
   if (SD_GetCSD(csd) != 0)
   {
      return 0;
   }

   //如果为SDHC卡，按照下面方式计算
   if ((csd[0] & 0xC0) == 0x40) //CSD V2.0
   {
      Capacity = (csd[9] + ((uint32_t)csd[8] << 8) + ((uint32_t)(csd[7] & 63) << 16) + 1) << 10;
   }
   else    //CSD V1.0
   {
      //下面代码为网上版本
      ////////////formula of the capacity///////////////
      //
      //  memory capacity = BLOCKNR * BLOCK_LEN
      //
      //	BLOCKNR = (C_SIZE + 1)* MULT
      //
      //           C_SIZE_MULT+2
      //	MULT = 2
      //
      //               READ_BL_LEN
      //	BLOCK_LEN = 2
      /**********************************************/
      /*
      //C_SIZE
      i = csd[6] & 0x03;
      i <<= 8;
      i += csd[7];
      i <<= 2;
      i += ((csd[8] & 0xc0) >> 6);

      //C_SIZE_MULT
      r1 = csd[9] & 0x03;
      r1 <<= 1;
      r1 += ((csd[10] & 0x80) >> 7);

      //BLOCKNR
      r1 += 2;
      temp = 1;
      while (r1)
      {
          temp *= 2;
          r1--;
      }
      Capacity = ((uint32_t)(i + 1)) * ((uint32_t)temp);//BLOCKNR

      // READ_BL_LEN
      i = csd[5] & 0x0f;

      //BLOCK_LEN
      temp = 1;
      while (i)
      {
          temp *= 2;
          i--;
      }
      //The final result
      Capacity *= (uint32_t)temp;//容量/字节
      //Capacity /= 512;
      */
      //                                                        ( C_SIZE +1 ) * MULT                                                    * BLOCK_LEN
      //Capacity = (((csd[6]&0x03)<<10) | (csd[7]<<2) | ((csd[8]&0xC0)>>6) + 1) * (1 << ((((csd[9]&0x03)<<1) | ((csd[10]&0x80)>>7)) + 2)) << (csd[5] & 0x0f);//容量/字节
      Capacity = (((csd[6] & 0x03) << 10) | (csd[7] << 2) | ((csd[8] & 0xC0) >> 6) + 1) * (1 << ((((csd[9] & 0x03) << 1) | ((csd[10] & 0x80) >> 7)) + 2)); //容量/扇区
   }
   return (uint32_t)Capacity;
}

/*******************************************************************************
* Function Name  : SD_ReadSingleBlock
* Description    : 读SD卡的一个block
* Input          : uint32_t sector 取地址（sector值，非物理地址） 
*                  uint8_t *buffer 数据存储地址（大小至少512byte） 
* Output         : None
* Return         : uint8_t r1 
*                   0： 成功
*                   other：失败
*******************************************************************************/
unsigned char SD_ReadSingleBlock(unsigned long sector, unsigned char *buffer)
{
   uint8_t r1;

   //设置为高速模式
//    SPI_SetSpeed(SPI_SPEED_HIGH);

   if (SD_Type < SD_TYPE_V2HC)
   {
      //如果不是SDHC，将sector地址转成byte地址
      sector = sector << 9;
   }

   r1 = SD_SendCommand(CMD17, sector, 0); //读命令

   if (r1 != 0x00)
   {
      return r1;
   }

   r1 = SD_ReceiveData(buffer, 512, RELEASE);
   if (r1 != 0)
   {
      return r1;   //读数据出错！
   }
   else
   {
      return 0;
   }
}


/*******************************************************************************
* Function Name  : SD_WriteSingleBlock
* Description    : 写入SD卡的一个block
* Input          : uint32_t sector 扇区地址（sector值，非物理地址） 
*                  uint8_t *buffer 数据存储地址（大小至少512byte） 
* Output         : None
* Return         : uint8_t r1 
*                   0： 成功
*                   other：失败
*******************************************************************************/
unsigned char SD_WriteSingleBlock(unsigned long sector, const unsigned char *data)
{
   unsigned char r1;
   unsigned short i;
   unsigned short retry;

   //设置为高速模式
//    SPI_SetSpeed(SPI_SPEED_HIGH);

   //如果不是SDHC，给定的是sector地址，将其转换成byte地址
   if (SD_Type < SD_TYPE_V2HC)
   {
      sector = sector << 9;
   }

   r1 = SD_SendCommand(CMD24, sector, 0x00);
   if (r1 != 0x00)
   {
      return r1;  //应答不正确，直接返回
   }

   //开始准备数据传输
   SD_CS_ENABLE();
   //先放3个空数据，等待SD卡准备好
   SPI_ReadWriteByte(0xff);
   SPI_ReadWriteByte(0xff);
   SPI_ReadWriteByte(0xff);
   //放起始令牌0xFE
   SPI_ReadWriteByte(0xFE);

   //放一个sector的数据
   for (i = 0; i < 512; i++)
   {
      SPI_ReadWriteByte(*data++);
   }
   //发2个Byte的dummy CRC
   SPI_ReadWriteByte(0xff);
   SPI_ReadWriteByte(0xff);

   //等待SD卡应答
   r1 = SPI_ReadWriteByte(0xff);
   if ((r1 & 0x1F) != 0x05)
   {
      SD_CS_DISABLE();
      return r1;
   }

   //等待操作完成
   retry = 0;
   while (!SPI_ReadWriteByte(0xff))
   {
      retry++;
      if (retry > 0xfffe)        //如果长时间写入没有完成，报错退出
      {
         SD_CS_DISABLE();
         return 1;           //写入超时返回1
      }
   }

   //写入完成，片选置1
   SD_CS_DISABLE();
   SPI_ReadWriteByte(0xff);

   return 0;
}



/*******************************************************************************
* Function Name  : SD_ReadMultiBlocks
* Description    : 读SD卡的多个block
* Input          : uint32_t sector 取地址（sector值，非物理地址） 
*                  uint8_t *buffer 数据存储地址（大小至少512byte）
*                  uint8_t count 连续读count个block
* Output         : None
* Return         : uint8_t r1 
*                   0： 成功
*                   other：失败
*******************************************************************************/
unsigned char SD_ReadMultiBlocks(unsigned long sector, unsigned char *buffer, unsigned char count)
{
   unsigned char r1;

   //设置为高速模式
//    SPI_SetSpeed(SPI_SPEED_HIGH);

   if (SD_Type < SD_TYPE_V2HC)
   {
      //如果不是SDHC，将sector地址转成byte地址
      sector = sector << 9;
   }
   //SD_WaitReady();
   //发读多块命令
   r1 = SD_SendCommand(CMD18, sector, 0); //读命令
   if (r1 != 0x00)
   {
      return r1;
   }
   //开始接收数据
   do
   {
      if (SD_ReceiveData(buffer, 512, NO_RELEASE) != 0x00)
      {
         break;
      }
      buffer += 512;
   }
   while (--count);

   //全部传输完毕，发送停止命令
   SD_SendCommand(CMD12, 0, 0);
   //释放总线
   SD_CS_DISABLE();
   SPI_ReadWriteByte(0xFF);

   if (count != 0)
   {
      return count;   //如果没有传完，返回剩余个数
   }
   else
   {
      return 0;
   }
}


/*******************************************************************************
* Function Name  : SD_WriteMultiBlocks
* Description    : 写入SD卡的N个block
* Input          : uint32_t sector 扇区地址（sector值，非物理地址） 
*                  uint8_t *buffer 数据存储地址（大小至少512byte）
*                  uint8_t count 写入的block数目
* Output         : None
* Return         : uint8_t r1 
*                   0： 成功
*                   other：失败
*******************************************************************************/
unsigned char SD_WriteMultiBlocks(unsigned long sector, const unsigned char *data, unsigned char count)
{
   unsigned char r1;
   unsigned short i;

   //设置为高速模式
//    SPI_SetSpeed(SPI_SPEED_HIGH);

   //如果不是SDHC，给定的是sector地址，将其转换成byte地址
   if (SD_Type < SD_TYPE_V2HC)
   {
      sector = sector << 9;
   }
   //如果目标卡不是MMC卡，启用ACMD23指令使能预擦除
   if (SD_Type != SD_TYPE_MMC)
   {
      r1 = SD_SendCommand(ACMD23, count, 0x00);
   }
   //发多块写入指令
   r1 = SD_SendCommand(CMD25, sector, 0x00);
   if (r1 != 0x00)
   {
      return r1;  //应答不正确，直接返回
   }

   //开始准备数据传输
   SD_CS_ENABLE();
   //先放3个空数据，等待SD卡准备好
   SPI_ReadWriteByte(0xff);
   SPI_ReadWriteByte(0xff);

   //--------下面是N个sector写入的循环部分
   do
   {
      //放起始令牌0xFC 表明是多块写入
      SPI_ReadWriteByte(0xFC);

      //放一个sector的数据
      for (i = 0; i < 512; i++)
      {
         SPI_ReadWriteByte(*data++);
      }
      //发2个Byte的dummy CRC
      SPI_ReadWriteByte(0xff);
      SPI_ReadWriteByte(0xff);

      //等待SD卡应答
      r1 = SPI_ReadWriteByte(0xff);
      if ((r1 & 0x1F) != 0x05)
      {
         SD_CS_DISABLE();    //如果应答为报错，则带错误代码直接退出
         return r1;
      }

      //等待SD卡写入完成
      if (SD_WaitReady() == 1)
      {
         SD_CS_DISABLE();    //等待SD卡写入完成超时，直接退出报错
         return 1;
      }

      //本sector数据传输完成
   }
   while (--count);

   //发结束传输令牌0xFD
   r1 = SPI_ReadWriteByte(0xFD);
   if (r1 == 0x00)
   {
      count =  0xfe;
   }

   if (SD_WaitReady())
   {
      return 3; //等待超时
   }

   //写入完成，片选置1
   SD_CS_DISABLE();
   SPI_ReadWriteByte(0xff);

   return count;   //返回count值，如果写完则count=0，否则count=1
}

/************** (C) COPYRIGHT 广州正虹科技发展有限公司 ********END OF FILE****/

