/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "BMPXX80.h"
#include "icm20948.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BMP280_ADDR 0xEC // Adresse I2C (0x76 << 1)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Redirection printf vers Tera Term
int _write(int file, char *ptr, int len) {
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();

  /* USER CODE BEGIN 2 */
  printf("\r\n--- Systeme Avionique : Valeurs Reelles ---\r\n");

  ssd1306_Init();
  ssd1306_Fill(Black);
  ssd1306_UpdateScreen();

  // Initialisation via bibliothèque (lit les réglages de calibration) [cite: 87]
  BMP280_Init(&hi2c1, BMP280_TEMPERATURE_16BIT, BMP280_STANDARD, BMP280_FORCEDMODE);

  printf("Capteur calibre et pret.\r\n");

  //Initialisation du gyro via la bibliothèque
  if (ICM20948_Init(&hi2c1) == 0) {
      printf("Gyro ICM-20948 Pret et configure !\r\n");
  } else {
      printf("Erreur d'initialisation du Gyroscope.\r\n");
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
    {
        // --- 1. Variables pour stocker les mesures ---
        float temperature = 0.0f;
        int32_t pressure_pa = 0;

        float ax = 0.0f, ay = 0.0f, az = 0.0f;
        float gx = 0.0f, gy = 0.0f, gz = 0.0f;

        char oled_buf[32];

        // --- 2. Lecture de tous les capteurs ---
        uint8_t bmp_ok   = (BMP280_ReadTemperatureAndPressure(&temperature, &pressure_pa) == 0);
        uint8_t accel_ok = (ICM20948_ReadAccel(&ax, &ay, &az) == 0);
        uint8_t gyro_ok  = (ICM20948_ReadGyro(&gx, &gy, &gz) == 0);

        // --- 3. Début du dessin sur l'écran (On nettoie tout) ---
        ssd1306_Fill(Black);

        // ==========================================
        // PARTIE GAUCHE : DONNÉES MÉTÉO (BMP280)
        // ==========================================
        if (bmp_ok) {
            // Astuce de découpage pour éviter le plantage du %f
            int t_entier = (int)temperature;
            int t_dec = (int)((temperature - t_entier) * 100);
            if(t_dec < 0) t_dec = -t_dec;
            int p_hpa = pressure_pa / 100;

            // Trace sur Tera Term (Optionnel)
            printf("T: %d.%02d C | P: %d hPa\r\n", t_entier, t_dec, p_hpa);

            // Affichage OLED
            ssd1306_SetCursor(2, 15);
            sprintf(oled_buf, "T:%d.%01d C", t_entier, t_dec / 10);
            ssd1306_WriteString(oled_buf, Font_7x10, White);

            ssd1306_SetCursor(2, 35);
            sprintf(oled_buf, "P:%d", p_hpa);
            ssd1306_WriteString(oled_buf, Font_7x10, White);

            ssd1306_SetCursor(2, 47);
            ssd1306_WriteString("hPa", Font_7x10, White);
        } else {
            ssd1306_SetCursor(2, 25);
            ssd1306_WriteString("BMP: ERR", Font_7x10, White);
        }

        // --- Ligne de séparation au milieu de l'écran ---
        ssd1306_Line(62, 0, 62, 64, White);

        // ==========================================
        // PARTIE DROITE : HORIZON ARTIFICIEL (ICM20948)
        // ==========================================
        // Centre de la moitié droite (X = 96, Y = 32)
        int center_x = 96;
        int center_y = 32;

        // Dessin du réticule central fixe (L'avion)
        ssd1306_DrawCircle(center_x, center_y, 6, White);
        ssd1306_DrawPixel(center_x, center_y, White);

        if (accel_ok) {
            // Calcul des décalages d'inclinaison
            // 'ay' fait monter/descendre (tangage), 'ax' fait tourner (roulis)
            int pitch_offset = (int)(ay * 18.0f);
            int roll_effect = (int)(ax * 15.0f);

            // Coordonnées de la ligne d'horizon mobile
            int x1 = center_x - 22;
            int y1 = center_y + pitch_offset - roll_effect;

            int x2 = center_x + 22;
            int y2 = center_y + pitch_offset + roll_effect;

            // Blocage de sécurité pour ne pas dessiner hors de l'écran
            if (y1 < 0) y1 = 0;  if (y1 > 63) y1 = 63;
            if (y2 < 0) y2 = 0;  if (y2 > 63) y2 = 63;

            // Dessin de l'horizon
            ssd1306_Line(x1, y1, x2, y2, White);

            // Trace console
            int ax_int = (int)(ax * 100);
            int ay_int = (int)(ay * 100);
            printf("ICM -> Ax: %d | Ay: %d\r\n", ax_int, ay_int);
        } else {
            // Croix d'erreur si le capteur est débranché
            ssd1306_Line(76, 12, 116, 52, White);
            ssd1306_Line(116, 12, 76, 52, White);
        }

        // --- 4. Envoi de toute l'image construite vers l'OLED ---
        ssd1306_UpdateScreen();

        // Délai très court (50 ms) pour une animation fluide de l'horizon (20 FPS)
        HAL_Delay(50);

      /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_RCCEx_EnableMSIPLLMode();
}

/**
  * @brief I2C1 Initialization Function
  */
static void MX_I2C1_Init(void)
{
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00B07CB4;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  */
static void MX_USART2_UART_Init(void)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief GPIO Initialization Function
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LD3_Pin */
  GPIO_InitStruct.Pin = LD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD3_GPIO_Port, &GPIO_InitStruct);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
