# STM32 Tap Tempo Metronome

A tap tempo metronome implemented using **STM32 Nucleo-F446RE**.
The project calculates BPM from the interval between button presses, displays the result on an OLED, and generates metronome beats using a PWM-controlled buzzer.

## Features

- Calculate BPM from four button taps
- Display BPM and beat status on an OLED
- Generate metronome sound using PWM
- Long press to stop and reset the metronome

## Hardware

- STM32 Nucleo-F446RE
- SSD1306 OLED Display (I2C)
- Push Button
- Passive Buzzer

## Development Environment

- **Language:** C
- **IDE:** STM32CubeIDE
- **Framework:** STM32 HAL Driver

