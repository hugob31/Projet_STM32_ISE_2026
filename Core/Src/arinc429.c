/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : arinc429.c
  * @brief          : Couche logique d'encapsulation ARINC 429
  ******************************************************************************
  */
/* USER CODE END Header */

#include "arinc429.h"

/**
 * @brief Calcule la parité impaire d'un mot de 32 bits
 */
uint8_t Calcule_Parite_Impaire(uint32_t word) {
    uint8_t count = 0;
    for (int i = 0; i < 31; i++) {
        if ((word >> i) & 1) {
            count++;
        }
    }
    // Parite impaire : si le nombre de 1 est pair, on retourne 1 pour rendre le total impair
    return (count % 2 == 0) ? 1 : 0;
}

/**
 * @brief Génère un mot ARINC 429 complet (32 bits) à partir de la pression
 */
uint32_t Generer_Mot_ARINC429_Pression(uint32_t pression_hpa) {
    uint32_t mot_arinc = 0;

    // 1. Insertion du Label 256 octal (bits inverses = 0x76)
    mot_arinc |= (ARINC_LABEL_256_OCTAL & 0xFF); // Bits 1 à 8

    // 2. Champs SDI force à 0 (Bits 9 et 10)
    mot_arinc &= ~(0x3 << 8);

    // 3. Payload de la donnee de pression (Bits 11 à 29)
    uint32_t data_masked = (pression_hpa & 0x7FFFF); // Securite sur 19 bits max
    mot_arinc |= (data_masked << 10);

    // 4. Champs SSM force à 0 (Bits 30 et 31 pour "Normal Operation")
    mot_arinc &= ~(0x3 << 29);

    // 5. Calcul et injection du bit de parité impaire (Bit 32)
    if (Calcule_Parite_Impaire(mot_arinc)) {
        mot_arinc |= (1UL << 31);
    }

    return mot_arinc;
}
