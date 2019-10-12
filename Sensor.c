/* * BinClock.c * Jarrod Olivier * Modified for EEE3095S/3096S by Keegan 
 Crankshaw * August 2019 * * <JRDMUH002> <DDNAAD001> * 08 OCTOBER 2019
*/

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h> //For printf functions
#include <stdlib.h> // For system functions
#include <mcp3004.h>//for adc
#include "Sensor.h"
#include "CurrentTime.h"

//Global variables
int sample_time=1;
int hours, mins, secs; //use for system
long lastInterruptTime = 0; //Used for button debounce
int RTC; //Holds the RTC instance
int alarm_ready;
int alarm_timer;
int HH,MM,SS;
int pot;
int ldr;
float temp_sensor;
int RTC_hours, RTC_mins, RTC_secs;
void initGPIO(void){
	/* 
	 * Sets GPIO using wiringPi pins. see pinout.xyz for specific wiringPi pins
	 * You can also use "gpio readall" in the command line to get the pins
	 * Note: wiringPi does not use GPIO or board pin numbers (unless specifically set to that mode)
	 */
	printf("Setting up\n");
	wiringPiSetup(); //This is the default mode. If you want to change pinouts, be aware
	
	RTC = wiringPiI2CSetup(RTCAddr); //Set up the RTC
//setup the ADC	
	mcp3004Setup(BASE, SPI_ADC);




	//Set up the Buttons
	for(int j; j < sizeof(BTNS)/sizeof(BTNS[0]); j++){
		pinMode(BTNS[j], INPUT);
		pullUpDnControl(BTNS[j], PUD_UP);
	}
	
	//Attach interrupts to Buttons
	//Write your logic here
	wiringPiISR(0, INT_EDGE_RISING, &reset); //sets up interrupt on pin30
	wiringPiISR(5, INT_EDGE_RISING, &hourInc);//sets up interrupt on pin5
	printf("BTNS done\n");
	printf("Setup done\n");
}


/*
 * The main function
 * This function is called, and calls all relevant functions we've written
 */
int main(void){
	initGPIO();	//Set random time (3:04PM
	wiringPiI2CWriteReg8(RTC, SEC, 0x80);//writes to seconds register so that external crystal can oscillate
	toggleTime(); //initialises/sets initial clock time
	
	// Repeat this until we shut down
	for (;;){

		updateTime(); //fetches RTC Time
		sensors(); //updates sensor values
		delay(1000*sample_time); //how often the program samples
	}
	return 0;
}


//check alarm status and take appropriate actions
void checkAlarm(void){
	if (alarm_ready==0){ //alarm has been tripped
	alarm_timer = wiringPiI2CReadReg8(RTC, SEC)-0x80; //increments alarm timer
	if (alarm_timer>=180){ //has 3 minutes passed?
		alarm_ready=1;}//alarm is ready to sound again
		}
}

void sensors(void){
                pot = analogRead(BASE); //834 is my max value for potentiometer
                pot = (int) pot;
                ldr = analogRead(BASE+1);
                temp_sensor = analogRead(BASE+2);
                temp_sensor = (temp_sensor * (3.3/1024)-0.5)/0.01;
                printf("POT: %d | ",pot);
                printf("LDR: %d | ",ldr);
                printf("TEMP: %.1f1 C\n ",temp_sensor);}

void updateTime(void){
//Fetch the time from the RTC
                RTC_hours = wiringPiI2CReadReg8(RTC, HOUR); //retrieves hours v$
                RTC_mins = wiringPiI2CReadReg8(RTC, MIN); //retrieves minutes v$
                RTC_secs = wiringPiI2CReadReg8(RTC, SEC)-0x80; //retrieves seco$}
printf("The system time is: %x:%x:%x\n", RTC_hours, RTC_mins, RTC_secs);}

void reset(void){
	printf("beep beep");}


//printf("Actual time is: %x: %x %x\n")


/*
 * Change the hour format to 12 hours
 */
int hFormat(int hours){
	/*formats to 12h*/
	if (hours >= 24){
		hours = 0;
	}
	else if (hours > 12){
		hours -= 12;
	}
	return (int)hours;
}


/*
 * hexCompensation
 * This function may not be necessary if you use bit-shifting rather than decimal checking for writing out time values
 */
int hexCompensation(int units){
	/*Convert HEX or BCD value to DEC where 0x45 == 0d45 
	  This was created as the lighXXX functions which determine what GPIO pin to set HIGH/LOW
	  perform operations which work in base10 and not base16 (incorrect logic) 
	*/
	int unitsU = units%0x10;

	if (units >= 0x50){
		units = 50 + unitsU;
	}
	else if (units >= 0x40){
		units = 40 + unitsU;
	}
	else if (units >= 0x30){
		units = 30 + unitsU;
	}
	else if (units >= 0x20){
		units = 20 + unitsU;
	}
	else if (units >= 0x10){
		units = 10 + unitsU;
	}
	return units;
}


/*
 * decCompensation
 * This function "undoes" hexCompensation in order to write the correct base 16 value through I2C
 */
int decCompensation(int units){
	int unitsU = units%10;

	if (units >= 50){
		units = 0x50 + unitsU;
	}
	else if (units >= 40){
		units = 0x40 + unitsU;
	}
	else if (units >= 30){
		units = 0x30 + unitsU;
	}
	else if (units >= 20){
		units = 0x20 + unitsU;
	}
	else if (units >= 10){
		units = 0x10 + unitsU;
	}
	return units;
}


/*
 * hourInc
 * Fetch the hour value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 23 hours in a day
 * Software Debouncing should be used
 */
void hourInc(void){
	//Debounce
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		printf("Interrupt 1 triggered, %x\n", hours);
		//Fetch RTC Time
		hours = wiringPiI2CReadReg8(RTC, HOUR);// reads current RTC value		
		hours= hexCompensation(hours); //converts to decimal from hex

		if(hours==24){ //checks if overflow
		hours=0;} //resets in case of overflow
		else{
		hours++;} //increments hours
		hours = decCompensation(hours); //converts back to hex
		wiringPiI2CWriteReg8(RTC, HOUR, hours); //writes to hours register of RTC
		//Increase hours by 1, ensuring not to overflow
		//Write hours back to the RTC
	}
	lastInterruptTime = interruptTime;
}

/* 
 * minInc
 * Fetch the minute value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 60 minutes in an hour
 * Software Debouncing should be used
 */
void minInc(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		printf("Interrupt 2 triggered, %x\n", mins);
		//Fetch RTC Time
		 mins = wiringPiI2CReadReg8(RTC, MIN); //reads RTC minute value
		mins = hexCompensation(mins);//converts to decimal
		//Increase minutes by 1, ensuring not to overflow
		if (mins<59){ //checks for overflow
		mins++;} //increments when not about to overflow
		else{
		mins=0;} //resets if about to overflow
		//Write minutes back to the RTC
		 mins = decCompensation(mins); //converts back to hex
		wiringPiI2CWriteReg8(RTC, MIN, mins); //writes modified value to RTC minutes register
	}
	lastInterruptTime = interruptTime;
}

//This interrupt will fetch current time from another script and write it to the clock registers
//This functions will toggle a flag that is checked in main
void toggleTime(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		HH = getHours();
		MM = getMins();
		SS = getSecs();

		HH = hFormat(HH);
		HH = decCompensation(HH);
		wiringPiI2CWriteReg8(RTC, HOUR, HH);

		MM = decCompensation(MM);
		wiringPiI2CWriteReg8(RTC, MIN, MM);

		SS = decCompensation(SS);
		wiringPiI2CWriteReg8(RTC, SEC, 0b10000000+SS);

	}
	lastInterruptTime = interruptTime;
}
