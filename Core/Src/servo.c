#include "servo.h"

// Variables privées au module
static TIM_HandleTypeDef *servo_htim;
static uint32_t servo_channel;

// Fonction mathématique privée
static uint32_t calcul_rapport_cyclique(float yaw){
    // 1. Sécurité logicielle : on bride le yaw calculé entre -90° et +90°
    if (yaw > 90.0f) yaw = 90.0f;
    if (yaw < -90.0f) yaw = -90.0f;

    // 2. Équation de mise à l'échelle (Mapping)
    // À 0°, on veut 1500 µs.
    // À 90°, on veut ajouter 500 µs (pour atteindre 2000 µs).
    // Donc le ratio est de 500 / 90.
    float amplitude_max = 500.0f; // Remplace par 1000.0f si tu veux tester la plage 500-2500

    float nb_ticks = 1500.0f + (yaw * (amplitude_max / 90.0f));

    // 3. Sécurités mécaniques (Pour éviter d'abîmer les engrenages en plastique)
    if (nb_ticks < 1000.0f) nb_ticks = 1000.0f; // Met à 500.0f si amplitude_max = 1000.0f
    if (nb_ticks > 2000.0f) nb_ticks = 2000.0f; // Met à 2500.0f si amplitude_max = 1000.0f

    return (uint32_t)nb_ticks;
}

// Initialisation du servomoteur
void Servo_Init(TIM_HandleTypeDef *htim, uint32_t channel) {
    servo_htim = htim;
    servo_channel = channel;

    // Démarrage de la génération PWM matérielle
    HAL_TIM_PWM_Start(servo_htim, servo_channel);

    // Position neutre au démarrage (1500 µs)
    __HAL_TIM_SET_COMPARE(servo_htim, servo_channel, 1500);
}

// Mise à jour de la position
void Servo_UpdateYaw(float yaw) {
    // Calcul de l'impulsion requise
    uint32_t pulse = calcul_rapport_cyclique(yaw);

    // Écriture matérielle Bare-Metal
    __HAL_TIM_SET_COMPARE(servo_htim, servo_channel, pulse);
}
