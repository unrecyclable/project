/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define pwm5_Pin GPIO_PIN_0
#define pwm5_GPIO_Port GPIOA
#define pwm6_Pin GPIO_PIN_1
#define pwm6_GPIO_Port GPIOA
#define pwm7_Pin GPIO_PIN_2
#define pwm7_GPIO_Port GPIOA
#define pwm8_Pin GPIO_PIN_3
#define pwm8_GPIO_Port GPIOA
#define pwm1_Pin GPIO_PIN_8
#define pwm1_GPIO_Port GPIOA
#define pwm2_Pin GPIO_PIN_9
#define pwm2_GPIO_Port GPIOA
#define pwm3_Pin GPIO_PIN_10
#define pwm3_GPIO_Port GPIOA
#define pwm4_Pin GPIO_PIN_11
#define pwm4_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
