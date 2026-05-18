/**
  ******************************************************************************
  * @file           : sd_logger.h
  * @brief          : Pilote d'enregistrement CSV pour carte SD via FatFS
  ******************************************************************************
  */

#ifndef __SD_LOGGER_H__
#define __SD_LOGGER_H__

#include "stm32l4xx_hal.h"
#include "fatfs.h" // Bibliothèque générée par CubeMX

// Prototypes
uint8_t SD_Logger_Init(void);
uint8_t SD_Logger_Write(uint32_t time_ms, float temp, int32_t press_pa, float ax, float ay, float az);

#endif /* __SD_LOGGER_H__ */
