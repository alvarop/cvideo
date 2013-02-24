//
// Component video test
//
#include "LPC17xx.h"
#include <stdint.h>

#define LED2_PIN (22)
#define VIDEO_PIN (0) // P2.0
#define HSYNC_PIN (1) // P2.1

#define LINE_PERIOD (6350) // ~15.75 kHz
#define FRONT_PORCH (150) // 1.5us
#define HSYNC_WIDTH (485) // 4.85us
#define BACK_PORCH (485) // 4.85us

#define VSYNC_END (9*2)
#define TOTAL_LINES (262+9)

#define PIXEL_START (HSYNC_WIDTH + BACK_PORCH)

#define SET_BLACK LPC_GPIO2->FIOCLR = (1 << HSYNC_PIN);LPC_GPIO2->FIOSET = (1 << VIDEO_PIN)
#define SET_WHITE LPC_GPIO2->FIOCLR = (1 << VIDEO_PIN);LPC_GPIO2->FIOSET = (1 << HSYNC_PIN)
#define SET_HSYNC LPC_GPIO2->FIOCLR = (1 << VIDEO_PIN);LPC_GPIO2->FIOCLR = (1 << HSYNC_PIN)

volatile uint32_t systick_counter = 0;
volatile uint32_t line = 0;

volatile uint32_t color = 0;

void SysTick_Handler (void)
{
	systick_counter++;
}

void TIMER0_IRQHandler(void) {
  uint32_t ir = LPC_TIM0->IR;
  
  if(ir & 0x1) {
    // Clear MR0 interrupt flag
    LPC_TIM0->IR = 0x1;
    
    if(++line == TOTAL_LINES) {
      line = 0;
      // The first 9 lines are vsync pulses
      LPC_TIM0->MR0 = LINE_PERIOD/2;
      LPC_GPIO0->FIOSET = (1 << LED2_PIN);
    } else if(line == VSYNC_END) {
      LPC_TIM0->MR0 = LINE_PERIOD;
      LPC_GPIO0->FIOCLR = (1 << LED2_PIN);
    }
    
    SET_HSYNC;
    
    if(line >= VSYNC_END) {
      LPC_TIM0->MR1 = HSYNC_WIDTH;
    } else if((line > 5) && (line < 12)) {
      LPC_TIM0->MR1 = LPC_TIM0->MR0 - HSYNC_WIDTH/2;
    } else {;
      LPC_TIM0->MR1 = HSYNC_WIDTH/2;
    }
    
  } else if(ir & 0x2) {
    // Clear MR1 interrupt flag
    LPC_TIM0->IR = 0x2;

    SET_BLACK;
  } else if(ir & 0x4) {
    // Clear MR2 interrupt flag
    LPC_TIM0->IR = 0x4;
    // Shouldn't draw on the first 20 lines!
    if(line >= (VSYNC_END + 10)) {
      if(color) {
        SET_WHITE;
      } else {
        SET_BLACK;
      }
      
      if(LPC_TIM0->TC > (LINE_PERIOD - FRONT_PORCH)) {
        LPC_TIM0->MR2 = PIXEL_START;
      }
    }
        
  } else if(ir & 0x8) {
    // Clear MR3 interrupt flag
    LPC_TIM0->IR = 0x8;
  }
  
}

int main() {
  
  SystemInit();

  SysTick_Config(SystemCoreClock/1000 - 1); // Generate interrupt each 1 ms
  
  // Setup timer0
  LPC_SC->PCLKSEL0 |= (1 << 2); // Use CPU clock for timer0
  
  // Interrupt and reset on MR0, interrupt on MR1 and MR2
  LPC_TIM0->MCR = (3 << 0) | (1 << 3) | (1 << 6); 
  LPC_TIM0->MR0 = LINE_PERIOD;
  LPC_TIM0->MR1 = 0;
  LPC_TIM0->MR2 = PIXEL_START;
  LPC_TIM0->TCR = 0x2;          // reset counter
  LPC_TIM0->TCR = 0x1;          // Enable timer
  
  // Setup P1.23 as output
  LPC_GPIO0->FIODIR |= (1 << LED2_PIN);
  
  // Setup video pin as output
  LPC_GPIO2->FIODIR |= (1 << VIDEO_PIN);
  LPC_GPIO2->FIODIR |= (1 << HSYNC_PIN);
  
  // HSYNC high
  SET_HSYNC;
  
  NVIC_EnableIRQ(TIMER0_IRQn);
  
  uint32_t next_toggle = 5000;
  
  for(;;) {

    if(systick_counter >= next_toggle) {
      // Toggle LED
      next_toggle += 5000;
      color ^= 1;
    }
    
    __WFI(); // Sleep until next systick
       
  }
}

