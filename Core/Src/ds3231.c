#include "ds3231.h"

#define DS3231_ADDRESS 0b01101000

extern I2C_HandleTypeDef hi2c1;
extern PeripheralStates i2c1State;

// Converts a nice binary number to the horrible and stinky format the ds3231 rtc uses
static unsigned char format_number_to_timekeeping_register(unsigned char binary) {
    // Get the tens digit
    unsigned char tens = binary / 10;
    // Isolate the ones digit
    binary -= 10 * tens;

    //   X   tens (6-4)    ones (3-0)
    // | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
    return (tens<<4) | binary;
}

// Does the opposite of the above function
static unsigned char format_timekeeping_register_to_number(unsigned char timeKeepingRegister) {
    // Get the tens digit
    int tens = ((timeKeepingRegister & 0x70)>>4);
    // Isolate the ones digit
    timeKeepingRegister &= (0x0F);

    return tens * 10 + timeKeepingRegister;
}

static void time_to_ds3231TimeKeepingRegisters(Time* time, Time* ds3231Time) {
    // Format all the normal binary numbers to the ds3231 format
    ds3231Time->seconds = format_number_to_timekeeping_register(time->seconds);
    ds3231Time->minutes = format_number_to_timekeeping_register(time->minutes);
    // Bit 6 is the 24 hour format bit (it has to be on for the 24 hour format)
    ds3231Time->hours = (1<<6) | format_number_to_timekeeping_register(time->hours);
    ds3231Time->dayOfWeek = format_number_to_timekeeping_register(time->dayOfWeek);
    ds3231Time->dayOfMonth = format_number_to_timekeeping_register(time->dayOfMonth);
    ds3231Time->month = format_number_to_timekeeping_register(time->month);
    ds3231Time->year = format_number_to_timekeeping_register(time->year);
}

static void ds3231TimeKeepingRegisters_to_time(Time* ds3231Time, Time* time) {
    time->seconds = format_timekeeping_register_to_number(ds3231Time->seconds);
    time->minutes = format_timekeeping_register_to_number(ds3231Time->minutes);
    // Bit 6 is the 24 hour format bit (we need to clear it before calling the function)
    time->hours = format_timekeeping_register_to_number((ds3231Time->hours & ~(1<<6)));
    time->dayOfWeek = format_number_to_timekeeping_register(ds3231Time->dayOfWeek);
    time->dayOfMonth = format_timekeeping_register_to_number(ds3231Time->dayOfMonth);
    time->month = format_timekeeping_register_to_number(ds3231Time->month);
    time->year = format_timekeeping_register_to_number(ds3231Time->year);
}

int set_ds3231_time(Time* time) {   // 1 on failure, 0 on success
    Time ds3231Time;
    time_to_ds3231TimeKeepingRegisters(time, &ds3231Time);

    HAL_StatusTypeDef status = i2c_write_registers()

    return 0;
}

int set_ds3231_alarm(Time* time) {  // 1 on failure, 0 on success
    Time ds3231Alarm;
    time_to_ds3231TimeKeepingRegisters(time, &ds3231Alarm);

    return 0;
}

int get_ds3231_time(Time* time) {   // 1 on failure, 0 on success
    // Some bullshit to get the time

    return 0;
}