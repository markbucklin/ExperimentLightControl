#include <Arduino.h>
#include <math.h>

// Control
#include <Encoder.h>
const int ENCODER_PIN_BUTTON = 16;
const int ENCODER_PIN_A = 18;
const int ENCODER_PIN_B = 17;
const int ENCODER_COUNTS_PER_REV = 80;
Encoder encoder(ENCODER_PIN_A, ENCODER_PIN_B);

// ON/OFF Button
const int ONOFF_BUTTON_PIN = 21;

// TRIGGER INPUT
const int TRIGGER_INPUT_PIN = 0;
volatile bool isRunning = false;
elapsedMicros usSinceStart;
volatile uint32_t startTimeMicros;

// LED
#include <WS2812Serial.h>
const int LED_COUNT = 64;
const int LED_PIN = 10;
byte drawingMemory[LED_COUNT * 3];          //  3 bytes per LED
DMAMEM byte displayMemory[LED_COUNT * 12];  // 12 bytes per LED
WS2812Serial ledStrip(LED_COUNT, displayMemory, drawingMemory, LED_PIN,
                      WS2812_GRB);

#define RED 0xFF0000
#define GREEN 0x00FF00
#define BLUE 0x0000FF
#define YELLOW 0xFFFF00
#define PINK 0xFF1088
#define ORANGE 0xE05800
#define WHITE 0xFFFFF
const int numColorOptions = 7;
const uint32_t colorOption[] = {RED, GREEN, BLUE, YELLOW, PINK, ORANGE, WHITE};
typedef uint32_t color_t;  // TODO

typedef struct {
  float angle;
  float width;
  color_t color;
  volatile bool on;
} radial_light_source_t;
radial_light_source_t lightSource = {0, 0.2, RED, true};

// =============================================================================
// Function Declarations
// =============================================================================
static inline bool wrap(auto *unwrappedInput, const auto wrapLimit);
inline float normalizeEncoderPosition(const auto encoderCount);
static inline void toggleColor(void);
void handleTriggerRisingEdge(void);
void handleTriggerFallingEdge(void);
static inline void startAcquisition(void);
static inline void stopAcquisition(void);
static void sendHeader();
static void sendData();

// =============================================================================
// Setup & Loop
// =============================================================================
void setup() {
  // delay(200);
  Serial.begin(115200);

  // Encoder Pins
  pinMode(15, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(ENCODER_PIN_BUTTON, INPUT_PULLUP);
  digitalWrite(15, HIGH);
  digitalWrite(14, LOW);

  // On/Off Button
  pinMode(ONOFF_BUTTON_PIN - 1, OUTPUT);  // gnd
  digitalWrite(ONOFF_BUTTON_PIN - 1, LOW);
  pinMode(ONOFF_BUTTON_PIN, INPUT_PULLUP);

  // Trigger Input
  pinMode(TRIGGER_INPUT_PIN, INPUT);
  attachInterrupt(TRIGGER_INPUT_PIN, handleTriggerRisingEdge, RISING);

  // LED Setup
  ledStrip.begin();
  delay(10);
}

void loop() {
  elapsedMicros usLoop = 0;
  // if (!isRunning) {
  //   return;
  // };

  // Read Encoder
  static long priorPosition = 0;
  long newPosition = encoder.read();
  if (newPosition != priorPosition) {
    if (wrap(&newPosition, ENCODER_COUNTS_PER_REV / 2)) {
      encoder.write(newPosition);
    }
  }
  priorPosition = newPosition;

  // Calculate Updated Angle/Boundaries
  lightSource.angle = normalizeEncoderPosition(newPosition);
  auto sourceLeftBound = lightSource.angle - lightSource.width / 2;
  auto sourceRightBound = lightSource.angle + lightSource.width / 2;
  auto ledLeftBound = sourceLeftBound * LED_COUNT;
  auto ledRightBound = sourceRightBound * LED_COUNT;

  static elapsedMillis millisSinceToggle = 0;
  if (digitalRead(ENCODER_PIN_BUTTON) == LOW) {
    if (millisSinceToggle > 150) {
      toggleColor();
      millisSinceToggle = 0;
    }
  }

  if (digitalRead(ONOFF_BUTTON_PIN) == LOW) {
    lightSource.on = true;
  } else {
    lightSource.on = false;
  }

  // Update ledStrip
  if (lightSource.on) {
    // Most common case -> not wrapping around ends of strip
    auto *pxLower = &ledLeftBound;
    auto *pxUpper = &ledRightBound;
    wrap(pxUpper, LED_COUNT / 2);
    wrap(pxLower, LED_COUNT / 2);
    auto inBtwVal = lightSource.color;
    auto outBtwVal = 0;
    if ((*pxLower) > (*pxUpper)) {
      // Switch upper and lower limit
      pxUpper = &ledLeftBound;
      pxLower = &ledRightBound;
      outBtwVal = inBtwVal;
      inBtwVal = 0;
    }
    for (int i = 0; i < ledStrip.numPixels(); i++) {
      auto pxPosition = i - LED_COUNT / 2;
      // Check if pixel at current index [i] is in range around angle
      if (pxPosition >= *pxLower && pxPosition <= *pxUpper) {
        ledStrip.setPixel(i, inBtwVal);
      } else {
        ledStrip.setPixel(i, outBtwVal);
      }
    }
  } else {
    // All LEDs Off
    for (int i = 0; i < ledStrip.numPixels(); i++) {
      ledStrip.setPixel(i, 0);
    }
  }
  ledStrip.show();
}

// =============================================================================
// Start & Stop Acquisition Functions (called in trigger input interrupts)
// =============================================================================
static inline void startAcquisition(void) {
  usSinceStart = 0;
  startTimeMicros = usSinceStart;
  isRunning = true;
  sendHeader();
}
static inline void stopAcquisition(void) { isRunning = false; }  // todo

// =============================================================================
// TASKS: DATA_TRANSFER
// =============================================================================

const String delimiter = ',';
void sendHeader() {
  Serial.flush();
  Serial.print(String(String("timestamp [us]") + delimiter + "angle" +
                      delimiter + "width" + delimiter + "color" + delimiter +
                      "on" + "\n"));
}

void sendData() {
  // Send Current state of lightSource in a dataframe
  // float angle;
  // float width;
  // color_t color;
  // volatile bool on;

  // Convert to String class
  const String timestamp = (uint32_t)usSinceStart;
  const String angle = String(lightSource.angle);
  const String width = String(lightSource.width);
  const String color = String(lightSource.color);
  const String on = String(lightSource.on);
  const String endline = String("\n");

  // Print ASCII Strings
  Serial.print(timestamp + delimiter + angle + delimiter + width + delimiter +
               color + delimiter + on + endline);
}

// =============================================================================
// Input Trigger Interrupt functions
// =============================================================================
void handleTriggerRisingEdge(void) {
  static elapsedMillis msSinceLastTrigger = 0;
  if (!isRunning || (msSinceLastTrigger > 2000)) {
    startAcquisition();
  }
  msSinceLastTrigger = 0;
  attachInterrupt(TRIGGER_INPUT_PIN, handleTriggerFallingEdge, FALLING);
}

void handleTriggerFallingEdge(void) {
  if (isRunning) {
    sendData();
  }
}  // todo

// =============================================================================
// Help Functions for Rotary Encoder and LEDs
// =============================================================================
static inline bool wrap(auto *unwrappedInput, const auto wrapLimit) {
  // wraps input to within range of +/- wrapLimit
  bool isChanged = false;
  while (abs(*unwrappedInput) > wrapLimit) {
    *unwrappedInput -= copysign(2 * wrapLimit, *unwrappedInput);
    isChanged = true;
  }
  return isChanged;
}

inline float normalizeEncoderPosition(const auto encoderCount) {
  // maps rotary encoder count to normalized position in rotation
  float normAngle = (float)encoderCount / ENCODER_COUNTS_PER_REV;
  return normAngle;
}

static inline void toggleColor(void) {
  static int currentColorIndex = 0;
  currentColorIndex = (currentColorIndex + 1) % numColorOptions;
  lightSource.color = colorOption[currentColorIndex];
}