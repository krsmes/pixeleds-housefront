/*
 * Project pixeleds-housefront
 * Description: Lighting up the front of the house with 660 addressable LEDs
 * Author: K Smith <krsjunk@netscape.net>
 * Date: 2022-08-10
 */

#include "pixeleds-library.h"

/* default
PixCol edgeColor = PixCol::BLACK;
PixCol mainColor = PixCol::BLACK;
PixCol lWindowColor = 0x352503;
PixCol lWindowEdgeColor = PixCol::BLACK;
PixCol rWindowColor = 0x352503;
PixCol rWindowEdgeColor = PixCol::BLACK;
PixCol porchCenterColor = PixCol::ORANGE_RED;
PixCol porchEdgeColor = PixCol::DARK_ORANGE; 
*/

/* christmas */
PixCol edgeColor = PixCol::RED;
PixCol mainColor = PixCol::DARK_GREEN;
PixCol lWindowColor = PixCol::hsv(Hue::RED, 1.0, 0.1); // very dark
PixCol lWindowEdgeColor = PixCol::BLACK;
PixCol rWindowColor = PixCol::hsv(Hue::RED, 1.0, 0.1); // very dark
PixCol rWindowEdgeColor = PixCol::BLACK;
PixCol porchCenterColor = PixCol::ORANGE_RED;
PixCol porchEdgeColor = PixCol::BLACK;

/*
 * front house strip is 36 feet
 * 37 colors means one per foot (the first one "blends" in on both ends)
 * left window is approx foot 8 & 9
 * porch is approx foot 17-20 (16 feet on each side)
 * right window is approx foot 28 & 29
 */
PixPal pal_house1 = {37, new PixCol[37] {
        // left and right end fade in
        edgeColor,
        // left 6 feet
        mainColor, mainColor, mainColor, mainColor, mainColor,
        // left window
        lWindowEdgeColor, lWindowColor, lWindowColor, lWindowColor, lWindowColor, lWindowEdgeColor,
        // 6 feet left window to door
        mainColor, mainColor, mainColor, mainColor,
        // door
        porchEdgeColor, porchCenterColor, porchCenterColor, porchCenterColor, porchCenterColor, porchEdgeColor,
        // 6 feet right window to door
        mainColor, mainColor, mainColor, mainColor,
        // right window
        rWindowEdgeColor, rWindowColor, rWindowColor, rWindowColor, rWindowColor, rWindowEdgeColor,
        // right 6 feet
        mainColor, mainColor, mainColor, mainColor, mainColor,
} };

// this outputs the given pallate along the entire strip but constantly fades every other LED in and out
void fade_alternating(PixAniData* data) {
    PixCol from = PixCol::BLACK;
    PixCol target;
    // ratio of palette size to pixel count 
    // e.g. pallet of 4 colors with a pixel count of 20 is .2, 
    //      so for every pixel you take 20% of the way between two pallete colors
    float palpix_ratio = float(data->paletteCount()) / (data->pixelCount - 1);
    // loop through each pixel
    for (int idx = 0; idx < data->pixelCount; idx++) {
        // choose computed target color from palette based on current pixel index
        target = data->paletteColor(idx * palpix_ratio);
        // set the pixel to a color between black and the target color based on pct of current cycle
        // alternate every other pixel on / off (idx % 2)
        // fade alternates in / out (data->cycleCount % 2, 1.0 - data->cyclePct, data->cyclePct)
        data->pixels[idx] = (idx % 2 == data->cycleCount % 2) 
            ? from.interpolate(target, 1.0 - data->cyclePct) 
            : from.interpolate(target, data->cyclePct);
    }
}



/*
 * setup & loop
 */

#define PHOTO_SENSOR A0
int lightSensorValue;

Pixeleds px = Pixeleds(new PixCol[660] {0}, 660, 0, SK6812W, ORDER_GRB); // house front LEDs
//Pixeleds px = Pixeleds(new PixCol[660] {0}, 660, 0, SK6812W, ORDER_GRB); // shorter test LED set
//Pixeleds px = Pixeleds(new PixCol[144] {0}, 144, 0, SK6812W, ORDER_GRB); // light tube

void setup() {
    Particle.variable("lightSensor", &lightSensorValue, INT);
    px.setup();
//    px.rainbow();
    px.startAnimation(&fade_alternating, &pal_house1, 60000);
}

void loop() {
    lightSensorValue = analogRead(PHOTO_SENSOR);
    px.update(millis());
}