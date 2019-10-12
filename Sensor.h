#ifndef SENSOR_H
#define SENSOR_H


// Function definitions
int hFormat(int hours);
int hexCompensation(int units);
int decCompensation(int units);
void initGPIO(void);
void secPWM(int units);
void hourInc(void);
void minInc(void);
void toggleTime(void);
void sensors(void);
void checkSampling(void);
void checkAlarm(void);
void updateTime(void);
void reset(void);
// define constants
const char RTCAddr = 0x6f;
const char SEC = 0x00; // see register table in datasheet
const char MIN = 0x01;
const char HOUR = 0x02;
const char TIMEZONE = 2; // +02H00 (RSA)


// defines
#define BASE 100
#define SPI_ADC 0
#define SPI_DAC 1

// define pins
const int LEDS[] = {0,2,3,25,7,22,21,27,4,6};
const int SECS = 1;
const int BTNS[] = {0,1}; // B0, B1


#endif
