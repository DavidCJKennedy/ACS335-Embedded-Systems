/*----------------------------------------------------------------------------
 * CMSIS-RTOS 'main' function template
 *---------------------------------------------------------------------------*/

#define osObjectsPublic                     // define objects in main module
#include "osObjects.h"                      // RTOS object definitions
#include "stm32f4xx.h"
#include "my_headers.h"
#include "Thread.h"




/*
 * main: initialize and start the system
 */
 
int main (void) {
  osKernelInitialize ();                    //Initialize CMSIS-RTOS

	/* Initialise any peripherals or system components */
	
	uint16_t data_size = 1;										//Declares a dataToSend array that will store the required register address on the LIS3DSH
	uint32_t data_timeout = 1000;							//Declares a data size integer, required as a parameter in HAL_SPI_Transmit/Recieve, specfies only a single address will be accessed at a time
		
	Initialise_LED_BUTTON();									//Initialise the LED and Button
	Initialise_SPI(data_size, data_timeout);  //Initialise the SPI
	InitialiseIRQ();													//Initialise the NVIC
	Init_Tilt_Detector_Thread();							//Initialise the Tilt detector thread
	Init_Blink_Red_LED_Thread();							//Initialise the Blink LED thread

  osKernelStart ();                         // start thread execution 
	while(1){};																//While loop so the program doesn't terminate
}

