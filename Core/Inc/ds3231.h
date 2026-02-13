#ifndef DS3231_H
#define DS3231_H 1

#include "main.h"
#include "i2c.h"

typedef struct {
  unsigned char seconds;
  unsigned char minutes;
  unsigned char hours;
  unsigned char dayOfWeek;
  unsigned char dayOfMonth;
  unsigned char month;
  unsigned char year;
} Time;

// Set the time on the DS3231
int set_ds3231_time(Time* time);    // 1 on failure, 0 on success

// Set the time for the alarm to be triggered on the DS3231
int set_ds3231_alarm(Time* time);   // 1 on failure, 0 on success

// Get the current time
int get_ds3231_time(Time* time);    // 1 on failure, 0 on success

#endif // DS3231_H