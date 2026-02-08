/* This is the source for all of this mess 
 * https://github.com/faisalill/BMP390/blob/main/BMP390.cpp */

#include "bmp390.h"

#define u16(x, y) ((unsigned short)((x<<8) | y))

#define BMP390_I2C_ADDR_SEC 0x77
#define BMP3_REG_CHIP_ID 0x00
#define BMP3_REG_PWR_CTRL 0x1B
#define BMP3_REG_OSR 0X1C
#define BMP3_REG_ODR 0x1D
#define BMP3_REG_CONFIG 0x1F
#define BMP3_REG_DATA 0x04
#define BMP3_REG_CALIB_DATA 0x31
#define BMP3_REG_CMD 0x7E

#define BMP390_CHIP_ID 0x60

#define BMP3_MODE_SLEEP 0x00
#define BMP3_MODE_FORCED 0x01
#define BMP3_MODE_NORMAL 0x03

#define BMP3_NO_OVERSAMPLING 0x00
#define BMP3_OVERSAMPLING_2X 0x01
#define BMP3_OVERSAMPLING_4X 0x02
#define BMP3_OVERSAMPLING_8X 0x03
#define BMP3_OVERSAMPLING_16X 0x04
#define BMP3_OVERSAMPLING_32X 0x05

#define BMP3_IIR_FILTER_DISABLE 0x00

#define BMP3_ODR_25_HZ 0x03

#define BMP3_SOFT_RESET_CMD 0xB6

extern I2C_HandleTypeDef hi2c2;
extern PeripheralStates i2c1State;
double seaLevelPressure = 1013.25; // hPa

// Some compensation constants
double t1, t2, t3, t_lin;
double p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11;

static double compensate_temperature(int uncomp) {
    double partial_data1 = (double)(uncomp - t1);
    double partial_data2 = (double)(partial_data1 * t2);
    t_lin = partial_data2 + (partial_data1 * partial_data1) * t3;
    return t_lin;
}

static double compensate_pressure(int uncomp) {
    double partial_data1 = p6 * t_lin;
    double partial_data2 = p7 * (t_lin * t_lin);
    double partial_data3 = p8 * (t_lin * t_lin * t_lin);
    double partial_out1 = p5 + partial_data1 + partial_data2 + partial_data3;

    partial_data1 = p2 * t_lin;
    partial_data2 = p3 * (t_lin * t_lin);
    partial_data3 = p4 * (t_lin * t_lin * t_lin);
    double partial_out2 = (double)uncomp * (p1 + partial_data1 + partial_data2 + partial_data3);
    
    partial_data1 = (double)uncomp * (double)uncomp;
    double partial_data_p9_p10 = p9 + p10 * t_lin;
    double partial_data3_calc = partial_data1 * partial_data_p9_p10;
    double partial_data4 = partial_data3_calc + ((double)uncomp * (double)uncomp * (double)uncomp) * p11;

    return partial_out1 + partial_out2 + partial_data4;
}

int bmp390_init() {
    // Send the soft reset command
    unsigned char cmd = BMP3_SOFT_RESET_CMD;
    write_registers(&hi2c1, BMP390_I2C_ADDR_SEC, BMP3_REG_CMD, &cmd, 1);

    // Wait for the command to go through
    while (i2c1State == BUSY);

    // Now we get the calibration data
    unsigned char calibData[21];
    read_registers(&hi2c1, BMP390_I2C_ADDR_SEC, BMP3_REG_CALIB_DATA, calibData, 21);

    // Wait for the command to go through
    while (i2c1State == BUSY);

    int isAllZero = 1;
    for (int i=0; i<21; i++) {
        if (calibData[i] != 0) {
            isAllZero = 0;
            break;
        }
    }

    if (isAllZero == 1) {
        return 1;
    }

    // Basically this mess of calibration data is a combination of 8 and 16 bit registers
    // These registers mean fuck all to me, but there values are important to compensating
    // pressure and temperature
    
    // See defintion at the top for u16
    unsigned short par_t1_u = u16(reg_data[1], reg_data[0]);
    unsigned short par_t2_u = u16(reg_data[3], reg_data[2]);
    unsigned char   par_t3_s = (unsigned char)reg_data[4];

    short  par_p1_s = (short)u16(reg_data[6], reg_data[5]);
    short  par_p2_s = (short)u16(reg_data[8], reg_data[7]);
    unsigned char   par_p3_s = (unsigned char)reg_data[9];
    unsigned char   par_p4_s = (unsigned char)reg_data[10];
    unsigned short par_p5_u = u16(reg_data[12], reg_data[11]);
    unsigned short par_p6_u = u16(reg_data[14], reg_data[13]);
    unsigned char   par_p7_s = (unsigned char)reg_data[15];
    unsigned char   par_p8_s = (unsigned char)reg_data[16];
    short  par_p9_s = (short)u16(reg_data[18], reg_data[17]);
    unsigned char   par_p10_s = (unsigned char)reg_data[19];
    unsigned char   par_p11_s = (unsigned char)reg_data[20];

    t1 = (double)par_t1_u / 0.00390625f;
    t2 = (double)par_t2_u / 1073741824.0f;
    t3 = (double)par_t3_s / 281474976710656.0f;

    p1 = ((double)par_p1_s - 16384.0) / 1048576.0f;
    p2 = ((double)par_p2_s - 16384.0) / 536870912.0f;
    p3 = (double)par_p3_s / 4294967296.0f;
    p4 = (double)par_p4_s / 137438953472.0f;
    p5 = (double)par_p5_u / 0.125f;
    p6 = (double)par_p6_u / 64.0f;
    p7 = (double)par_p7_s / 256.0f;
    p8 = (double)par_p8_s / 32768.0f;
    p9 = (double)par_p9_s / 281474976710656.0f;
    p10 = (double)par_p10_s / 281474976710656.0f;
    p11 = (double)par_p11_s / 36893488147419103232.0f;

    return 0;
}

void set_sea_level_pressure(double pressure) {
    seaLevelPressure = pressure;
}

double get_altitude() {
    unsigned char bmpData[6];

    // Get register preassure and temperature data from the BMP390
    read_registers(&hi2c1, BMP390_I2C_ADDR_SEC, BMP3_REG_DATA, bmpData, 6);

    while (i2c1State == BUSY);

    // Convert the 8 bit registers to 32 bit numbers
    int uncompensatedPressure = (int)((bmpData[2] << 16) | (bmpData[1]<<8) | (bmpData[0]));
    int uncompensatedTemperature = (int)((bmpData[5] << 16) | (bmpData[4]<<8) | (bmpData[3]));

    // YOU MUST CALL TEMPERATURE COMPENSATION BEFORE PRESSURE
    double temperature = compensate_temperature(uncompensatedTemperature);
    double pressure = compensate_pressure(uncompensatedPressure);

    // I gotta finish the altitude formula
    const double R = 8.31432;
    const double M = 0.0289644;
    const double g = 9.90665;
    double altitude;

    // Convert pressure and temperature to altitude in meters (assuming we're not to high)
    if (pressure/seaLevelPressure > 0.000001) {
        altitude = log(pressure/seaLevelPressure) * R * T / (g * M * -1);
    }
    else {
        altitude = 100000;
    }

    // Convert meters to feet
    return altitude * 3.2808399;
}