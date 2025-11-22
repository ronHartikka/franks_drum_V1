# Frank's Drum V1 Project Status

## Current Status - 2025-11-18
**ISSUE**: Brown-out cycle on standalone power. System boots only when USB connected.

### Recent Changes (2025-11-18)
- Changed minPulseMicroseconds: 7500 → 0 → 7500 μs (line 38)
- Changed pedal voltage range: 850-2777 mV → 200-3000 mV (lines 40-41)
- Added cutoff: pedal voltage >= 3000 mV → no pulse (PW = 0, no GPIO toggle)
- Initialized PW = 0 to prevent startup pulses
- Added ADS1115 startup stabilization (100ms delay + dummy reads)

### Boot Issue Problem
**Symptom**: MCU enters brown-out reboot cycle when powered standalone with solenoid connected.
- Solenoid energizes during boot → draws current → 5V regulator browns out → reboot → repeat
- **Root cause**: D3 (GPIO0) is a **boot mode pin** - must be HIGH during boot
- Solenoid driver may pull GPIO0 LOW during power-up, preventing normal boot

**Root Cause Found (2025-11-22)**:
- Setup code had `digitalWrite(SOLENOID_PIN, LOW)` with wrong comment "Solenoid off"
- Circuit is ACTIVE LOW: D3 HIGH = solenoid OFF, D3 LOW = solenoid ON
- So setup() was actually turning solenoid ON, causing brownout when plugged in
- **Fix**: Changed to `digitalWrite(SOLENOID_PIN, HIGH)` in setup()

**Current Behavior**:
- Boots standalone with solenoid connected (even if solenoid plugged in before 24V)
- One weak drum beat during boot (GPIO0 floats LOW during bootloader)
- After boot, works correctly

**Optional Future Improvement**:
- Add external 10K pull-up resistor from D3 to 3.3V to prevent boot-time pulse, OR
- Move solenoid control to non-boot-critical GPIO (D1/D2/D4)

## Previous Status
Successfully uploaded and tested on NodeMCU ESP8266. Tempo range 30-150 BPM working correctly.

## What We've Done
1. ✅ Copied code from Claude_Drums_V2/Claude_Drums_V2.ino
2. ✅ Created project structure in franks_drum/franks_drum_V1/
3. ✅ Set up Git repository
4. ✅ Created GitHub repository: https://github.com/ronHartikka/franks_drum_V1
5. ✅ Pushed initial code to GitHub
6. ✅ Created README.md with project documentation
7. ✅ Created STATUS.md for project tracking
8. ✅ Adjusted tempo range: minTempo 46→30 BPM, maxTempo 145→150 BPM
9. ✅ Compiled and uploaded to NodeMCU ESP8266 via /dev/tty.SLAB_USBtoUART
10. ✅ Verified working - serial output confirmed correct operation
11. ✅ Tested minTempo with stopwatch - confirmed accurate at 30 BPM

## Next Steps
1. Test encoder functionality and tempo changes in real use
2. Test pedal input and pulse width modulation
3. Fine-tune pulse width parameters if needed (currently 7500-20000 μs)
4. Test button toggle for pedal enable/disable
5. Further calibration based on real-world testing

## Important Info

### Hardware
- **MCU**: NodeMCU 1.0 (ESP8266)
- **D0 (GPIO16)**: Built-in LED indicator
- **D3 (GPIO0)**: Solenoid control output
- **D5 (GPIO14)**: Encoder CLK (with pullup)
- **D6 (GPIO12)**: Encoder DT (with pullup)
- **D7 (GPIO13)**: Encoder button (with pullup)
- **ADS1115**: I2C ADC for pedal input (differential channels 0-1)

### Upload Command (Arduino CLI)
```bash
# Find port first
ls /dev/tty.* | grep usb

# Upload (adjust port as needed)
arduino-cli upload -p /dev/tty.usbserial-XXXX --fqbn esp8266:esp8266:nodemcuv2 .
```

### Before Uploading
- Close Arduino IDE completely (or at least Serial Monitor)
- NodeMCU needs to be plugged in
- May need to press FLASH button during upload

### Key Parameters (in code)
- **Tempo Range**: 30-150 BPM (lines 30-32)
- **Default Tempo**: 90 BPM (calculated as average)
- **Min Pulse Width**: 7500 μs (line 38)
- **Max Pulse Width**: 20000 μs (line 39)
- **Pedal Voltage Range**: 850-2777 mV (lines 40-41)
- **ADS1115 Gain**: GAIN_ONE (±4.096V, 0.125mV/bit)

### Serial Monitor Settings
- **Baud Rate**: 115200
- Monitor shows:
  - Tempo changes (position and direction)
  - Pedal voltage readings
  - Pulse width in microseconds
  - Button press events

## Problem We're Solving
Creating an automated drum controller with:
- Adjustable tempo via rotary encoder (BPM control)
- Dynamic drum strike intensity controlled by foot pedal
- Variable pulse width to solenoid based on pedal pressure
- Visual metronome feedback via built-in LED

## Dependencies
- **Adafruit ADS1X15** library (for ADS1115 ADC)
  ```bash
  arduino-cli lib install "Adafruit ADS1X15"
  ```

## Notes
- Solenoid control polarity appears inverted in code (lines 148, 165)
  - LOW = ON, HIGH = OFF (may need verification with actual hardware)
- Encoder uses interrupt-based detection for responsive operation
- Button toggles pedal enable/disable state
- Metronome timing uses millis() for beat intervals, micros() for pulse width precision
