//
// Component video test
//
#include "LPC17xx.h"
#include <stdint.h>

#define LED2_PIN (22)
#define DBG_PIN (21)
#define DBG2_PIN (3)

#define VIDEO_PIN (0) // P2.0
#define HSYNC_PIN (1) // P2.1

#define LINE_PERIOD (6350) // ~15.75 kHz
#define PIXEL_PERIOD (200) // 26ish columns for now
#define FRONT_PORCH (150) // 1.5us
#define HSYNC_WIDTH (485) // 4.85us
#define BACK_PORCH (485) // 4.85us

#define VSYNC_END (9*2)
#define TOTAL_LINES (262+9)

#define PIXEL_START (HSYNC_WIDTH + BACK_PORCH)

//#define SET_BLACK LPC_GPIO2->FIOCLR = (1 << HSYNC_PIN);LPC_GPIO2->FIOSET = (1 << VIDEO_PIN)
//#define SET_WHITE LPC_GPIO2->FIOCLR = (1 << VIDEO_PIN);LPC_GPIO2->FIOSET = (1 << HSYNC_PIN)
//#define SET_HSYNC LPC_GPIO2->FIOCLR = (1 << VIDEO_PIN);LPC_GPIO2->FIOCLR = (1 << HSYNC_PIN)

#define SET_BLACK LPC_GPIO2->FIOPIN = (1 << VIDEO_PIN)
#define SET_WHITE LPC_GPIO2->FIOPIN = (1 << HSYNC_PIN)
#define SET_HSYNC LPC_GPIO2->FIOPIN = 0


volatile uint32_t systick_counter = 0;
volatile uint32_t scanline = 0;

volatile uint32_t draw = 0;

volatile uint32_t half_frame = 0;

volatile uint32_t color = 0;

volatile int32_t col = 0;
volatile int32_t row = 0;

uint32_t next_toggle = 500;

#define WIDTH_PX (190)

uint8_t screen[2][WIDTH_PX];

#define TOTAL_STATES (3)
#define STATE_IDLE (0)
#define STATE_DRAWING (1)
#define STATE_COMPUTE (2)
volatile uint32_t state = STATE_IDLE;

void SysTick_Handler (void)
{
	systick_counter++;
}

void TIMER0_IRQHandler(void) {
  uint32_t ir = LPC_TIM0->IR;
  
  if(ir & 0x1) {
    // Clear MR0 interrupt flag
    LPC_TIM0->IR = 0x1;
    
    if(++scanline == TOTAL_LINES) {
      scanline = 0;
      LPC_GPIO0->FIOSET = (1 << DBG_PIN);
      // The first 9 lines are vsync pulses
      LPC_TIM0->MR0 = LINE_PERIOD/2;
      
      row = -1;
      
      // Shouldn't draw on the first 20 lines! 
      LPC_TIM0->MCR &= ~(1 << 6); // Disable pixel draw interrupt
      
      half_frame++;
      state = STATE_IDLE;
    } else if(scanline == VSYNC_END) {
      LPC_TIM0->MR0 = LINE_PERIOD;
      state = STATE_COMPUTE;
    } else if(scanline >= (VSYNC_END + 10)) {
      LPC_TIM0->MCR |= (1 << 6); // Enable pixel draw interrupt
    }
    
    row++;
    col = 0;
    
    SET_HSYNC;
    
    if(scanline >= VSYNC_END) {
      LPC_TIM0->MR1 = HSYNC_WIDTH;
    } else if((scanline > 5) && (scanline < 12)) {
      LPC_TIM0->MR1 = LPC_TIM0->MR0 - HSYNC_WIDTH/2;
    } else {
      LPC_TIM0->MR1 = HSYNC_WIDTH/2;
    }
    
  } else if(ir & 0x2) {
    // Clear MR1 interrupt flag
    LPC_TIM0->IR = 0x2;
    LPC_GPIO0->FIOCLR = (1 << DBG_PIN);
    SET_BLACK;
  } else if(ir & 0x4) {
    // Clear MR2 interrupt flag
    LPC_TIM0->IR = 0x4;

    col++;
    state = STATE_DRAWING;
  } else if(ir & 0x8) {
    // Clear MR3 interrupt flag
    LPC_TIM0->IR = 0x8;
  }
}

void fn_idle() {
  if(systick_counter >= next_toggle) {
    // Toggle LED
    LPC_GPIO0->FIOPIN ^= (1 << LED2_PIN);
    //color ^= 1;
    next_toggle += 500;
  }
}

void fn_drawing() {
  uint8_t *line = screen[!(scanline&0x2)];
  for(uint16_t x = 0; x < WIDTH_PX; x++) {
    if(line[x]) {
      SET_WHITE;
    } else {
      SET_BLACK;
    }
  }

  SET_BLACK;
  state = STATE_IDLE;
}

void fn_compute() {
  
  state = STATE_IDLE;
}

void (*fn[TOTAL_STATES])() = {fn_idle, fn_drawing, fn_compute};

int main() {
  
  SystemInit();

  //SysTick_Config(SystemCoreClock/1000 - 1); // Generate interrupt each 1 ms
  
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
  LPC_GPIO0->FIODIR |= (1 << DBG_PIN);
  LPC_GPIO0->FIODIR |= (1 << DBG2_PIN);
  
  // Setup video pin as output
  LPC_GPIO2->FIODIR |= (1 << VIDEO_PIN);
  LPC_GPIO2->FIODIR |= (1 << HSYNC_PIN);
  
  // HSYNC high
  SET_HSYNC;
  //
  for(uint16_t x = 0; x < WIDTH_PX; x++) {
    screen[0][x] = !(x & 0x01);
  }

  for(uint16_t x = 0; x < WIDTH_PX; x++) {
    screen[1][x] = !!(x & 0x01);
  }

  NVIC_EnableIRQ(TIMER0_IRQn);
  
  for(;;) {
    
    if(state < TOTAL_STATES) {
      fn[state]();
    }
    __WFI(); // Sleep until next systick
       
  }
}

