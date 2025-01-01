/*  Atlas Development Program [Copyright (c) 2024 drait]
    
    file: awm.h
    recap: Atlas Window Manager, very simple but complete window manager for Atlas using Atlas Advanced Graphics Driver

    But how does it work?
    Atlas Window Manager asks 3 arguments,
        The framebuffer data structure
        The backbuffer address
        The PSF1 font data structure

    AWM is based on Atlas Advanced Graphics which uses double buffering.
    Double Buffering is a well popular technic of rendering that has the benefits of
    being fast and fixing some issues.
    On how it works is very simple, there is the main framebuffer which the user sees and the backbuffer,
    AAG when it draws a rectangle to the backbuffer, and then using memcpy it copies the backbuffer to the
    framebuffer, the reason is because the backbuffer is created and allocated into the memory which is the
    reason why is fast.
*/

#pragma once
#include <drivers/AtlasAdvancedGraphicsDriver/aagd.h>
#include <math.h>
#include "window.h"
#include <gui/agl/agl.h>

#define MAX_WINDOWS 10 // Maximum number of windows supported




class AtlasWindowManager {
private:
    AtlasAdvancedGraphics* gfx;
    uint32_t desktopBgColor;
    uint32_t windowCount;
    Window* windows[MAX_WINDOWS];
    uint32_t* backbuffer;
    size_t backbufferSize;
    bool dirtyWindows[MAX_WINDOWS]; // Array to track dirty windows

public:
    uint32_t MouseCursorBuffer[16 * 16];
    uint32_t MouseCursorBufferAfter[16 * 16];
    
    AtlasWindowManager(AtlasAdvancedGraphics* gfx, uint32_t desktopBgColor);
    void AddWindow(Window* window);
    void RemoveWindow(int index);
    void RenderDesktop();
    void RenderWindows();
    void FocusWindow(int32_t index);
    void Update();
    void OnMouseDown(uint32_t mouseX, uint32_t mouseY);
    void OnMouseUp();
    void OnMouseMove(uint32_t mouseX, uint32_t mouseY);
    void MarkWindowDirty(uint32_t index);
    void DrawOverlayMouseCursor(uint8_t* mouseBitmap, PixPoint position, uint32_t color);
    void ClearMouseCursor(uint8_t* mouseBitmap, PixPoint position);
    void Maximize(uint32_t index);
    void Minimize(uint32_t index);
};

extern AtlasWindowManager* awm;

extern bool ShowWindowContext;
extern bool ShowWindowContextPointOfView;