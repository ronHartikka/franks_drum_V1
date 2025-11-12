// Rotary Encoder Reader with Interrupts (Including Button)
// Minimal ISR approach - processing done in main loop
// Adapted for NodeMCU 1.0 (ESP8266)

#include <Adafruit_ADS1X15.h>

#define BUILTIN_LED_PIN D0 // Built-in LED on NodeMCU at pin GPIO16 (D0)
#define SOLENOID_PIN D3  // GPIO 0 to control solenoid

// Pin definitions (using GPIO numbers)
const int ENCODER_CLK = 14;  // GPI014 (D5 on NodeMCU board) - Safe to use
const int ENCODER_DT = 12;   // GPI012 (D6 on NodeMCU board) - Safe to use
const int ENCODER_SW = 13;   // GPIO13 (D7 on NodeMCU board) - Safe to use

// Function declarations
void ICACHE_RAM_ATTR encoderISR();
void ICACHE_RAM_ATTR buttonISR();
void processEncoder(byte encoded);
void onButtonPressed();
float readPedalVoltage();
int calculatePulseWidth(float pedal_mV);

// ADS1115 ADC
Adafruit_ADS1115 ads;

// Encoder variables
volatile bool encoderFlag = false;
volatile byte encoderState = 0;

long minTempo = 30; // minimum tempo allowed
long maxTempo = 150; // maximum tempo allowed
long defaultTempo = long (maxTempo + minTempo)/2; // default Tempo
long Tempo = defaultTempo; // start off at default
long encoderPosition = Tempo; // copy to encoder position
byte pedalEnable = true; // if true, set solenoid PW according to pedal

// Metronome variables
int minPulseMicroseconds = 7500; // minimum PW in microseconds
int maxPulseMicroseconds = 20000; // maximum PW in microseconds
float maxPedal_mV = 2777;  // max expected differential reading from ADS1115 differential (channel 0 is HI)
float minPedal_mV = 850; // min expected differential reading from ADS1115
float pedal_mV = maxPedal_mV; // current pedal reading in millivolts
int PW = minPulseMicroseconds; // pulse width in microseconds
unsigned long lastBeatTime = 0;
unsigned long beatInterval = 0;
bool ledState = false;
unsigned long ledOnTime = 0;

// State tracking
byte lastEncoded = 0;

// Button variables
volatile bool buttonFlag = false;
bool isPulsesEnabled = true;  // whether drum makes sound
unsigned long buttonPressTime = 0;  // track when button pressed
bool buttonCurrentlyPressed = false;  // track button state
const unsigned long LONG_PRESS_THRESHOLD = 500;  // milliseconds

void setup() {
  Serial.begin(115200);

  // Initialize ADS1115
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS1115!");
    while (1);
  }

  // Configure ADS1115: Gain 1, continuous mode, differential 0-1
  ads.setGain(GAIN_ONE);  // 1x gain +/- 4.096V  1 bit = 0.125mV
  ads.startADCReading(ADS1X15_REG_CONFIG_MUX_DIFF_0_1, /*continuous=*/true);

  // Configure LED and solenoid pins as outputs
  pinMode(BUILTIN_LED_PIN, OUTPUT);
  digitalWrite(BUILTIN_LED_PIN, HIGH); // LED off (built-in LED is active LOW)

  pinMode(SOLENOID_PIN, OUTPUT);
  digitalWrite(SOLENOID_PIN, LOW); // Solenoid off (assuming active HIGH)

  // Calculate initial beat interval (60000 ms per minute / BPM)
  beatInterval = 60000 / Tempo;

  // Configure encoder pins as inputs with pullups
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);

  // Read initial state
  lastEncoded = (digitalRead(ENCODER_CLK) << 1) | digitalRead(ENCODER_DT);

  // Attach interrupts to both encoder pins
  // ESP8266 can use interrupts on most GPIO pins
  attachInterrupt(digitalPinToInterrupt(ENCODER_CLK), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_DT), encoderISR, CHANGE);

  // Attach interrupt to button pin (trigger on any change - press and release)
  attachInterrupt(digitalPinToInterrupt(ENCODER_SW), buttonISR, CHANGE);

  Serial.println("Rotary Encoder Ready (with button interrupt - hardware debounced)");
  Serial.print("Tempo: ");
  Serial.print(Tempo);
  Serial.println(" BPM");
}

void loop() {
  static long lastPosition = 0;
  unsigned long currentTime = millis();

  // Process encoder if flag is set
  if (encoderFlag) {
    noInterrupts();
    byte currentState = encoderState;
    encoderFlag = false;
    interrupts();

    processEncoder(currentState);
  }

  // Process button if flag is set
  if (buttonFlag) {
    buttonFlag = false;
    onButtonPressed();
  }

  // Check if position changed
  if (encoderPosition != lastPosition) {
    Serial.print("Position: ");
    Serial.print(encoderPosition);  // Print position
    Serial.print(" (");
    if (encoderPosition > lastPosition) {
      Serial.println("CW)");
    } else {
      Serial.println("CCW)");
    }

    // Update beat interval when tempo changes
    beatInterval = 60000 / Tempo;
    lastPosition = encoderPosition;
  }

  // Metronome logic
  // Check if it's time for a new beat
  if (currentTime - lastBeatTime >= beatInterval) {
    // Read pedal voltage and calculate pulse width
    pedal_mV = readPedalVoltage();
    if (pedalEnable){
      PW = calculatePulseWidth(pedal_mV);
    }

    // Turn on LED (always) and solenoid (only if pulses enabled)
    digitalWrite(BUILTIN_LED_PIN, LOW); // Turn LED on (active LOW)
    if (isPulsesEnabled) {
      //digitalWrite(SOLENOID_PIN, HIGH); // Turn solenoid on (assuming active HIGH)
      digitalWrite(SOLENOID_PIN, LOW); // Turn solenoid on (assuming active HIGH)
    }
    ledState = true;
    ledOnTime = micros(); // Use micros for microsecond precision
    lastBeatTime = currentTime;

    // Debug output
    Serial.print("Pedal: ");
    Serial.print(pedal_mV);
    Serial.print("mV, PW: ");
    Serial.print(PW);
    Serial.println("μs");
  }

  // Turn off LED and solenoid after PW microseconds
  if (ledState && (micros() - ledOnTime >= PW)) {
    digitalWrite(BUILTIN_LED_PIN, HIGH); // Turn LED off (active LOW)
    if (isPulsesEnabled) {
      //digitalWrite(SOLENOID_PIN, LOW); // Turn solenoid off
      digitalWrite(SOLENOID_PIN, HIGH); // Turn solenoid off
    }
    ledState = false;
  }

 // delay(1);
}

// Minimal ISR - just capture state and set flag
// ICACHE_RAM_ATTR ensures ISR is stored in RAM for faster execution
void ICACHE_RAM_ATTR encoderISR() {
  encoderState = (digitalRead(ENCODER_CLK) << 1) | digitalRead(ENCODER_DT);
  encoderFlag = true;
}

// Button ISR - track press and release times (hardware debounced)
void ICACHE_RAM_ATTR buttonISR() {
  if (digitalRead(ENCODER_SW) == LOW) {
    // Button pressed down
    buttonPressTime = millis();
    buttonCurrentlyPressed = true;
  } else {
    // Button released - set flag for processing
    buttonCurrentlyPressed = false;
    buttonFlag = true;
  }
}

// Function called when button is released - handles short/long press
void onButtonPressed() {
  unsigned long pressDuration = millis() - buttonPressTime;

  if (pressDuration >= LONG_PRESS_THRESHOLD) {
    // Long press: toggle pulses (silence/enable drum)
    isPulsesEnabled = !isPulsesEnabled;
    Serial.print("Long press - Pulses ");
    Serial.println(isPulsesEnabled ? "ENABLED" : "DISABLED");
  } else {
    // Short press:
    if (!isPulsesEnabled) {
      // If currently silent, re-enable pulses (keep pedalEnable state)
      isPulsesEnabled = true;
      Serial.println("Short press - Pulses RE-ENABLED");
    } else {
      // If pulses enabled, toggle pedal control (original behavior)
      pedalEnable = !pedalEnable;
      Serial.print("Short press - Pedal control ");
      Serial.println(pedalEnable ? "ENABLED" : "DISABLED (hold last PW)");
    }
  }
}

// Process encoder state in main loop - 1 count per mechanical detent
void processEncoder(byte encoded) {
  // Only count when we reach the detent position (both pins HIGH = 0b11)
  // and we came from a different state
  if (encoded == 0b11 && lastEncoded != 0b11) {
    // Determine direction based on which pin went high last
    // If CLK (bit 1) was the last to go high, it's clockwise
    // If DT (bit 0) was the last to go high, it's counter-clockwise

    // Check previous state to determine direction:
    // 0b01 -> 0b11 means CLK went high (counter-clockwise)
    // 0b10 -> 0b11 means DT went high (clockwise)
    if (lastEncoded == 0b01 && encoderPosition > minTempo) {
      Tempo = --encoderPosition;  // CLK went high last = counter-clockwise
    } else if (lastEncoded == 0b10 and encoderPosition < maxTempo) {
      Tempo = ++encoderPosition;  // DT went high last = clockwise
    }
  }

  lastEncoded = encoded;
}

// Read pedal voltage from ADS1115 differential input
float readPedalVoltage() {
  int16_t adc = ads.getLastConversionResults();
  // Convert to millivolts: ADC value * 0.125mV per bit (for gain 1)
  return adc * 0.125;
}

// Calculate pulse width based on pedal voltage (linear mapping)
int calculatePulseWidth(float pedal_mV) {
  // Constrain input to expected range
  pedal_mV = constrain(pedal_mV, minPedal_mV, maxPedal_mV);

  // Linear mapping: higher voltage = shorter pulse
  // map(value, fromLow, fromHigh, toLow, toHigh)
  // Note: we reverse the mapping since higher voltage gives shorter pulse
  return map(pedal_mV, minPedal_mV, maxPedal_mV, maxPulseMicroseconds, minPulseMicroseconds);
}
