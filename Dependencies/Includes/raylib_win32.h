#pragma once

// Standard headers (put these first so macros don’t break them)
#include <cstdarg>
#include <cstdbool>
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cctype>

// --- Windows.h setup ---
#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX

    // Temporarily rename symbols that collide with raylib
    #define CloseWindow CloseWindowWin32
    #define Rectangle   RectangleWin32
    #define ShowCursor  ShowCursorWin32
    #define LoadImageA  LoadImageAWin32
    #define LoadImageW  LoadImageWWin32
    #define DrawTextA   DrawTextAWin32
    #define DrawTextW   DrawTextWWin32
    #define DrawTextExA DrawTextExAWin32
    #define DrawTextExW DrawTextExWWin32
    #define PlaySoundA  PlaySoundAWin32

#ifdef _WIN32
    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX

    // Rename only if not already safe
    #define WindowsCloseWindow CloseWindow
    #include <windows.h>
    #undef CloseWindow   // remove macro so raylib can define its function
#else
    #include <windows.h>
#endif

    // Undefine the macros so Raylib can use its own versions
    #undef CloseWindow
    #undef Rectangle
    #undef ShowCursor
    #undef LoadImage
    #undef LoadImageA
    #undef LoadImageW
    #undef DrawText
    #undef DrawTextA
    #undef DrawTextW
    #undef DrawTextEx
    #undef DrawTextExA
    #undef DrawTextExW
    #undef PlaySoundA
#endif

// --- Raylib ---
extern "C" {
    #include "raylib.h"
    #include "rlgl.h"
}
#include "raymath.h"

// --- Helpers ---
inline bool IsMouseDoubleClicked(int button, double maxDelay = 0.25)
{
    static double lastClickTime[3] = { 0.0, 0.0, 0.0 };
    double now = GetTime();

    if (IsMouseButtonPressed(button)) {
        int idx = (button == MOUSE_BUTTON_LEFT) ? 0 :
                  (button == MOUSE_BUTTON_RIGHT) ? 1 : 2;

        if (now - lastClickTime[idx] <= maxDelay) {
            lastClickTime[idx] = 0.0;
            return true;
        }
        lastClickTime[idx] = now;
    }
    return false;
}


#ifdef _WIN32
inline void SetWindowClickThrough(bool enable) {
    HWND hwnd = (HWND)GetWindowHandle(); // Raylib helper to get Win32 handle
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

    if (enable) {
        exStyle |= WS_EX_LAYERED | WS_EX_TRANSPARENT;
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
        SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    }
    else {
        exStyle &= ~WS_EX_TRANSPARENT; // keep layered but not transparent-to-clicks
        SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
    }
}

inline void SetWindowTopMost(bool enable) {
    HWND hwnd = (HWND)GetWindowHandle(); // Raylib helper to get native Win32 HWND
    if (enable) {
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    else {
        SetWindowPos(hwnd, HWND_NOTOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

inline int GetTaskbarHeight(int monitorIndex) {
	if (monitorIndex == -1)
		monitorIndex = GetCurrentMonitor();

    if (monitorIndex < 0 || monitorIndex >= GetMonitorCount())
        return 0;

    // Raylib gives us monitor rectangle
    int monX = (int)GetMonitorPosition(monitorIndex).x;
    int monY = (int)GetMonitorPosition(monitorIndex).y;
    int monW = GetMonitorWidth(monitorIndex);
    int monH = GetMonitorHeight(monitorIndex);

    RECT monitorRect;
    monitorRect.left   = monX;
    monitorRect.top    = monY;
    monitorRect.right  = monX + monW;
    monitorRect.bottom = monY + monH;

    MONITORINFO mi;
    mi.cbSize = sizeof(MONITORINFO);

    HMONITOR hMon = MonitorFromRect(&monitorRect, MONITOR_DEFAULTTONEAREST);
    if (GetMonitorInfo(hMon, &mi)) {
        int fullW = mi.rcMonitor.right - mi.rcMonitor.left;
        int fullH = mi.rcMonitor.bottom - mi.rcMonitor.top;
        int workW = mi.rcWork.right - mi.rcWork.left;
        int workH = mi.rcWork.bottom - mi.rcWork.top;

        if (workH < fullH) {
            // taskbar top or bottom
            return fullH - workH;
        }
        else if (workW < fullW) {
            // taskbar left or right
            return fullW - workW;
        }
    }

    return 0; // hidden or fullscreen
}

inline void HideFromTaskbar() {
    HWND hwnd = (HWND)GetWindowHandle();
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

    // remove "AppWindow", add "ToolWindow"
    exStyle &= ~WS_EX_APPWINDOW;
    exStyle |= WS_EX_TOOLWINDOW;

    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);

    // Apply changes
    ShowWindow(hwnd, SW_HIDE);
    ShowWindow(hwnd, SW_SHOW);
}

inline Vector2 GetCursorPosition() {
    HWND hwnd = (HWND)GetWindowHandle();  // Obtain the window handle from Raylib
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(hwnd, &p);
    return Vector2{ (float)p.x, (float)p.y };
}

#include <atomic>
#include <functional>
#include <vector>
#include <string>
#include <thread>
#include <memory>

struct Hotkey {
    int id;
    int modifiers;
    int key;
    std::function<void()> callback;

    Hotkey(int id, int modifiers, int key, std::function<void()> callback)
        : id(id), modifiers(modifiers), key(key), callback(callback) { }

    ~Hotkey() {
        UnregisterHotKey(NULL, id);
    }
};

class GlobalHotkey {
    std::vector<std::unique_ptr<Hotkey>> hotkeys;
    std::thread worker;
    std::atomic<bool> running;

public:
    GlobalHotkey() : running(false) {}

    ~GlobalHotkey() {
        Stop();
    }

    void AddHotkey(int id, int modifiers, int key, std::function<void()> callback) {
        hotkeys.push_back(std::make_unique<Hotkey>(id, modifiers, key, callback));
    }

    void Start() {
        running = true;
        worker = std::thread(&GlobalHotkey::HotkeyThread, this);
    }

    void Stop() {
        if (running) {
            running = false;
            PostThreadMessage(GetThreadId(worker.native_handle()), WM_QUIT, 0, 0);
            if (worker.joinable()) worker.join();
        }
    }

private:
    void HotkeyThread() {
        // Register all hotkeys in THIS thread
        for (auto& hk : hotkeys) {
            if (!RegisterHotKey(NULL, hk->id, hk->modifiers, hk->key)) {
                std::string msg = "Failed to register global hotkey (id=" + std::to_string(hk->id) + ")";
                MessageBoxA(NULL, msg.c_str(), "Error", MB_OK);
            }
        }

        MSG msg;
        while (running && GetMessage(&msg, NULL, 0, 0)) {
            if (msg.message == WM_HOTKEY) {
                int id = (int)msg.wParam;
                for (auto& hk : hotkeys) {
                    if (hk->id == id && hk->callback) {
                        hk->callback();
                    }
                }
            }
        }

        // cleanup
        for (auto& hk : hotkeys) {
            UnregisterHotKey(NULL, hk->id);
        }
    }
};
#endif