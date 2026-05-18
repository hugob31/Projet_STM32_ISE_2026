/**
  ******************************************************************************
  * @file           : sd_logger.c
  * @brief          : Fonctions d'écriture sur la carte MicroSD
  ******************************************************************************
  */

#include "sd_logger.h"
#include <stdio.h>
#include <string.h>

// Variables globales de la bibliothèque FatFS
FATFS fs;
FIL fil;
FRESULT fres;

/**
  * @brief Initialise la carte SD et crée le fichier avec les en-têtes
  * @retval 0 si succès, 1 si erreur
  */
uint8_t SD_Logger_Init(void) {
    fres = f_mount(&fs, "", 1);
    if (fres != FR_OK) {
        printf("Erreur f_mount : %d\r\n", fres);
        return 1;
    }

    fres = f_open(&fil, "VOL_001.CSV", FA_WRITE | FA_OPEN_APPEND); // Correction ici
    if (fres == FR_OK) {
        f_printf(&fil, "Temps(ms),Temp(C),Press(hPa),Ax(g),Ay(g),Az(g)\n");
        f_close(&fil);
        return 0;
    }

    printf("Erreur f_open : %d\r\n", fres);
    return 1;
}
/*
uint8_t SD_Logger_Init(void) {
    // 1. Montage du système de fichiers
    fres = f_mount(&fs, "", 1);
    if (fres != FR_OK) {
        return 1; // Erreur de montage (carte absente ou mal câblée)
    }

    // 2. Ouverture (ou création) du fichier VOL_001.CSV
    fres = f_open(&fil, "VOL_001.CSV", FA_WRITE | FA_OPEN_APPEND | FA_CREATE_ALWAYS);
    if (fres == FR_OK) {
        // 3. Écriture de la première ligne (Les colonnes pour Excel)
        f_printf(&fil, "Temps(ms),Temp(C),Press(hPa),Ax(g),Ay(g),Az(g)\n");

        // 4. On ferme toujours le fichier pour sauvegarder !
        f_close(&fil);
        return 0; // Succès
    }

    return 1; // Erreur de création du fichier
}
*/
/**
  * @brief Enregistre une ligne de données dans le fichier CSV
  * @retval 0 si succès, 1 si erreur
  */
uint8_t SD_Logger_Write(uint32_t time_ms, float temp, int32_t press_pa, float ax, float ay, float az) {
    // 1. Ouverture du fichier en mode "Append" (Ajout à la fin)
    fres = f_open(&fil, "VOL_001.CSV", FA_WRITE | FA_OPEN_APPEND);

    if (fres == FR_OK) {
        // 2. Conversion manuelle des floats (car f_printf ne gère pas les %f par défaut)
        int t_entier = (int)temp;
        int t_dec = (int)((temp - t_entier) * 100); if(t_dec < 0) t_dec = -t_dec;

        int p_hpa = press_pa / 100;

        int ax_entier = (int)ax; int ax_dec = (int)((ax - ax_entier) * 100); if(ax_dec < 0) ax_dec = -ax_dec;
        int ay_entier = (int)ay; int ay_dec = (int)((ay - ay_entier) * 100); if(ay_dec < 0) ay_dec = -ay_dec;
        int az_entier = (int)az; int az_dec = (int)((az - az_entier) * 100); if(az_dec < 0) az_dec = -az_dec;

        // 3. Écriture de la ligne formatée
        f_printf(&fil, "%lu,%d.%02d,%d,%d.%02d,%d.%02d,%d.%02d\n",
                 time_ms, t_entier, t_dec, p_hpa, ax_entier, ax_dec, ay_entier, ay_dec, az_entier, az_dec);

        // 4. Fermeture pour "sauvegarder" physiquement sur la carte SD
        f_close(&fil);
        return 0; // Succès
    }

    return 1; // Erreur d'écriture
}
