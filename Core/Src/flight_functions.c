#include "flight_functions.h"

//================================================================================
// Defines
//================================================================================

// A bunch of I2C device addresses
#define DS3231_ADDRESS 0b01101000

//================================================================================
// Variables
//================================================================================

// External variables
extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

// Some sensor structs with identifying information and read buffers
Sensor internalAM2320 = {0, {0, 0, 0, 0}, AM2320_ADDR, "Internal Temp and Humidity"};
Sensor externalAM2320 = {0, {0, 0, 0, 0}, AM2320_ADDR, "External Temp and Humidity"};
Sensor no2 = {0, {0, 0, 0, 0}, 0, "NO2 Sensor"};
Sensor bmp390 = {0, {0, 0, 0, 0}, 0, "Altitude Sensor"};

// UART Buffers
UartBuffer gpsRx;
UartBuffer esp32p4Tx, esp32p4Rx;
UartBuffer swarmTx, swarmRx;

// Flight State Tracker
FlightStates flightState = PREFLIGHT;

//================================================================================
// DS3231 Real Time Clock Code
//================================================================================

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

int write_to_ds3231_registers(unsigned char startingReg, unsigned char* data, int size) {
    unsigned char* buffer = (unsigned char*)malloc(sizeof(unsigned char) * startingReg + size);
    buffer[0] = startingReg;
    memcpy(&buffer[1], data, size);

    if (HAL_I2C_Master_Transmit_DMA(&hi2c1, DS3231_ADDRESS, buffer, size+1) != HAL_OK) {
        // I need to figure out some error handeling
        return 1;
    }

    // Start a timer and add an extra flag to 

    return 0;
}

int set_ds3231_time(Time* time) {   // 1 on failure, 0 on success
    Time ds3231Time;
    time_to_ds3231TimeKeepingRegisters(time, &ds3231Time);

    write_to_ds3231_registers()

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

//
//
//

int get_position(double* lat, double* lon) {

}

//
//
//

static void esp32_p4_transmit() {
    
}

static void esp32_p4_recieve() {
    if (esp32p4Rx.byteBuffer != '\n') {
        esp32p4Rx.buffer[esp32p4Rx.idx++] = esp32p4Rx.byteBuffer;
        return;
    }

    // Check if we recieved the ACK signal from the ESP32-P4 after a command
    // cant use strcmp because commands end in \n
    if (memcmp(esp32p4Rx.buffer, "OK\n", 3) == 0) {

    }
}

static void swarm_recieve() {
    if (swarmRx.byteBuffer != '\n') {
        swarmRx.buffer[swarmRx.idx++] = swarmRx.byteBuffer;
        return;
    }

    // Switch stages command
    if (memcmp(swarmRx.buffer, "SET STAGE", 9) == 0) {
        const char* stages[] = {"TAKEOFF", "ASCENT", "CRUISE", "DESCENT", "LANDING", "RECOVERY"};

        for (int stage=0; stage<7; stage++) {
            if (memcmp(&swarmRx.buffer[9], stages[stage], strlen(stages[i])) == 0) {
                
            }
        }
    }
    // Take reading command to test individual components
    else if (memcmp(swarmRx.buffer, "TEST", 4)) {

    }
}

//UART RX Callback Functions
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {

    switch (huart->Instance) {
    // Data from the GPS via USART
    case USART1:
        break;

    // Data from the ESP32-P4 via USART
    case USART2:
    esp
        break;

    // Data recieved over Swarm via USART
    case USART3:
        swarm_recieve();
        HAL_UART_Receive_IT(huart, &swarmRx.byteBuffer, sizeof(swarmRx.byteBuffer));
        break;
    default:
        break;
    }
}

//
//
//

void takeoff() {

}

void ascent() {

}

void cruise() {

}

void descent() {

}

void landing() {

}

void recovery() {

}