// Experiment Light Control
// Mark Bucklin
// March 4, 2018

#include <Arduino.h>
#include <DigitalIO.h>
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

// TRIGGER INPUT & TIMING
const int TRIGGER_INPUT_PIN = 0;
const uint32_t frameTimeoutMicros = 2 * 1000000;
elapsedMicros microsSinceAcquisitionStart;
elapsedMicros microsSinceFrameStart;
volatile time_t currentFrameTimestamp;
// volatile time_t currentFrameDuration;
volatile uint32_t currentFrameCount;

// STATUS
volatile bool isRunning = false;

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

// ETL ETL ETL  ETL  ETL  ETL  ETL  ETL  ETL
#include <queue.h>
typedef struct {
  uint32_t count;
  uint64_t timestamp;
  time_t duration;
  radial_light_source_t data;
  bool filled;
} triggered_frame_t;

typedef etl::queue<triggered_frame_t, 8> frame_queue_t;
frame_queue_t frameDataQueue = {};
triggered_frame_t &currentFrameData = frameDataQueue.front();

// =============================================================================
// Function Declarations
// =============================================================================
static inline void beginAcquisition(void);
static inline void beginDataFrame(void);
const triggered_frame_t &fillDataFrame(const radial_light_source_t &data);
static inline void endDataFrame(void);
static inline void endAcquisition(void);
static void sendHeader();
static void sendData();
void handleTriggerRisingEdge(void);
void handleTriggerFallingEdge(void);
static inline void handleFrameTimeout(void);
bool updateLightSource(radial_light_source_t *);
static inline bool wrap(auto *unwrappedInput, const auto wrapLimit);
inline float normalizeEncoderPosition(const auto encoderCount);
static inline void toggleColor(color_t *);

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

  // LED Setup
  ledStrip.begin();
  delay(10);

  // Trigger Input
  pinMode(TRIGGER_INPUT_PIN, INPUT);
  isRunning = false;
  attachInterrupt(TRIGGER_INPUT_PIN, handleTriggerRisingEdge, RISING);
}

void loop() {
  handleFrameTimeout();

  updateLightSource(&lightSource);

  // Keep Filling currentFrameData with most recent State of light source
  if (currentFrameData.filled != true) {
    currentFrameData = fillDataFrame(lightSource);
  }
}

// =============================================================================
// Start & Stop Acquisition Functions (called in trigger input interrupts)
// =============================================================================
static inline void beginAcquisition(void) {
  sendHeader();
  isRunning = true;

  // Clear Frame-Data Queue
  frameDataQueue.clear();

  // Initialize Session & Frame Timers & Frame Counter
  currentFrameCount = 0;
  microsSinceAcquisitionStart = 0;
  microsSinceFrameStart = microsSinceAcquisitionStart;
  // currentFrameDuration = microsSinceFrameStart;
  beginDataFrame();
  attachInterrupt(TRIGGER_INPUT_PIN, handleTriggerFallingEdge, FALLING);
}
static inline void beginDataFrame(void) {
  // Latch timestamp and designate/allocate current sample
  currentFrameTimestamp = microsSinceAcquisitionStart;
  // Gapless update of time-since-frame-start from prior duration
  // microsSinceFrameStart -= currentFrameDuration;
  microsSinceFrameStart = microsSinceAcquisitionStart - currentFrameTimestamp;
  // Increment Frame Counter
  currentFrameCount += 1;

  // Push/emplace empty first frame into queue and get reference
  currentFrameData = frameDataQueue.push();

  // Fill with data thats available at beginning of data-frame
  currentFrameData.count = currentFrameCount;
  currentFrameData.timestamp = currentFrameTimestamp;
}
const triggered_frame_t &fillDataFrame(const radial_light_source_t &data) {
  memcpy(&(currentFrameData.data), &data, sizeof(data));
  currentFrameData.filled = true;
  return currentFrameData;
}
static inline void endDataFrame(void) {
  // Latch Frame Duration and Send Data
  // currentFrameDuration = microsSinceFrameStart;
  currentFrameData.duration = microsSinceFrameStart;
}
static inline void endAcquisition(void) {
  // Change running state and reset trigger interrupt to rising edge
  isRunning = false;
  attachInterrupt(TRIGGER_INPUT_PIN, handleTriggerRisingEdge, RISING);
}

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

  // todo-------------here  frameDataQueue.pop()
  // framedata.count, framedata.timestamp, framedata.duration
  // framedata.data

  // Convert to String class
  const String timestamp = currentFrameTimestamp;
  const String angle = String(lightSource.angle);
  const String width = String(lightSource.width);
  const String color = String(lightSource.color, HEX);
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
  // Handle first rising edge of first trigger
  // (interrupt only active if isRunning = false)
  if (!isRunning) {
    beginAcquisition();
  }
}

void handleTriggerFallingEdge(void) {
  if (isRunning) {
    endDataFrame();
    sendData();
    beginDataFrame();
  }
}
static inline void handleFrameTimeout(void) {
  // Acquisition Timeout
  if (isRunning) {
    if (microsSinceFrameStart > frameTimeoutMicros) {
      endAcquisition();
    }
  }
}

// =============================================================================
// Help Functions for Rotary Encoder and LEDs
// =============================================================================
bool updateLightSource(radial_light_source_t *lightSourcePointer) {
  // create reference to light-source for code clarity
  radial_light_source_t &lightSourceRef = *lightSourcePointer;

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
  lightSourceRef.angle = normalizeEncoderPosition(newPosition);
  auto sourceLeftBound = lightSourceRef.angle - lightSourceRef.width / 2;
  auto sourceRightBound = lightSourceRef.angle + lightSourceRef.width / 2;
  auto ledLeftBound = sourceLeftBound * LED_COUNT;
  auto ledRightBound = sourceRightBound * LED_COUNT;

  static elapsedMillis millisSinceToggle = 0;
  if (fastDigitalRead(ENCODER_PIN_BUTTON) == LOW) {
    if (millisSinceToggle > 150) {
      toggleColor(&(lightSourceRef.color));
      millisSinceToggle = 0;
    }
  }

  if (fastDigitalRead(ONOFF_BUTTON_PIN) == LOW) {
    lightSourceRef.on = true;
  } else {
    lightSourceRef.on = false;
  }

  // Update ledStrip
  if (lightSourceRef.on) {
    // Most common case -> not wrapping around ends of strip
    auto *pxLower = &ledLeftBound;
    auto *pxUpper = &ledRightBound;
    wrap(pxUpper, LED_COUNT / 2);
    wrap(pxLower, LED_COUNT / 2);
    auto inBtwVal = lightSourceRef.color;
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

  // Update LEDs and return current light-source specification
  ledStrip.show();
  return true;
}

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

static inline void toggleColor(color_t *colorPtr) {
  static int currentColorIndex = 0;
  currentColorIndex = (currentColorIndex + 1) % numColorOptions;
  *colorPtr = colorOption[currentColorIndex];
}
