#include "servo.h"

// Variables "static" = privées à ce fichier. Le main.c ne peut pas les modifier par erreur !
static TIM_HandleTypeDef *servo_htim;
static uint32_t servo_channel;
static float current_yaw = 0.0f; // Garde l'angle en mémoire entre chaque appel

// Initialisation du servomoteur
void Servo_Init(TIM_HandleTypeDef *htim, uint32_t channel) {
    servo_htim = htim;
    servo_channel = channel;

    // Démarrage de la génération PWM matérielle
    HAL_TIM_PWM_Start(servo_htim, servo_channel);

    // Position neutre au démarrage (1500 µs)
    __HAL_TIM_SET_COMPARE(servo_htim, servo_channel, 1500);
}

// Mise à jour de la position en fonction du gyroscope
void Servo_UpdateYaw(float gz_rate, float dt) {
    // 1. Intégration mathématique
    current_yaw += (gz_rate * dt);

    // 2. Saturation mécanique (pour ne pas casser le moteur)
    if (current_yaw > 90.0f) current_yaw = 90.0f;
    if (current_yaw < -90.0f) current_yaw = -90.0f;

    // 3. Calcul de la largeur d'impulsion (Pulse en µs)
    uint32_t pulse = 1500 + (int32_t)(current_yaw * 5.55f);

    // 4. Écriture directe dans le registre matériel CCR (Bare-Metal)
    __HAL_TIM_SET_COMPARE(servo_htim, servo_channel, pulse);
}
