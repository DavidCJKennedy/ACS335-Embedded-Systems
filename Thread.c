
#include "cmsis_os.h"																				//CMSIS RTOS header file
#include "stm32f4xx.h"
#include "my_headers.h"

void Tilt_Detector_Thread (void const *argument); // Declares the tilt detector thread function
osThreadId tid_Tilt_Detector_Thread; // Declares the ID of the tilt detector thread, makes for easy referencing when calling OS methods
osThreadDef (Tilt_Detector_Thread, osPriorityNormal, 1, 0); // Declares the tilt detector thread, with normal priority and 1 instance exists

void Blink_Red_LED_Thread (void const *argument); // Declares the blink red LED thread function
osThreadId tid_Blink_Red_LED_Thread; //Declares the ID of the blink red LED thread, makes for easy referencing when calling OS methods
osThreadDef (Blink_Red_LED_Thread, osPriorityNormal, 1, 0); // Declares the blink red LED thread, with normal priority and 1 instance exists

/*----------------------------------------------------------------------------
 *      Thread 1 Tilt Detector
 *---------------------------------------------------------------------------*/
 
/* Code to initialise the Tilt dectector thread */
 
int Init_Tilt_Detector_Thread(void){
  tid_Tilt_Detector_Thread = osThreadCreate (osThread(Tilt_Detector_Thread), NULL); //Creates Tilt Dectector object declared above and assigns the tid
  if (!tid_Tilt_Detector_Thread) return(-1); //Checks that the thread object has been created
  return(0); //If not created return 0
}

/* Code to define operation of the tilt detector thread */

void Tilt_Detector_Thread (void const *argument) {
	
	//uint16_t data_size=1; //Declares a data size integer, required as a parameter in HAL_SPI_Transmit/Recieve, specfies only a single address will be accessed at a time
	//uint32_t data_timeout=1000; //Declares a time out integer, required as a parameter in HAL_SPI_Transmit/Recieve, specifies the maximum time to wait for the SPI action to complete. Stops systems from freezing if no response
	//uint8_t dataToSend[1]; //Declares a dataToSend array that will store the required register address on the LIS3DSH
	uint16_t deBounce= 0; //Declares a deBounce variable, will be used to make sure correct connection has been made between push button contacts
	
	osSignalSet(tid_Tilt_Detector_Thread, 0x01); //Set the flag of tilt detector thread so it will be used.  
		
  while (1) {

		//GetDataForAccelerometer(data_size, data_timeout); // Calls the un-used function required prior to part 3 to get accelerometer data on the LIS3DSH and light LEDs accordingly
		
		while(((GPIOA->IDR & 0x00000001) == 0x00000001) & (EnableIdleLed == 0)) { //While button is pressed 
			deBounce++;			          //Increase deBounce counter
			if (deBounce > 200) {			//If button propperly pressed EnableIdleLED = 1 allowing thread transfer
				EnableIdleLed = 1;
			}
		}
			
		if (((GPIOA->IDR & 0x00000001) != 0x00000001) & (EnableIdleLed == 1)) { //While button not pressed and thread transfer allowed
			
			deBounce = 0;																		//Reset debounce counter
			HAL_NVIC_DisableIRQ(EXTI0_IRQn);								//Disable NVIC 
			GPIOD->BSRR = 1<<(12+16);												//Turn off green LED
			GPIOD->BSRR = 1<<(13+16);												//Turn off orange LED
			GPIOD->BSRR = 1<<(14+16);												//Turn off red LED
			GPIOD->BSRR = 1<<(15+16);												//Turn off blue LED
			osSignalSet(tid_Blink_Red_LED_Thread, 0x01);		//Set the flag of blink red LED thread to break thread out of wait
			osSignalWait(0x01, osWaitForever);							//Suspend current thread until appropriate thread is set
		}
    osThreadYield ();                                     
  }
}


/*----------------------------------------------------------------------------
 *      Thread 2 Blink Red LED Thread
 *---------------------------------------------------------------------------*/

/* Code to initialise the Blink Red LED thread */

int Init_Blink_Red_LED_Thread(void){
	tid_Blink_Red_LED_Thread = osThreadCreate(osThread(Blink_Red_LED_Thread), NULL);  //Creates Blink Red LED object declared above and assigns the tid
	if(!tid_Blink_Red_LED_Thread) return(-1); //Checks that the thread object has been created
	return(0); //If not created return 0
}

/* Code to define operation of the Blink Red LED thread */

void Blink_Red_LED_Thread(void const *argument){	
	uint16_t deBounce= 0; //Declares a deBounce variable, will be used to make sure correct connection has been made between push button contacts
	osSignalClear(tid_Blink_Red_LED_Thread,0x01);  //Clear flag on the Blink Red LED thread so it resumes at a later point
	
	while(1){
			osSignalWait(0x01, osWaitForever); //Suspend current thread until appropriate thread is set
			osSignalSet(tid_Blink_Red_LED_Thread, 0x01); //Set the flag of blink red LED thread to break thread out of wait
			
			while((TIM2->SR&0x0001)!=1){ //Implement 1 second delay using TIM2
				GPIOD->BSRR = 1<<14; //Enable the red LED
				
				while(((GPIOA->IDR & 0x00000001) == 0x00000001) & (EnableIdleLed == 1)) { //While button is pressed 
					deBounce++;							//Increase deBounce counter
			
					if (deBounce > 200) {		//If button propperly pressed EnableIdleLED = 0 allowing thread transfer
						EnableIdleLed = 0;
					}
				}
				
				if (((GPIOA->IDR & 0x00000001) != 0x00000001) & (EnableIdleLed == 0)) { //While button not pressed and thread transfer allowed
					deBounce = 0;																	//Reset debounce counter
					InitialiseIRQ();															//Enable NVIC 
					osSignalSet(tid_Tilt_Detector_Thread, 0x01);	//Set the flag of tilt detector LED thread to break thread out of wait
					osSignalWait(0x01, osWaitForever);						//Suspend current thread until appropriate thread is set
				}
			}
			
			TIM2->SR &= ~1;			//Resets the flag 
			
			while((TIM2->SR&0x0001)!=1){	//Implement 1 second delay using TIM2
				GPIOD->BSRR = 1<<(14+16);		//Turn off the red LED
				while(((GPIOA->IDR & 0x00000001) == 0x00000001) & (EnableIdleLed == 1)) {	//While button is pressed 
					deBounce++;							//Increase deBounce counter
	
					if (deBounce > 200) {		//If button propperly pressed EnableIdleLED = 0 allowing thread transfer
						EnableIdleLed = 0;
					}
				}
				
				if (((GPIOA->IDR & 0x00000001) != 0x00000001) & (EnableIdleLed == 0)) {	 //While button not pressed and thread transfer allowed
					deBounce = 0;																//Reset debounce counter
					InitialiseIRQ();														//Enable NVIC 
					osSignalSet(tid_Tilt_Detector_Thread, 0x01);//Set the flag of tilt detector LED thread to break thread out of wait
					osSignalWait(0x01, osWaitForever);					//Suspend current thread until appropriate thread is set
				}
			}		
			
			TIM2->SR &= ~1;		//Resets the flag 
			
		osThreadYield();
	}
}
