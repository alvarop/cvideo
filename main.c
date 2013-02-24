//
// Component video test
//
#include "LPC17xx.h"
#include <stdint.h>

#define LED2_PIN (22)

int main() {
  
  SystemInit();
  
    // Setup P1.23 as output
  LPC_GPIO0->FIODIR |= (1 << LED2_PIN);
    
  for(;;) {
    for(uint32_t delay = 0; delay < 10000000; delay++) {
       __asm("NOP");
    }
    
    // Turn LED ON
    LPC_GPIO0->FIOSET = (1 << LED2_PIN);
    
    for(uint32_t delay = 0; delay < 10000000; delay++) {
       __asm("NOP");
    }
    
    // Turn LED OFF
    LPC_GPIO0->FIOCLR = (1 << LED2_PIN);
  }
}
