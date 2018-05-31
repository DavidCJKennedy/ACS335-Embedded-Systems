#include "stm32f4xx.h"

void Initialise_LED_BUTTON(void); //Declaration for the function to initialise the four LEDs and TIM2 so LED can blink
void Initialise_SPI(uint16_t, uint32_t); //Declaration for the function to initialise the SPI and GPIOs
void InitialiseIRQ(void); //Declaration for the function to initialise NVIC
// void GetDataForAccelerometer(uint16_t, uint32_t); //Declaration for the un-used function required prior to part 3 to get accelerometer data on the LIS3DSH and light LEDs accordingly
extern SPI_HandleTypeDef SPI_Params; //Declaration of structure handle for the parameters of the SPI, makes handler, external so can be used anywhere 
extern uint8_t EnableIdleLed; //Declares of variable to determine whether the button has been pressed or not, external so can be used in multiple threads