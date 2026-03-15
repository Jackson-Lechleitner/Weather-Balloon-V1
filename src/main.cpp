#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>

#include <HardwareSerial.h>

#include <DS3231.h>
#include <BMP390.h>

#define SD_CS_PIN 10

#define RTC_INT_PIN 2
#define BUFFER_SIZE 512

typedef struct {
  int8_t data[BUFFER_SIZE];
  int size;
} Buffer;

typedef enum {PREFLIGHT, TAKEOFF, CRUISE, LANDING, RECOVERY} FlightStates;

// DS3231 Functions
void rtc_alarm_isr();
void set_alarm(int minutesFromNow);

// BMP390 get data
double get_atm_pressure();
// External AM2320 reading
double get_external_temperature();
// Internal AM2320 reading
double get_internal_temperature();

// Hah, funny typo (don't fix it)
int serial_str_to_numer(int startIdx, int endIdx);

// Hardware peripherals on the L4
HardwareTimer hwTimer(TIM2);
HardwareSerial gps(USART1);

// Time keeper
DS3231 rtc;
BMP390 bmp(PC1, PC0);
DateTime rtcTime;
FlightStates flightState = PREFLIGHT;
Buffer ser1Tx, ser1Rx;

void setup() {
  // Usart 2, command interface
  Serial.begin(115200); 
  // Usart 1, GPS interface
  gps.begin(9600);

  // Should only need 20 characters
  ser1Rx.size = 0;
  while (ser1Rx.size == 20) {
    // Wait to recv data
    while (!Serial.available());

    // Add the char to the buffer
    ser1Rx.data[ser1Rx.size++] = Serial.read();
  }

  // Alarm can be ignored for now
  rtc.turnOffAlarm(1);

  // Set time (also converts recieved string to numbers)
  rtc.setSecond(serial_str_to_numer(0, 1));
  rtc.setMinute(serial_str_to_numer(3, 4));
  rtc.setHour(serial_str_to_numer(6, 7));
  rtc.setDate(serial_str_to_numer(9, 10));
  rtc.setMonth(serial_str_to_numer(12, 13));
  rtc.setYear(serial_str_to_numer(15, 16));

  // Set an interrupt on the DS3231 INT pin
  pinMode(RTC_INT_PIN, INPUT_PULLUP);
  attachInterrupt(RTC_INT_PIN, rtc_alarm_isr, FALLING);
}

void loop() {
  switch (flightState) {
  case PREFLIGHT:

    break;
  case TAKEOFF:
  
    break;
  case CRUISE:
    // Power down the device after the rtc interrupt
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI); 
    break;
  case LANDING:
    
    break;
  case RECOVERY:
    break;
  default:
    break;
  }
}

void rtc_alarm_isr() {
  Serial.println("Huzzah");
  set_alarm(1);
}

void set_alarm(int minutesFromNow) {
  // Disable the alarm
  rtc.turnOffAlarm(1);

  // Get the time
  bool hr12, pmFlag;
  int minutes = rtc.getMinute();
  int hours = rtc.getHour(hr12, pmFlag);
  minutes += minutesFromNow;
  
  // Check for minutes overflow
  if (minutes > 59) {
    // Advance hours
    hours++;
    // Check for hours overflow
    if (hours > 23) hours = 0;
    minutes = 0;
  }
  
  // Update and enable the rtc alarm
  rtc.setAlarm1Simple(hours, minutes);
  rtc.turnOnAlarm(1);
}

// BMP390 get data
double get_atm_pressure() {
  bmp3_data data = bmp.get_bmp_values();
  return (float)data.pressure;
}

// External AM2320 reading
double get_external_temperature() {
  return 0.0;
}

// Internal AM2320 reading
double get_internal_temperature() {
  return 0.0;
}

// "Numer"
int serial_str_to_numer(int startIdx, int endIdx) {
  int num = 0;
  for (int i=startIdx; i<=endIdx; i++) {
    num *= 10;
    num += ser1Rx.data[i] - '0';
  }

  return num;
}
