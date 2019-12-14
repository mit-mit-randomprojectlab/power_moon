/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

// This is a modified version of the original SAMD tone implementation
// that uses two separate functions, one for each of timer 4 and timer 5
// on the SAMD

#include "Tone1.h"
#include "variant.h"

#define WAIT_TC16_REGS_SYNC(x) while(x->COUNT16.STATUS.bit.SYNCBUSY);

uint32_t toneMaxFrequency = F_CPU / 2;
uint32_t lastOutputPin1 = 0xFFFFFFFF;
uint32_t lastOutputPin2 = 0xFFFFFFFF;

volatile uint32_t *portToggleRegister1;
volatile uint32_t *portToggleRegister2;
volatile uint32_t *portClearRegister1;
volatile uint32_t *portClearRegister2;
volatile uint32_t portBitMask1;
volatile uint32_t portBitMask2;
volatile int64_t toggleCount1;
volatile int64_t toggleCount2;
volatile bool toneIsActive1 = false;
volatile bool toneIsActive2 = false;
volatile bool firstTimeRunning1 = false;
volatile bool firstTimeRunning2 = false;

#define TONE_TC1         TC4
#define TONE_TC1_IRQn    TC4_IRQn
#define TONE_TC2         TC5
#define TONE_TC2_IRQn    TC5_IRQn
#define TONE_TC_TOP     0xFFFF
#define TONE_TC_CHANNEL 0

void TC4_Handler (void) __attribute__ ((weak, alias("Tone_Handler1")));
void TC5_Handler (void) __attribute__ ((weak, alias("Tone_Handler2")));

static inline void resetTC (Tc* TCx)
{
  // Disable TCx
  TCx->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
  WAIT_TC16_REGS_SYNC(TCx)

  // Reset TCx
  TCx->COUNT16.CTRLA.reg = TC_CTRLA_SWRST;
  WAIT_TC16_REGS_SYNC(TCx)
  while (TCx->COUNT16.CTRLA.bit.SWRST);
}

void tone1 (uint32_t outputPin, uint32_t frequency, uint32_t duration)
{
  // Configure interrupt request
  NVIC_DisableIRQ(TONE_TC1_IRQn);
  NVIC_ClearPendingIRQ(TONE_TC1_IRQn);
  
  if(!firstTimeRunning1)
  {
    firstTimeRunning1 = true;
    
    NVIC_SetPriority(TONE_TC1_IRQn, 0);
      
    // Enable GCLK for TC4 and TC5 (timer counter input clock)
    GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5));
    while (GCLK->STATUS.bit.SYNCBUSY);
  }
  
  if (toneIsActive1 && (outputPin != lastOutputPin1))
    noTone1(lastOutputPin1);

  //
  // Calculate best prescaler divider and comparator value for a 16 bit TC peripheral
  //

  uint32_t prescalerConfigBits;
  uint32_t ccValue;

  ccValue = toneMaxFrequency / frequency - 1;
  prescalerConfigBits = TC_CTRLA_PRESCALER_DIV1;
  
  uint8_t i = 0;
  
  while(ccValue > TONE_TC_TOP)
  {
    ccValue = toneMaxFrequency / frequency / (2<<i) - 1;
    i++;
    if(i == 4 || i == 6 || i == 8) //DIV32 DIV128 and DIV512 are not available
     i++;
  }
  
  switch(i-1)
  {
    case 0: prescalerConfigBits = TC_CTRLA_PRESCALER_DIV2; break;
    
    case 1: prescalerConfigBits = TC_CTRLA_PRESCALER_DIV4; break;
    
    case 2: prescalerConfigBits = TC_CTRLA_PRESCALER_DIV8; break;
    
    case 3: prescalerConfigBits = TC_CTRLA_PRESCALER_DIV16; break;
    
    case 5: prescalerConfigBits = TC_CTRLA_PRESCALER_DIV64; break;
      
    case 7: prescalerConfigBits = TC_CTRLA_PRESCALER_DIV256; break;
    
    case 9: prescalerConfigBits = TC_CTRLA_PRESCALER_DIV1024; break;
    
    default: break;
  }

  toggleCount1 = (duration > 0 ? frequency * duration * 2 / 1000UL : -1LL);

  resetTC(TONE_TC1);

  uint16_t tmpReg = 0;
  tmpReg |= TC_CTRLA_MODE_COUNT16;  // Set Timer counter Mode to 16 bits
  tmpReg |= TC_CTRLA_WAVEGEN_MFRQ;  // Set TONE_TC mode as match frequency
  tmpReg |= prescalerConfigBits;
  TONE_TC1->COUNT16.CTRLA.reg |= tmpReg;
  WAIT_TC16_REGS_SYNC(TONE_TC1)

  TONE_TC1->COUNT16.CC[TONE_TC_CHANNEL].reg = (uint16_t) ccValue;
  WAIT_TC16_REGS_SYNC(TONE_TC1)

  portToggleRegister1 = &(PORT->Group[g_APinDescription[outputPin].ulPort].OUTTGL.reg);
  portClearRegister1 = &(PORT->Group[g_APinDescription[outputPin].ulPort].OUTCLR.reg);
  portBitMask1 = (1ul << g_APinDescription[outputPin].ulPin);

  // Enable the TONE_TC interrupt request
  TONE_TC1->COUNT16.INTENSET.bit.MC0 = 1;
  
  if (outputPin != lastOutputPin1)
  {
    lastOutputPin1 = outputPin;
    digitalWrite(outputPin, LOW);
    pinMode(outputPin, OUTPUT);
    toneIsActive1 = true;
  }

  // Enable TONE_TC
  TONE_TC1->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
  WAIT_TC16_REGS_SYNC(TONE_TC1)
  
  NVIC_EnableIRQ(TONE_TC1_IRQn);
}

void noTone1 (uint32_t outputPin)
{
  /* 'tone' need to run at least once in order to enable GCLK for
   * the timers used for the tone-functionality. If 'noTone' is called
   * without ever calling 'tone' before then 'WAIT_TC16_REGS_SYNC(TCx)'
   * will wait infinitely. The variable 'firstTimeRunning' is set the
   * 1st time 'tone' is set so it can be used to detect wether or not
   * 'tone' has been called before.
   */
  if(firstTimeRunning1)
  {
    resetTC(TONE_TC1);
    digitalWrite(outputPin, LOW);
    toneIsActive1 = false;
  }
}

void tone2 (uint32_t outputPin, uint32_t frequency, uint32_t duration)
{
  // Configure interrupt request
  NVIC_DisableIRQ(TONE_TC2_IRQn);
  NVIC_ClearPendingIRQ(TONE_TC2_IRQn);
  
  if(!firstTimeRunning2)
  {
    firstTimeRunning2 = true;
    
    NVIC_SetPriority(TONE_TC2_IRQn, 0);
      
    // Enable GCLK for TC4 and TC5 (timer counter input clock)
    GCLK->CLKCTRL.reg = (uint16_t) (GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_ID(GCM_TC4_TC5));
    while (GCLK->STATUS.bit.SYNCBUSY);
  }
  
  if (toneIsActive2 && (outputPin != lastOutputPin2))
    noTone2(lastOutputPin2);

  //
  // Calculate best prescaler divider and comparator value for a 16 bit TC peripheral
  //

  uint32_t prescalerConfigBits;
  uint32_t ccValue;

  ccValue = toneMaxFrequency / frequency - 1;
  prescalerConfigBits = TC_CTRLA_PRESCALER_DIV1;
  
  uint8_t i = 0;
  
  while(ccValue > TONE_TC_TOP)
  {
    ccValue = toneMaxFrequency / frequency / (2<<i) - 1;
    i++;
    if(i == 4 || i == 6 || i == 8) //DIV32 DIV128 and DIV512 are not available
     i++;
  }
  
  switch(i-1)
  {
    case 0: prescalerConfigBits = TC_CTRLA_PRESCALER_DIV2; break;
    
    case 1: prescalerConfigBits = TC_CTRLA_PRESCALER_DIV4; break;
    
    case 2: prescalerConfigBits = TC_CTRLA_PRESCALER_DIV8; break;
    
    case 3: prescalerConfigBits = TC_CTRLA_PRESCALER_DIV16; break;
    
    case 5: prescalerConfigBits = TC_CTRLA_PRESCALER_DIV64; break;
      
    case 7: prescalerConfigBits = TC_CTRLA_PRESCALER_DIV256; break;
    
    case 9: prescalerConfigBits = TC_CTRLA_PRESCALER_DIV1024; break;
    
    default: break;
  }

  toggleCount2 = (duration > 0 ? frequency * duration * 2 / 1000UL : -1LL);

  resetTC(TONE_TC2);

  uint16_t tmpReg = 0;
  tmpReg |= TC_CTRLA_MODE_COUNT16;  // Set Timer counter Mode to 16 bits
  tmpReg |= TC_CTRLA_WAVEGEN_MFRQ;  // Set TONE_TC mode as match frequency
  tmpReg |= prescalerConfigBits;
  TONE_TC2->COUNT16.CTRLA.reg |= tmpReg;
  WAIT_TC16_REGS_SYNC(TONE_TC2)

  TONE_TC2->COUNT16.CC[TONE_TC_CHANNEL].reg = (uint16_t) ccValue;
  WAIT_TC16_REGS_SYNC(TONE_TC2)

  portToggleRegister2 = &(PORT->Group[g_APinDescription[outputPin].ulPort].OUTTGL.reg);
  portClearRegister2 = &(PORT->Group[g_APinDescription[outputPin].ulPort].OUTCLR.reg);
  portBitMask2 = (1ul << g_APinDescription[outputPin].ulPin);

  // Enable the TONE_TC interrupt request
  TONE_TC2->COUNT16.INTENSET.bit.MC0 = 1;
  
  if (outputPin != lastOutputPin2)
  {
    lastOutputPin2 = outputPin;
    digitalWrite(outputPin, LOW);
    pinMode(outputPin, OUTPUT);
    toneIsActive2 = true;
  }

  // Enable TONE_TC
  TONE_TC2->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
  WAIT_TC16_REGS_SYNC(TONE_TC2)
  
  NVIC_EnableIRQ(TONE_TC2_IRQn);
}

void noTone2 (uint32_t outputPin)
{
  /* 'tone' need to run at least once in order to enable GCLK for
   * the timers used for the tone-functionality. If 'noTone' is called
   * without ever calling 'tone' before then 'WAIT_TC16_REGS_SYNC(TCx)'
   * will wait infinitely. The variable 'firstTimeRunning' is set the
   * 1st time 'tone' is set so it can be used to detect wether or not
   * 'tone' has been called before.
   */
  if(firstTimeRunning2)
  {
    resetTC(TONE_TC2);
    digitalWrite(outputPin, LOW);
    toneIsActive2 = false;
  }
}

#ifdef __cplusplus
extern "C" {
#endif

void Tone_Handler1 (void)
{
  if (toggleCount1 != 0)
  {
    // Toggle the ouput pin
    *portToggleRegister1 = portBitMask1;

    if (toggleCount1 > 0)
      --toggleCount1;

    // Clear the interrupt
    TONE_TC1->COUNT16.INTFLAG.bit.MC0 = 1;
  }
  else
  {
    resetTC(TONE_TC1);
    *portClearRegister1 = portBitMask1;
    toneIsActive1 = false;
  }
}

void Tone_Handler2 (void)
{
  if (toggleCount2 != 0)
  {
    // Toggle the ouput pin
    *portToggleRegister2 = portBitMask2;

    if (toggleCount2 > 0)
      --toggleCount2;

    // Clear the interrupt
    TONE_TC2->COUNT16.INTFLAG.bit.MC0 = 1;
  }
  else
  {
    resetTC(TONE_TC2);
    *portClearRegister2 = portBitMask2;
    toneIsActive2 = false;
  }
}

#ifdef __cplusplus
}
#endif
