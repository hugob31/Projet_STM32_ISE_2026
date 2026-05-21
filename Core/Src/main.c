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
#include "fatfs.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "BMPXX80.h"
#include "icm20948.h"
#include "sd_logger.h"
#include "arinc429.h"
#include "servo.h"
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

SPI_HandleTypeDef hspi3;

TIM_HandleTypeDef htim15;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI3_Init(void);
static void MX_TIM15_Init(void);
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

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_FATFS_Init();
  MX_SPI3_Init();
  MX_TIM15_Init();
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

  //Init carte SD
  //AJout délais
  HAL_Delay(1000);
  if (SD_Logger_Init() == 0) {
        printf("Carte SD initialisee ! Fichier VOL_001.CSV cree.\r\n");
    } else {
        printf("ERREUR Carte SD.\r\n");
    }


  // Initialisation du Servomoteur
  HAL_Delay(1000);
  Servo_Init(&htim15, TIM_CHANNEL_2);
  //HAL_TIM_PWM_Start(&htim16, TIM_CHANNEL_1);
  printf("Servomoteur initialise.\r\n");
  float yaw = 0.0f;
  // TEST BRUTE-FORCE : On force l'impulsion à 2000 µs (+90°)
    __HAL_TIM_SET_COMPARE(&htim15, TIM_CHANNEL_2, 2000);
    HAL_Delay(1000); // On attend 1 seconde pour voir s'il bouge
    __HAL_TIM_SET_COMPARE(&htim15, TIM_CHANNEL_2, 1000); // Puis on le force à -90°


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    while (1)
    {
    	//__HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 1500);
        // --- 1. Variables pour stocker les mesures ---
        float temperature = 0.0f;
        int32_t pressure_pa = 0;

        float ax = 0.0f, ay = 0.0f, az = 0.0f;
        float gx = 0.0f, gy = 0.0f, gz = 0.0f;

        char oled_buf[32];

        // --- 2. Lecture de tous les capteurs ---
        uint8_t bmp_ok   = (BMP280_ReadTemperatureAndPressure(&temperature, &pressure_pa) == 0);
        uint8_t accel_ok = (ICM20948_ReadAccel(&ax, &ay, &az) == 0);
        ICM20948_ReadGyro(&gx, &gy, &gz);

        // --- 3. Début du dessin sur l'écran (On nettoie tout) ---
        ssd1306_Fill(Black);

        // ==========================================
		// GESTION DU SERVOMOTEUR (LACET / YAW)
		// ==========================================
		if (gz > 2.0f || gz < -2.0f) {
			// Intégration de la vitesse pour obtenir l'angle
			yaw -= (gz * 0.05f);
		}

		// 1. Bridage mathématique de l'angle (-90° à +90°)
		if (yaw > 90.0f) yaw = 90.0f;
		if (yaw < -90.0f) yaw = -90.0f;

		// 2. Calcul direct de l'impulsion (5.55 µs par degré)
		uint32_t pulse = 1500 + (int32_t)(yaw * 5.55f);

		// 3. Sécurité mécanique absolue
		if (pulse < 1000) pulse = 1000;
		if (pulse > 2000) pulse = 2000;

		// 4. Ordre matériel direct au Timer 15 (On bypass le fichier servo.c)
		__HAL_TIM_SET_COMPARE(&htim15, TIM_CHANNEL_2, pulse);

		// 5. Affichage Tera Term complet pour vérifier que le Pulse suit bien l'angle
		printf("GZ: %.2f | Angle YAW: %.2f | Pulse PWM: %lu us\r\n", gz, yaw, pulse);
        //Servo_UpdateYaw(yaw);
        //__HAL_TIM_SET_COMPARE(&htim16, TIM_CHANNEL_1, 1500);
		//if (accel_ok) {
			// On met à jour l'angle.
			// On passe 'gz' (la vitesse lue du gyro) et '0.05f' (le temps de boucle 50ms)
			//Servo_UpdateYaw(gz, 0.05f);
		//}


        // ==========================================
        // PARTIE GAUCHE : DONNÉES MÉTÉO (BMP280)
        // ==========================================
        if (bmp_ok) {
            int t_entier = (int)temperature;
            int t_dec = (int)((temperature - t_entier) * 100);
            if(t_dec < 0) t_dec = -t_dec;
            int p_hpa = pressure_pa / 100;

            // ==========================================
            // Génération du mot ARINC 429
            // ==========================================

            uint32_t mon_mot_arinc = Generer_Mot_ARINC429_Pression(p_hpa);
            printf("Pression: %d hPa | Mot ARINC 429: 0x%08lX\r\n", p_hpa, mon_mot_arinc);

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
        int center_x = 96;
        int center_y = 32;

        ssd1306_DrawCircle(center_x, center_y, 6, White);
        ssd1306_DrawPixel(center_x, center_y, White);

        if (accel_ok) {
            int pitch_offset = (int)(ay * 18.0f);
            int roll_effect = (int)(ax * 15.0f);

            int x1 = center_x - 22;
            int y1 = center_y + pitch_offset - roll_effect;

            int x2 = center_x + 22;
            int y2 = center_y + pitch_offset + roll_effect;

            if (y1 < 0) { y1 = 0; }
            if (y1 > 63) { y1 = 63; }
            if (y2 < 0) { y2 = 0; }
            if (y2 > 63) { y2 = 63; }

            ssd1306_Line(x1, y1, x2, y2, White);
        } else {
            ssd1306_Line(76, 12, 116, 52, White);
            ssd1306_Line(116, 12, 76, 52, White);
        }

        // ==========================================
        // ENREGISTREMENT CARTE SD
        // ==========================================
        if (bmp_ok && accel_ok) {
            // On écrit sur la carte SD
            uint8_t sd_status = SD_Logger_Write(HAL_GetTick(), temperature, pressure_pa, ax, ay, az);

            if (sd_status == 0) {
                // Si l'écriture a réussi, on allume le "voyant" d'enregistrement sur l'OLED
                ssd1306_DrawPixel(120, 5, White);
                ssd1306_DrawPixel(121, 5, White); // On en met 4 pour faire un petit carré visible
                ssd1306_DrawPixel(120, 6, White);
                ssd1306_DrawPixel(121, 6, White);
            }
        }

        // --- 4. Envoi de toute l'image construite vers l'OLED ---
        ssd1306_UpdateScreen();

        // Délai pour dicter la vitesse d'enregistrement (50ms = 20 lignes par seconde)
        HAL_Delay(50);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
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

  /** Initializes the CPU, AHB and APB buses clocks
  */
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

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
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

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */
  SET_BIT(hspi3.Instance->CR2, SPI_CR2_FRXTH);   // Force le seuil FIFO en 8-bit
  CLEAR_BIT(hspi3.Instance->CR2, SPI_CR2_NSSP);  // Supprime les impulsions parasites

  /* USER CODE END SPI3_Init 2 */

}

/**
  * @brief TIM15 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM15_Init(void)
{

  /* USER CODE BEGIN TIM15_Init 0 */

  /* USER CODE END TIM15_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM15_Init 1 */

  /* USER CODE END TIM15_Init 1 */
  htim15.Instance = TIM15;
  htim15.Init.Prescaler = 31;
  htim15.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim15.Init.Period = 19999;
  htim15.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim15.Init.RepetitionCounter = 0;
  htim15.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim15) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim15, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim15) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim15, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim15, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim15, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM15_Init 2 */

  /* USER CODE END TIM15_Init 2 */
  HAL_TIM_MspPostInit(&htim15);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
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
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : SD_CS_Pin */
  GPIO_InitStruct.Pin = SD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SD_CS_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
