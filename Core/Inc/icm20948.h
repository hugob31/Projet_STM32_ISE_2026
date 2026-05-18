/**
  ******************************************************************************
  * @file           : icm20948.h
  * @brief          : Pilote simplifie pour le Gyro/Accel ICM-20948 en I2C
  ******************************************************************************
  */

#ifndef __ICM20948_H__
#define __ICM20948_H__

#include "stm32l4xx_hal.h" // Pour l'accès aux fonctions HAL I2C

// Adresse I2C pour la carte Adafruit (0x69 << 1)
#define ICM20948_ADDR         0xD2

// Registres principaux (Banque 0)
#define ICM20948_REG_WHO_AM_I   0x00
#define ICM20948_REG_PWR_MGMT_1 0x06
#define ICM20948_REG_PWR_MGMT_2 0x07
#define ICM20948_REG_ACCEL_XOUT 0x2D
#define ICM20948_REG_GYRO_XOUT  0x33
#define ICM20948_REG_BANK_SEL   0x7F

// Prototypes des fonctions
uint8_t ICM20948_Init(I2C_HandleTypeDef *hi2c);
uint8_t ICM20948_ReadAccel(float *ax, float *ay, float *az);
uint8_t ICM20948_ReadGyro(float *gx, float *gy, float *gz);

#endif /* __ICM20948_H__ */
