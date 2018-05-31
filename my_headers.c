#include "stm32f4xx.h"
#include "cmsis_os.h"

SPI_HandleTypeDef SPI_Params; //Declares the structure handle for the parameters of the SPI, saves having to do mutiple initialisations 
uint8_t EnableIdleLed = 0; //Declares a variable to determine whether the button has been pressed or not

/* Definition for the function to initialise the four LED's and TIM2 so LED can blink */

void Initialise_LED_BUTTON(void){
	
	// Initialize GPIO Ports for LEDs
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN; // Enable Port D clock 
	GPIOD->MODER |= GPIO_MODER_MODER12_0; // GPIOD port 12, green LED
	GPIOD->MODER |= GPIO_MODER_MODER13_0; // GPIOD port 13, orange LED
	GPIOD->MODER |= GPIO_MODER_MODER14_0; // GPIOD port 14, red LED
	GPIOD->MODER |= GPIO_MODER_MODER15_0; // GPIOD port 15, blue LED
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; //Enable Port A Clock for the user push button
	
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // Enable timer 2 clock
	TIM2->CR1 &= ~0x00000016; //Set the counter to an upcounter by making bit 4 of control register 1 low, enable counter by making bit 0 = high
	TIM2->CR1 &= ~0x0000008; //Enable one pulse mode by setting bit 3 of control register low
	TIM2->PSC = 8400-1; //Sets the prescalar value to 8400, clockFreq = fCK_PSC/PSC[15:0]+1
	TIM2->ARR = 10000-1; //Sets the auto-reload value, number when reached up-counter will stop 
	TIM2->EGR = 1; //Re-initialise the counter and generates an update of the registers
	TIM2->CR1 |= 1; 
}

/* Definition for the function to initialise the SPI and GPIO's */

void Initialise_SPI(uint16_t dataSize, uint32_t dataTimeout){
	
	uint8_t dataToSend[1]; //Declares a dataToSend array that will store the required register address on the LIS3DSH 
	
	GPIO_InitTypeDef GPIOA_Params; //Declares the structure handle for the parameters of GPIOA, saves having to do mutiple initialisations 
	GPIO_InitTypeDef GPIOE_Params; //Declares the structure handle for the parameters of GPIOE, saves having to do mutiple initialisations 
	GPIO_InitTypeDef GPIOE_Params_I; //Declares the structure handle for the parameters of GPIOA specifically to be used to set up interrupts, saves having to do mutiple initialisations 
	
/* Configure paramters for the SPI */
	
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN; //Enable the clock for SPI1
	SPI_Params.Instance = SPI1; //Selects which SPI to use
	SPI_Params.Init.Mode = SPI_MODE_MASTER; //Sets STM32F407 to act as the master and control the LIS3DSH
	SPI_Params.Init.NSS = SPI_NSS_SOFT; //Sets the LIS3DSH to be controlled by the software 
	SPI_Params.Init.Direction = SPI_DIRECTION_2LINES; //Enables full-duplex communication by the SPI
	SPI_Params.Init.DataSize = SPI_DATASIZE_8BIT; //Sets the data packet size of the SPI to 8-bit
	SPI_Params.Init.CLKPolarity = SPI_POLARITY_HIGH; //Sets the idle polarity for the clock line high  
	SPI_Params.Init.CLKPhase = SPI_PHASE_2EDGE; //Sets the data line to change phase on the second transition of the clock line
	SPI_Params.Init.FirstBit = SPI_FIRSTBIT_MSB; //Sets the transmission to be sent MSB first
	SPI_Params.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32; //Determine the SPI colck rate from the main APB2 clock, APB2 clock = 84MHz, 84Mhz/32 = 2.625MHz 
	HAL_SPI_Init(&SPI_Params); //Configure SPI1 using the declared parameters 
	
/* Configure the SCL, SDO and SDI pins of GPIOA */
	
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN; //Enable the clock for GPIOA 
	GPIOA_Params.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7; //Selects pins 5, 6 and 7. The SPI clock, data line and output data respectively  
	GPIOA_Params.Alternate = GPIO_AF5_SPI1; //Selects alternative function 5 which corresponds to SPI1
	GPIOA_Params.Mode = GPIO_MODE_AF_PP; //Selects alternative push pull mode
	GPIOA_Params.Speed = GPIO_SPEED_FAST; //Selects fast speed
	GPIOA_Params.Pull = GPIO_NOPULL; //Selects no pull-up or pull down activation
	HAL_GPIO_Init(GPIOA, &GPIOA_Params); //Configures GPIOA using the declared parameters
	
/* Configure the SPI pins on GPIOE */
	
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN; //Enable the clock for GPIOE
	GPIOE_Params.Pin = GPIO_PIN_3; //Selects Pin 3, the CS line used to initiate data communication
	GPIOE_Params.Mode = GPIO_MODE_OUTPUT_PP; //Selects normal push pull mode
	GPIOE_Params.Speed = GPIO_SPEED_FAST; //Selects fast speed
	GPIOE_Params.Pull = GPIO_PULLUP; //Selects pull-up activiation 
	HAL_GPIO_Init(GPIOE, &GPIOE_Params); //Configures GPIOE using the declared parameters
	
/* Enable the interrupts pins on GPIOE */ 
	
	GPIOE_Params_I.Pin = GPIO_PIN_0; //Selects Pin 0, corresponding to INT1 to enable the interupt line 
	GPIOE_Params_I.Mode = GPIO_MODE_IT_RISING; //Configures interupts to be signalled on a rising high. Idle value = 1, so must first have falling edge then rising edge
	GPIOE_Params_I.Speed = GPIO_SPEED_FAST; //Selects fast speed
	HAL_GPIO_Init(GPIOE, &GPIOE_Params_I); //Configures GPIOE using the declared parameters
	GPIOE->BSRR = GPIO_PIN_3; //Sets the communication enable line high on the SPI to end communication process
	__HAL_SPI_ENABLE(&SPI_Params); //Enable the SPI because initialisation is complete
	
/* Enable the correct axes and output data rate on LIS3DSH */
	
	dataToSend[0] = 0x20; //Address for control register 4 on the LIS3DSH, to be able to send data to set up axes and ODR configuration
	GPIOE->BSRR = GPIO_PIN_3<<16; //Set the communication enable line on the SPI to low so communication can be initialised
	HAL_SPI_Transmit(&SPI_Params,dataToSend,dataSize,dataTimeout); //Transmit the address of the register to be read on the LIS3DSH
	dataToSend[0] = 0x13; //Enable X and Y axis by setting bit 0 and 1 high. Set output data rate to 6.25Hz, this gives satifactory response and little flickering  
	HAL_SPI_Transmit(&SPI_Params,dataToSend,dataSize,dataTimeout); //Transmits new data to CR4 on LIS3DSH via the SPI channel
	GPIOE->BSRR = GPIO_PIN_3; //Sets the communication enable line high on the SPI to end communication process
	
/* Enable interrupts on the LIS3DSH */ 
	
	dataToSend[0] = 0x23; //Address for control register 3 on the LIS3DSH, to be able to send data to set up interrupts 
	GPIOE->BSRR = GPIO_PIN_3<<16; //Set the communication enable line on the SPI to low so communication can be initialised
	HAL_SPI_Transmit(&SPI_Params,dataToSend,dataSize,dataTimeout); //Transmit the address of the register to be read on the LIS3DSH
	dataToSend[0] = 0xC8; //Enables DRDY so interrupts can be triggered only when data available. Sets INT1_EN high, enabling INT1. Sets IEA high so interupt signals are active high. 
	HAL_SPI_Transmit(&SPI_Params,dataToSend,dataSize,dataTimeout); //Transmits new data to CR3 on LIS3DSH via the SPI channel
	GPIOE->BSRR = GPIO_PIN_3;  //Sets the communication enable line high on the SPI to end communication process
	
}

/* Definition for the function to initialise NVIC */

void InitialiseIRQ(void){	//Definition for the function to initialise NVIC 
	HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0); //Sets priority of EXTIO in the NVIC to the lowest priority
	HAL_NVIC_ClearPendingIRQ(EXTI0_IRQn); //Clears any flags on EXTIO
	NVIC->ISER[0] = 1<<6; //Enables interupts on EXTIO (1<<6) because EXT0_1 is bit 6 of interrupt vector table. Runs faster than enabling interupts because dont need to call function	
}

/* Definition of the handler for when NVIC is triggered by the DRDY signaling data is ready. DRDY is connected to PEO and hence EXTIO */

void EXTI0_IRQHandler(void){
	
	uint8_t dataToSend[1]; //Declares a dataToSend array that will store the required register address on the LIS3DSH
	uint16_t dataSize=1; //Declares a data size integer, required as a parameter in HAL_SPI_Transmit/Recieve, specfies only a single address will be accessed at a time
	uint32_t dataTimeout=1000; //Declares a time out integer, required as a parameter in HAL_SPI_Transmit/Recieve, specifies the maximum time to wait for the SPI action to complete. Stops systems from freezing if no response
	uint8_t Y_Reg_H; //Declares an 8-bit integer to store the returned Y-Axis accelerometer data
	uint8_t Y_Reg_H_Data; //Declares an 8-bit integer to store the returned Y-Axis accelerometer data without the sign bit
	uint8_t X_Reg_H; //Declares an 8-bit integer to store the returned X-Axis accelerometer data
	uint8_t X_Reg_H_Data; //Declares an 8-bit integer to store the returned X-Axis accelerometer data without the sign bit
		
	//if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0)!=RESET) {	//Checks if data is really available 
	//	__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0); //Reset the data ready pin if data is available
		
/* Read the most significant 8 bits of X axis data */
		
		dataToSend[0] = 0x80|0x29; //Address for OUT_X_H register, that contains the X-Axis accelerometer information
		GPIOE->BSRR = GPIO_PIN_3<<16; //Set the communication enable line on the SPI low so communication can be initialised
		HAL_SPI_Transmit(&SPI_Params,dataToSend,dataSize,dataTimeout); //Transmit the address of the register to be read on the LIS3DSH
		dataToSend[0] = 0x00; //Set data to send blank, because data is being recieved not sent
		HAL_SPI_Receive(&SPI_Params,dataToSend,dataSize,dataTimeout); //Recieve data from the LIS3DSH via the SPI channel
		GPIOE->BSRR = GPIO_PIN_3; //Sets the communication enable line high on the SPI to end communication process
		X_Reg_H = *SPI_Params.pRxBuffPtr; //Get the accelerometer data that has been sent along the SPI channel and store in X-Axis variable 
	
/* Read the most significant 8 bits of Y axis data */
		
		dataToSend[0] = 0x80|0x2B; //Address for OUT_Y_H register, that contains the Y-Axis accelerometer information
		GPIOE->BSRR = GPIO_PIN_3<<16; //Set the communication enable line on the SPI to low so communication can be initialised
		HAL_SPI_Transmit(&SPI_Params,dataToSend,dataSize,dataTimeout); //Transmit the address of the register to be read on the LIS3DSH
		dataToSend[0] = 0x00; //Set data to send blank, because data is being recieved not sent
		HAL_SPI_Receive(&SPI_Params,dataToSend,dataSize,dataTimeout); //Recieve data from the LIS3DSH via the SPI channel
		GPIOE->BSRR = GPIO_PIN_3; //Sets the communication enable line high on the SPI to end communication process
		Y_Reg_H = *SPI_Params.pRxBuffPtr; //Get the accelerometer data that has been sent along the SPI channel and store in Y-Axis variable

		X_Reg_H_Data = X_Reg_H; //Set X-axis data variable without sign bit equal to X-axis data with sign bit
		Y_Reg_H_Data = Y_Reg_H; //Set Y-axis data variable without sign bit equal to Y-axis data with sign bit
		
		X_Reg_H_Data &= ~ 0x80; //Remove sign bit from X-axis data to leave true value 
		Y_Reg_H_Data &= ~ 0x80; //Remove sign bit from Y-axis data to leave true value 
	
/* Light corresponding LEDs to board orientation */
	
		if (((Y_Reg_H_Data < 120) && (Y_Reg_H_Data > 10))) { //Creation of dead zone in the Y axis. Between the values of 10 and 120, the board is said to be non-horizontal
			if((Y_Reg_H&0x80) == 0x80){ //If sign bit is high, board is in negative Y axis so light blue LED, turn off orange LED as precaution 
				GPIOD->BSRR |= (1<<15);
				GPIOD->BSRR |= (1<<(13+16));
			} else { 										//Else sign bit is low, board is in positive Y axis. Therefore light orange LED and turn off blue LED as precaution 
				GPIOD->BSRR |= (1<<13);
				GPIOD->BSRR |= (1<<(15+16));
			} 
		} else { 											//The board is inside the deadband and said to be horizontal, turn off Y-axis LEDs
			GPIOD->BSRR |= (1<<(13+16));
			GPIOD->BSRR |= (1<<(15+16));
		}
	
		if (((X_Reg_H_Data < 120) && (X_Reg_H_Data > 10))) { //Creation of dead zone in the X axis. Between the values of 10 and 120, the board is said to be non-horizontal
			if((X_Reg_H&0x80) == 0x80){ //If sign bit is high, board is in negative X axis so light green LED, turn off red LED as precaution
				GPIOD->BSRR |= (1<<12);
				GPIOD->BSRR |= (1<<(14+16));
			} else {										//Else sign bit is low, board is in positive X axis. Therefore light red LED and turn off green LED as precaution
				GPIOD->BSRR |= (1<<14);
				GPIOD->BSRR |= (1<<(12+16));
			} 
		} else {											//The board is inside the deadband and said to be horizontal, turn off X-axis LEDs
			GPIOD->BSRR |= (1<<(12+16));
			GPIOD->BSRR |= (1<<(14+16));
		}
	
		HAL_NVIC_ClearPendingIRQ(EXTI0_IRQn); //Clear EXTIO interrupt from NVIC as the interupt has been handled 
		EXTI->PR|=(1<<1); //Clear bit in EXTI-PR that corresponds to EXTIO interrupt, so interupt can be called again 
}


/* Definition for the un-used function required prior to part 3 to get accelerometer data on the LIS3DSH and light LED's accordingly */

/*
void GetDataForAccelerometer(uint16_t dataSize, uint32_t dataTimeout){

	uint8_t dataToSend[1]; //Declares a dataToSend array that will store the required register address on the LIS3DSH
	uint8_t Y_Reg_H; //Declares an 8-bit integer to store the returned Y-Axis accelerometer data
	uint8_t Y_Reg_H_Data; //Declares an 8-bit integer to store the returned Y-Axis accelerometer data without the sign bit 
	uint8_t X_Reg_H; //Declares an 8-bit integer to store the returned X-Axis accelerometer data
	uint8_t X_Reg_H_Data; //Declares an 8-bit integer to store the returned X-Axis accelerometer data without the sign bit
	

	if(__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_0)==SET) { //Check to see if the interrupt line has been set because accelerometer data is available 
		__HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0); //If interrupt sign has been set, clear interrupt so it can be used again
		
//		Read the most significant 8 bits of X axis data
		
		dataToSend[0] = 0x80|0x29; //Address for OUT_X_H register, that contains the X-Axis accelerometer information
		GPIOE->BSRR = GPIO_PIN_3<<16; //Set the communication enable line on the SPI low so communication can be initialised
		HAL_SPI_Transmit(&SPI_Params,dataToSend,dataSize,dataTimeout); //Transmit the address of the register to be read on the LIS3DSH
		dataToSend[0] = 0x00; //Set data to send blank, because data is being recieved not sent
		HAL_SPI_Receive(&SPI_Params,dataToSend,dataSize,dataTimeout); //Recieve data from the LIS3DSH via the SPI channel
		GPIOE->BSRR = GPIO_PIN_3; //Sets the communication enable line high on the SPI to end communication process
		X_Reg_H = *SPI_Params.pRxBuffPtr; //Get the accelerometer data that has been sent along the SPI channel and store in X-Axis variable 
	
// Read the most significant 8 bits of Y axis data
		
		dataToSend[0] = 0x80|0x02B; //Address for OUT_Y_H register, that contains the X-Axis accelerometer information
		GPIOE->BSRR = GPIO_PIN_3<<16; //Set the communication enable line on the SPI to low so communication can be initialised
		HAL_SPI_Transmit(&SPI_Params,dataToSend,dataSize,dataTimeout); //Transmit the address of the register to be read on the LIS3DSH
		dataToSend[0] = 0x00; //Set data to send blank, because data is being recieved not sent
		HAL_SPI_Receive(&SPI_Params,dataToSend,dataSize,dataTimeout); //Recieve data from the LIS3DSH via the SPI channel
		GPIOE->BSRR = GPIO_PIN_3; //Sets the communication enable line high on the SPI to end communication process
		Y_Reg_H = *SPI_Params.pRxBuffPtr; //Get the accelerometer data that has been sent along the SPI channel and store in Y-Axis variable

		X_Reg_H_Data = X_Reg_H; //Set X-axis data variable without sign bit equal to X-axis data with sign bit
		Y_Reg_H_Data = Y_Reg_H; //Set Y-axis data variable without sign bit equal to Y-axis data with sign bit
		
		X_Reg_H_Data &= ~ 0x80; //Remove sign bit from X-axis data to leave true value 
		Y_Reg_H_Data &= ~ 0x80; //Remove sign bit from Y-axis data to leave true value 
	
// Light corresponding LED's to board orientation
	
		if (((Y_Reg_H_Data < 120) && (Y_Reg_H_Data > 10))) { //Creation of dead zone in the Y axis. Between the values of 10 and 120, the board is said to be non-horizontal
			if((Y_Reg_H&0x80) == 0x80){ //If sign bit is high, board is in negative Y axis so light blue LED, turn off orange LED as precaution 
				GPIOD->BSRR |= (1<<15);
				GPIOD->BSRR |= (1<<(13+16));
			} else { 										//Else sign bit is low, board is in positive Y axis. Therefore light orange LED and turn off blue LED as precaution 
				GPIOD->BSRR |= (1<<13);
				GPIOD->BSRR |= (1<<(15+16));
			} 
		} else { 											//The board is inside the deadband and said to be horizontal, turn off Y-axis LED's
			GPIOD->BSRR |= (1<<(13+16));
			GPIOD->BSRR |= (1<<(15+16));
		}
	
		if (((X_Reg_H_Data < 120) && (X_Reg_H_Data > 10))) { //Creation of dead zone in the X axis. Between the values of 10 and 120, the board is said to be non-horizontal
			if((X_Reg_H&0x80) == 0x80){ //If sign bit is high, board is in negative X axis so light green LED, turn off red LED as precaution
				GPIOD->BSRR |= (1<<12);
				GPIOD->BSRR |= (1<<(14+16));
			} else {										//Else sign bit is low, board is in positive X axis. Therefore light red LED and turn off green LED as precaution
				GPIOD->BSRR |= (1<<14);
				GPIOD->BSRR |= (1<<(12+16));
			} 
		} else {											//The board is inside the deadband and said to be horizontal, turn off X-axis LED's
			GPIOD->BSRR |= (1<<(12+16));
			GPIOD->BSRR |= (1<<(14+16));
		}
	}
}
*/