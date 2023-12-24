#include <math.h>

/* Function to linearly interpolate between a0 and a1
 * Weight w should be in the range [0.0, 1.0]
 */
float interpolate(float a0, float a1, float w) {
    // You may want clamping by inserting:
    if (0.0 > w) return a0;
    if (1.0 < w) return a1;

    return (a1 - a0) * w + a0;
    /* // Use this cubic interpolation [[Smoothstep]] instead, for a smooth appearance:
     * return (a1 - a0) * (3.0 - w * 2.0) * w * w + a0;
     *
     * // Use [[Smootherstep]] for an even smoother result with a second derivative equal to zero on boundaries:
     * return (a1 - a0) * ((w * (w * 6.0 - 15.0) + 10.0) * w * w * w) + a0;
     */
}

typedef struct {
    float x, y;
} vector2;

/* Create pseudorandom direction vector
 */
vector2 randomGradient(int ix, int iy) {
    // No precomputed gradients mean this works for any number of grid coordinates
    const unsigned w = 8 * sizeof(unsigned);
    const unsigned s = w / 2; // rotation width
    unsigned a = ix, b = iy;
    a *= 3284157443; b ^= a << s | a >> w-s;
    b *= 1911520717; a ^= b << s | b >> w-s;
    a *= 2048419325;
    float random = a * (3.14159265 / ~(~0u >> 1)); // in [0, 2*Pi]
    vector2 v;
    v.x = cos(random); v.y = sin(random); // TODO:precompute screen matrix sized buffer of cos and sin values, expensive!
    return v;
}

// Computes the dot product of the distance and gradient vectors.
float dotGridGradient(int ix, int iy, float x, float y) {
    // Get gradient from integer coordinates
    vector2 gradient = randomGradient(ix, iy);

    // Compute the distance vector
    float dx = x - (float)ix;
    float dy = y - (float)iy;

    // Compute the dot-product
    return (dx*gradient.x + dy*gradient.y);
}

// TODO: make perlin 3D for time varying output
// Compute Perlin noise at coordinates x, y
float perlin(float x, float y) {
    // Determine grid cell coordinates
    int x0 = (int)floor(x);
    int x1 = x0 + 1;
    int y0 = (int)floor(y);
    int y1 = y0 + 1;

    // Determine interpolation weights
    // Could also use higher order polynomial/s-curve here
    float sx = x - (float)x0;
    float sy = y - (float)y0;

    // Interpolate between grid point gradients
    float n0, n1, ix0, ix1, value;

    n0 = dotGridGradient(x0, y0, x, y);
    n1 = dotGridGradient(x1, y0, x, y);
    ix0 = interpolate(n0, n1, sx);

    n0 = dotGridGradient(x0, y1, x, y);
    n1 = dotGridGradient(x1, y1, x, y);
    ix1 = interpolate(n0, n1, sx);

    value = interpolate(ix0, ix1, sy);
    return value; // Will return in range -1 to 1. To make it in range 0 to 1, multiply by 0.5 and add 0.5
}

// vim: sw=2
// // // // // // // // // // // // // // // // // // // //
// // // // // // // // // // // // // // // // // // // //
//
// LED Perlin
//
// // // // // // // // // // // // // // // // // // // //
// // // // // // // // // // // // // // // // // // // //

#define WIDTH 16
#define HEIGHT 16
#define TIME 1  // TODO: experiment how much memory esp got

// FastLED Config
#define LED_DATA_PIN 2 // TODO: change me
#define LED_ITEMS WIDTH * HEIGHT * TIME
#define LED_TYPE WS2812B // TODO: change me
#define LED_COLOR_ORDER RGB
#define LED_BRIGHTNESS 16 // 0 - 255
#define LED_CORRECTION HighNoonSun

CRGB leds[LED_ITEMS]; // Global LEDs array
int LOOP_COUNT;
vector2 noise[TIME][HEIGHT][WIDTH];

// TODO: change me (maybe?)
void setupSerial() {
  Serial.begin(SERIAL_BAUD, SERIAL_8N1);
  delay(2000);
  Serial.println();
  Serial.printf("[SERL] Serial initiated (%d baud)\n", SERIAL_BAUD);
}

void turnOffLeds() { 
  for (int i = 0; i < LED_ITEMS; i++) { leds[i] = CRGB::Black; }
  FastLED.show();
}

void setupFastLed() {
  Serial.println("[LEDS] Initiating LEDs...");

  FastLED.addLeds<LED_TYPE, LED_DATA_PIN, LED_COLOR_ORDER>(leds, LED_ITEMS);
  FastLED.setCorrection(LED_CORRECTION);
  FastLED.setBrightness(ledBrightness);

  turnOffLeds();
  FastLED.show();

  Serial.println("[LEDS] Initiated LEDs");
}

void setup() {
    setupSerial();
    setupFastLed();

    LOOP_COUNT = 0;
    noise = { 0.0 };

    // precompute 3D volume of perlin noise ,
    // one dimension for time and the othersf

    for (int t = 0; t < TIME; ++t) {

        // precompute screen at frame t

        if (t % 10) {
            Serial.printf("[INFO] Precomputing: %f%\n", t / TIME);
        }

        for (int ix = 0; ix < HEIGHT; ++ix) {
            for (int iy = 0; iy < WIDTH; ++iy) {
                noise[t][ix][iy] = perlin((float)ix, (float)iy);
            }
        }
    }

    Serial.printf("[INFO] Precomputing: done\n");
}

void loop() {

    int t = LOOP_COUNT % TIME;

    Serial.printf("[INFO] Iteration: %d % %d = %d\n", LOOP_COUNT, TIME, t);

    for (int ix = 0; ix < HEIGHT; ++ix) {

        Serial.printf("[DATA] ");

        for (int iy = 0; iy < WIDTH; ++iy) {
            // primitively map float to rgb 
            // (TODO: make map to a visually more pleasing spectrum)
            bool hilo = noise[t][ix][iy] > 0.5;
            leds[ix * WIDTH + iy] = hilo ? CRGB::White : CRGB::Black;
            Serial.printf("%s", hilo ? "1" : "0");
        }

        Serial.printf("\n");
    }
    
    Serial.printf("\n");

    FastLED.(show);
    delay(10000);
    LOOP_COUNT++;
}
