#ifndef SERVO_H
#define SERVO_H

#include "main.h" // Pour avoir accès aux types HAL (TIM_HandleTypeDef, etc.)

// Prototypes des fonctions
void Servo_Init(TIM_HandleTypeDef *htim, uint32_t channel);
void Servo_UpdateYaw(float gz_rate, float dt);

#endif /* SERVO_H */
