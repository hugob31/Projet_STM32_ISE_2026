/*
 * delays.c
 *
 *	The MIT License.
 *  Created on: 11.07.2018
 *      Author: Mateusz Salamon
 *      www.msalamon.pl
 *
 */
#include "main.h"
#include "stm32l4xx_hal.h"


#include "delays.h"

void Delay_us(uint32_t us) {
    // On remplace htim3 par une boucle simple pour éviter l'erreur
    uint32_t count = us * 8; // Approximation pour 80MHz
    while(count--) {
        __NOP();
    }
}
