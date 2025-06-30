/*
 * Pro Micro 5 V / 16 MHz  ▸ Tesla-P40 dual-fan PWM driver
 *  – 25 kHz phase-correct on OC1A (D9)
 *  – duty = 0   → hard-low  (fan off)
 *  – duty = 1-99→ 25 kHz PWM
 *  – duty = 100 → Hi-Z      (spec-compliant 100 %)
 */

#include <Arduino.h>
#include "SerialTransfer.h"

SerialTransfer myTransfer;

/* ---------- pin map ------------------------------------------------------ */
const uint8_t PWM_PIN = 9;      // OC1A – D9 on Pro Micro

/* ---------- timer constants ---------------------------------------------- */
const uint32_t PWM_FREQ_HZ = 25000;               // 25 kHz
const uint16_t TCNT1_TOP   = F_CPU / (2 * PWM_FREQ_HZ);  // = 320 @ 16 MHz

/* ---------- struct sent from Python -------------------------------------- */
struct {
  uint8_t duty;   // 0–100  (%)
} rxPayload;

/* ------------------------------------------------------------------------- */
void setup()
{
  Serial.begin(115200);
  myTransfer.begin(Serial);

  pinMode(PWM_PIN, OUTPUT);      // default state

  /*--- Timer 1: phase-correct PWM, top = ICR1 -----------------------------*/
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  // COM1A1:0 = 10 (Clear on up-count, Set on down-count)  → OC1A active
  // WGM13:10 = 1010 (phase-correct PWM, TOP = ICR1)
  // CS10     = 1    (prescaler = 1)
  TCCR1A |= (1 << COM1A1) | (1 << WGM11);
  TCCR1B |= (1 << WGM13)  | (1 << CS10);

  ICR1 = TCNT1_TOP;   // sets 25 kHz
}

/* ------------------------------------------------------------------------- */
void loop()
{
  if (myTransfer.available())
  {
    /* echo back any bytes we got (compat with original python) */
    for (uint16_t i = 0; i < myTransfer.bytesRead; i++)
      myTransfer.packet.txBuff[i] = myTransfer.packet.rxBuff[i];
    myTransfer.sendData(myTransfer.bytesRead);

    uint16_t idx = 0;
    idx = myTransfer.rxObj(rxPayload, idx);

    setPwmDuty(rxPayload.duty);
  }
}

/* ---------- helper: translate 0–100 % into PWM --------------------------- */
void setPwmDuty(uint8_t duty)
{
  if (duty == 0)
  {
    /* hard-LOW – fan fully off */
    pinMode(PWM_PIN, OUTPUT);
    digitalWrite(PWM_PIN, LOW);
  }
  else if (duty >= 100)
  {
    /* open-drain Hi-Z – spec-compliant 100 % duty */
    pinMode(PWM_PIN, INPUT);
  }
  else
  {
    /* normal PWM 1-99 % */
    pinMode(PWM_PIN, OUTPUT);
    OCR1A = (uint16_t)((uint32_t)duty * TCNT1_TOP) / 100;
  }
}
