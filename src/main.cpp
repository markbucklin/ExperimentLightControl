#include <Arduino.h>
#include <math.h>

// Control

#include <Encoder.h>
const int ENCODER_PIN_BUTTON = 16;
const int ENCODER_PIN_A = 18;
const int ENCODER_PIN_B = 17;
const int ENCODER_COUNTS_PER_REV = 80;
const int ENCODER_COUNTS_PER_DETENT = 4;
const int ENCODER_DETENTS_PER_REV =
    ENCODER_COUNTS_PER_REV / ENCODER_COUNTS_PER_DETENT;
Encoder encoder(ENCODER_PIN_A, ENCODER_PIN_B);

// LED
#include <WS2812Serial.h>
const int LED_COUNT = 64;
const int LED_PIN = 10;
byte drawingMemory[LED_COUNT * 3];          //  3 bytes per LED
DMAMEM byte displayMemory[LED_COUNT * 12];  // 12 bytes per LED
WS2812Serial leds(LED_COUNT, displayMemory, drawingMemory, LED_PIN, WS2812_GRB);

inline bool wrap(auto *unwrappedInput, const auto wrapLimit);
inline float normalizeEncoderPosition(const auto encoderCount);

#define RED 0xFF0000
#define GREEN 0x00FF00
#define BLUE 0x0000FF
#define YELLOW 0xFFFF00
#define PINK 0xFF1088
#define ORANGE 0xE05800
#define WHITE 0xFFFFFF
int currentColor = BLUE;
bool currentState[LED_COUNT];

// Setup
void setup() {
  delay(200);
  Serial.begin(115200);

  // Encoder Pins
  pinMode(14, OUTPUT);
  pinMode(15, OUTPUT);
  digitalWrite(14, LOW);
  digitalWrite(15, HIGH);
  pinMode(ENCODER_PIN_BUTTON, INPUT_PULLUP);

  // LED Setup
  leds.begin();
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
    Serial.println(normalizeEncoderPosition(newPosition), 3);
  }
  priorPosition = newPosition;

  // Update LEDs
  for (int i = 0; i < leds.numPixels(); i++) {
    if (currentState[i]) {
      leds.setPixel(i, currentColor);
    } else {
      leds.setPixel(i, 0);
    }
  }
  leds.show();
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
