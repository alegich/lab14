// MeasurementOfDistance.c
// Runs on LM4F120/TM4C123
// Use SysTick interrupts to periodically initiate a software-
// triggered ADC conversion, convert the sample to a fixed-
// point decimal distance, and store the result in a mailbox.
// The foreground thread takes the result from the mailbox,
// converts the result to a string, and prints it to the
// Nokia5110 LCD.  The display is optional.
// January 15, 2016

/* This example accompanies the book
   "Embedded Systems: Introduction to ARM Cortex M Microcontrollers",
   ISBN: 978-1469998749, Jonathan Valvano, copyright (c) 2015

 Copyright 2016 by Jonathan W. Valvano, valvano@mail.utexas.edu
    You may use, edit, run or distribute this file
    as long as the above copyright notice remains
 THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 VALVANO SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL,
 OR CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 For more information about my classes, my research, and my books, see
 http://users.ece.utexas.edu/~valvano/
 */

// Slide pot pin 3 connected to +3.3V
// Slide pot pin 2 connected to PE2(Ain1) and PD3
// Slide pot pin 1 connected to ground


#include "ADC.h"
#include "..//tm4c123gh6pm.h"
#include "Nokia5110.h"
#include "TExaS.h"
#include "..//FlagUtils.h"

void EnableInterrupts(void);  // Enable interrupts

char String[10]; // null-terminated ASCII string
unsigned long Distance;   // units 0.001 cm
unsigned long ADCdata;    // 12-bit 0 to 4095 sample
unsigned long Flag = 0;       // 1 means valid Distance, 0 means Distance is empty

double round(double);
//********Convert****************
// Convert a 12-bit binary ADC sample into a 32-bit unsigned
// fixed-point distance (resolution 0.001 cm).  Calibration
// data is gathered using known distances and reading the
// ADC value measured on PE1.  
// Overflow and dropout should be considered 
// Input: sample  12-bit ADC sample
// Output: 32-bit distance (resolution 0.001cm)
unsigned long Convert(unsigned long sample){
	//const unsigned long size = 2000;
	double sampleD = sample;
  //return round((sampleD) / 2.0475);  // replace this line with real code
	return ((sample * 500 + 512) >> 10);
}

// Initialize SysTick interrupts to trigger at 40 Hz, 25 ms
void SysTick_Init(unsigned long period){
  NVIC_ST_CTRL_R = 0;           // disable SysTick during setup
  NVIC_ST_RELOAD_R = period - 1;     // 25 ms
  NVIC_ST_CURRENT_R = 0;        // any write to current clears it
  NVIC_SYS_PRI3_R = NVIC_SYS_PRI3_R&0x00FFFFFF; // priority 0                
  NVIC_ST_CTRL_R = 0x00000007;  // enable with core clock and interrupts
}
// executes every 25 ms, collects a sample, converts and stores in mailbox
void SysTick_Handler(void){
	unsigned long newData;
	
	// toggle PF2 two times for profiling
	INVOKE(TOGGLE_FLAG, PIN1, GPIO_PORTF_DATA_R, GPIO_PORTF_DATA_R);
	newData = ADC0_In(); // read input data
	
	// convert data and put it to mailbox only if it is changed
	if (newData != ADCdata) {
	  ADCdata = newData;
		Distance = Convert(newData);
	  Flag = 1;
	}
	
	// toggle PF2 to measure ISR execution time
	INVOKE(TOGGLE_FLAG, PIN1, GPIO_PORTF_DATA_R);
}

// PF1 is output for debugging
void InitOutput() {
	// Regular function
	INVOKE(CLEAR_FLAG, PMC1, GPIO_PORTF_PCTL_R);
	
	// Clear flag for pin 1 in registers Analog mode, Alternate function
	INVOKE(CLEAR_FLAG, PIN1, GPIO_PORTF_AMSEL_R, GPIO_PORTF_AFSEL_R);
	
	// Set flag for pin 1 in registers Direction, 8-mA Drive Select, Digital enable
	INVOKE(SET_FLAG, PIN1, GPIO_PORTF_DIR_R, GPIO_PORTF_DEN_R);
}

int sprintf(char*, const char*, ...);
char* strcpy(char*, const char*);
//-----------------------UART_ConvertDistance-----------------------
// Converts a 32-bit distance into an ASCII string
// Input: 32-bit number to be converted (resolution 0.001cm)
// Output: store the conversion in global variable String[10]
// Fixed format 1 digit, point, 3 digits, space, units, null termination
// Examples
//    4 to "0.004 cm"  
//   31 to "0.031 cm" 
//  102 to "0.102 cm" 
// 2210 to "2.210 cm"
//10000 to "*.*** cm"  any value larger than 9999 converted to "*.*** cm"
void UART_ConvertDistance(unsigned long n){
// as part of Lab 11 you implemented this function
	if (n <= 9999)
	{
		double d = n;
    sprintf((char*)String, "%1.3f cm", (d / 1000));
	}
	else
	{
		strcpy((char*)String, "*.*** cm");
	}
}

// main1 is a simple main program allowing you to debug the ADC interface
int main1(void){ 
  TExaS_Init(ADC0_AIN1_PIN_PE2, SSI0_Real_Nokia5110_Scope);
  ADC0_Init();    // initialize ADC0, channel 1, sequencer 3
  EnableInterrupts();
  while(1){ 
    ADCdata = ADC0_In();
  }
}
// once the ADC is operational, you can use main2 to debug the convert to distance
int main2(void){ 
  TExaS_Init(ADC0_AIN1_PIN_PE2, SSI0_Real_Nokia5110_NoScope);
  ADC0_Init();    // initialize ADC0, channel 1, sequencer 3
  Nokia5110_Init();             // initialize Nokia5110 LCD
	Nokia5110_OutString("Lab14");
  EnableInterrupts();
  while(1){ 
    ADCdata = ADC0_In();
    Nokia5110_SetCursor(0, 0);
    Distance = Convert(ADCdata);
    UART_ConvertDistance(Distance); // from Lab 11
    Nokia5110_OutString(String);    // output to Nokia5110 LCD (optional)
  }
}
// once the ADC and convert to distance functions are operational,
// you should use this main to build the final solution with interrupts and mailbox
int main(void){ 
  volatile unsigned long delay;
  TExaS_Init(ADC0_AIN1_PIN_PE2, SSI0_Real_Nokia5110_Scope);
// initialize ADC0, channel 1, sequencer 3
	ADC0_Init();
// initialize Nokia5110 LCD (optional)
	Nokia5110_Init(); 
// initialize SysTick for 40 Hz interrupts
	SysTick_Init(2000000);
// initialize profiling on PF1 (optional)
	InitOutput();
//    wait for clock to stabilize
  Nokia5110_SetCursor(0, 0);
	Nokia5110_OutString("Lab14");
  Nokia5110_SetCursor(0, 1);
	
  EnableInterrupts();
// print a welcome message  (optional)
	Nokia5110_OutString("Hello");
  while(1){ 
// read mailbox
		while (!Flag) {
		}
// output to Nokia5110 LCD (optional)
		UART_ConvertDistance(Distance);
		Nokia5110_SetCursor(0, 2);
		Nokia5110_OutString(String);
		Flag = 0;
  }
}
