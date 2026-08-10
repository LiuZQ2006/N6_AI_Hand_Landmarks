/**
  ******************************************************************
  * @file    调试代码.h
  * @brief   SD卡背景图片读取模块
  *          - 初始化SD卡并挂载FATFS文件系统
  *          - 检测SD卡是否可用，设置就绪标志
  *          - 后续可在此扩展：从SD卡读取背景图片数据
  ******************************************************************
  */

#ifndef __DEBUG_CODE_H
#define __DEBUG_CODE_H

#include "stm32h7xx.h"
#include "ff.h"

/* ================================================================= */
/* 🔑 全局标志位 */
/* ================================================================= */

/** SD卡就绪标志:
 *  0 = SD卡未就绪（未初始化 / 挂载失败 / 文件不存在）
 *  1 = SD卡就绪（文件系统挂载成功，background.bin 文件存在）
 */
extern volatile uint8_t sd_card_ready;

/* 背景图片数据缓冲区指针（指向SDRAM） */
extern uint8_t *bg_image_buffer;

/* 背景图片文件大小（字节） */
extern uint32_t bg_image_size;

/* ================================================================= */
/* 📦 FATFS 相关全局变量（供调试使用） */
/* ================================================================= */

extern FATFS sd_fs;          /* SD卡文件系统对象 */
extern char  sd_path[4];     /* 逻辑驱动器路径，如 "0:/" */

/* ================================================================= */
/* 🛠 函数声明 */
/* ================================================================= */

/**
  * @brief  SD卡初始化 + 文件系统挂载 + 背景文件检测
  * @note   调用后会设置 sd_card_ready 标志
  *         如果SD卡就绪，同时会记录背景文件大小到 bg_image_size
  * @retval FRESULT: FR_OK 表示挂载成功，否则返回错误码
  */
FRESULT SD_Init_And_Check(void);

/**
  * @brief  从SD卡读取整个背景图片到指定缓冲区
  * @param  buffer: 目标缓冲区指针（需分配足够空间，建议在SDRAM中）
  * @param  buf_size: 缓冲区大小（字节）
  * @param  bytes_read: 输出参数，实际读取到的字节数
  * @retval FRESULT: FR_OK 表示读取成功
  * @note   调用前应确保 sd_card_ready == 1
  */
FRESULT SD_ReadBackground(uint8_t *buffer, uint32_t buf_size, uint32_t *bytes_read);

#endif /* __DEBUG_CODE_H */
