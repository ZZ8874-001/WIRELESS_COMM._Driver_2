/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32f3xx_hal.h"

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
#define VIN_Pin GPIO_PIN_0
#define VIN_GPIO_Port GPIOA
#define VOUT_Pin GPIO_PIN_1
#define VOUT_GPIO_Port GPIOA
#define IND1_Pin GPIO_PIN_3
#define IND1_GPIO_Port GPIOA
#define ENA_Pin GPIO_PIN_14
#define ENA_GPIO_Port GPIOB
#define ENB_Pin GPIO_PIN_15
#define ENB_GPIO_Port GPIOB
#define PULSEA_Pin GPIO_PIN_8
#define PULSEA_GPIO_Port GPIOA
#define PULSEB_Pin GPIO_PIN_9
#define PULSEB_GPIO_Port GPIOA
#define SYNC_R_Pin GPIO_PIN_11
#define SYNC_R_GPIO_Port GPIOA
#define CHARGE_EN_Pin GPIO_PIN_3
#define CHARGE_EN_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

#define BOARD_NUM 4
#define BOARD_UID_WORDS 3
#define BOARD_UID_0 { 0x00180012 , 0x59304310 , 0x20363852 }
#define BOARD_UID_1 { 0x00010005 , 0x5130430B , 0x20303157 }
#define BOARD_UID_2 { 0x00140014 , 0x53304310 , 0x20393041 }
#define BOARD_UID_3 { 0x0016000A , 0x51304316 , 0x20313142 }
extern int8_t IDCard;
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
