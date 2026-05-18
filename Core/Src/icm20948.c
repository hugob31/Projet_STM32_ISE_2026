/**
  ******************************************************************************
  * @file           : icm20948.c
  * @brief          : Fonctions de communication pour l'ICM-20948
  ******************************************************************************
  */

#include "icm20948.h"

// Variable statique locale pour mémoriser quel port I2C on utilise
static I2C_HandleTypeDef *icm_i2c;

/**
  * @brief  Initialise le capteur ICM-20948 et le sort du mode veille
  * @retval 0 si succès, 1 si erreur (capteur introuvable)
  */
uint8_t ICM20948_Init(I2C_HandleTypeDef *hi2c) {
    icm_i2c = hi2c;
    uint8_t device_id = 0;
    uint8_t config_val = 0;

    // 1. Par sécurité, on force la sélection de la Banque 0
    config_val = 0x00;
    HAL_I2C_Mem_Write(icm_i2c, ICM20948_ADDR, ICM20948_REG_BANK_SEL, 1, &config_val, 1, 100);

    // 2. Vérification de la présence du capteur (WHO_AM_I)
    HAL_I2C_Mem_Read(icm_i2c, ICM20948_ADDR, ICM20948_REG_WHO_AM_I, 1, &device_id, 1, 100);
    if (device_id != 0xEA) {
        return 1; // Erreur : Mauvais composant ou problème de câblage
    }

    // 3. Sortie du mode sommeil (Reset du bit SLEEP et sélection de la meilleure horloge)
    config_val = 0x01; // Auto-select clock source
    HAL_I2C_Mem_Write(icm_i2c, ICM20948_ADDR, ICM20948_REG_PWR_MGMT_1, 1, &config_val, 1, 100);

    // 4. Activation complète de l'accéléromètre et du gyroscope
    config_val = 0x00; // Active les 6 axes
    HAL_I2C_Mem_Write(icm_i2c, ICM20948_ADDR, ICM20948_REG_PWR_MGMT_2, 1, &config_val, 1, 100);

    return 0; // Tout est parfait
}

/**
  * @brief  Lit les valeurs de l'accéléromètre et les convertit en "g"
  */
uint8_t ICM20948_ReadAccel(float *ax, float *ay, float *az) {
    uint8_t raw_data[6];

    if (HAL_I2C_Mem_Read(icm_i2c, ICM20948_ADDR, ICM20948_REG_ACCEL_XOUT, 1, raw_data, 6, 100) == HAL_OK) {
        // Recomposition des données 16 bits (High byte << 8 | Low Byte)
        int16_t x = (int16_t)((raw_data[0] << 8) | raw_data[1]);
        int16_t y = (int16_t)((raw_data[2] << 8) | raw_data[3]);
        int16_t z = (int16_t)((raw_data[4] << 8) | raw_data[5]);

        // Échelle par défaut : +/- 2g (Sensibilité : 16384 LSB/g)
        *ax = (float)x / 16384.0f;
        *ay = (float)y / 16384.0f;
        *az = (float)z / 16384.0f;
        return 0;
    }
    return 1; // Erreur I2C
}

/**
  * @brief  Lit les valeurs du gyroscope et les convertit en degrés par seconde (°/s)
  */
uint8_t ICM20948_ReadGyro(float *gx, float *gy, float *gz) {
    uint8_t raw_data[6];

    if (HAL_I2C_Mem_Read(icm_i2c, ICM20948_ADDR, ICM20948_REG_GYRO_XOUT, 1, raw_data, 6, 100) == HAL_OK) {
        int16_t x = (int16_t)((raw_data[0] << 8) | raw_data[1]);
        int16_t y = (int16_t)((raw_data[2] << 8) | raw_data[3]);
        int16_t z = (int16_t)((raw_data[4] << 8) | raw_data[5]);

        // Échelle par défaut : +/- 250 dps (Sensibilité : 131 LSB/dps)
        *gx = (float)x / 131.0f;
        *gy = (float)y / 131.0f;
        *gz = (float)z / 131.0f;
        return 0;
    }
    return 1; // Erreur I2C
}
