
int rainbow(CRGB * leds, uint32_t num_leds) {
    static uint8_t hue = 0;
    fill_rainbow( leds, num_leds, ++hue, 7);    
    return 0;
}

int boa_rainbow(CRGB * leds, uint32_t num_leds) {
    // kinda hacky, assumes boa has 64 LEDs with 2 strips of 32 folded over
    static uint8_t hue = 0;
    fill_rainbow( leds, 32, hue, 7);    
    for (int i=0; i<32; i++) {
      leds[63-i] = leds[i];
    }
    hue++;
    return 0;
}

int confetti(CRGB * leds, uint32_t num_leds) {
    static uint8_t hue = 0;
    
    // random colored speckles that blink in and fade smoothly
    fadeToBlackBy( leds, num_leds, 7);
    if (random8() > 100) {
      int pos = random8(num_leds);
      leds[pos] += CHSV( hue + random8(64), 200, 255);
    }

    hue++;
    return 0;
}

int sinelon(CRGB * leds, uint32_t num_leds) {
    static uint8_t hue = 0;

    // a colored dot sweeping back and forth, with fading trails
    fadeToBlackBy( leds, num_leds, 20);
    int pos = beatsin16(11,0,num_leds);
    leds[pos] |= CHSV( hue, 255, 255);
    pos = beatsin16(13,0,num_leds);
    leds[pos] |= CHSV( hue + 128, 255, 255);
    
    hue++;
    return 0;
} 

#define PALETTE_CYCLE_TIME 2500

//--------------------------------------------------------------------------------------------------
// palettes
//--------------------------------------------------------------------------------------------------

DEFINE_GRADIENT_PALETTE( Mode_Yalda_gp ) {
   0,   0,  0, 0,
   40, 0x3B, 0, 0x72,
   80,   0,  0, 0,
  120,  0x19, 0x29, 0x70,
   160,   0,  0, 0,
  200,  0, 0x8B, 0x7B,
  255, 0, 0, 0
  };


DEFINE_GRADIENT_PALETTE( Green_Purple_gp ) {
  0, 0, 0, 0,
  32, 0, 127, 0,
  64, 0, 0, 0,
  96, 127, 0, 127,
  128, 0, 0, 0,
  160, 0, 127, 0,
  192, 0, 0, 0,
  224, 127, 0, 127,
  255, 0, 0, 0,
};

DEFINE_GRADIENT_PALETTE( Orange_Blue_gp ) {
  0, 0, 0, 0,
  32, 0, 0, 128,
  64, 0, 0, 0,
  96, 128, 128, 0,
  128, 0, 0, 0,
  160, 0, 0, 128,
  192, 0, 0, 0,
  224, 128, 128, 0,
  255, 0, 0, 0,
};


DEFINE_GRADIENT_PALETTE( Rainbow1_gp ) {
  0, 0, 0, 0,
  32,  255,  0,  0, // Red
  64, 0, 0, 0,
  96,  171, 85,  0, // Orange
  128, 0, 0, 0,
  160,  171,171,  0, // Yellow
  192, 0, 0, 0,
  224,  171, 85,  0, // Orange
  255, 0, 0, 0,
};


DEFINE_GRADIENT_PALETTE( Rainbow2_gp ) {
  0, 0, 0, 0,
  32, 0,  0xCE1, 0xD1, // Turquoise
  64, 0, 0, 0,
  96,    0,255,  0, // Green
  128, 0, 0, 0,
  160,    0,171, 85, // Aqua
  192, 0, 0, 0,
  224,    0,255,  0, // Green
  255, 0, 0, 0,
};


DEFINE_GRADIENT_PALETTE( Rainbow3_gp ) {
  0, 0, 0, 0,
  32,    0,171, 85, // Aqua
  64, 0, 0, 0,
  96,    0,  0,255, // Blue
  128, 0, 0, 0,
  160,   85,  0,171, // Purple
  192, 0, 0, 0,
  224,    0,  0,255, // Blue
  255, 0, 0, 0,
};


DEFINE_GRADIENT_PALETTE( Rainbow4_gp ) {
  0, 0, 0, 0,
  32,   85,  0,171, // Purple
  64, 0, 0, 0,
  96,  171,  0, 85, // Pink
  128, 0, 0, 0,
  160,  255,  0,  0, // Red
  192, 0, 0, 0,
  224,  171,  0, 85, // Pink
  255, 0, 0, 0,
};


DEFINE_GRADIENT_PALETTE( My_Rainbow_gp ) {
      0,  255,  0,  0, // Red
     32,  171,171,  0, // Yellow
     64,  0, 0, 0,
     96,    0,255,  0, // Green
    128,    0,171, 85, // Aqua
    160,    0,  0,255, // Blue
     192,  0, 0, 0,    
    224,   85,  0,171, // Purple
    255,  255,  0,  0 // and back to Red
};

const TProgmemRGBGradientPalettePtr g_palettes[] = {
    Green_Purple_gp,
    Orange_Blue_gp,
    Rainbow1_gp,
    Rainbow2_gp,
    Rainbow3_gp,
    Rainbow4_gp,
 };

const uint8_t g_palette_count = 
  sizeof( g_palettes) / sizeof( TProgmemRGBGradientPalettePtr );

void FillLEDsFromPaletteColors(CRGB * leds, uint32_t num_leds, CRGBPalette16 &palette, uint8_t colorIndex, uint8_t step) {    
    for( int i = 0; i < num_leds; i++, colorIndex+=step) {
        leds[i] = ColorFromPalette( palette, colorIndex, 255, LINEARBLEND);
    }
}

uint8_t g_current_palette_number = 0;
CRGBPalette16 g_current_palette( CRGB::Black);
CRGBPalette16 target_palette( CRGB::Black);
CRGBPalette16 *g_target_palette = &target_palette;
uint8_t g_palette_offset = 0;
uint32_t g_last_palette_time = 0;

int moving_palette(CRGB * leds, uint32_t num_leds) {
    if (millis() - g_last_palette_time > PALETTE_CYCLE_TIME) {
      g_current_palette_number = addmod8( g_current_palette_number, 1, g_palette_count);
      g_last_palette_time = millis();
    }
  
    g_palette_offset++;
    *g_target_palette = g_palettes[g_current_palette_number];
    nblendPaletteTowardPalette( g_current_palette, *g_target_palette, 24);
    FillLEDsFromPaletteColors(leds, num_leds, g_current_palette, g_palette_offset, 7);

    return 0;
}

int mode_yalda(CRGB * leds, uint32_t num_leds) {
    g_palette_offset++;
    g_current_palette = Mode_Yalda_gp;
    FillLEDsFromPaletteColors(leds, num_leds, g_current_palette, g_palette_offset, 9);

    return 0;
}

