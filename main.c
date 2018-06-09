#define PE0 0x01
#define PE1 0x02
#define PE2 0x04
#define PE3 0x08
#define PE4 0x10
#define PE5 0x20
#include "tm4c123gh6pm.h"
#include <stdint.h>
#include "PLL.h"
#define LIGHT	(*((volatile unsigned long *)0x40025038))
#define SOIL (*((volatile unsigned long*)0x40007004))
#define OUTPUT (*((volatile unsigned long*)0x40007200))
#define SWITCH (*((volatile unsigned long *)0x40025004))

void PortF_Init(void);
void PortD_Init(void);
void Delay(void);

//Analog to digital converter (ADC)
//This only works for 1 ADC

//Note: the values obtained such as PE5 are hardcoded in #define
//The ADC value hexavalues are obtained in a datasheet

//Initialize ADC0
void ADC0_Init(void) {
	volatile unsigned long delay;
	//Initialize Port E
  SYSCTL_RCGC2_R |= 0x00000010;
	//a delay is used to ensure port E is fully initialized
  delay = SYSCTL_RCGC2_R;       
	//PE5 is a input
  GPIO_PORTE_DIR_R &= ~(PE4 | PE5); 
  //Enable PE5 as a alternate functionality as a ADC  
  GPIO_PORTE_AFSEL_R |= (PE4 |PE5); 
	//Disable I/O for PE5
  GPIO_PORTE_DEN_R &= ~(PE4|PE5); 
	//Enable analogy for PE5  
  GPIO_PORTE_AMSEL_R |= (PE4|PE5);  
	
	//Enable ADC0
  SYSCTL_RCGC0_R |= 0x00010000; 
  delay = SYSCTL_RCGC2_R;  
  //125K sample speed
  SYSCTL_RCGC0_R &= ~0x00000300;
	//Sequence 3 is the highest priority  
  ADC0_SSPRI_R = 0x0123; 
	//From here on we will use sample sequencer 3.
	
	//We disable sequencer 3 and apply settings
  ADC0_ACTSS_R &= ~0x0008;     
	//Sequencer 2 and 3 is a software trigger (Bits are set to 0)
  ADC0_EMUX_R &= ~0xFF00;    
	//We use channel 8 (Channel 8 is PE3 alt value obtained in datasheet)
	//We also use channel 9 (Channel 9 is PE4 alt value obtained in datasheet)
  ADC0_SSMUX3_R = 0x0098; 	
	//disable temperature sensor and different sample
	//enable the last sequence number to sample and trigger interrupt bit 1
	//once sample is done
  ADC0_SSCTL3_R = 0x0006;
//Enable sequencer 3	
  ADC0_ACTSS_R |= 0x0008;     
}
//ADC0 Input
unsigned long ADC0_In(void) {
  unsigned long result;
	//Start sample sequencer 3
  ADC0_PSSI_R = 0x0008;     
  //Wait until the conversion finishes	
  while((ADC0_RIS_R&0x08)==0){};  
  //Get result from a 12 bit FIFO
  result = ADC0_SSFIFO3_R&0xFFF; 
	//Acknowledge sample sequencer 3 conversion complete
  ADC0_ISC_R = 0x0008;           
	//return the value.
  return result;
}

// Linked data structure
struct State {
  unsigned long Out; 
  unsigned long Time; 
	unsigned char Led;
  unsigned long Next[4];
}; 

// define the state names
typedef const struct State STyp;
#define idle  								0
#define fill_tank							1
#define watering_plant    		2

//Soil  - PE - 1
//Water - PE - 2	
	
// Moore state machine with outputs to the traffic light and the pedistrians lights
// time is called from the systic timer for 10ms
STyp FSM[3]={		 // Soil, Water
// Pump Output
//										00       01      10       		11
 {0, 500, 0x08, {fill_tank, idle, fill_tank, watering_plant}}, //idle
 {0, 200, 0x02, {fill_tank, idle, fill_tank, watering_plant}}, //fill tank
 {1, 600, 0x04, {fill_tank, idle, fill_tank, watering_plant}}
}; //water plant
 

unsigned long value,value2;
unsigned long WATER;
unsigned int soil;
unsigned int T;
unsigned long S;
//sample usage of ADC
int main(void) {
	//Set the board to run at 80MHZ clock (without it the code will 
	//expereience a hardware fault). This must stay
	PLL_Init();
	//Initialize ADC 0
	ADC0_Init();
	PortF_Init();
	//LIGHT = 0x06;
	PortD_Init();
	
	
	while (1) {
		//Obtain the ADC value. (0 - 3.3 V to 0 - 4095 ). Note if there are multiple channels, we call this function again
		//Because we have 2 channels, we call ADC0_In() two times and we should get different ADC values.
		WATER = ADC0_In();
		//OUTPUT = 1;
		//value2 = ADC0_In();
		
		// Check Water
		if(WATER >= 0x100 && SOIL == 0){T = 1;} // Soil, Water
		else if(WATER >= 0x100 && SOIL == 1){T =  3;}
		else if (WATER< 0x100 && SOIL == 0){T = 2;}
		else {T = 0;}
		
		LIGHT = FSM[S].Led;
		OUTPUT = FSM[S].Out;
		S = FSM[S].Next[T];
		Delay();
		
		//if(SWITCH == 0){LIGHT = 0xC;}
		
	}
}



void PortF_Init(void){ volatile unsigned long delay;
  SYSCTL_RCGCGPIO_R |= 0x20; // 1) activate port D
                                    // 2) no need to unlock PD3-0
  GPIO_PORTF_AMSEL_R &= ~0x1F;      // 3) disable analog functionality on PD3-0
  GPIO_PORTF_PCTL_R &= ~0x000FFFFF; // 4) GPIO configure PD3-0 as GPIO
  GPIO_PORTF_DIR_R |= 0x0E;   // 5) make PD3-0 out
	GPIO_PORTF_DIR_R &= ~0x11;
  GPIO_PORTF_AFSEL_R &= ~0x1F;// 6) disable alt funct on PD3-0
  GPIO_PORTF_DR8R_R |= 0x1F;  // enable 8 mA drive
  GPIO_PORTF_DEN_R |= 0x1F;   // 7) enable digital I/O on PD3-0 
	GPIO_PORTF_PUR_R |= 0x11;
}


void PortD_Init(void){
	SYSCTL_RCGCGPIO_R |= 0x08; // 1) activate port D
                                    // 2) no need to unlock PD3-0
  GPIO_PORTD_AMSEL_R &= ~0x81;      // 3) disable analog functionality on PD3-0
  GPIO_PORTD_PCTL_R &= ~0xF000000F; // 4) GPIO configure PD3-0 as GPIO
  GPIO_PORTD_DIR_R &= ~0x01;   // 5) make PD3-0 out
	GPIO_PORTD_DIR_R |= 0x80; // D1 output
  GPIO_PORTD_AFSEL_R &= ~0x81;// 6) disable alt funct on PD3-0
  GPIO_PORTD_DR8R_R |= 0x81;  // enable 8 mA drive
  GPIO_PORTD_DEN_R |= 0x81;   // 7) enable digital I/O on PD3-0 
}

void Delay(void){unsigned long volatile time;
	time = (727240*200/91);
  while(time){
		time--;
  }
}
