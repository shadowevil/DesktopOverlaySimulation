#pragma once
#include "raylib_win32.h"

//--------------------------------------------------------------------------------------
// HSV to RGB color conversion (helper)
//--------------------------------------------------------------------------------------
inline Color HsvToColor(float h, float s, float v) {
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 30.0f, 2.0f) - 1.0f));
    float m = v - c;

    float r, g, b;
    if (h < 60) { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }

    return {
        (unsigned char)((r + m) * 255),
        (unsigned char)((g + m) * 255),
        (unsigned char)((b + m) * 255),
        255
    };
}

inline Color ShadeCycle(float baseHue, float time, float cycleSpeed = 0.5f) {
    // baseHue: the color hue to keep (e.g. ~45.0f for sand-yellow)
    // time: usually GetTime()
    // cycleSpeed: how fast the shading oscillates

    float h = baseHue;
    float s = 0.5f; // fixed saturation (muted, sandy look)
    float v = 0.8f + 0.2f * sinf(time * cycleSpeed); // oscillate brightness

    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 30.0f, 2.0f) - 1.0f));
    float m = v - c;

    float r, g, b;
    if (h < 60) { r = c; g = x; b = 0; }
    else if (h < 120) { r = x; g = c; b = 0; }
    else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; }
    else if (h < 300) { r = x; g = 0; b = c; }
    else { r = c; g = 0; b = x; }

    return {
        (unsigned char)((r + m) * 255),
        (unsigned char)((g + m) * 255),
        (unsigned char)((b + m) * 255),
        255
    };
}