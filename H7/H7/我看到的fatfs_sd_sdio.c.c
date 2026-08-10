/**
  ******************************************************************************
  * @file    bsp_sdio_sd.c
  * @author  fire
  * @version V1.0
  * @date    2018-xx-xx
  * @brief   SDIO sd�����������������ļ�ϵͳ��
  ******************************************************************************
  * @attention
  *
  * ʵ��ƽ̨:����  STM32 H743 ������  
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :http://firestm32.taobao.com
  *
  ******************************************************************************
  */
#include "./drivers/fatfs_sd_sdio.h"
#include <stdio.h>
#include <string.h>
#include "./sd_card/bsp_sdio_sd.h"
#include "ff_gen_drv.h"
#include "./led/bsp_led.h" 
/* Disk status */
static volatile DSTATUS Stat = STA_NOINIT;

extern SD_HandleTypeDef uSdHandle;
//���ͱ�־λ
extern volatile uint8_t TX_Flag;
//���ܱ�־λ
extern volatile uint8_t RX_Flag; 
const Diskio_drvTypeDef  SD_Driver =
{
  SD_initialize,
  SD_status,
  SD_read, 
#if  _USE_WRITE == 1
  SD_write,
#endif /* _USE_WRITE == 1 */
  
#if  _USE_IOCTL == 1
  SD_ioctl,
#endif /* _USE_IOCTL == 1 */
};
DSTATUS SD_initialize(BYTE lun)
{
    Stat = STA_NOINIT;
    if(BSP_SD_Init() == HAL_OK)
    {    
        Stat &= ~STA_NOINIT;
    }
    return Stat;
}

DSTATUS SD_status(BYTE lun){

    Stat = STA_NOINIT;
    if(HAL_SD_GetCardState(&uSdHandle) == HAL_SD_CARD_TRANSFER)
    {
        Stat &= ~STA_NOINIT;
    }
    return Stat;
}
DRESULT SD_read(BYTE lun,//��������������豸ʱ�õ�(0...)
                BYTE *buff,//���ݻ�����
                DWORD sector, //�����׵�ַ
                UINT count)//��������(1..128)
{
  DRESULT res = RES_ERROR;
  uint32_t alignedAddr;

  if((DWORD)buff&3)
  {
    DRESULT res = RES_OK;
    DWORD scratch[BLOCKSIZE / 4];

    while (count--)
    {
      res = disk_read(0,(void *)scratch, sector++, 1);

      if (res != RES_OK)
      {
        break;
      }
      memcpy(buff, scratch, BLOCKSIZE);
      buff += BLOCKSIZE;
    }
    return res;
  }

  /* H7的SDMMC使用IDMA，即使polling模式也绕过CPU缓存。
     必须先Clean（刷出脏数据），读完后Invalidate（丢弃旧缓存行） */
  alignedAddr = (uint32_t)buff & ~0x1F;
  SCB_CleanDCache_by_Addr((uint32_t*)alignedAddr, count*BLOCKSIZE + ((uint32_t)buff - alignedAddr));

  if(HAL_SD_ReadBlocks(&uSdHandle, (uint8_t*)buff,
                       (uint32_t)(sector),
                       count, 5000) == HAL_OK)
  {
    /* 关键！让CPU重新从内存读取IDMA写进去的数据 */
    SCB_InvalidateDCache_by_Addr((uint32_t*)alignedAddr, count*BLOCKSIZE + ((uint32_t)buff - alignedAddr));
    res = RES_OK;
  }

  return res;
}
  
DRESULT SD_write(BYTE lun,//��������������豸ʱ�õ�(0...)
                 const BYTE *buff,//���ݻ�����
                 DWORD sector, //�����׵�ַ
                 UINT count)//��������(1..128)
{
    DRESULT res = RES_ERROR;
    uint32_t alignedAddr;

    if((DWORD)buff&3)
    {
      DRESULT res = RES_OK;
      DWORD scratch[BLOCKSIZE / 4];

      while (count--)
      {
        memcpy( scratch,buff,BLOCKSIZE);
        res = disk_write(0,(void *)scratch, sector++, 1);
        if (res != RES_OK)
        {
          break;
        }
        buff += BLOCKSIZE;
      }
      return res;
    }

    /* 写之前Clean缓存，确保IDMA能从内存读到正确数据 */
    alignedAddr = (uint32_t)buff & ~0x1F;
    SCB_CleanDCache_by_Addr((uint32_t*)alignedAddr, count*BLOCKSIZE + ((uint32_t)buff - alignedAddr));

    if(HAL_SD_WriteBlocks(&uSdHandle, (uint8_t*)buff,
                          (uint32_t)(sector),
                          count, 5000) == HAL_OK)
    {
      res = RES_OK;
    }
    return res;
}
DRESULT SD_ioctl(BYTE lun,BYTE cmd, void *buff){
    DRESULT res = RES_ERROR;
    HAL_SD_CardInfoTypeDef CardInfo;

    if (Stat & STA_NOINIT) return RES_NOTRDY;

    switch (cmd)
    {
    /* Make sure that no pending write process */
    case CTRL_SYNC :
      res = RES_OK;
      break;

    /* Get number of sectors on the disk (DWORD) */
    case GET_SECTOR_COUNT :
      HAL_SD_GetCardInfo(&uSdHandle, &CardInfo);
      *(DWORD*)buff = CardInfo.LogBlockNbr;
      res = RES_OK;
      break;

    /* Get R/W sector size (WORD) */
    case GET_SECTOR_SIZE :
      HAL_SD_GetCardInfo(&uSdHandle, &CardInfo);
      *(WORD*)buff = CardInfo.LogBlockSize;
      res = RES_OK;
      break;

    /* Get erase block size in unit of sector (DWORD) */
    case GET_BLOCK_SIZE :
      HAL_SD_GetCardInfo(&uSdHandle, &CardInfo);
      *(DWORD*)buff = CardInfo.LogBlockSize / BLOCK_SIZE;
      res = RES_OK;
      break;

    default:
      res = RES_PARERR;
    }
    return res;
}
/*****************************END OF FILE****************************/

