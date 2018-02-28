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
WS2812Serial ledStrip(LED_COUNT, displayMemory, drawingMemory, LED_PIN,
                      WS2812_GRB);

#define RED 0xFF0000
#define GREEN 0x00FF00
#define BLUE 0x0000FF
#define YELLOW 0xFFFF00
#define PINK 0xFF1088
#define ORANGE 0xE05800
#define WHITE 0xFFFFF
const uint32_t colorOption[] = {RED, GREEN, BLUE, YELLOW, PINK, ORANGE, WHITE};
// TODO
typedef uint32_t color_t;
typedef struct {
  const WS2812Serial &strip;
  bool on;
} led_control_t;
led_control_t ledControl = {ledStrip, true};

typedef struct {
  float angle;
  float width;
  color_t color;
} radial_light_source_t;
radial_light_source_t lightSource = {0, 0.2, RED};

// Function Declarations
inline bool wrap(auto *unwrappedInput, const auto wrapLimit);
inline float normalizeEncoderPosition(const auto encoderCount);
void handleEncoderButtonDown(void);
void handleEncoderButtonUp(void);

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
  attachInterrupt(ENCODER_PIN_BUTTON, handleEncoderButtonDown, FALLING);
  attachInterrupt(ENCODER_PIN_BUTTON, handleEncoderButtonUp, RISING);

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
    // Serial.println(normalizeEncoderPosition(newPosition), 3);
  }
  priorPosition = newPosition;

  // Calculate Updated Angle/Boundaries
  lightSource.angle = normalizeEncoderPosition(newPosition);
  auto sourceLeftBound = max(-0.5, lightSource.angle - lightSource.width / 2);
  auto sourceRightBound = min(0.5, lightSource.angle + lightSource.width / 2);
  auto ledLeftBound = map(sourceLeftBound, -1, 1, -LED_COUNT, LED_COUNT);
  auto ledRightBound = map(sourceRightBound, -1, 1, -LED_COUNT, LED_COUNT);

  // Update ledStrip
  for (int i = 0; i < ledStrip.numPixels(); i++) {
    auto pxPosition = i - LED_COUNT / 2 + 0.5;
    if (pxPosition >= ledLeftBound && pxPosition <= ledRightBound &&
        ledControl.on) {
      ledStrip.setPixel(i, lightSource.color);
    } else {
      ledStrip.setPixel(i, 0);
    }
  }
  Serial.print(lightSource.angle);
  Serial.print(sourceLeftBound);
  Serial.print(ledLeftBound);
  Serial.print(sourceRightBound);
  Serial.println(ledRightBound);
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

elapsedMicros downUpMicros;
void handleEncoderButtonDown(void) { downUpMicros = 0; }
void handleEncoderButtonUp(void) {
  if (downUpMicros < 100000) {
    // Toggle Led On State
    ledControl.on = !ledControl.on;
  } else {
    // Toggle Color
    static int currentColorIndex = 0;
    currentColorIndex = (currentColorIndex + 1) % 7;
    lightSource.color = colorOption[currentColorIndex];
  }
}