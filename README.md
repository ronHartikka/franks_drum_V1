# Frank's Drum V1

Arduino drum controller with rotary encoder and pedal input for NodeMCU ESP8266.

## Features

- Rotary encoder for tempo control (46-145 BPM)
- Pedal input via ADS1115 ADC for dynamic control
- Solenoid control with variable pulse width
- Built-in LED metronome indicator
- Button press to toggle pedal enable/disable

## Hardware Requirements

- NodeMCU 1.0 (ESP8266)
- Rotary encoder (with button)
- ADS1115 ADC module
- Solenoid actuator
- Pedal input sensor

## Pin Configuration

- D0 (GPIO16): Built-in LED
- D3 (GPIO0): Solenoid control
- D5 (GPIO14): Encoder CLK
- D6 (GPIO12): Encoder DT
- D7 (GPIO13): Encoder button

## Dependencies

- Adafruit ADS1X15 library

## License

MIT
