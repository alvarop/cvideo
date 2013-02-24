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

int main() {
  
  SystemInit();

  SysTick_Config(SystemCoreClock/1000 - 1); // Generate interrupt each 1 ms

  // Setup P1.23 as output
  LPC_GPIO0->FIODIR |= (1 << LED2_PIN);
  
  // Setup video pin as output
  LPC_GPIO2->FIODIR |= (1 << VIDEO_PIN);
    
  for(;;) {

    if(0 == (systick_counter % 500)) {
      // Toggle LED
      LPC_GPIO0->FIOPIN ^= (1 << LED2_PIN);
    }
    
    // Toggle pin to check how often systick we wake up (should be 1ms)
    LPC_GPIO2->FIOPIN ^= (1 << VIDEO_PIN);
    
    __WFI(); // Sleep until next systick
       
  }
}
