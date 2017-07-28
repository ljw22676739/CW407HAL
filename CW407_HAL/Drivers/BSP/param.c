/*
*********************************************************************************************************
*
*	模块名称 : 应用程序参数模块
*	文件名称 : param.c
*	版    本 : V1.0
*	说    明 : 读取和保存应用程序的参数
*	修改记录 :
*		版本号  日期        作者     说明
*		V1.0    2016-10-01 ljw  正式发布
*
*	Copyright (C), 2016-2026, 广州正虹科技发展有限公司
*
*********************************************************************************************************
*/

#include "bsp.h"
#include "param.h"
#include "bsp_cpu_flash.h"


/* Global variables ---------------------------------------------------------*/
__align(8) PARAM_T g_tParam;


/* 将16KB 一个扇区的空间预留出来做为参数区 For MDK */
//const uint8_t para_flash_area[16*1024] __attribute__((at(ADDR_FLASH_SECTOR_3)));

/* Private function prototypes ----------------------------------------------*/
static uint8_t FlashCopy(uint32_t dest_addr, uint32_t src_addr, uint32_t _ulSize);

/*---------------------------------------------------------------------------*/
/*                          Private functions                                */
/*---------------------------------------------------------------------------*/

/******************************************************************************
  Function:     FlashCopy
  Description:  在FLASH中,从指定区域拷贝到指定区域.大小以字为单位
  Input:        uint32_t dest_addr  FLASH目标地址(4字节对齐)
                uint32_t src_addr   FLAHS源地址(4字节对齐)
                uint16_t count      拷贝数量/字节(必须是2的倍数)
  Return:       0:拷贝成功
                1:拷贝失败
                2:拷贝大小有误
  Others:       这是一个特殊拷贝函数,对数据校验正确后,才写入成功标志0xaaaaaaaa,所以拷贝区域不包括该标志
                这个函数使用比较特殊.目前没加块地址计算等,不能作为通用FLASH拷贝.仅用于拷贝系统参数
******************************************************************************/
static uint8_t FlashCopy(uint32_t dest_addr, uint32_t src_addr, uint32_t _ulSize)
{
    uint32_t i;
    uint16_t * src,*dest;
    uint32_t addr = dest_addr;
    /*Variable used for Erase procedure*/
    static FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PAGEError = 0;

    /* 长度为0 时不继续操作  */
    if (_ulSize == 0)
    {
        return 0;
    }

    /* 长度为奇数时不继续操作,保证半字对齐,因为是按照半字方式写入 */
    if ((_ulSize % 2) != 0)
    {
        return 2;
    }

    __set_PRIMASK(1);                                                /* 关中断 */
    /* Unlock the Flash to enable the flash control register access *************/
    HAL_FLASH_Unlock();

    //把配置信息写入Param backup区域
    /* Fill EraseInit structure*/
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = bsp_GetSector(addr);           //FLASH_USER_START_ADDR;
    i = dest_addr - EraseInitStruct.PageAddress + _ulSize + 4;/* 注意最后要写入结束标志位4字节 */
    EraseInitStruct.NbPages = i / FLASH_PAGE_SIZE;
    if (i % FLASH_PAGE_SIZE)
    {
        EraseInitStruct.NbPages++;
    }

    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
    {
        goto PROGRAM_ERROR;
    }

    /* 区域拷贝，但不包括结束标志 */
    addr = dest_addr;
    src = (uint16_t *)src_addr;

    for (i = 0; i < _ulSize / 2; i++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr, *src++) != HAL_OK)
        {
            goto PROGRAM_ERROR;
        }

        addr += 2;
    }

    /* 数据校验 */
    dest = (uint16_t *)dest_addr;
    src = (uint16_t *)src_addr;
    i = _ulSize >> 1;
    while (i--)
    {
        if (*src++ != *dest++)
        {
            goto PROGRAM_ERROR;
        }
    }

    /* 数据校验正确，写入结束标志 */
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, (uint32_t)dest, FLASH_DATA_END_FLAG) != HAL_OK) //写入结束标志
    {
        goto PROGRAM_ERROR;
    }

    if (*(uint32_t *)(dest_addr + _ulSize) != FLASH_DATA_END_FLAG)
    {
#if SAVE_PARAMETER_DEBUG_EN
        DEBUG_PrintfString("Flash data copy fail.\n");
#endif
        goto PROGRAM_ERROR;
    }

#if SAVE_PARAMETER_DEBUG_EN
    DEBUG_PrintfString("Flash data copy sucess.\n");
#endif
    HAL_FLASH_Lock();
    __set_PRIMASK(0);                                                // 开中断
    return 0;
    
PROGRAM_ERROR:
    /* Flash 加锁，禁止写Flash控制寄存器 */
    /* Lock the Flash to disable the flash control register access (recommended
     to protect the FLASH memory against possible unwanted operation) *********/
    HAL_FLASH_Lock();
    __set_PRIMASK(0);                                                // 开中断
    return 1;
}


/*
*********************************************************************************************************
*	函 数 名: LoadParam
*	功能说明: 从Flash读参数到g_tParam
*	形    参：无
*	返 回 值: 0:读取数据成功,参数区Parameter和Parameter backup区域数据都正常
            1:数据从参数区Parameter读取成功，但参数区Parameter拷贝到参数备份区Parameter backup写入失败
            2:数据从参数备份区Parameter backup读取成功，但参数备份区Parameter backup拷贝到参数区Parameter写入失败
            3:恢复默认参数成功
            4:恢复默认参数,但保存到FLASH时有错误
            5:其他错误
*********************************************************************************************************
*/
uint8_t LoadParam(void)
{
    uint8_t res;

    if (*(uint32_t *)(PARAM_START_ADDR + sizeof(PARAM_T) + 4) == FLASH_DATA_END_FLAG) //参数保存区数据正常
    {
#ifdef PARAM_SAVE_TO_FLASH
        /* 从参数区Param读取CPU Flash中的参数 */
        bsp_ReadCpuFlash(PARAM_START_ADDR + 4, (uint8_t *)&g_tParam, sizeof(PARAM_T)); //注意前4个字节是起始标志
#endif

#if SAVE_PARAMETER_DEBUG_EN
        DEBUG_PrintfString("Read flash data ok.\n");
#endif

        /* 检查参数备份区数据 */
        if (*(uint32_t *)(PARAM_BACKUP_START_ADDR + sizeof(PARAM_T) + 4) == FLASH_DATA_END_FLAG)
        {
#if SAVE_PARAMETER_DEBUG_EN
            DEBUG_PrintfString("Flash backup data ok.\n");
#endif
            return 0;
        } else
        {
            /* 把参数区Param的数据拷贝备份区Param backup */
            res = FlashCopy(PARAM_BACKUP_START_ADDR, PARAM_START_ADDR, sizeof(PARAM_T) / 4 + 1); //拷贝大小为拷贝数据内容+FLASH 数据区起始标志(4Byte),结束标志自动写入
#if SAVE_PARAMETER_DEBUG_EN
            if (res == 0)
            {
                DEBUG_PrintfString("copy parameter to flash backup data ok.\n");
            } else
            {
                DEBUG_PrintfString("copy parameter to flash backup data fail.\n");
            }
#endif
            return res;
        }
    } else                                                             /* 参数保存区数据异常 */
    {
#if SAVE_PARAMETER_DEBUG_EN
        DEBUG_PrintfString("Flash data region have no parameter.\n");
#endif

        /* 检查参数备份区数据 */
        if (*(uint32_t *)(PARAM_BACKUP_START_ADDR + sizeof(PARAM_T) + 4) == FLASH_DATA_END_FLAG)
        {

#if SAVE_PARAMETER_DEBUG_EN
            DEBUG_PrintfString("Flash backup data ok.\n");
#endif
            /* 从参数备份区Param backup读取CPU Flash中的参数 */
            bsp_ReadCpuFlash(PARAM_BACKUP_START_ADDR + 4, (uint8_t *)&g_tParam, sizeof(PARAM_T)); //注意前4个字节是起始标志

            /* 把参数区Param的数据拷贝备份区Param backup */
            res = FlashCopy(PARAM_START_ADDR, PARAM_BACKUP_START_ADDR, sizeof(PARAM_T) / 4 + 1); //拷贝大小为拷贝数据内容+FLASH 数据区起始标志(4Byte),结束标志自动写入
            if (res == 0)
            {
#if SAVE_PARAMETER_DEBUG_EN
                DEBUG_PrintfString("copy parameter to flash backup data ok.\n");
#endif
                return res;
            } else
            {
#if SAVE_PARAMETER_DEBUG_EN
                DEBUG_PrintfString("copy parameter to flash backup data fail.\n");
#endif
                return 2;
            }
        } else                                                         /* 参数备份区Parameter backup数据异常 */
        {
#if SAVE_PARAMETER_DEBUG_EN
            DEBUG_PrintfString("Flash have no parameter.\nSet default parameter.\n");
#endif

            if (SetDefaultParam() == 0)
            {

#if SAVE_PARAMETER_DEBUG_EN
                DEBUG_PrintfString("Default parameter save sucessful.\n");
#endif
                return 3;                                            //3:恢复默认参数成功
            } else
            {
#if SAVE_PARAMETER_DEBUG_EN
                DEBUG_PrintfString("Default parameter save fail.\n");
#endif
                return 4;                                            //4:恢复默认参数,但保存到FLASH时有错误
            }
        }
    }
}

/*
*********************************************************************************************************
*	函 数 名: SaveParam
*	功能说明: 将全局变量g_tParam 写入到CPU内部Flash
*	形    参: 无
*	返 回 值: 0:保存成功
             1:参数保存区Param写入失败
             2:参数备份区Param backup写入失败
*********************************************************************************************************
*/
uint8_t SaveParam(void)
{
    uint8_t res;
#ifdef PARAM_SAVE_TO_FLASH
    /* 将全局的参数变量保存到 CPU Flash */
    res = bsp_WriteCpuFlash(PARAM_START_ADDR, (unsigned char *)&g_tParam, sizeof(PARAM_T));
#endif

#if SAVE_PARAMETER_DEBUG_EN
    if (res == 0)
    {
        DEBUG_PrintfString("Flash data region write successfully.\n");
    } else
    {
        DEBUG_PrintfString("Flash data region write fail.\n");
    }
#endif

    /* 把参数区Param的数据拷贝备份区Param backup */
    /* 拷贝大小为拷贝数据内容+FLASH 数据区起始标志(4Byte),结束标志自动写入 */
    res = FlashCopy(PARAM_BACKUP_START_ADDR, PARAM_START_ADDR, sizeof(PARAM_T) + 4); //拷贝尺寸要加上其实标志4个字节

#if SAVE_PARAMETER_DEBUG_EN
    if (res == 0)
    {
        DEBUG_PrintfString("copy data to backup region successfully.\n");
    } else
    {
        DEBUG_PrintfString("copy data to backup region fail.\n");
    }
#endif
    return res;
}

/* ******************************************************************************************************
*	函 数 名: SetDefaultParam
*	功能说明: 设置参数为默认值,并保存到FLASH中
*   形    参: 无
*	返 回 值: 0:保存成功
             1:参数保存区Param写入失败
             2:参数备份区Param backup写入失败
****************************************************************************************************** */
uint8_t SetDefaultParam(void)
{
    uint8_t i;
    static const char mn[] = "88888820150001";                       //默认MN号,注意不能超过14个字符
    static const char default_apn[] = "cmnet";                       //默认apn号

    g_tParam.Version = guiVersion;

    for (i = 0; i < 16; i++)
    {
        g_tParam.DevNO[i] = '\0';
    }
    strncpy(g_tParam.DevNO, mn, sizeof(mn));

    g_tParam.Pwd = 123456;
    g_tParam.u16_SaveRtdInterval = 1;
    g_tParam.u8_ST = 39;

    g_tParam.Server[0].ip[0] = 120;
    g_tParam.Server[0].ip[1] = 197;
    g_tParam.Server[0].ip[2] = 59;
    g_tParam.Server[0].ip[3] = 4;
    g_tParam.Server[0].port = 9895;
    g_tParam.Server[0].rtd_interval = 1;
    g_tParam.Server[0].uip_conn = NULL;
    g_tParam.Server[0].heartbeart_period = 60;

    /* 公司控制后台,不要修改！ */
    g_tParam.Server[1].ip[0] = 120;
    g_tParam.Server[1].ip[1] = 197;
    g_tParam.Server[1].ip[2] = 59;
    g_tParam.Server[1].ip[3] = 4;
    g_tParam.Server[1].port = 0;
    g_tParam.Server[1].rtd_interval = 1;
    g_tParam.Server[1].uip_conn = NULL;
    g_tParam.Server[1].heartbeart_period = 60;

    g_tParam.Server[2].ip[0] = 192;
    g_tParam.Server[2].ip[1] = 168;
    g_tParam.Server[2].ip[2] = 1;
    g_tParam.Server[2].ip[3] = 100;
    g_tParam.Server[2].port = 0;
    g_tParam.Server[2].rtd_interval = 1;
    g_tParam.Server[2].uip_conn = NULL;
    g_tParam.Server[2].heartbeart_period = 60;

    g_tParam.Server[3].ip[0] = 192;
    g_tParam.Server[3].ip[1] = 168;
    g_tParam.Server[3].ip[2] = 1;
    g_tParam.Server[3].ip[3] = 100;
    g_tParam.Server[3].port = 0;
    g_tParam.Server[3].rtd_interval = 1;
    g_tParam.Server[3].uip_conn = NULL;
    g_tParam.Server[3].heartbeart_period = 60;

    g_tParam.local_ip[0] = IP_ADDR0;
    g_tParam.local_ip[1] = IP_ADDR1;
    g_tParam.local_ip[2] = IP_ADDR2;
    g_tParam.local_ip[3] = IP_ADDR3;

    g_tParam.FanChannel = 5;
    g_tParam.CleanChannel = 4;
    g_tParam.threshold = 100;                                        //开关量阈值100uA,实际值100mA

    g_tParam.NTP_En = 1;                                             //默认使能NTP对时功能
    g_tParam.SystemFlag = 0x01;                                      //g_tParam.SystemFlag

    g_tParam.apn[0] = '3';
    g_tParam.apn[1] = 'g';
    g_tParam.apn[2] = 'n';
    g_tParam.apn[3] = 'e';
    g_tParam.apn[4] = 't';
    g_tParam.apn[5] = '\0';

    return SaveParam();                                              /* 将新参数写入Flash */
}

/***************************** 安富莱电子 www.armfly.com (END OF FILE) *********************************/
