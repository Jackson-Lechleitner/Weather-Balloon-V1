#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>
#include <string.h>
#include <math.h>

#include <HardwareSerial.h>
#include <HardwareTimer.h>

#include <DS3231.h>
#include <BMP390.h>
#include <AM232X.h>

#define SD_CS_PIN 10
#define RTC_INT_PIN 2
#define BUFFER_SIZE 512
#define RTC_INPUT_SIZE 24

typedef enum { PREFLIGHT, TAKEOFF, CRUISE, LANDING, RECOVERY } FlightStates;

struct SensorReadings {
  double internalTempC;
  double internalHumidityPct;
  double externalTempC;
  double externalHumidityPct;
  double bmpPressure;
  double bmpTemperatureC;
};

struct Bmp390Readings {
  double pressure;
  double temperatureC;
};

// DS3231 Functions
void rtc_alarm_isr();
void set_alarm(int minutesFromNow);
void service_rtc_alarm();
bool read_rtc_time_from_serial(char* rtcRxBuffer, size_t bufferSize);

// Sensor functions
bool read_am2320(AM232X& sensor, double& temperatureC, double& humidityPct);
bool read_bmp390(double& pressure, double& temperatureC);
void read_all_sensors(SensorReadings& readings);
void print_sensor_readings(const SensorReadings& readings);
void log_sensor_data();

// BMP390 / altitude
double pressure_to_altitude(double pressurePa, double seaLevelPressurePa = 101325.0);

// GPS
void get_gps_position();

// Command helpers
void process_command(const char* command);
void trim_line_endings(char* str);

// Hah, funny typo (don't fix it)
int serial_str_to_numer(const char* str, int startIdx, int endIdx);

// Hardware peripherals on the L4
HardwareTimer hwTimer(TIM2);
HardwareSerial gps(USART1);

// On some STM32 cores this may need explicit SDA/SCL pins instead of (1).
TwoWire Wire2(1); // I2C2

// Time keeper
DS3231 rtc;
BMP390 bmp(PC1, PC0);
AM232X internalAm2320(&Wire2);
AM232X externalAm2320(&Wire);

// State
volatile bool rtcAlarmTriggered = false;
bool sdReady = false;
FlightStates flightState = PREFLIGHT;

SensorReadings sensorReadings;

const char* LOG_FILE_NAME = "flight.csv";

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);

  gps.begin(9600);
  gps.setTimeout(25);

  Wire.begin();
  Wire.setClock(100000);

  Wire2.begin();
  Wire2.setClock(100000);

  SPI.begin();
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  sdReady = SD.begin(SD_CS_PIN);
  if (!sdReady) {
    Serial.println("SD init failed");
  } else {
    Serial.println("SD init ok");
  }

  // If your DS3231 library requires an explicit begin/init call, do it here.
  // If your BMP390 / AM2320 libraries require begin/init calls, do it here too.

  Serial.println("Send RTC time as: ss:mm:hh:dd:mm:yy");
  char rtcRxBuffer[RTC_INPUT_SIZE] = {0};
  if (read_rtc_time_from_serial(rtcRxBuffer, sizeof(rtcRxBuffer))) {
    rtc.turnOffAlarm(1);

    rtc.setSecond(serial_str_to_numer(rtcRxBuffer, 0, 1));
    rtc.setMinute(serial_str_to_numer(rtcRxBuffer, 3, 4));
    rtc.setHour(serial_str_to_numer(rtcRxBuffer, 6, 7));
    rtc.setDate(serial_str_to_numer(rtcRxBuffer, 9, 10));
    rtc.setMonth(serial_str_to_numer(rtcRxBuffer, 12, 13));
    rtc.setYear(serial_str_to_numer(rtcRxBuffer, 15, 16));

    Serial.println("RTC time set");
  } else {
    Serial.println("RTC input failed or incomplete");
  }

  pinMode(RTC_INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(RTC_INT_PIN), rtc_alarm_isr, FALLING);

  set_alarm(1);
}

void loop() {
  if (rtcAlarmTriggered) {
    rtcAlarmTriggered = false;
    service_rtc_alarm();
  }

  switch (flightState) {
    case PREFLIGHT:
      if (Serial.available()) {
        char commandBuffer[64];
        size_t commandLen = Serial.readBytesUntil('\n', commandBuffer, sizeof(commandBuffer) - 1);
        commandBuffer[commandLen] = '\0';
        trim_line_endings(commandBuffer);
        process_command(commandBuffer);
      }
      break;

    case TAKEOFF:
      read_all_sensors(sensorReadings);
      print_sensor_readings(sensorReadings);
      log_sensor_data();
      break;

    case CRUISE:
      if (rtcAlarmTriggered) {
        rtcAlarmTriggered = false;
        service_rtc_alarm();
        read_all_sensors(sensorReadings);
        log_sensor_data();
      }

      // Enter stop mode until the RTC alarm or another interrupt wakes the MCU.
      // After waking from STOP, some STM32 cores need clock re-init depending on framework/version.
      HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
      break;

    case LANDING:
      read_all_sensors(sensorReadings);
      print_sensor_readings(sensorReadings);
      log_sensor_data();
      break;

    case RECOVERY:
      break;

    default:
      break;
  }
}

void rtc_alarm_isr() {
  rtcAlarmTriggered = true;
}

void service_rtc_alarm() {
  rtc.turnOffAlarm(1);
  set_alarm(1);
}

void set_alarm(int minutesFromNow) {
  rtc.turnOffAlarm(1);

  bool hr12 = false, pmFlag = false;
  int minutes = rtc.getMinute();
  int hours = rtc.getHour(hr12, pmFlag);

  int totalMinutes = minutes + minutesFromNow;
  hours += totalMinutes / 60;
  minutes = totalMinutes % 60;
  hours %= 24;

  rtc.setAlarm1Simple(hours, minutes);
  rtc.turnOnAlarm(1);
}

bool read_rtc_time_from_serial(char* rtcRxBuffer, size_t bufferSize) {
  if (rtcRxBuffer == nullptr || bufferSize < 2) {
    return false;
  }

  size_t index = 0;
  unsigned long startTime = millis();

  while (millis() - startTime < 15000UL && index < bufferSize - 1) {
    while (!Serial.available()) {
      if (millis() - startTime >= 15000UL) {
        rtcRxBuffer[index] = '\0';
        return false;
      }
    }

    char c = (char)Serial.read();

    if (c == '\r') {
      continue;
    }

    if (c == '\n') {
      break;
    }

    rtcRxBuffer[index++] = c;
  }

  rtcRxBuffer[index] = '\0';
  return index >= 17;
}

bool read_am2320(AM232X& sensor, double& temperatureC, double& humidityPct) {
  temperatureC = sensor.getTemperature();
  humidityPct = sensor.getHumidity();

  if (isnan(temperatureC) || isnan(humidityPct)) {
    return false;
  }

  return true;
}

bool read_bmp390(double& pressure, double& temperatureC) {
  bmp3_data data = bmp.get_bmp_values();

  pressure = (double)data.pressure;
  temperatureC = (double)data.temperature;

  return true;
}

void read_all_sensors(SensorReadings& readings) {
  readings.internalTempC = NAN;
  readings.internalHumidityPct = NAN;
  readings.externalTempC = NAN;
  readings.externalHumidityPct = NAN;
  readings.bmpPressure = NAN;
  readings.bmpTemperatureC = NAN;

  read_am2320(internalAm2320, readings.internalTempC, readings.internalHumidityPct);
  read_am2320(externalAm2320, readings.externalTempC, readings.externalHumidityPct);
  read_bmp390(readings.bmpPressure, readings.bmpTemperatureC);
}

void print_sensor_readings(const SensorReadings& readings) {
  Serial.print("Internal AM2320 Temp C: ");
  Serial.println(readings.internalTempC);

  Serial.print("Internal AM2320 Humidity %: ");
  Serial.println(readings.internalHumidityPct);

  Serial.print("External AM2320 Temp C: ");
  Serial.println(readings.externalTempC);

  Serial.print("External AM2320 Humidity %: ");
  Serial.println(readings.externalHumidityPct);

  Serial.print("BMP390 Pressure: ");
  Serial.println(readings.bmpPressure);

  Serial.print("BMP390 Temp C: ");
  Serial.println(readings.bmpTemperatureC);

  Serial.print("BMP390 Altitude estimate m: ");
  Serial.println(pressure_to_altitude(readings.bmpPressure));
}

void log_sensor_data() {
  if (!sdReady) {
    return;
  }

  File logFile = SD.open(LOG_FILE_NAME, FILE_WRITE);
  if (!logFile) {
    Serial.println("Log file open failed");
    return;
  }

  logFile.print(millis());
  logFile.print(',');

  logFile.print((isnan(sensorReadings.internalTempC) ? 0.0 : sensorReadings.internalTempC), 2);
  logFile.print(',');
  logFile.print((isnan(sensorReadings.internalHumidityPct) ? 0.0 : sensorReadings.internalHumidityPct), 2);
  logFile.print(',');

  logFile.print((isnan(sensorReadings.externalTempC) ? 0.0 : sensorReadings.externalTempC), 2);
  logFile.print(',');
  logFile.print((isnan(sensorReadings.externalHumidityPct) ? 0.0 : sensorReadings.externalHumidityPct), 2);
  logFile.print(',');

  logFile.print((isnan(sensorReadings.bmpPressure) ? 0.0 : sensorReadings.bmpPressure), 2);
  logFile.print(',');
  logFile.print((isnan(sensorReadings.bmpTemperatureC) ? 0.0 : sensorReadings.bmpTemperatureC), 2);
  logFile.print(',');
  logFile.println(pressure_to_altitude(sensorReadings.bmpPressure), 2);

  logFile.close();
}

double pressure_to_altitude(double pressurePa, double seaLevelPressurePa) {
  if (pressurePa <= 0.0 || seaLevelPressurePa <= 0.0) {
    return NAN;
  }

  return 44330.0 * (1.0 - pow(pressurePa / seaLevelPressurePa, 0.1903));
}

void process_command(const char* command) {
  if (strcmp(command, "TAKEOFF") == 0) {
    Serial.println("TAKEOFF command received, switching to TAKEOFF state");
    flightState = TAKEOFF;
    return;
  }

  if (strcmp(command, "AM2320 INTERNAL TEMP") == 0) {
    double tempC = NAN, humidityPct = NAN;
    read_am2320(internalAm2320, tempC, humidityPct);
    Serial.println(tempC);
    return;
  }

  if (strcmp(command, "AM2320 INTERNAL HUMIDITY") == 0) {
    double tempC = NAN, humidityPct = NAN;
    read_am2320(internalAm2320, tempC, humidityPct);
    Serial.println(humidityPct);
    return;
  }

  if (strcmp(command, "AM2320 EXTERNAL TEMP") == 0) {
    double tempC = NAN, humidityPct = NAN;
    read_am2320(externalAm2320, tempC, humidityPct);
    Serial.println(tempC);
    return;
  }

  if (strcmp(command, "AM2320 EXTERNAL HUMIDITY") == 0) {
    double tempC = NAN, humidityPct = NAN;
    read_am2320(externalAm2320, tempC, humidityPct);
    Serial.println(humidityPct);
    return;
  }

  if (strcmp(command, "BMP390") == 0) {
    double pressure = NAN, tempC = NAN;
    read_bmp390(pressure, tempC);
    Serial.print("Pressure: ");
    Serial.println(pressure);
    Serial.print("Temperature: ");
    Serial.println(tempC);
    return;
  }

  if (strcmp(command, "BMP390 ALTITUDE") == 0) {
    double pressure = NAN, tempC = NAN;
    read_bmp390(pressure, tempC);
    Serial.println(pressure_to_altitude(pressure));
    return;
  }

  if (strcmp(command, "GPS") == 0) {
    get_gps_position();
    return;
  }

  if (strcmp(command, "LOG") == 0) {
    read_all_sensors(sensorReadings);
    log_sensor_data();
    Serial.println("Logged");
    return;
  }

  Serial.println("Invalid command");
}

void trim_line_endings(char* str) {
  if (str == nullptr) {
    return;
  }

  size_t len = strlen(str);
  while (len > 0 && (str[len - 1] == '\r' || str[len - 1] == '\n' || str[len - 1] == ' ' || str[len - 1] == '\t')) {
    str[len - 1] = '\0';
    len--;
  }
}

void get_gps_position() {
  char gpsBuffer[256];
  unsigned long prevTime = millis();

  while (millis() - prevTime < 3000UL) {
    if (gps.available()) {
      size_t len = gps.readBytesUntil('\n', gpsBuffer, sizeof(gpsBuffer) - 1);
      gpsBuffer[len] = '\0';
      trim_line_endings(gpsBuffer);
      if (len > 0) {
        Serial.println(gpsBuffer);
      }
    }
  }
}

int serial_str_to_numer(const char* str, int startIdx, int endIdx) {
  int num = 0;

  if (str == nullptr || startIdx < 0 || endIdx < startIdx) {
    return 0;
  }

  for (int i = startIdx; i <= endIdx; i++) {
    if (str[i] < '0' || str[i] > '9') {
      continue;
    }
    num *= 10;
    num += str[i] - '0';
  }

  return num;
}