/**
  ******************************************************************
  * @file    调试代码.c
  * @brief   SD卡背景图片读取模块实现
  *          - 初始化SD卡 + FATFS文件系统挂载
  *          - 检测背景文件是否存在
  *          - 读取背景图片到SDRAM缓冲区
  * @note    此文件独立于 main.c，不影响现有串口/动画逻辑
  ******************************************************************
  */

#include "stm32h7xx.h"
#include "调试代码.h"
#include "./FATFS/ff_gen_drv.h"
#include "./led/bsp_led.h"

/* SD卡底层驱动结构体（定义在 fatfs_sd_sdio.c 中） */
extern const Diskio_drvTypeDef SD_Driver;

/* ================================================================= */
/* 🔑 全局变量定义 */
/* ================================================================= */

/** SD卡就绪标志: 0=未就绪, 1=已就绪（文件系统挂载成功且背景文件存在） */
volatile uint8_t sd_card_ready = 0;

/** 背景图片数据缓冲区（指向SDRAM，由调用方分配） */
uint8_t *bg_image_buffer = NULL;

/** 背景图片文件大小 */
uint32_t bg_image_size = 0;

/* ================================================================= */
/* 📦 FATFS 相关全局变量 */
/* ================================================================= */

FATFS sd_fs;          /* SD卡文件系统工作区 */
char  sd_path[4];     /* 逻辑驱动器路径，如 "0:/" */

/* 背景文件名定义（可根据实际文件名修改） */
#define BG_FILENAME   "0:/background.bin"

/* ================================================================= */
/* 🛠 SD卡初始化 + 文件系统挂载 + 背景文件检测 */
/* ================================================================= */

/**
  * @brief  初始化SD卡，挂载FATFS文件系统，并检测背景文件是否存在
  * @note   调用后会设置全局标志 sd_card_ready
  *         - 成功: sd_card_ready = 1, bg_image_size 记录文件大小
  *         - 失败: sd_card_ready = 0
  * @retval FRESULT
  */
FRESULT SD_Init_And_Check(void)
{
    FRESULT res;
    FIL     bg_file;
    UINT    br;

    /* 默认：未就绪 */
    sd_card_ready = 0;
    bg_image_size = 0;

    /* ----------------------------------------------------------- */
    /* 第1步：注册SD卡驱动到FATFS（绑定逻辑驱动器号）              */
    /* ----------------------------------------------------------- */
    if (FATFS_LinkDriver(&SD_Driver, sd_path) != 0)
    {
        /* 驱动注册失败（可能已注册或驱动器号用尽） */
        LED_RED;
        return FR_INVALID_DRIVE;
    }

    /* ----------------------------------------------------------- */
    /* 第2步：挂载SD卡文件系统                                      */
    /* ----------------------------------------------------------- */
    res = f_mount(&sd_fs, sd_path, 1);
    if (res != FR_OK)
    {
        /* 挂载失败：可能SD卡未插入 / 卡损坏 / 无FAT文件系统 */
        LED_RED;
        return res;
    }

    /* ----------------------------------------------------------- */
    /* 第3步：尝试打开背景文件，验证SD卡可读                       */
    /* ----------------------------------------------------------- */
    res = f_open(&bg_file, BG_FILENAME, FA_READ);
    if (res != FR_OK)
    {
        /* 背景文件不存在或无法打开 */
        LED_RED;
        return res;
    }

    /* 记录文件大小，供后续读取使用 */
    bg_image_size = f_size(&bg_file);

    /* 关闭文件（后续读取时再打开） */
    f_close(&bg_file);

    /* ----------------------------------------------------------- */
    /* 🎉 全部通过！SD卡就绪                                        */
    /* ----------------------------------------------------------- */
    sd_card_ready = 1;
    LED_BLUE;   /* 蓝灯表示SD卡就绪 */

    return FR_OK;
}

/* ================================================================= */
/* 🛠 从SD卡读取背景图片到缓冲区 */
/* ================================================================= */

/**
  * @brief  从SD卡读取完整背景图片文件到指定缓冲区
  * @param  buffer:     目标缓冲区指针（建议放在SDRAM 0xD0000000区域）
  * @param  buf_size:   缓冲区可用大小（字节）
  * @param  bytes_read: [输出] 实际读取的字节数
  * @retval FRESULT
  * @note   调用前应确保 sd_card_ready == 1
  */
FRESULT SD_ReadBackground(uint8_t *buffer, uint32_t buf_size, uint32_t *bytes_read)
{
    FRESULT res;
    FIL     bg_file;
    UINT    br;

    /* 参数检查 */
    if (buffer == NULL || bytes_read == NULL)
    {
        return FR_INVALID_PARAMETER;
    }

    *bytes_read = 0;

    /* 检查SD卡是否就绪 */
    if (sd_card_ready == 0)
    {
        return FR_NOT_READY;
    }

    /* 打开背景文件 */
    res = f_open(&bg_file, BG_FILENAME, FA_READ);
    if (res != FR_OK)
    {
        return res;
    }

    /* 安全检查：文件大小不能超过缓冲区 */
    if (f_size(&bg_file) > buf_size)
    {
        f_close(&bg_file);
        return FR_DENIED;   /* 缓冲区太小 */
    }

    /* 读取整个文件 */
    res = f_read(&bg_file, buffer, f_size(&bg_file), &br);
    if (res == FR_OK)
    {
        *bytes_read = br;
    }

    /* 关闭文件 */
    f_close(&bg_file);

    return res;
}
