#pragma once
#include <math.h>
#include <liba/stdint.h>
#include <drivers/AtlasAdvancedGraphicsDriver/aagd.h>
#include <gui/agl/agl.h>

struct Window {
    private:
    uint32_t index;
    

    public:
    PixPoint Position;
    uint32_t Width, Height;
    uint32_t OldWidth, OldHeight;
    const char* Title;

    void* windowBuffer;
    AtlasGraphicsLibrary* agl;
    
    PixPoint OldPosition;
    bool ShowTitleBar;
    bool ShowBorder;
    bool Hidden;
    bool Moveable;
    bool Resizeable;
    bool Closeable;

    bool isFocused;
    bool isMoving;
    bool isResizing;
    bool isMaximized;
    bool isMinimized;
    
    void (*RenderContent)(AtlasAdvancedGraphics* gfx, uint32_t x, uint32_t y); // Optional content renderer

    // Default constructor
    Window()
        : Position({0, 0}), Width(0), Height(0), OldWidth(0), OldHeight(0),
          Title(""), ShowTitleBar(true), ShowBorder(true), Hidden(false),
          OldPosition({0,0}),
          Moveable(true), Resizeable(true), Closeable(true), 
          isFocused(false), isMoving(false), isResizing(false), 
          isMaximized(false), isMinimized(false),
          RenderContent(nullptr), windowBuffer(nullptr) {}

    // Parameterized constructor
    Window(PixPoint pos, uint32_t width, uint32_t height, const char* title,
           void (*RenderContent)(AtlasAdvancedGraphics* gfx, uint32_t x, uint32_t y) = nullptr)
        : Position(pos), Width(width), Height(height), OldWidth(0), OldHeight(0),
          Title(title), ShowTitleBar(true), ShowBorder(true), Hidden(false),
          OldPosition({0,0}),
          Moveable(true), Resizeable(true), Closeable(true), 
          isFocused(false), isMoving(false), isResizing(false), 
          isMaximized(false), isMinimized(false),
          RenderContent(RenderContent), windowBuffer(nullptr) {}

    void SetPosition(PixPoint position) {
        Position = position;
    }

    PixPoint GetPosition() {
        return Position;
    }

    void SetWidth(uint32_t width) {
        Width = width;
    }

    void SetHeight(uint32_t height) {
        Height = height;
    }

    uint32_t GetWidth() {
        return Width;
    }

    uint32_t GetHeight() {
        return Height;
    }

    void SetTitle(const char* newTitle) {
        Title = newTitle;
    }

    const char* GetTitle() {
        return Title;
    }
};
