#!/usr/bin/env python3
"""
Tesla P40 → Pro Micro dual-fan controller
----------------------------------------

• Reads GPU temperature every 2 s
• Maps 40 °C → 0 % | 80 °C → 100 %  (clamped 0-100 %)
• Sends one duty-cycle byte via pySerialTransfer
• Optional --mock-temp N overrides GPU temperature (bench testing)
"""

import argparse
import sys
import time

from pySerialTransfer import pySerialTransfer as txfer
import GPUtil

PORT_NAME = "/dev/arduino-fans"
BAUD      = 115200
LO_T, HI_T = 40.0, 80.0           # linear ramp region


def get_gpu_temp():
    """Return Tesla P40 temperature (°C); fall back to hottest GPU."""
    gpus = GPUtil.getGPUs()
    for g in gpus:
        if "Tesla P40" in g.name:
            return g.temperature
    return max(g.temperature for g in gpus)


def temp_to_duty(temp_c):
    """Linear map: 40 °C->0 %, 80 °C->100 % (clamped)."""
    if temp_c <= LO_T:
        return 0
    if temp_c >= HI_T:
        return 100
    return int(round((temp_c - LO_T) / (HI_T - LO_T) * 100))


def main(mock_temp):
    try:
        link = txfer.SerialTransfer(PORT_NAME, BAUD, timeout=0.25)
        link.open()
    except Exception as e:
        sys.exit(f"❌ Could not open {PORT_NAME}: {e}")

    print(f"✓ Serial link open on {PORT_NAME} @ {BAUD}")

    while True:
        temp_c = mock_temp if mock_temp is not None else get_gpu_temp()
        duty   = temp_to_duty(temp_c)

        # send single byte (0-100)
        link.tx_buff[0] = duty
        link.send(1)

        # echo check
        if link.available():
            echo = link.rx_buff[0]
            ok   = "✓" if echo == duty else "⚠"
            print(f"{ok} Temp {temp_c:5.1f} °C → Duty {duty:3d}% (echo {echo})")
        else:
            print(f"⚠ No echo this cycle  Temp {temp_c:5.1f} °C  Duty {duty:3d}%")

        time.sleep(2)


if __name__ == "__main__":
    argp = argparse.ArgumentParser()
    argp.add_argument("--mock-temp", type=float,
                      help="Override GPU reading with a fixed temperature (°C)")
    args = argp.parse_args()
    main(args.mock_temp)
