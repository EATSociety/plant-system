#define PE0 0x01
#define PE1 0x02
#define PE2 0x04
#define PE3 0x08
#define PE4 0x10
#define PE5 0x20
#include "tm4c123gh6pm.h"
#include <stdint.h>
#include "PLL.h"

//void Delay(unsigned int);
//void ADC0_Init(void);
//unsigned long ADC0_In(void);
//void Init_PortF(void);

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
  GPIO_PORTE_DIR_R &= ~PE5; 
  //Enable PE5 as a alternate functionality as a ADC  
  GPIO_PORTE_AFSEL_R |= PE5; 
	//Disable I/O for PE5
  GPIO_PORTE_DEN_R &= ~PE5; 
	//Enable analogy for PE5  
  GPIO_PORTE_AMSEL_R |= PE5;  
	
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
	//Sequencer 3 is a software trigger (Bits are set to 0)
  ADC0_EMUX_R &= ~0xF000;    
	//We use channel 8 (Channel 8 is PE3 alt value obtained in datasheet)
  ADC0_SSMUX3_R = 0x0008; 	
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
unsigned long value;

void Init_PortF(void){
	unsigned int delay;
	SYSCTL_RCGC2_R |= 0x00000020;  // 1) F
	delay = SYSCTL_RCGC2_R;        // 2) No need to unlock
	GPIO_PORTF_AMSEL_R &= ~0x0A;   //3) Disable analog for PF3, PF1
	GPIO_PORTF_PCTL_R &= ~0x0000F0F0; //4) Enable regular GPIO
	GPIO_PORTF_DIR_R |= 0x0A;      //5) Outputs for PF3, PF1
	GPIO_PORTF_AFSEL_R &= ~0x0A;   //6) Regular Functions on PF3, PF1
	GPIO_PORTF_DEN_R |= 0x0A;      //7) Enable digital on PE3, PF1
}

void Delay(unsigned int x){unsigned long volatile time;
	
  time = (727240*200/91) * x/2;  // 0.1sec
  while(time){
		time--;
  }
}


//sample usage of ADC
int main() {
	//Set the board to run at 80MHZ clock (without it the code will 
	//expereience a hardware fault). This must stay
	PLL_Init();
	//Initialize ADC 0
	Init_PortF();
	ADC0_Init();
	GPIO_PORTF_DATA_R = 0x02;
	while (1) {
		//Obtain the ADC value. (0 - 3.3 V to 0 - 4095 )
		value = ADC0_In();
		
		if (value < 0xF00){
		GPIO_PORTF_DATA_R = 0x08;//0x02;
		}
		else{GPIO_PORTF_DATA_R = 0x02;}

	}
	return 0;
}

