#include "awm.h"
#include <drivers/userinput/mouse/mouse.h>
#include <drivers/userinput/keyboard/keyboard.h>
#include <drivers/scheduling/PIT.h>
#include <cstr.h>
#include <arch/x86/memory/heap.h>
#include <arch/x86/memory/memory.h>

uint8_t MousePointerBitmap[] = {
    0b10000000, 0b00000000, 
    0b11000000, 0b00000000, 
    0b11100000, 0b00000000, 
    0b11110000, 0b00000000, 
    0b11111000, 0b00000000, 
    0b11111100, 0b00000000, 
    0b11111110, 0b00000000, 
    0b11111111, 0b00000000, 
    0b11111111, 0b10000000, 
    0b11111111, 0b11000000, 
    0b11111111, 0b11100000, 
    0b11111111, 0b11110000, 
    0b11111000, 0b00000000, 
    0b11110000, 0b00000000, 
    0b11100000, 0b00000000, 
    0b11000000, 0b00000000, 
};

class WindowDefaultStyle {
    public:
    uint64_t
        WindowBorder                = 0x414559,
        WindowTitleBar              = 0x232634,
        WindowTitleBarActive        = 0x303446,
        TitleBarText                = 0xc6d0f5,
        TitleBarTextUnactive        = 0x414559,
        WindowBackground            = 0x232634,

        Close                       = 0xe78284,
        CloseHovered                = 0xea999c,
        ClosePressed                = 0xb36566,

        Maximize                    = 0x292c3c, // 0xa6d189
        MaximizeHovered             = 0x292c3c,
        MaximizePressed             = 0x292c3c,

        Minimize                    = 0x292c3c, // 0xe5c890
        MinimizeHovered             = 0x292c3c,
        MinimizePressed             = 0x292c3c,

        InactiveButton              = 0x232634;
};

WindowDefaultStyle wds;
AtlasWindowManager* awm;

bool ShowWindowContext = false;
bool ShowWindowContextPointOfView = false;

double LastFrameTime = 0;    // Time of the last frame
uint64_t FrameCount = 0;     // Total number of frames rendered
double AvgFPS = 0;           // Average FPS

// Constructor for the window manager
AtlasWindowManager::AtlasWindowManager(AtlasAdvancedGraphics* gfx, uint32_t desktopBgColor)
    : gfx(gfx), desktopBgColor(desktopBgColor), windowCount(0) {
        PIT::SetDivisor(65535);
    }

// Add a new window to the manager
void AtlasWindowManager::AddWindow(Window* window) {
    if (windowCount < MAX_WINDOWS) {
        window->windowBuffer = AAiMalloc(window->Width * window->Height * sizeof(uint32_t)); // Allocate memory for the buffer

        memset(window->windowBuffer, 0, window->Width * window->Height * sizeof(uint32_t));

        if (window->windowBuffer == nullptr) {
            abg->AAiPrint("awm: failed to allocate memory for window buffer");
            abg->NextLine();
            return;
        }
        
        AtlasGraphicsLibrary* hwndAGL = new AtlasGraphicsLibrary(window->windowBuffer, window->Width, window->Height, gfx->psf1_font);
        window->agl = hwndAGL;

        windows[windowCount] = window;
        windowCount++;
        FocusWindow(windowCount - 1); // Focus the newly added window
    }
}

void AtlasWindowManager::RemoveWindow(int index) {
    if (index < windowCount) {
        AAiFree(windows[index]->windowBuffer);

        for (int i = index; i < windowCount - 1; i++) {
            windows[i] = windows[i + 1];
        }
        windowCount--;

        // Focus on the previous window if there is one
        if (index > 0) {
            FocusWindow(index - 1);
        } else if (windowCount > 0) {
            FocusWindow(-1);
        }
    }
}

uint64_t mouseOffsetX = 0;
uint64_t mouseOffsetY = 0;

void AtlasWindowManager::OnMouseDown(uint32_t mouseX, uint32_t mouseY) {
    for (int i = windowCount - 1; i >= 0; i--) { // Start with the top-most window
        Window* window = windows[i];

        if (window->isMinimized) {
            continue;
        }

        if (mouseX >= window->Position.X && mouseX <= window->Position.X + window->Width &&
            mouseY >= window->Position.Y && mouseY <= window->Position.Y + window->Height) {
            
            FocusWindow(i);
            MarkWindowDirty(i);

            if (mouseY <= window->Position.Y + 20) { // Check if the click is on the title bar
                // Check close button
                if (mouseX >= window->Position.X + window->Width - 40 &&
                    mouseX <= window->Position.X + window->Width) {
                    // Handle Close Button
                    MarkWindowDirty(i);
                    RemoveWindow(i); // Example function to close the window
                    return;
                }
                // Check maximize button
                else if (mouseX >= window->Position.X + window->Width - 80 &&
                         mouseX <= window->Position.X + window->Width - 40) {
                    // Handle Maximize Button
                    MarkWindowDirty(i);
                    Maximize(i); // Call the maximize handler
                    return;
                }
                // Check minimize button
                else if (mouseX >= window->Position.X + window->Width - 120 &&
                         mouseX <= window->Position.X + window->Width - 80) {
                    // Handle Minimize Button
                    MarkWindowDirty(i);
                    Minimize(i); // Implement Minimize function if necessary
                    return;
                }

                // Handle window movement
                window->isMoving = true;
                mouseOffsetX = mouseX - window->Position.X;
                mouseOffsetY = mouseY - window->Position.Y;
            } else if (mouseX >= window->Position.X + window->Width - 10 &&
                       mouseX <= window->Position.X + window->Width &&
                       mouseY >= window->Position.Y + window->Height - 10 &&
                       mouseY <= window->Position.Y + window->Height) {
                // Bottom-right corner for resizing
                window->isResizing = true;
            }
            break;
        } else {
            FocusWindow(-1);
        }
        MarkWindowDirty(i);
    }

    // Mark all windows as dirty to ensure they are redrawn
    for (uint32_t i = 0; i < windowCount; i++) {
        MarkWindowDirty(i);
    }
}

void AtlasWindowManager::Maximize(uint32_t index) {
    Window* window = windows[index];
    if (!window->isMaximized) {
        window->OldPosition = window->Position;
        window->OldHeight = window->Height;
        window->OldWidth = window->Width;

        window->Position.X = 0;
        window->Position.Y = 0;
        window->Height = gfx->framebuffer->Height;
        window->Width = gfx->framebuffer->Width;
        window->isMaximized = true;
    } else {
        window->Height = window->OldHeight;
        window->Width = window->OldWidth;
        window->Position = window->OldPosition;
        window->isMaximized = false;
        FocusWindow(index);
    }

    if (window->windowBuffer != nullptr) {
        AAiFree(window->windowBuffer);
    }

    window->windowBuffer = AAiMalloc(window->Width * window->Height * sizeof(uint32_t));
    if (window->windowBuffer == nullptr) {
        abg->AAiPrint("awm: failed to allocate memory for window buffer");
        abg->NextLine();
        return;
    }

    memset(window->windowBuffer, 0, window->Width * window->Height * sizeof(uint32_t));

    for (uint32_t i = 0; i < windowCount; i++) {
        MarkWindowDirty(i);
    }
}

void AtlasWindowManager::Minimize(uint32_t index) {
    Window* window = windows[index];
    window->isMinimized = true; // Example of minimizing the window
    MarkWindowDirty(index);
}

void AtlasWindowManager::OnMouseUp() {
    for (int i = 0; i < windowCount; i++) {
        windows[i]->isMoving = false;
        windows[i]->isResizing = false;
        MarkWindowDirty(i);
    }
}

void AtlasWindowManager::OnMouseMove(uint32_t mouseX, uint32_t mouseY) {
    const uint32_t MIN_WIDTH = 100;
    const uint32_t MIN_HEIGHT = 100;

    for (int i = 0; i < windowCount; i++) {
        Window* window = windows[i];
        if (window->isFocused) {
            if (window->isMoving || window->isResizing) {
                MarkWindowDirty(i); // Mark the affected window as dirty
            }

            if (window->isMoving && window->Moveable && !window->isResizing) {
                window->Position.X = mouseX - mouseOffsetX;
                window->Position.Y = mouseY - mouseOffsetY;
            } else if (window->isResizing && window->Resizeable) {
                int32_t newWidth = mouseX - window->Position.X;
                int32_t newHeight = mouseY - window->Position.Y;

                if (window->windowBuffer != nullptr) {
                    AAiFree(window->windowBuffer);
                }

                // Check for crossing the left boundary
                if (newWidth < MIN_WIDTH) {
                    newWidth = MIN_WIDTH;
                }

                // Adjust position if resizing crosses the left boundary
                if (mouseX < window->Position.X) {
                    window->Position.X = window->Position.X;
                    newWidth = 100; // Maintain window width
                }

                // Check for crossing the top boundary
                if (newHeight < MIN_HEIGHT) {
                    newHeight = MIN_HEIGHT;
                }

                // Adjust position if resizing crosses the top boundary
                if (mouseY < window->Position.Y) {
                    window->Position.Y = window->Position.Y;
                    newHeight = 100; // Maintain window height
                }

                // Apply the adjusted dimensions
                window->Width = newWidth;
                window->Height = newHeight;

                window->windowBuffer = AAiMalloc(window->Width * window->Height * sizeof(uint32_t));
                if (window->windowBuffer == nullptr) {
                    abg->AAiPrint("awm: failed to allocate memory for resized window buffer");
                    abg->NextLine();
                    return;
                }

                memset(window->windowBuffer, 0, window->Width * window->Height * sizeof(uint32_t));

                MarkWindowDirty(i);
            }
        }
    }
}

void AtlasWindowManager::ClearMouseCursor(uint8_t* mouseBitmap, PixPoint position) {
    int xMax = 16;
    int yMax = 16;

    int diffX = gfx->framebuffer->Width + position.X;
    int diffY = gfx->framebuffer->Height + position.Y;

    // prevent to draw outside of framebuffer

    if (diffX < 16) xMax = diffX;
    if (diffY < 16) yMax = diffY;

    for (int y = 0; y < yMax; y++) {
        for (int x = 0; x < xMax; x++) {
            int bit = y * 16 + x;
            int byte = bit / 8;

            if ((mouseBitmap[byte] & (0b10000000 >> (x % 8)))) {
                if (gfx->GetPix(position.X + x, position.Y + y) == MouseCursorBufferAfter[x + y * 16]) {
                    gfx->PutPix(position.X + x, position.Y + y, MouseCursorBuffer[x + y * 16]);
                }
            }
        }
    }
}

void AtlasWindowManager::DrawOverlayMouseCursor(uint8_t* mouseBitmap, PixPoint position, uint32_t color) {
    int xMax = 16;
    int yMax = 16;

    int diffX = gfx->framebuffer->Width + position.X;
    int diffY = gfx->framebuffer->Height + position.Y;

    // prevent to draw outside of framebuffer

    if (diffX < 16) xMax = diffX;
    if (diffY < 16) yMax = diffY;

    for (int y = 0; y < yMax; y++) {
        for (int x = 0; x < xMax; x++) {
            int bit = y * 16 + x;
            int byte = bit / 8;

            if ((mouseBitmap[byte] & (0b10000000 >> (x % 8)))) {
                MouseCursorBuffer[x + y * 16] = gfx->GetPix(position.X + x, position.Y + y);
                gfx->PutPix(position.X + x, position.Y + y, color);
                MouseCursorBufferAfter[x + y * 16] = gfx->GetPix(position.X + x, position.Y + y);
            }
        }
    }
}

// Render the desktop background
void AtlasWindowManager::RenderDesktop() {
    gfx->DrawRectangle(0, 0, gfx->framebuffer->Width, gfx->framebuffer->Height, desktopBgColor, true);
}

// Render all the windows
void AtlasWindowManager::RenderWindows() {
    for (uint32_t i = 0; i < windowCount; i++) {
        if (!dirtyWindows[i]) continue;
        Window* window = windows[i];

        if (window->Hidden || window->isMinimized) continue; // Skip rendering hidden windows

        // Draw the window background
        gfx->DrawRectangle(window->Position.X, window->Position.Y + (window->ShowTitleBar ? 20 : 0), window->Width, window->Height - (window->ShowTitleBar ? 20 : 0), wds.WindowBackground, true);

        // Draw the window title bar if enabled
        if (window->ShowTitleBar) {
            gfx->DrawRectangle(window->Position.X, window->Position.Y, window->Width, 20, window->isFocused ? wds.WindowTitleBarActive : wds.WindowTitleBar, true);
            gfx->DrawText(window->Title, window->Position.X + 5, window->Position.Y + 5, 1, window->isFocused ? wds.TitleBarText : wds.TitleBarTextUnactive);

            // close button
            gfx->DrawRectangle(window->Position.X + window->Width - 40, window->Position.Y, 40, 20, window->isFocused ? wds.Close : wds.InactiveButton, true);
            gfx->DrawLine(window->Position.X + window->Width - 40, window->Position.Y, window->Position.X + window->Width - 40, window->Position.Y + 19, wds.WindowBorder);

            // maximize button
            gfx->DrawRectangle(window->Position.X + window->Width - 80, window->Position.Y, 40, 20, window->isFocused ? wds.Maximize : wds.InactiveButton, true);
            gfx->DrawLine(window->Position.X + window->Width - 81, window->Position.Y, window->Position.X + window->Width - 81, window->Position.Y + 19, wds.WindowBorder);

            // minimize button
            gfx->DrawRectangle(window->Position.X + window->Width - 121, window->Position.Y, 40, 20, window->isFocused ? wds.Minimize : wds.InactiveButton, true);
            gfx->DrawLine(window->Position.X + window->Width - 121, window->Position.Y, window->Position.X + window->Width - 121, window->Position.Y + 19, wds.WindowBorder);

            
        }

        // Render the window buffer
        uint32_t* buffer = (uint32_t*)window->windowBuffer;
        int bufferStartY = window->Position.Y + (window->ShowTitleBar ? 20 : 0);
        for (int y = 0; y < window->Height - (window->ShowTitleBar ? 20 : 0); y++) {
            for (int x = 0; x < window->Width; x++) {
                gfx->PutPix(window->Position.X + x, bufferStartY + y, buffer[y * window->Width + x]);
            }
        }

        // Draw a white cube in the buffer
        int cubeSize = 200; // Size of the cube
        int cubeX = window->Width / 2 - cubeSize / 2; // Center the cube horizontally
        int cubeY = window->Height / 2 - cubeSize / 2; // Center the cube vertically

        for (int y = 0; y < cubeSize; y++) {
            for (int x = 0; x < cubeSize; x++) {
                if (cubeX + x < window->Width && cubeY + y < window->Height) {
                    buffer[(cubeY + y) * window->Width + (cubeX + x)] = 0xFFFFFF; // White color
                }
            }
        }

        if (window->ShowBorder) {
            gfx->DrawLine(window->Position.X, window->Position.Y + 20, window->Position.X - 1 + window->Width, window->Position.Y + 20, wds.WindowBorder);
            gfx->DrawRectangle(window->Position.X, window->Position.Y, window->Width, window->Height, wds.WindowBorder, false);
        }

        // Render custom content if provided
        if (window->RenderContent) {
            window->RenderContent(gfx, window->Position.X, window->Position.Y + (window->ShowTitleBar ? 20 : 0));
        }

        //gfx->DrawText(to_hstring((uint64_t)window->windowBuffer), window->Position.X +10, window->Position.Y +30, 1, AAG_COLOR_WHITE);

        if (ShowWindowContext) {
            gfx->DrawRectangle(window->Position.X, window->Position.Y, window->Width, window->Height, AAG_COLOR_BLUE, false);
            gfx->DrawLine(window->Position.X, window->Position.Y, window->Position.X - 1 + window->Width, window->Position.Y - 1 + window->Height, AAG_COLOR_BLUE);
        }

        if (ShowWindowContextPointOfView) {
            gfx->DrawLine(gfx->framebuffer->Width / 2, gfx->framebuffer->Height / 2, window->Position.X, window->Position.Y, AAG_COLOR_BLUE); // top left
            gfx->DrawLine(gfx->framebuffer->Width / 2, gfx->framebuffer->Height / 2, window->Position.X + window->Width, window->Position.Y, AAG_COLOR_BLUE); // top right
            gfx->DrawLine(gfx->framebuffer->Width / 2, gfx->framebuffer->Height / 2, window->Position.X, window->Position.Y + window->Height, AAG_COLOR_BLUE); // bottom left
            gfx->DrawLine(gfx->framebuffer->Width / 2, gfx->framebuffer->Height / 2, window->Position.X + window->Width, window->Position.Y + window->Height, AAG_COLOR_BLUE); // bottom right
        }
        dirtyWindows[i] = false;
    }
}

// Focus a specific window
void AtlasWindowManager::FocusWindow(int32_t index) {
    if (index == -1) {
        for (uint32_t i = 0; i < windowCount; i++) {
            windows[i]->isFocused = false;
        }
    } else if (index < windowCount) {
        Window* focusedWindow = windows[index]; // Store the focused window
        // Shift all windows above the focused one down by one
        for (uint32_t i = index; i < windowCount - 1; i++) {
            windows[i] = windows[i + 1];
        }
        // Place the focused window at the end of the array
        windows[windowCount - 1] = focusedWindow;

        // Mark only the focused window as active
        for (uint32_t i = 0; i < windowCount; i++) {
            windows[i]->isFocused = (i == windowCount - 1);
        }
    }
}

void AtlasWindowManager::MarkWindowDirty(uint32_t index) {
    if (index < windowCount) {
        dirtyWindows[index] = true;
    }
}

// Update the window manager (render the desktop and windows)
void AtlasWindowManager::Update() {

    double currentTime = PIT::TimeSinceBoot;
    double deltaTime = currentTime - LastFrameTime;

    if (deltaTime > 0) { // Avoid division by zero
        AvgFPS = FrameCount / currentTime; // Calculate average FPS
    }

    LastFrameTime = currentTime; // Update last frame time
    FrameCount++; // Increment the frame count

    bool isAnyWindowDirty = false;
    for (uint32_t i = 0; i < windowCount; i++) {
        if (dirtyWindows[i]) {
            isAnyWindowDirty = true;
            break;
        }
    }

    
    if (windowCount == 0 || isAnyWindowDirty) {
        RenderDesktop();
    }
    RenderWindows();
    DrawOverlayMouseCursor(MousePointerBitmap, MousePosition, AAG_COLOR_WHITE);
    

    //OnMouseDown(MousePosition.X, MousePosition.Y);
    //OnMouseMove(MousePosition.X, MousePosition.Y);

    if (IsMoving()) {
        OnMouseMove(MousePosition.X, MousePosition.Y);
    }

    if (IsLeftPressed()) {
        OnMouseDown(MousePosition.X, MousePosition.Y);
    } else {
        OnMouseUp();
    }

    /* if (IsMoving() && IsLeftPressed()) {
        OnMouseDown(MousePosition.X, MousePosition.Y);
        OnMouseMove(MousePosition.X, MousePosition.Y);
    } */

    gfx->DrawText(to_string(AvgFPS), 10, 10, 1, 0xFFFFFF);

    gfx->SwapBuffer();
}