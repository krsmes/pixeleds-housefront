/*
 * Project pixeleds-housefront
 * Description: Lighting up the front of the house with 660 addressable LEDs
 * Author: K Smith <krsjunk@netscape.net>
 * Created: 2022-08-10
 * Updated: 2024-12-18
 */

#include "Particle.h"
#include "pixeleds-library.h"
#include "pixeleds-colors.h"

#define PHOTO_SENSOR A0
#define PIXEL_PIN 0
#define PIXEL_COUNT 660  // house_front:660, light_bar:144, test:100
#define PIXEL_TYPE SK6812W
#define PIXEL_ORDER ORDER_GRBW

#define ANIMATION_FADE_IN_MS 30000
#define ANIMATION_CYLCLE_MS 345000
#define ANIMATION_FADE_MS 45000

SerialLogHandler logHandler(LOG_LEVEL_INFO);

Pixeleds px = Pixeleds(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE, PIXEL_ORDER); 

// color set 1
PixCol edgeColor1 = Color::BLACK;
PixCol mainColor1 = Color::BLACK;
PixCol windowColor1 = Color::MIDNIGHT_BLUE;
PixCol windowEdgeColor1 = Color::BLACK;
PixCol porchCenterColor1 = Color::ORANGE_RED;
PixCol porchEdgeColor1 = Color::BLACK;
// color set 2
PixCol edgeColor2 = Color::BLACK;
PixCol mainColor2 = Color::BLACK;
PixCol windowColor2 = Color::INDIGO;
PixCol windowEdgeColor2 = Color::BLACK;
PixCol porchCenterColor2 = Color::ORANGE_RED;
PixCol porchEdgeColor2 = Color::BLACK;

/* christmas 
// color set 1
PixCol edgeColor1 = Color::BLACK;
PixCol mainColor1 = PixCol::hsv(Hue::GREEN, 1.0, 0.1); // very dark
PixCol windowColor1 = PixCol::hsv(Hue::RED, 1.0, 0.1); // very dark
PixCol windowEdgeColor1 = Color::BLACK;
PixCol porchCenterColor1 = Color::ORANGE_RED;
PixCol porchEdgeColor1 = Color::BLACK;
// color set 2
PixCol edgeColor2 = Color::BLACK;
PixCol mainColor2 = PixCol::hsv(Hue::RED, 1.0, 0.1); // very dark
PixCol windowColor2 = PixCol::hsv(Hue::GREEN, 1.0, 0.1); // very dark
PixCol windowEdgeColor2 = Color::BLACK;
PixCol porchCenterColor2 = Color::ORANGE_RED;
PixCol porchEdgeColor2 = Color::BLACK;
*/

PixPal palette = PixPal::create({
    // FIRST SET
    edgeColor1,
    // left 5 feet
    mainColor1, mainColor1, mainColor1, mainColor1, mainColor1,
    // left window
    windowEdgeColor1, windowColor1, windowColor1, windowColor1, windowColor1, windowEdgeColor1,
    // 4 feet left window to door
    mainColor1, mainColor1, mainColor1, mainColor1,
    // door
    porchEdgeColor1, porchCenterColor1, porchCenterColor1, porchCenterColor1, porchCenterColor1, porchEdgeColor1,
    // 4 feet right window to door
    mainColor1, mainColor1, mainColor1, mainColor1,
    // right window
    windowEdgeColor1, windowColor1, windowColor1, windowColor1, windowColor1, windowEdgeColor1,
    // right 5 feet
    mainColor1, mainColor1, mainColor1, mainColor1, mainColor1,

    // SECOND SET
    edgeColor2,
    // left 5 feet
    mainColor2, mainColor2, mainColor2, mainColor2, mainColor2,
    // left window
    windowEdgeColor2, windowColor2, windowColor2, windowColor2, windowColor2, windowEdgeColor2,
    // 4 feet left window to door
    mainColor2, mainColor2, mainColor2, mainColor2,
    // door
    porchEdgeColor2, porchCenterColor2, porchCenterColor2, porchCenterColor2, porchCenterColor2, porchEdgeColor2,
    // 4 feet right window to door
    mainColor2, mainColor2, mainColor2, mainColor2,
    // right window
    windowEdgeColor2, windowColor2, windowColor2, windowColor2, windowColor2, windowEdgeColor2,
    // right 5 feet
    mainColor2, mainColor2, mainColor2, mainColor2, mainColor2,
});

struct AnimationConfig {
    // Animation timing (all in milliseconds)
    long mainCycleDuration = ANIMATION_CYLCLE_MS;
    long mainCycleFade = ANIMATION_FADE_MS;
    long flasherInterval = 0;//1000;
    
    // Animation parameters
    bool gradientSmooth = true;
    float gradientThreshold = 0.7;
    float gradientMidpoint = 0.5;
    float gradientPower = 2.0;
    float transitionPower = 2.2;
};


// Constants that don't need to be configurable
const float QUADRATIC_POWER = 2.0;
const float CUBIC_POWER = 3.0;
const float SMOOTHSTEP_Q_COEFF = 2.0;
const float SMOOTHSTEP_C_COEFF = 1.0;

/**
 * Calculate smoothstep value using configurable coefficients
 */
float calculateSmoothstep(float progress) {
    return SMOOTHSTEP_Q_COEFF * pow(progress, QUADRATIC_POWER) -
           SMOOTHSTEP_C_COEFF * pow(progress, CUBIC_POWER);
}

/**
 * Calculate progress through the current hold portion of the animation cycle
 * @param cyclePct Current position in the overall cycle (0.0 to 1.0)
 * @param holdPortion Duration of each hold as a portion of total cycle
 * @param transitionPortion Duration of each transition as a portion of total cycle
 * @return Progress through current hold (0.0 to 1.0), or 0.0 if in transition
 */
float calculateHoldProgress(float cyclePct, float holdPortion, float transitionPortion) {
    // Get position within a single hold+transition segment
    float cyclePosition = fmod(cyclePct, holdPortion + transitionPortion);

    // If we're in the hold portion, return progress through it (0.0 to 1.0)
    // Otherwise return 0.0 (we're in a transition)
    return cyclePosition < holdPortion ? cyclePosition / holdPortion : 0.0;
}

/**
 * Calculate the current transition state based on cycle position
 */
float calculateTransitionState(float cyclePosition, float holdPortion, float transitionPortion) {
    if (cyclePosition < holdPortion) {
        // First hold
        return 0.0;
    } else if (cyclePosition < holdPortion + transitionPortion) {
        // First transition (1->2)
        float transitionProgress = (cyclePosition - holdPortion) / transitionPortion;
        return calculateSmoothstep(transitionProgress);
    } else if (cyclePosition < holdPortion + transitionPortion + holdPortion) {
        // Second hold
        return 1.0;
    } else {
        // Second transition (2->1)
        float transitionProgress = (cyclePosition - (holdPortion * 2 + transitionPortion)) / transitionPortion;
        return 1.0 - calculateSmoothstep(transitionProgress);
    }
}

/**
 * Calculate gradient color for a given position and intensity
 */
PixCol calculateGradientColor(PixAniData* data, float position, float intensity, int setOffset) {
    AnimationConfig* config = (AnimationConfig*)data->data;
    int setCount = data->paletteCount() / 2;

    if (config->gradientSmooth) {
        // Smooth color interpolation using built-in palette interpolation
        float palettePosition = position * setCount + setOffset;
        return data->paletteColor(palettePosition).scale(intensity);
    } else {
        // Modified gradient effect with plateau
        int set = (int)(position * setCount);
        float setPos = (position * setCount) - set;
        set = min(set, setCount - 1);

        // Calculate distance from center of set
        float normalizedPos = abs(setPos - config->gradientMidpoint) * 2;

        // Calculate intensity with plateau
        float gradientIntensity;
        if (normalizedPos <= config->gradientThreshold) {
            // Within threshold - maintain full intensity
            gradientIntensity = 1.0;
        } else {
            // Beyond threshold - apply power falloff
            gradientIntensity = pow((1.0 - (normalizedPos - config->gradientThreshold)/(1.0 - config->gradientThreshold)), config->gradientPower);
        }

        return data->paletteColor(set + setOffset).scale(intensity * gradientIntensity);
    }
}


/**
 * Initial fade-in animation that transitions from black to the first color set
 * Uses only odd-numbered pixels (even pixels stay black)
 *
 * @param data Animation data structure
 *             data->data contains transition time for main animation
 */
void initial_fade_in(PixAniData* data) {
    AnimationConfig* config = (AnimationConfig*)data->data;
    float fadeInPower = config->gradientPower;
    float fadeProgress = pow(data->cyclePct, fadeInPower);

    for (int i = 0; i < data->pixelCount; i++) {
        if (i % 2 == 0) {
            data->pixels[i] = Color::BLACK;
            continue;
        }
        float position = (float)i / (data->pixelCount - 1);
        data->pixels[i] = calculateGradientColor(data, position, fadeProgress, 0);
    }
}

/**
 * Main animation that alternates between two color sets
 * Uses odd pixels for first set, even pixels for second set
 * Includes configurable transition and hold times (hold time is how long each set is displayed)
 *
 * @param data Animation data structure
 *             data->data contains animation configuration as ptr to AnimationConfig
 */
void dynamic_gradient_alternating(PixAniData* data) {
    AnimationConfig* config = (AnimationConfig*)data->data;
    
    float transitionPortion = (float)config->mainCycleFade / data->cycleDuration;
    float holdPortion = (1.0 - (transitionPortion * 2)) / 2;
    float holdProgress = calculateHoldProgress(data->cyclePct, holdPortion, transitionPortion);
    float setTransition = calculateTransitionState(data->cyclePct, holdPortion, transitionPortion);
    
    float firstSetIntensity = pow(1.0 - setTransition, config->transitionPower);
    float secondSetIntensity = pow(setTransition, config->transitionPower);
    
    // Flash the first pixel during the hold portion
    long flashDuration = config->flasherInterval * holdProgress;
    bool isFlashOn = flashDuration > 0 && (data->updated % config->flasherInterval) < flashDuration;
    data->pixels[0] = isFlashOn ? PixCol::scale(Color::WHITE, holdProgress * 0.33) : Color::BLACK;
    
    for (int i = 1; i < data->pixelCount; i++) {
        float position = (float)i / (data->pixelCount - 1);
        float intensity = (i % 2 == 0) ? secondSetIntensity : firstSetIntensity;
        int setOffset = (i % 2 == 0) ? data->paletteCount() / 2 : 0;
        
        data->pixels[i] = calculateGradientColor(data, position, intensity, setOffset);
    }
}


static AnimationConfig config;
static int lightSensorValue;

/**
 * Setup function to initialize the LED strip and start animations
 *
 * Parameters for initial fade-in:
 * - initial_fade_in_ms fade-in (cycle and duration)
 */
void setup() {
    // Log.info("pixeleds-housefront started");
    // Particle.variable("lightSensor", &lightSensorValue, INT);

    px.setup();
    px.setPixels(Color::BLACK);

    // Start with initial fade-in animation
    px.startAnimation(&initial_fade_in, &palette, ANIMATION_FADE_IN_MS, ANIMATION_FADE_IN_MS, (int)&config);

    // Particle.publish("pixeleds-housefront setup");
}

/**
 * Main loop function - required for animation updates
 *
 * Main animation will start automatically after fade-in
 */
void loop() {
    // lightSensorValue = analogRead(PHOTO_SENSOR);

    // When initial animation is inactive, start the main gradient animation
    if (!px.isAnimationActive()) {
        // Log.info("pixeleds-housefront main animation starting");
        px.startAnimation(&dynamic_gradient_alternating, &palette, config.mainCycleDuration, -1, (int)&config);
        // Particle.publish("pixeleds-housefront main animation started");
    }

    px.update(millis());
}
