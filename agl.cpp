#include "agl.h"
#include <drivers/userinput/mouse/mouse.h>

class AGLDefaultStyle {
    public:
    uint64_t
        Text                    = 0xc6d0f5,
        Button                  = 0x303446,
        ButtonHovered           = 0x414559,
        ButtonPressed           = 0x292c3c,
        ButtonBorder            = 0x414559;
};

AGLDefaultStyle ads;

AtlasGraphicsLibrary::AtlasGraphicsLibrary(void* windowBuffer, uint32_t windowWidth, uint32_t windowHeight, PSF1_FONT* psf1_font) {
    WindowBuffer = windowBuffer;
    WindowWidth = windowWidth;
    WindowHeight = windowHeight;
    PSF1_font = PSF1_font;
}

void AtlasGraphicsLibrary::PutPix(uint32_t x, uint32_t y, uint32_t colour) {
    // Check if the WindowWidth are within the bounds of the window buffer
    if (x >= WindowWidth || y >= WindowHeight) {
        return; // Out of bounds, do nothing
    }

    *(uint32_t*)((uint64_t)WindowBuffer + (x * 4) + (y * WindowWidth * 4)) = colour;
}


void AtlasGraphicsLibrary::AddText(const char* value, uint32_t scale, uint32_t x, uint32_t y) {
    for (const char* c = value; *c != '\0'; c++) {
        char* fontPtr = (char*)PSF1_font->glyphBuffer + (*c * PSF1_font->psf1_header->charSize);

        for (uint32_t cy = 0; cy < 16; cy++) { // 16 rows per character
            for (uint32_t cx = 0; cx < 8; cx++) { // 8 columns per character
                if (fontPtr[cy] & (0x80 >> cx)) {
                    for (uint32_t sy = 0; sy < scale; sy++) {
                        for (uint32_t sx = 0; sx < scale; sx++) {
                            PutPix(x + (cx * scale) + sx, y + (cy * scale) + sy, ads.Text);
                        }
                    }
                }
            }
        }
        x += 8 * scale;
    }
}

void AtlasGraphicsLibrary::AddButton(const char* label, uint32_t x, uint32_t y, uint32_t width, uint32_t height, bool& buttonState) {
    uint32_t labelWidth = GetTextWidth(label, 1);
    
    if (width == 0) {
        width = labelWidth + 16;
    }

    if (height == 0) {
        height = 30;
    }

    uint32_t labelX = x + (width - labelWidth) / 2;
    uint32_t labelY = y + (height - 16) / 2;
    
    if (IsDragging() == false) {
        if (MousePosition.X >= x && MousePosition.X <= x + width && MousePosition.Y >= y && MousePosition.Y <= y + height) {
            DrawRectangle(x, y, width, height, ads.ButtonHovered, true);
            AddText((char*)label, 1, labelX, labelY);
        } else {
            DrawRectangle(x, y, width, height, ads.Button, true);
            AddText((char*)label, 1, labelX, labelY);
        } 
        
        if (IsLeftPressed()) {
            if (MousePosition.X >= x && MousePosition.X <= x + width && MousePosition.Y >= y && MousePosition.Y <= y + height) {
                DrawRectangle(x, y, width, height, ads.ButtonPressed, true);
                AddText((char*)label, 1, labelX, labelY);
                buttonState = !buttonState;
            }
        }
    } else {
        DrawRectangle(x, y, width, height, ads.Button, true);
        AddText((char*)label, 1, labelX, labelY);
    }

    DrawRectangle(x, y, width, height, ads.ButtonBorder, false);
}

void AtlasGraphicsLibrary::DrawLine(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t colour) {
    if (x1 >= WindowWidth || y1 >= WindowHeight || x2 >= WindowWidth || y2 >= WindowHeight) {
        return; // Out of bounds, do nothing
    }
    
    int dx = abs((int)x2 - (int)x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs((int)y2 - (int)y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;

    while (true) {
        PutPix(x1, y1, colour);  // Put pixel on the backbuffer
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void AtlasGraphicsLibrary::DrawRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t colour, bool fill) {
    // Draw the edges
    for (uint32_t i = 0; i < width; i++) {
        PutPix(x + i, y, colour); // Top edge
        PutPix(x + i, y + height - 1, colour); // Bottom edge
    }
    for (uint32_t i = 0; i < height; i++) {
        PutPix(x, y + i, colour); // Left edge
        PutPix(x + width - 1, y + i, colour); // Right edge
    }

    // Fill the rectangle if needed
    if (fill) {
        for (uint32_t i = 0; i < height; i++) {
            for (uint32_t j = 0; j < width; j++) {
                PutPix(x + j, y + i, colour);
            }
        }
    }
}

uint32_t AtlasGraphicsLibrary::GetTextWidth(const char* text, uint32_t scale) {
    uint32_t width = 0;
    for (const char* c = text; *c != '\0'; c++) {
        width += 8 * scale; // Assuming each character is 8px wide at scale 1
    }
    return width;
}