#define NOMINMAX
#include "raylib_win32.h"
#include <random>
#include <string>
#include "Config.h"
#include "SandSimulation.h"
#include "SnowSimulation.h"
#include "FireworksSimulation.h"
#include "DrawingSimulation.h"

// Random generator
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

ConfigManager configManager;

int main() {
    Config* config = configManager.GetConfig();
    GlobalHotkey hotkey;

    SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
    InitWindow(1, 1, "Raylib Window");
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    int display = (config->ActiveMonitor == -1 ? GetCurrentMonitor() : config->ActiveMonitor);
    auto monitorPos = GetMonitorPosition(display);
    int screenWidth = GetMonitorWidth(display) - 1;
    int screenHeight = GetMonitorHeight(display) - 1;
	SetWindowSize(screenWidth, screenHeight);
    SetWindowPosition((int)monitorPos.x, (int)monitorPos.y);
    SetTargetFPS(60);

    HideFromTaskbar();
	SetWindowTopMost(config->TopMost);
	SetWindowClickThrough(config->MousePassthrough);

    hotkey.AddHotkey(1, 0, VK_F2, [&] {
        config->MousePassthrough = !config->MousePassthrough;
        SetWindowClickThrough(config->MousePassthrough);
		bool should_focus = !config->MousePassthrough;
        if (should_focus)
            SetWindowFocused();
        });

	hotkey.AddHotkey(2, MOD_CONTROL, 'Y', [&] {
		config->TopMost = !config->TopMost;
		SetWindowTopMost(config->TopMost);
		});

    hotkey.Start();

	std::unordered_map<ActiveSimulation, std::unique_ptr<ISimulation>> simulations;

    std::unique_ptr<ISimulation> sim;
	switch ((ActiveSimulation)config->ActiveSim) {
	case ActiveSimulation::Sand:
		sim = std::make_unique<SandSimulation>();
		break;
	case ActiveSimulation::Snow:
		sim = std::make_unique<SnowSimulation>();
		break;
    case ActiveSimulation::Fireworks:
        sim = std::make_unique<FireworksSimulation>();
        break;
    case ActiveSimulation::Drawing:
        sim = std::make_unique<DrawingSimulation>(config->DrawingSimConfig);
        break;
	default:
		sim = std::make_unique<SandSimulation>();
		break;
	}

    double escHeldTime = 0.0;
    bool shouldExit = false;

    while (!shouldExit) {
        float dt = GetFrameTime();

        // Escape-hold logic
        if (IsKeyDown(KEY_ESCAPE)) {
            escHeldTime += dt;
            if (escHeldTime >= 2.5f) { // 2.5 seconds hold
                shouldExit = true;
            }
        }
        else {
            escHeldTime = 0.0;
        }

        sim->Update();

        BeginDrawing();
        ClearBackground(BLANK);
        sim->Draw();
        sim->DrawUIOverlay();

        // Show exit prompt if Escape is pressed
        if (IsKeyDown(KEY_ESCAPE)) {
            int barWidth = (int)((escHeldTime / 2.5f) * 200);
            if (barWidth > 200) barWidth = 200;

            DrawRectangle(GetScreenWidth() / 2 - 110, GetScreenHeight() / 2 - 40, 220, 60, Fade(BLACK, 0.7f));
            DrawText("Hold ESC to quit", GetScreenWidth() / 2 - 80, GetScreenHeight() / 2 - 30, 20, RAYWHITE);
            DrawRectangle(GetScreenWidth() / 2 - 100, GetScreenHeight() / 2, 200, 20, DARKGRAY);
            DrawRectangle(GetScreenWidth() / 2 - 100, GetScreenHeight() / 2, barWidth, 20, RED);
        }

        bool isControlDown = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);

		if (isControlDown && IsKeyPressed(KEY_ONE))
		{
            ShowCursor();
			simulations[config->ActiveSim] = std::move(sim);
			config->ActiveSim = ActiveSimulation::Sand;
			if (simulations.find(ActiveSimulation::Sand) != simulations.end())
				sim = std::move(simulations[ActiveSimulation::Sand]);
			else
				sim = std::make_unique<SandSimulation>();
		}
		else if (isControlDown && IsKeyPressed(KEY_TWO))
		{
            ShowCursor();
			simulations[config->ActiveSim] = std::move(sim);
			config->ActiveSim = ActiveSimulation::Snow;
			if (simulations.find(ActiveSimulation::Snow) != simulations.end())
				sim = std::move(simulations[ActiveSimulation::Snow]);
			else
				sim = std::make_unique<SnowSimulation>();
		}
		else if (isControlDown && IsKeyPressed(KEY_THREE))
		{
            ShowCursor();
			simulations[config->ActiveSim] = std::move(sim);
			config->ActiveSim = ActiveSimulation::Fireworks;
			if (simulations.find(ActiveSimulation::Fireworks) != simulations.end())
				sim = std::move(simulations[ActiveSimulation::Fireworks]);
			else
				sim = std::make_unique<FireworksSimulation>();
		}
		else if (isControlDown && IsKeyPressed(KEY_FOUR))
		{
			simulations[config->ActiveSim] = std::move(sim);
			config->ActiveSim = ActiveSimulation::Drawing;
			if (simulations.find(ActiveSimulation::Drawing) != simulations.end())
				sim = std::move(simulations[ActiveSimulation::Drawing]);
			else
				sim = std::make_unique<DrawingSimulation>(config->DrawingSimConfig);
		}

        EndDrawing();
    }

    hotkey.Stop();
    CloseWindow();
    return 0;
}