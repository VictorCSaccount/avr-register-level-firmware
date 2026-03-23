# ATmega32 RGB LED Controller

A register-level embedded firmware project for the ATmega32 microcontroller.
Controls an RGB LED system through a 4-digit 7-segment display menu, with
PWM output, analog dimming via ADC, strobe effect, and three-button navigation
featuring software debouncing and auto-repeat — all driven from a single
Timer2 ISR.

---


## Project Overview

This project was developed and deployed on the EASYAVR V7 development board by MikroElektronika.

## Hardware Platform

- Board: EASYAVR V7 (MikroElektronika)
- MCU: Microchip AVR

## Demo

![Project Demo](Proiect2/image.png)

## Hardware overview

| Component | Connection |
|---|---|
| RGB LED (Red) | PB3 — Timer0 PWM (OC0) |
| RGB LED (Green) | PD5 — Timer1 PWM (OC1A) |
| RGB LED (Blue) | PD4 — Timer1 PWM (OC1B) |
| 7-segment display (digit select) | PORTA bits 0–3 |
| 7-segment display (segments) | PORTC bits 0–6 |
| Button OK | PD2 (active LOW) |
| Button NEXT | PD3 (active LOW) |
| Button BACK | PB2 (active LOW) |
| Analog dimmer potentiometer | ADC channel 6 (PC6) |

---

## Features

**PWM output**
Three independent PWM channels drive the RGB LED. Timer0 runs in Fast PWM
mode for Red; Timer1 (16-bit, ICR1 = 255) provides two channels for Green
and Blue. All duty cycles are computed from percentage values (0–100) using
integer arithmetic to avoid floating-point on AVR.

**Hierarchical menu on 7-segment display**
The display is multiplexed manually inside the Timer2 ISR — one digit
refreshed per tick. The menu has three depth levels:

```
Level 0 (meniu)   — top-level: RGB | DIM | dAmC | STRB
Level 1 (meniu2)  — sub-item selection
Level 2 (meniu3)  — value editing mode
```

**Software button debouncing and auto-repeat**
Buttons are polled every 1 ms inside the ISR. No external interrupts are
used. Each button tracks:
- press/release edge detection
- press duration (ms counter)
- auto-repeat after 1 s hold, firing every 250 ms (4 events/second)

This is implemented with three independent state machines sharing the same
structure, one per button.

**Analog dimmer (ADC)**
When the dAmC (dimmer) mode is enabled, the ADC samples channel 6 every
500 ms. The raw 10-bit value is scaled to a 0–100 range and applied as a
multiplier to the RGB PWM values in real time.

**Strobe effect**
When strobe mode is enabled, the PWM compare output enable bits (COM bits)
in TCCR0/TCCR1A are toggled inside the ISR: PWM is active for the first
100 ms of each 1 s cycle, then disabled for the remaining 900 ms. When
strobe is off, the outputs are always enabled.

---

## Timer2 ISR structure

The entire application logic runs inside `ISR(TIMER2_COMP_vect)`, triggered
at 1 ms intervals (8 MHz clock, prescaler 64, OCR2 = 125).

Each tick, in order:
1. Increment `ms` counter (0–999, wraps at 1000)
2. Strobe toggle logic
3. Dimmer ADC read (at ms == 500)
4. 7-segment display multiplexing (one digit per tick)
5. Button state machines for OK, NEXT, BACK

The `main()` loop is intentionally empty — it only initialises peripherals
and enables global interrupts, then spins. This is a deliberate design
choice for a single-task embedded application where all timing is
interrupt-driven.

---

## Button logic macros

The three button actions (`PROCESS_OK_BUTTON`, `PROCESS_NEXT_BUTTON`,
`PROCESS_BACK_BUTTON`) are implemented as `do { ... } while(0)` macros so
they can be called from both the short-press path and the auto-repeat path
without code duplication, and without the overhead of a function call inside
an ISR.

---

## PWM duty cycle formula

```c
OCRx = Dimm * ((Color * 255) / 100) / 100;
```

`Color` is 0–100 (percentage), `Dimm` is 0–100 (master brightness).
Integer-only arithmetic: the inner division maps the color percentage to a
0–255 scale, and the outer division applies the dimmer.

---

## Menu structure reference

```
[RGB]
  -> [red]  -> value 0-100  (OCR0)
  -> [grEE] -> value 0-100  (OCR1A)
  -> [bLUe] -> value 0-100  (OCR1B)

[dimm]
  -> [dim1] -> value 0-100  (all channels scaled)

[dAmC]  (analog dimmer via ADC)
  -> [On]   -> dimmer_enabled = 1
  -> [OFF]  -> dimmer_enabled = 0, restore manual PWM

[Strb]  (strobe)
  -> [On]   -> strobe_enabled = 1
  -> [OFF]  -> strobe_enabled = 0
```

---

## Build

This project targets AVR-GCC. Build with any AVR toolchain:

```bash
avr-gcc -mmcu=atmega32 -DF_CPU=8000000UL -O2 -o rgb_controller.elf main.c
avr-objcopy -O ihex rgb_controller.elf rgb_controller.hex
avrdude -c usbasp -p m32 -U flash:w:rgb_controller.hex
```

---

## Requirements

- ATmega32 running at 8 MHz (internal or external oscillator)
- AVR-GCC + avrdude (or any AVR IDE: Microchip Studio, PlatformIO)
- Common-cathode 7-segment display (4 digits)
- RGB LED (common cathode or three separate LEDs)
- Three momentary push-buttons (active LOW, pulled up via internal or
  external resistors)
- Potentiometer on ADC6 for analog dimming (optional)

---

## License

MIT
