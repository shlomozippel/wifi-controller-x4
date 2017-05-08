#define FASTLED_INTERRUPT_RETRY_COUNT 0
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include <FastLED.h>
#include <EEPROM.h>
#include "button.h"
#include "patterns.h"


#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

//---------------------------------------------------------------------------------------------------
// LED Strips
//---------------------------------------------------------------------------------------------------

#define NUM_STRIPS 4
#define NUM_LEDS  150
#define FRAMERATE 120
#define DEFAULT_BRIGHTNESS  200;

// Fast LED pins are RAW Pins, not d1 pins
#define CLOCK_PIN 16
#define LEDS_PIN1 12
#define LEDS_PIN2 13
#define LEDS_PIN3 14
#define LEDS_PIN4 15

CRGB led_strips[NUM_STRIPS][NUM_LEDS];

//---------------------------------------------------------------------------------------------------
// Rotenc stuff
//---------------------------------------------------------------------------------------------------

#define ROTENC_A      D1
#define ROTENC_B      D2
#define ROTENC_SW_OUT D3
#define ROTENC_SW_IN  D4

static volatile int32_t g_rotenc = 0;
static bool g_rotenc_clamped = false;
static int g_rotenc_min = 0;
static int g_rotenc_max = 0;

Button sw(ROTENC_SW_IN);

int8_t read_encoder()
{
  static int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  static uint8_t old_AB = 0;
  
  old_AB <<= 2; 
  old_AB |= digitalRead(ROTENC_B) << 1 | digitalRead(ROTENC_A);
  return ( enc_states[( old_AB & 0x0f )]);
}

void rotenc_isr() {
  g_rotenc += read_encoder();
}

void rotenc_reset(int new_value=0) {
    g_rotenc = new_value * 2;
}

int rotenc_raw() {
    return g_rotenc >> 1;
}

int rotenc() {
    int ret = rotenc_raw();
    if (g_rotenc_clamped) {
        if (ret < g_rotenc_min) ret = g_rotenc_min;
        if (ret > g_rotenc_max) ret = g_rotenc_max;
        rotenc_reset(ret);
    }
    return ret;
}

void rotenc_unclamped(int current_value = 0) {
    g_rotenc_clamped = false;
    rotenc_reset(current_value);
}

void rotenc_clamped(int current_value, int min, int max) {
   g_rotenc_clamped = true;
   g_rotenc_min = min;
   g_rotenc_max = max;
   rotenc_reset(current_value);
}

//---------------------------------------------------------------------------------------------------
// Pattern state management
//---------------------------------------------------------------------------------------------------

enum MAIN_STATE {
  STATE_RENDER,
  STATE_CHOOSE_STRIP,
  STATE_CHOOSE_PATTERN,
};
uint8_t state = STATE_RENDER;
uint8_t strip_being_edited = 0;

typedef int (*SimplePatternList[])(CRGB* leds, uint32_t num_leds);
uint8_t g_current_pattern[NUM_STRIPS] = {0};
uint8_t brightness;

SimplePatternList patterns = {
  //collision,
  rainbow,
  moving_palette,
  confetti,
  mode_yalda,
};

void leave_edit_mode() {
  state = STATE_RENDER;
  rotenc_clamped(brightness, 0, 255);
  save_config();
}

void button_press() {
  switch (state) {
    case STATE_RENDER: break;
    case STATE_CHOOSE_STRIP:
      state = STATE_CHOOSE_PATTERN;
      rotenc_unclamped(0);
      break;
    case STATE_CHOOSE_PATTERN:
      leave_edit_mode();
      break;
  }
}

void button_hold() {
  switch (state) {
    case STATE_RENDER: 
      state = STATE_CHOOSE_STRIP;
      rotenc_unclamped(0);
      break;
    case STATE_CHOOSE_STRIP:
    case STATE_CHOOSE_PATTERN:
      leave_edit_mode();
      break;
  }
}

//---------------------------------------------------------------------------------------------------
// Config
//---------------------------------------------------------------------------------------------------

void load_config() {
  brightness = EEPROM.read(0);
  for (int i=0; i<NUM_STRIPS; i++) {
    uint8_t val = EEPROM.read(1 + i);
    if (val > ARRAY_SIZE( patterns )) val = 0;
    g_current_pattern[i] = val;
  }
}

void save_config() {
  EEPROM.write(0, brightness);
  for (int i=0; i<NUM_STRIPS; i++) {
    EEPROM.write(1 + i, g_current_pattern[i]);
  }
  EEPROM.commit();
}

//---------------------------------------------------------------------------------------------------
// Setup & Loop
//---------------------------------------------------------------------------------------------------

void setup() {

  Serial.begin(115200);
  EEPROM.begin(256);
 
  // rotenc
  pinMode(ROTENC_A, INPUT_PULLUP);
  pinMode(ROTENC_B, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ROTENC_A), rotenc_isr, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTENC_B), rotenc_isr, CHANGE);

  // switch
  pinMode(ROTENC_SW_OUT, OUTPUT);
  digitalWrite(ROTENC_SW_OUT, LOW);

//  FastLED.addLeds<WS2812, LEDS_PIN1>(led_strips[0], NUM_LEDS).setCorrection(TypicalLEDStrip);
//  FastLED.addLeds<WS2812, LEDS_PIN2>(led_strips[1], NUM_LEDS).setCorrection(TypicalLEDStrip);
//  FastLED.addLeds<WS2812, LEDS_PIN3>(led_strips[2], NUM_LEDS).setCorrection(TypicalLEDStrip);
//  FastLED.addLeds<WS2812, LEDS_PIN4>(led_strips[3], NUM_LEDS).setCorrection(TypicalLEDStrip);

//  FastLED.addLeds<APA102, LEDS_PIN1, CLOCK_PIN>(leds, NUM_LEDS, RGB).setCorrection(TypicalLEDStrip);
//  FastLED.addLeds<APA102, LEDS_PIN2, CLOCK_PIN>(leds, NUM_LEDS, RGB).setCorrection(TypicalLEDStrip);
//  FastLED.addLeds<APA102, LEDS_PIN3, CLOCK_PIN>(leds, NUM_LEDS, RGB).setCorrection(TypicalLEDStrip);
//  FastLED.addLeds<APA102, LEDS_PIN4, CLOCK_PIN>(leds, NUM_LEDS, RGB).setCorrection(TypicalLEDStrip);

  FastLED.addLeds<WS2811_PORTA, NUM_STRIPS>(led_strips[0], NUM_LEDS).setCorrection(TypicalLEDStrip);
  

  state = STATE_RENDER;
  load_config();

  rotenc_clamped(brightness, 0, 255);
}

void loop() {
  // button press / state transitions
  sw.poll(button_press, button_hold);

  // changen pattern
  if (STATE_CHOOSE_PATTERN == state) {
    g_current_pattern[strip_being_edited] = rotenc() % ARRAY_SIZE(patterns);
  }

  // render
  for (int i=0; i<NUM_STRIPS; i++) {
    patterns[g_current_pattern[i]](led_strips[i], NUM_LEDS);
  }

  // blink the edited strip if we're choosing a strip to edit
  if (STATE_CHOOSE_STRIP == state) {
    strip_being_edited = rotenc() % NUM_STRIPS;
    if ((millis() / 600) & 1) {
       fill_solid(led_strips[strip_being_edited], NUM_LEDS, CRGB::Black);
    }
  }

  // fade all non-edited strips if we're editing
  if (STATE_RENDER != state) { 
    for (int i=0; i<NUM_STRIPS; i++) {
      if (i != strip_being_edited) {
        fade_video(led_strips[i], NUM_LEDS, 150);
      }
    }    
  } else {
    brightness = rotenc();
    FastLED.setBrightness(brightness);
  }
  
  FastLED.delay(1000/FRAMERATE);
  FastLED.countFPS();

  EVERY_N_SECONDS(1) {
      Serial.println(FastLED.getFPS());
  }
}
