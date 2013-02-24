//
// Component video test
//
#include "LPC17xx.h"
#include <stdint.h>

#define LED2_PIN (22)
#define VIDEO_PIN (0) // P2.0

volatile uint32_t systick_counter = 0;

void SysTick_Handler (void)
{
	systick_counter++;
}

void TIMER0_IRQHandler(void) {
  
  if(LPC_TIM0->IR & 0x1) {
    // Clear MR0 interrupt flag
    LPC_TIM0->IR = 0x1;
    
    // Toggle pin to check how often systick we wake up (should be 1ms)
    LPC_GPIO2->FIOPIN ^= (1 << VIDEO_PIN);
    
  }  
  
}

int main() {
  
  SystemInit();

  SysTick_Config(SystemCoreClock/1000 - 1); // Generate interrupt each 1 ms
  
  // Setup timer0
  LPC_SC->PCLKSEL0 |= (1 << 2); // Use CPU clock for timer0
  LPC_TIM0->MCR = 0x3;          // Interrupt and reset on MR0
  LPC_TIM0->MR0 = 3174;         // approx 31.5 KHz
  LPC_TIM0->TCR = 0x2;         // reset counter
  LPC_TIM0->TCR = 0x1;         // Enable timer
  
  // Setup P1.23 as output
  LPC_GPIO0->FIODIR |= (1 << LED2_PIN);
  
  // Setup video pin as output
  LPC_GPIO2->FIODIR |= (1 << VIDEO_PIN);
  
  NVIC_EnableIRQ(TIMER0_IRQn);
    
  for(;;) {

    if(0 == (systick_counter % 500)) {
      // Toggle LED
      LPC_GPIO0->FIOPIN ^= (1 << LED2_PIN);
    }
    
    __WFI(); // Sleep until next systick
       
  }
}

