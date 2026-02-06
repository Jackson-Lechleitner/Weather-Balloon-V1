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
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef enum {PREFLIGHT, TAKEOFF, ASCENT, CRUISE, DESCENT, LANDING, RECOVERY} FlightStates;
typedef enum {IDLE, TEMP, HUMIDITY} I2CStates;

typedef struct {
  unsigned char seconds;
  unsigned char minutes;
  unsigned char hours;
  unsigned char dayOfMonth;
  unsigned char month;
  unsigned char year;
} Time;

typedef struct {
  int data;
  int buffer[4];
  const int id;
  const char* name;
} Sensor;
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
int set_ds3231_time(Time* time);    // 1 on failure, 0 on success
int set_ds3231_alarm(Time* time);   // 1 on failure, 0 on success
int get_ds3231_time(Time* time);    // 1 on failure, 0 on success

int get_position(double* lat, double* lon);

void preflight();
void takeoff();
void ascent();
void cruise();
void descent();
void landing();
void recovery();
/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define MCO_Pin GPIO_PIN_0
#define MCO_GPIO_Port GPIOH
#define ADC1_IN1_Pin GPIO_PIN_0
#define ADC1_IN1_GPIO_Port GPIOC
#define SD_Card_CLK_Pin GPIO_PIN_1
#define SD_Card_CLK_GPIO_Port GPIOA
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define SMPS_EN_Pin GPIO_PIN_4
#define SMPS_EN_GPIO_Port GPIOA
#define SMPS_V1_Pin GPIO_PIN_5
#define SMPS_V1_GPIO_Port GPIOA
#define SMPS_PG_Pin GPIO_PIN_6
#define SMPS_PG_GPIO_Port GPIOA
#define SMPS_SW_Pin GPIO_PIN_7
#define SMPS_SW_GPIO_Port GPIOA
#define DS3231_INT_PIN_Pin GPIO_PIN_4
#define DS3231_INT_PIN_GPIO_Port GPIOC
#define DS3231_INT_PIN_EXTI_IRQn EXTI4_IRQn
#define Sensor_SCL_Pin GPIO_PIN_10
#define Sensor_SCL_GPIO_Port GPIOB
#define Sensor_SDA_Pin GPIO_PIN_11
#define Sensor_SDA_GPIO_Port GPIOB
#define LD4_Pin GPIO_PIN_13
#define LD4_GPIO_Port GPIOB
#define Internal_Temp_SCL_Pin GPIO_PIN_9
#define Internal_Temp_SCL_GPIO_Port GPIOA
#define Internal_Temp_SDA_Pin GPIO_PIN_10
#define Internal_Temp_SDA_GPIO_Port GPIOA
#define SD_Card_MISO_Pin GPIO_PIN_11
#define SD_Card_MISO_GPIO_Port GPIOA
#define SD_Card_MOSI_Pin GPIO_PIN_12
#define SD_Card_MOSI_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define GPS_TX_Pin GPIO_PIN_6
#define GPS_TX_GPIO_Port GPIOB
#define GPS_RX_Pin GPIO_PIN_7
#define GPS_RX_GPIO_Port GPIOB
#define SD_Card_CS_Pin GPIO_PIN_9
#define SD_Card_CS_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define AM2320_ADDR 0x5C

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
