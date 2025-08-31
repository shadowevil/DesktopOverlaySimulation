#define NOMINMAX
#include "raylib_win32.h"
#include <random>
#include <string>
#include "Config.h"
#include "SandSimulation.h"
#include "SnowSimulation.h"
#include "FireworksSimulation.h"

// Random generator
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

ConfigManager configManager;

int main() {
    Config config = configManager.GetConfig();
    GlobalHotkey hotkey;
    bool passthrough = config.MousePassthrough;
    bool topmost = config.TopMost;

    SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
    InitWindow(1, 1, "Raylib Window");
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    int display = (config.ActiveMonitor == -1 ? GetCurrentMonitor() : config.ActiveMonitor);
    auto monitorPos = GetMonitorPosition(display);
    int screenWidth = GetMonitorWidth(display) - 1;
    int screenHeight = GetMonitorHeight(display) - 1;
	SetWindowSize(screenWidth, screenHeight);
    SetWindowPosition((int)monitorPos.x, (int)monitorPos.y);
    SetTargetFPS(60);

    HideFromTaskbar();
	SetWindowTopMost(topmost);
	SetWindowClickThrough(passthrough);

    hotkey.AddHotkey(1, 0, VK_F2, [&passthrough] {
        passthrough = !passthrough;
        SetWindowClickThrough(passthrough);
        });

	hotkey.AddHotkey(2, MOD_CONTROL, 'Y', [&topmost] {
		topmost = !topmost;
		SetWindowTopMost(topmost);
		});

    hotkey.Start();

    config.ActiveSim = ActiveSimulation::Fireworks;

    std::unique_ptr<ISimulation> sim;
	switch ((ActiveSimulation)config.ActiveSim) {
	case ActiveSimulation::Sand:
		sim = std::make_unique<SandSimulation>();
		break;
	case ActiveSimulation::Snow:
		sim = std::make_unique<SnowSimulation>();
		break;
    case ActiveSimulation::Fireworks:
        sim = std::make_unique<FireworksSimulation>();
        break;
	default:
		sim = std::make_unique<SandSimulation>();
		break;
	}

    while (!WindowShouldClose()) {
        sim->Update();

        BeginDrawing();
        ClearBackground(BLANK);
        sim->Draw();
        sim->DrawUIOverlay();
        EndDrawing();
    }

    hotkey.Stop();
    CloseWindow();
    return 0;
}