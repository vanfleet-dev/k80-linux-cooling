/*
 * Pro Micro  →  Arctic P8 Max  –  25 kHz push-pull PWM (datasheet-correct)
 * ------------------------------------------------------------------------
 * • Fast-PWM mode 14 (TOP = ICR1) with non-inverting output:
 *      HIGH from BOTTOM to OCR1A, then LOW to TOP.
 * • Duty (0–100 %) from host maps directly to HIGH window.
 */

#include <Arduino.h>
#include "SerialTransfer.h"

SerialTransfer link;

/* --- 25 kHz parameters ----------------------------------------------- */
const uint8_t  PWM_PIN = 9;                    // D9 / PB5 / OC1A
const uint32_t PWM_HZ  = 25000;                // 25 kHz carrier
const uint16_t TOP     = F_CPU / PWM_HZ;       // 16 MHz / 25 kHz ≈ 640

void setup()
{
  Serial.begin(115200);
  link.begin(Serial);

  pinMode(PWM_PIN, OUTPUT);                    // push-pull drive

  /* Fast-PWM mode 14 : WGM13=1 WGM12=1 WGM11=1 WGM10=0
     COM1A1 = 1 , COM1A0 = 0  → non-inverting (HIGH first) */
  TCCR1A = (1 << COM1A1) | (1 << WGM11);
  TCCR1B = (1 << WGM13)  | (1 << WGM12) | (1 << CS10); // clk/1
  ICR1   = TOP;
  OCR1A  = 0;                                  // start fan full (will change)
}

void loop()
{
  if (!link.available()) return;

  /* echo back for debug */
  for (uint16_t i = 0; i < link.bytesRead; ++i)
    link.packet.txBuff[i] = link.packet.rxBuff[i];
  link.sendData(link.bytesRead);

  uint8_t duty = link.packet.rxBuff[0];        // 0–100 %

  /* Optional clamp: Arctic P-series stalls <15 % HIGH */
  if (duty < 15) duty = 0;                     // fan OFF below 15 %
  if (duty > 100) duty = 100;

  /* HIGH window = duty × TOP / 100 (datasheet) */
  OCR1A = (uint32_t)duty * TOP / 100;
}

