/*
 * Project pixeleds-housefront
 * Description: Lighting up the front of the house with 660 addressable LEDs
 * Author: K Smith <krsjunk@netscape.net>
 * Created: 2022-08-10
 * Updated: 2024-12-05
 */

#include "pixeleds-library.h"

#define PHOTO_SENSOR A0
int lightSensorValue;

//Pixeleds px = Pixeleds(new PixCol[210] {0}, 210, 0, SK6812W, ORDER_GRB);  // led strip
Pixeleds px = Pixeleds(new PixCol[660] {0}, 660, 0, SK6812W, ORDER_GRB); // house front

// color set 1
PixCol edgeColor1 = PixCol::BLACK;
PixCol mainColor1 = PixCol::hsv(Hue::GREEN, 1.0, 0.1); // very dark
PixCol lWindowColor1 = PixCol::hsv(Hue::RED, 1.0, 0.1); // very dark
PixCol lWindowEdgeColor1 = PixCol::BLACK;
PixCol rWindowColor1 = PixCol::hsv(Hue::RED, 1.0, 0.1); // very dark
PixCol rWindowEdgeColor1 = PixCol::BLACK;
PixCol porchCenterColor1 = PixCol::ORANGE_RED;
PixCol porchEdgeColor1 = PixCol::BLACK;

// color set 2
PixCol edgeColor2 = PixCol::BLACK;
PixCol mainColor2 = PixCol::hsv(Hue::RED, 1.0, 0.1); // very dark
PixCol lWindowColor2 = PixCol::hsv(Hue::GREEN, 1.0, 0.1); // very dark
PixCol lWindowEdgeColor2 = PixCol::BLACK;
PixCol rWindowColor2 = PixCol::hsv(Hue::GREEN, 1.0, 0.1); // very dark
PixCol rWindowEdgeColor2 = PixCol::BLACK;
PixCol porchCenterColor2 = PixCol::DARK_GOLDEN_ROD;
PixCol porchEdgeColor2 = PixCol::BLACK;

PixCol gradientColors[] = {
    // FIRST SET
    edgeColor1,
    // left 5 feet
    mainColor1, mainColor1, mainColor1, mainColor1, mainColor1,
    // left window
    lWindowEdgeColor1, lWindowColor1, lWindowColor1, lWindowColor1, lWindowColor1, lWindowEdgeColor1,
    // 4 feet left window to door
    mainColor1, mainColor1, mainColor1, mainColor1,
    // door
    porchEdgeColor1, porchCenterColor1, porchCenterColor1, porchCenterColor1, porchCenterColor1, porchEdgeColor1,
    // 4 feet right window to door
    mainColor1, mainColor1, mainColor1, mainColor1,
    // right window
    rWindowEdgeColor1, rWindowColor1, rWindowColor1, rWindowColor1, rWindowColor1, rWindowEdgeColor1,
    // right 5 feet
    mainColor1, mainColor1, mainColor1, mainColor1, mainColor1,

    // SECOND SET
    edgeColor2,
    // left 5 feet
    mainColor2, mainColor2, mainColor2, mainColor2, mainColor2,
    // left window
    lWindowEdgeColor2, lWindowColor2, lWindowColor2, lWindowColor2, lWindowColor2, lWindowEdgeColor2,
    // 4 feet left window to door
    mainColor2, mainColor2, mainColor2, mainColor2,
    // door
    porchEdgeColor2, porchCenterColor2, porchCenterColor2, porchCenterColor2, porchCenterColor2, porchEdgeColor2,
    // 4 feet right window to door
    mainColor2, mainColor2, mainColor2, mainColor2,
    // right window
    rWindowEdgeColor2, rWindowColor2, rWindowColor2, rWindowColor2, rWindowColor2, rWindowEdgeColor2,
    // right 5 feet
    mainColor2, mainColor2, mainColor2, mainColor2, mainColor2,
};
PixPal gradientPalette = { 74, gradientColors };

// Shared animation timing constants
const long INITIAL_FADE_IN_MS = 30000;
const long MAIN_CYCLE_DURATION_MS = 600000;
const long MAIN_CYCLE_FADE_MS = 45000;  // portion of main_cycle_duration spent fading

// Flash timing constants for first LED
const unsigned long FLASH_INTERVAL_MS = 1000;    // Time between flash starts

// Gradient and transition control constants
const bool GRADIENT_SMOOTH = true;     // Blend color segments together
const float GRADIENT_THRESHOLD = 0.7;   // controls width of full intensity section (0.0 to 1.0)
const float GRADIENT_MIDPOINT = 0.5;    // Center point of each color segment
const float GRADIENT_POWER = 2.0;       // Controls falloff rate: higher = sharper falloff to black
const float INTENSITY_POWER = 2.2;      // Power for gradual intensity changes
const float FADE_IN_POWER = 4.0;        // Power for initial fade-in curve

// Smoothstep formula coefficients
const float SMOOTHSTEP_Q_COEFF = 2.0;   // QUADRATIC coefficient
const float SMOOTHSTEP_C_COEFF = 1.0;   // CUBIC coefficient
// Q=3,C=2 for smooth, balanced S-curve, equal acceleration and deceleration
// Q=2,C=1 for gentler overall curve, transitions happen more gradually
// Q=6,C=4 for more aggressive S-curve, quicker to accelerate and decelerate
// Q=3,C=1 for asymmetric curve, fast acceleration, slow deceleration
const float QUADRATIC_POWER = 2.0;      // Power for x² terms
const float CUBIC_POWER = 3.0;          // Power for x³ terms


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
 * Calculate smoothstep value using configurable coefficients
 */
float calculateSmoothstep(float progress) {
    return SMOOTHSTEP_Q_COEFF * pow(progress, QUADRATIC_POWER) -
           SMOOTHSTEP_C_COEFF * pow(progress, CUBIC_POWER);
}

/**
 * Calculate gradient color for a given position and intensity
 */
PixCol calculateGradientColor(PixAniData* data, float position, float intensity, int segmentOffset) {
    int segmentCount = data->paletteCount() / 2;

    if (GRADIENT_SMOOTH) {
        // Smooth color interpolation using built-in palette interpolation
        float palettePosition = position * segmentCount + segmentOffset;
        return data->paletteColor(palettePosition).scale(intensity);
    } else {
        // Modified gradient effect with plateau
        int section = (int)(position * segmentCount);
        float sectionPos = (position * segmentCount) - section;
        section = min(section, segmentCount - 1);

        // Calculate distance from center of segment
        float normalizedPos = abs(sectionPos - GRADIENT_MIDPOINT) * 2;

        // Calculate intensity with plateau
        float gradientIntensity;
        if (normalizedPos <= GRADIENT_THRESHOLD) {
            // Within threshold - maintain full intensity
            gradientIntensity = 1.0;
        } else {
            // Beyond threshold - apply power falloff
            gradientIntensity = pow((1.0 - (normalizedPos - GRADIENT_THRESHOLD)/(1.0 - GRADIENT_THRESHOLD)), GRADIENT_POWER);
        }

        return data->paletteColor(section + segmentOffset).scale(intensity * gradientIntensity);
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
    // Create gradual fade-in using power curve
    float fadeProgress = pow(data->cyclePct, FADE_IN_POWER);

    for (int i = 0; i < data->pixelCount; i++) {
        if (i % 2 == 0) {
            // Keep even pixels black for initial fade-in
            data->pixels[i] = PixCol::BLACK;
            continue;
        }

        float position = (float)i / (data->pixelCount - 1);
        // Use the shared gradient color calculation with fade progress as intensity
        data->pixels[i] = calculateGradientColor(data, position, fadeProgress, 0);
    }
}

/**
 * Main animation that alternates between two color sets
 * Uses odd pixels for first set, even pixels for second set
 * Includes configurable transition and hold times
 *
 * @param data Animation data structure
 *             data->data contains transition time in milliseconds
 */
void dynamic_gradient_alternating(PixAniData* data) {
    // Calculate timing portions
    float singleTransitionMs = data->data;
    float cycleMs = data->cycleDuration;
    float transitionPortion = singleTransitionMs / cycleMs;
    float holdPortion = (1.0 - (transitionPortion * 2)) / 2;
    float holdProgress = calculateHoldProgress(data->cyclePct, holdPortion, transitionPortion);

    // Calculate transition state
    // A value from 0.0 to 1.0 that tracks the progress of transitioning between the two color sets,
    // where 0 means showing only the first set and 1 means showing only the second set
    float setTransition = calculateTransitionState(data->cyclePct, holdPortion, transitionPortion);

    // Calculate set intensities
    // The brightness level (0.0 to 1.0) of the color set
    //   decreases as setTransition increases for first set
    //   increases as setTransition increases for second set
    float firstSetIntensity = pow(1.0 - setTransition, INTENSITY_POWER);
    float secondSetIntensity = pow(setTransition, INTENSITY_POWER);

    // Handle first LED flashing
    long flashDuration = FLASH_INTERVAL_MS * holdProgress;
    bool isFlashOn = (data->updated % FLASH_INTERVAL_MS) < flashDuration;
    data->pixels[0] = isFlashOn ? PixCol::scale(PixCol::GRAY, holdProgress) : PixCol::BLACK;

    // Update all other pixels
    for (int i = 1; i < data->pixelCount; i++) {
        float position = (float)i / (data->pixelCount - 1);
        // Even LEDs use second set, odd LEDs use first set
        int setOffset = (i % 2 == 0) ? data->paletteCount() / 2 : 0;
        // Even LEDs fade with secondSetIntensity, odd with firstSetIntensity
        float intensity = (i % 2 == 0) ? secondSetIntensity : firstSetIntensity;

        data->pixels[i] = calculateGradientColor(data, position, intensity, setOffset);
    }
}


/**
 * Setup function to initialize the LED strip and start animations
 *
 * Parameters for initial fade-in:
 * - initial_fade_in_ms fade-in (cycle and duration)
 */
void setup() {
    Particle.variable("lightSensor", &lightSensorValue, INT);

    px.setup();
    px.setPixels(PixCol::BLACK);

    // Start with initial fade-in animation
    px.startAnimation(&initial_fade_in, &gradientPalette, INITIAL_FADE_IN_MS, INITIAL_FADE_IN_MS);
}

/**
 * Main loop function - required for animation updates
 *
 * Main animation will start automatically after fade-in
 */
void loop() {
    lightSensorValue = analogRead(PHOTO_SENSOR);

    // When initial animation is inactive, start the main gradient animation
    if (!px.isAnimationActive()) {
        px.startAnimation(&dynamic_gradient_alternating, &gradientPalette, MAIN_CYCLE_DURATION_MS, -1, MAIN_CYCLE_FADE_MS);
    }

    px.update(millis());
}
