#ifndef FATFS_SD_H
#define FATFS_SD_H

#include "stm32l4xx_hal.h"
#include "ff.h"
#include "diskio.h"

/* ---- Handle SPI à utiliser ---- */
extern SPI_HandleTypeDef hspi3;

/* ---- Broche Chip Select PA4 ---- */
#define SD_CS_GPIO_Port  GPIOA
#define SD_CS_Pin        GPIO_PIN_4

#define SD_CS_LOW()      HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET)
#define SD_CS_HIGH()     HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET)

/* ---- Types de carte SD ---- */
#define CT_MMC           0x01
#define CT_SD1           0x02
#define CT_SD2           0x04
#define CT_SDC           (CT_SD1 | CT_SD2)
#define CT_BLOCK         0x08

/* ---- Prototypes : DWORD pour sector (compatible ancienne FatFS) ---- */
DSTATUS SD_disk_initialize (BYTE pdrv);
DSTATUS SD_disk_status     (BYTE pdrv);
DRESULT SD_disk_read       (BYTE pdrv, BYTE *buff, DWORD sector, UINT count);
DRESULT SD_disk_write      (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count);
DRESULT SD_disk_ioctl      (BYTE pdrv, BYTE cmd, void *buff);

#endif /* FATFS_SD_H */
