/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "kalmanrollpitch.h"
#include "MPU9250.h"
#include "stdio.h"
#include "string.h"
#include "PID.h"
#include "EKFLite.h"

#define SAMPLE_TIME_ACC_MS  1
#define SAMPLE_TIME_GYR_MS  10000000
#define SAMPLE_TIME_COM_MS 200
#define SAMPLE_TIME_LED_MS 500

#define RAD_TO_DEG 57.2957795131f

//Peripherlas
I2C_HandleTypeDef hi2c1;
UART_HandleTypeDef huart2;
TIM_HandleTypeDef tim2;

MPU9250_Handle_t imu;
KalmanRollPitch ekf;
EKF_Handle_t ekfl;



void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);

static void MX_MPU9250_Init(void)
{

	imu.I2Chandle = &hi2c1;
	if(MPU9250_init(&imu) == 0)
	{
	  Error_Handler();
	}
}

int main(void)
{


  HAL_Init();

  SystemClock_Config();

  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_MPU9250_Init();

  float Q[2] = {0.1052360f, 0.1034907f};
  float R[3] = {0.0099270f, 0.0099270f, 0.011788f};

  KalmanRollPitch_Init(&ekf, 0.1f, Q, R);
  EKF_Init(&ekfl);

  uint32_t timerACC = 0;
  uint32_t timerGYR = 0;
  uint32_t timerCOM = 0;

  while (1)
  {

	  /* Accelerometer measurements and Kalman update */

	  if ((HAL_GetTick() - timerACC) >= SAMPLE_TIME_ACC_MS) {

		  MPU9250_ReadAccel(&imu);

		  KalmanRollPitch_Update(&ekf, imu.gyr, imu.acc, 0.0f);
		  //EKF_Update(&ekfl, imu.acc);

		  timerACC += SAMPLE_TIME_ACC_MS;
	  }
	  EKF_Update(&ekfl, imu.acc);

	  /* Gyro measurements and Kalman prediction */

	  if ((HAL_GetTick() - timerGYR) >= SAMPLE_TIME_GYR_MS) {

		  MPU9250_ReadGyro(&imu);

		  KalmanRollPitch_Predict(&ekf, imu.gyr, 0.001f * SAMPLE_TIME_GYR_MS);
		  //EKF_Predict(&ekfl, imu.gyr, 0.001f * SAMPLE_TIME_GYR_MS);
		  timerGYR += SAMPLE_TIME_GYR_MS;

	  }
	  EKF_Predict(&ekfl, imu.gyr, 0.001f * SAMPLE_TIME_GYR_MS);

	  if ((HAL_GetTick() - timerCOM) >= SAMPLE_TIME_COM_MS) {

		  uint8_t buf[64];
		  memset(buf, 0, sizeof buf);

//		  sprintf((char*)buf, "Roll:%f, Pitch:%f\r\n", ekf.phi * RAD_TO_DEG, ekf.theta * RAD_TO_DEG);
//
//		  HAL_UART_Transmit(&huart2, buf, sizeof(buf), HAL_MAX_DELAY);

		  sprintf((char*)buf, "RollKF:%f, PitchKF:%f\r\n", ekfl.roll, ekfl.pitch);

		  HAL_UART_Transmit(&huart2, buf, sizeof(buf), HAL_MAX_DELAY);

		  timerCOM += SAMPLE_TIME_COM_MS;

	  }

  }

}












void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
}

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

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

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

#ifdef  USE_FULL_ASSERT
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
