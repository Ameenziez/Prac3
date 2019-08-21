/*
 * BinClock.c
 * Jarrod Olivier
 * Modified for EEE3095S/3096S by Keegan Crankshaw
 * August 2019
 * 
 * <JRDMUH002> <DDNAAD001>
 * 17 August 2019
*/

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h> //For printf functions
#include <stdlib.h> // For system functions

#include "BinClock.h"
#include "CurrentTime.h"

//Global variables
int hours, mins, secs;
long lastInterruptTime = 0; //Used for button debounce
int RTC; //Holds the RTC instance

int HH,MM,SS;

void initGPIO(void){
	/* 
	 * Sets GPIO using wiringPi pins. see pinout.xyz for specific wiringPi pins
	 * You can also use "gpio readall" in the command line to get the pins
	 * Note: wiringPi does not use GPIO or board pin numbers (unless specifically set to that mode)
	 */
	printf("Setting up\n");
	wiringPiSetup(); //This is the default mode. If you want to change pinouts, be aware
	
	RTC = wiringPiI2CSetup(RTCAddr); //Set up the RTC
	
	//Set up the LEDS
	for(int i; i < sizeof(LEDS)/sizeof(LEDS[0]); i++){
	    pinMode(LEDS[i], OUTPUT);
	}
	
	//Set Up the Seconds LED for PWM
	//Write your logic here
	 pinMode(1, PWM_OUTPUT); //added this
    

	printf("LEDS done\n");
	
	//Set up the Buttons
	for(int j; j < sizeof(BTNS)/sizeof(BTNS[0]); j++){
		pinMode(BTNS[j], INPUT);
		pullUpDnControl(BTNS[j], PUD_UP);
	}
	
	//Attach interrupts to Buttons
	//Write your logic here
	wiringPiISR(30, INT_EDGE_RISING, &minInc); //sets up interrupt on pin30
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

		//Fetch the time from the RTC
		//Write your logic here
		hours = wiringPiI2CReadReg8(RTC, HOUR); //retrieves hours value
		mins = wiringPiI2CReadReg8(RTC, MIN); //retrieves minutes value
		secs = wiringPiI2CReadReg8(RTC, SEC)-0x80; //retrieves seconds value but subtracts the bit assigned to enable crystal

		//Function calls to toggle LEDs		//Write your logic here
				// Print out the time we have stored on our RTC
		printf("The current time is: %x:%x:%x\n", hours, mins, secs);
		
		lightHours(hFormat(hexCompensation(hours))); //converts to readable format for leds, converts to 12 hour time and displays it on leds
		
		
		lightMins( (hexCompensation(mins))); //converts to readable format for leds and displays it on the leds
		secPWM(hexCompensation(secs));//converts to readable format for led and applies pwm to the led
		//using a delay to make our program "less CPU hungry"
		delay(1000); //milliseconds
	}
	return 0;
}

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
 * Turns on corresponding LED's for hours
 */
void lightHours(int units){
	// Write your logic to light up the hour LEDs here

//	
	for (int i=0;i<=3;i++){ //
		digitalWrite(LEDS[i],(units & 1<<i));
	} //iterates through 'LEDS' array so that we are only working with the 'hours' section of the array.
}          // bitshifts the given unit in order to set individual bits to correct indices of array

/*
 * Turn on the Minute LEDs
 */
void lightMins(int units){
	//Write your logic to light up the minute LEDs here
       for (int j=4, k =0;j<=9 && k<=5;j++,k++){ //iterates through minutes part of 'LEDS' array
                digitalWrite(LEDS[j],(units & 1<<k)); // sets individual bits of unit to correct bits of array

}

}


/*
 * PWM on the Seconds LED
 * The LED should have 60 brightness levels
 * The LED should be "off" at 0 seconds, and fully bright at 59 seconds
 */
void secPWM(int units){
	// Write your logic here
	secs = units*1024;
	secs =  (int)  secs/60; //this fixes the ratio so that maximum occurs after 60 seconds and is linear in between 0 and 60
    	pwmWrite(SECS,secs);    //sets pwm  based on above ratio at a given instant
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
