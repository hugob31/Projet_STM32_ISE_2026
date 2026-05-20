/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : arinc429.h
  * @brief          : Prototypes et definitions pour l'encapsulation ARINC 429
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __ARINC429_H
#define __ARINC429_H

#include "stm32l4xx_hal.h"

/* ---- Définitions des champs ARINC 429 ---- */
#define ARINC_LABEL_256_OCTAL   0x76  // Label 256 octal avec bits inverses pour la transmission

/* ---- Prototypes des fonctions publiques ---- */
uint8_t Calcule_Parite_Impaire(uint32_t word);
uint32_t Generer_Mot_ARINC429_Pression(uint32_t pression_hpa);

#endif /* __ARINC429_H */
