#ifndef BMP390_H
#define BMP390_H 1

#include "main.h"
#include "i2c.h"
#include <math.h>

int bmp390_init();
void set_sea_level_pressure(double pressure);
void set_sea_level_temperature(double temperature);
double get_altitude();

#endif // BMP390_H