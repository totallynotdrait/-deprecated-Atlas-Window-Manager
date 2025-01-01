/*  Atlas Development Program [Copyright (c) 2024 drait]
    
    file: agl.h
    recap: Atlas Graphics Library, a dedicated library for developing a simple User Interface
*/
#pragma once
#include <math.h>
#include <liba/stdint.h>
#include <drivers/AtlasAdvancedGraphicsDriver/aagd.h>

class AtlasGraphicsLibrary {
    public:
    AtlasGraphicsLibrary(void* windowBuffer, uint32_t windowWidth, uint32_t windowHeight, PSF1_FONT* psf1_font);
    void* WindowBuffer;
    uint32_t WindowWidth;
    uint32_t WindowHeight;
    PSF1_FONT* PSF1_font;

    void PutPix(uint32_t x, uint32_t y, uint32_t colour);

    void DrawLine(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t colour);
    void DrawRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t colour, bool fill);
    uint32_t GetTextWidth(const char* text, uint32_t scale);

    void AddText(const char* value, uint32_t scale, uint32_t x, uint32_t y);
    void AddButton(const char* label, uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool& buttonState);
};