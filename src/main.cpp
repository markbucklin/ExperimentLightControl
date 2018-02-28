#include <Arduino.h>
#include <math.h>

// Control

#include <Encoder.h>
const int ENCODER_PIN_BUTTON = 16;
const int ENCODER_PIN_A = 18;
const int ENCODER_PIN_B = 17;
const int ENCODER_COUNTS_PER_REV = 80;
const int ENCODER_COUNTS_PER_DETENT = 4;
Encoder encoder(ENCODER_PIN_A, ENCODER_PIN_B);

// ON/OFF Button
const int ONOFF_BUTTON_PIN = 21;

// TRIGGER INPUT
const int TRIGGER_INPUT_PIN = 0;

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

// // Function Declarations
inline bool wrap(auto *unwrappedInput, const auto wrapLimit);
inline float normalizeEncoderPosition(const auto encoderCount);
void toggleColor(void);
void handleButtonDown(void);
void handleButtonUp(void);
void handleTriggerInput(void);

// Setup
void setup() {
  // delay(200);
  Serial.begin(115200);
  Serial.println("serialstarted");

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
  // attachInterrupt(ONOFF_BUTTON_PIN, handleButtonDown, FALLING);
  // attachInterrupt(ONOFF_BUTTON_PIN, handleButtonUp, RISING);

  // Trigger Input
  pinMode(TRIGGER_INPUT_PIN, INPUT);

  // LED Setup
  ledStrip.begin();
  delay(10);
}

void loop() {
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

inline bool wrap(auto *unwrappedInput, const auto wrapLimit) {
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

void toggleColor(void) {
  static int currentColorIndex = 0;
  currentColorIndex = (currentColorIndex + 1) % numColorOptions;
  lightSource.color = colorOption[currentColorIndex];
}

void handleButtonDown(void) {}
void handleButtonUp(void) {
  //
}

void handleTriggerInput() {
  static elapsedMicros usSinceLastTrigger = 0;
  if (usSinceLastTrigger > 1000000) {  // todo
    Serial.println(lightSource.angle);
  }
}
