//
// Component video test
//
#include "LPC17xx.h"
#include <stdint.h>

#define LED2_PIN (22)
#define VIDEO_PIN (0) // P2.0
#define HSYNC_PIN (1) // P2.1

#define LINE_PERIOD (6348) // ~15.75 kHz
#define FRONT_PORCH (150) // 1.5us
#define HSYNC_WIDTH (485) // 4.85us
#define BACK_PORCH (485) // 4.85us

volatile uint32_t systick_counter = 0;
volatile uint16_t line = 0;

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
    
  } else if(LPC_TIM0->IR & 0x2) {
    // Clear MR1 interrupt flag
    LPC_TIM0->IR = 0x2;
    
    // HSYNC
    if(LPC_GPIO2->FIOPIN & (1 << HSYNC_PIN)) {
      LPC_GPIO2->FIOCLR = (1 << HSYNC_PIN);
      LPC_TIM0->MR1 = FRONT_PORCH + HSYNC_WIDTH;
    } else {
      LPC_GPIO2->FIOSET = (1 << HSYNC_PIN);
      LPC_TIM0->MR1 = FRONT_PORCH;
    }
  } else if(LPC_TIM0->IR & 0x4) {
    // Clear MR2 interrupt flag
    LPC_TIM0->IR = 0x4;
        
  } else if(LPC_TIM0->IR & 0x8) {
    // Clear MR3 interrupt flag
    LPC_TIM0->IR = 0x8;
        
  }
  
}

int main() {
  
  SystemInit();

  SysTick_Config(SystemCoreClock/1000 - 1); // Generate interrupt each 1 ms
  
  // Setup timer0
  LPC_SC->PCLKSEL0 |= (1 << 2); // Use CPU clock for timer0
  LPC_TIM0->MCR = (3 << 0) | (1 << 3); // Interrupt and reset on MR0, interrupt on MR1
  LPC_TIM0->MR0 = LINE_PERIOD;
  LPC_TIM0->MR1 = FRONT_PORCH;
  LPC_TIM0->TCR = 0x2;          // reset counter
  LPC_TIM0->TCR = 0x1;          // Enable timer
  
  // Setup P1.23 as output
  LPC_GPIO0->FIODIR |= (1 << LED2_PIN);
  
  // Setup video pin as output
  LPC_GPIO2->FIODIR |= (1 << VIDEO_PIN);
  LPC_GPIO2->FIODIR |= (1 << HSYNC_PIN);
  
  // HSYNC high
  LPC_GPIO2->FIOSET = (1 << HSYNC_PIN);
  
  NVIC_EnableIRQ(TIMER0_IRQn);
    
  for(;;) {

    if(0 == (systick_counter % 500)) {
      // Toggle LED
      LPC_GPIO0->FIOPIN ^= (1 << LED2_PIN);
    }
    
    __WFI(); // Sleep until next systick
       
  }
}

