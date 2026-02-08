#ifndef FLIGHT_FUNCTIONS_H
#define FLIGHT_FUNCTIONS_H 1

#include "main.h"

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
typedef struct {
  unsigned char seconds;
  unsigned char minutes;
  unsigned char hours;
  unsigned char dayOfWeek;
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

typedef enum {IDLE, DS3231, AM2320} I2C1States;
typedef enum {IDLE, BMP390, AM2320} I2C2States;
typedef enum {IDLE, CMD_SENT, CMD_ERR} ESP32P4States;
/* USER CODE END ET */

/* Exported functions prototypes ---------------------------------------------*/

/* USER CODE BEGIN EFP */
int write_to_ds3231_registers(unsigned char startingReg, unsigned char* data, int size);  // 1 on failure, 0 on success
int set_ds3231_time(Time* time);    // 1 on failure, 0 on success
int set_ds3231_alarm(Time* time);   // 1 on failure, 0 on success
int get_ds3231_time(Time* time);    // 1 on failure, 0 on success

int get_position(double* lat, double* lon);

void takeoff();
void ascent();
void cruise();
void descent();
void landing();
void recovery();
/* USER CODE END EFP */

#endif // FLIGHT_FUNCTIONS_H