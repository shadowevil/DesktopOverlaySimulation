#pragma once
#include "raylib_win32.h"
#include "Config.h"
#include "Helper.h"
#include "Simulation.h"
#include <vector>
#include <random>

extern ConfigManager configManager;

class GrainOfSand {
public:
	GrainOfSand(int px, int py, Color c, int idx)
		: x(px), y(py), gridIndex(idx), velocity{ 0,0 }, color(c) {
	}

	void Draw() const {
		DrawPixel(x, y, color);
	}

	int x, y;
	int gridIndex;
	Vector2 velocity;
	Color color;

	float stillTime = 0.0f;   // NEW: time spent without moving
	int lastX = -1, lastY = -1;         // NEW: track last position
};

extern std::mt19937 gen;
extern std::uniform_real_distribution<float> dist01;

class SandSimulation : public ISimulation {
public:
    SandSimulationConfig config;

private:
    int taskbar_height = 0;

    std::vector<uint8_t> occupancy; // 0=empty, 1=occupied
    std::vector<GrainOfSand> grains;
    RenderTexture2D staticLayer;

public:
    SandSimulation()
        : ISimulation() {
		width = GetScreenWidth();
		height = GetScreenHeight();
        WindowTitle = "Sand Simulation - F2: Toggle Click-Through, Ctrl+Y: Toggle Topmost";

        occupancy.resize(width * height, 0);
        staticLayer = LoadRenderTexture(width, height);

        BeginTextureMode(staticLayer);
        ClearBackground(BLANK);
        EndTextureMode();

		SetWindowTitle(WindowTitle.c_str());

        config = configManager.GetConfig()->SandSimConfig;
    }

	~SandSimulation() {
		UnloadRenderTexture(staticLayer);
	}

private:
    void SpawnFountain(Vector2 mousePos, int density, Color color, int width) {
        float minExplosionSpeed = 2.0f;
        float maxExplosionSpeed = 5.0f;
        float spread = PI / 2.0f;
        float tilt = PI / 3.0f;

        for (int i = 0; i < density; i++) {
            float angleOffset = dist01(gen) * 2.0f * PI;
            float dist = sqrtf(dist01(gen)) * config.BrushRadius;
            int px = (int)(mousePos.x + cosf(angleOffset) * dist);
            int py = (int)(mousePos.y + sinf(angleOffset) * dist);

            if (px < 0 || py < 0 || px >= width) continue;

            float side = (dist01(gen) < 0.5f) ? PI : 2.0f * PI;
            float t = pow(dist01(gen), 1.5f);
            float angle = side - tilt - spread / 2.0f + t * spread;

            float speed = minExplosionSpeed + dist01(gen) * (maxExplosionSpeed - minExplosionSpeed);

            int idx = py * width + px;
            if (idx < 0 || idx >= (int)occupancy.size()) continue;

            if (occupancy[idx]) continue; // skip if already occupied

            GrainOfSand grain(px, py, color, idx);
            grain.velocity.x = cosf(angle) * speed;
            grain.velocity.y = sinf(angle) * speed;

            if (grain.velocity.y < 0.5f)
                grain.velocity.y = 0.5f + dist01(gen) * 1.0f;

            occupancy[idx] = 1;
            grains.push_back(grain);
        }
    }

    //--------------------------------------------------------------------------------------
    // Update sand positions with gravity + stacking
    //--------------------------------------------------------------------------------------
    void UpdateGrains(int width, int height) {
        std::vector<GrainOfSand> stillDynamic;
        stillDynamic.reserve(grains.size());

        float dt = GetFrameTime();

        for (auto& grain : grains) {
            // physics
            grain.velocity.y += config.Gravity;
            if (grain.velocity.y > config.MaxFallSpeed) grain.velocity.y = config.MaxFallSpeed;
            grain.velocity.x *= config.AirResistance;
            grain.velocity.x += (dist01(gen) - 0.5f) * 0.05f;
            grain.velocity.y += (dist01(gen) - 0.5f) * 0.02f;

            int gx = grain.x;
            int gy = grain.y;
            int idx = grain.gridIndex;

            int steps = (int)roundf(std::max(1.0f, grain.velocity.y));
            int newX = gx;
            int newY = gy;
            int newIdx = idx;

            bool moved = false;
            for (int s = 0; s < steps; s++) {
                if (newY + 1 >= taskbar_height) break;

                int idxBelow = newIdx + width;
                int idxBelowLeft = (newX > 0) ? newIdx + width - 1 : -1;
                int idxBelowRight = (newX < width - 1) ? newIdx + width + 1 : -1;

                if (idxBelow >= 0 && idxBelow < (int)occupancy.size() && occupancy[idxBelow] == 0) {
                    newY++; newIdx = idxBelow; moved = true;
                }
                else if (idxBelowLeft >= 0 && occupancy[idxBelowLeft] == 0) {
                    newX--; newY++; newIdx = idxBelowLeft; moved = true;
                }
                else if (idxBelowRight >= 0 && occupancy[idxBelowRight] == 0) {
                    newX++; newY++; newIdx = idxBelowRight; moved = true;
                }
                else break;
            }

            if (moved) {
                // reset still timer
                grain.stillTime = 0.0f;
                grain.lastX = newX;
                grain.lastY = newY;

                occupancy[idx] = 0;
                occupancy[newIdx] = 1;
                grain.x = newX;
                grain.y = newY;
                grain.gridIndex = newIdx;
                stillDynamic.push_back(grain);
            }
            else {
                // stayed in same spot
                if (grain.x == grain.lastX && grain.y == grain.lastY) {
                    grain.stillTime += dt;
                }
                else {
                    grain.stillTime = 0.0f;
                    grain.lastX = grain.x;
                    grain.lastY = grain.y;
                }

                if (grain.stillTime >= config.SettleThreshold) {
                    // finally settle to static
                    BeginTextureMode(staticLayer);
                    DrawPixel(grain.x, grain.y, grain.color);
                    EndTextureMode();
                    occupancy[idx] = 1;
                }
                else {
                    stillDynamic.push_back(grain);
                }
            }
        }

        grains.swap(stillDynamic);
    }

    //--------------------------------------------------------------------------------------
    // Draw all grains
    //--------------------------------------------------------------------------------------
    void DrawGrains() {
        // draw static layer once
        DrawTextureRec(staticLayer.texture, { 0, 0, (float)GetRenderWidth(), -(float)GetRenderHeight() }, { 0, 0 }, WHITE);

        // draw only dynamic grains
        for (const auto& grain : grains) {
            grain.Draw();
        }
    }

public:
    void Update() override {
        width = GetScreenWidth();
        height = GetScreenHeight();

        taskbar_height = height;
        if (configManager.GetConfig()->TaskbarAware)
            taskbar_height -= configManager.GetTaskbarHeight();
        else taskbar_height -= 1;

        int time_seconds = (int)GetTime();
        int wheel = (int)GetMouseWheelMove();

        if (wheel != 0) {
            if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
                // Ctrl modifies density
                config.MaxDensity += wheel;
                if (config.MaxDensity < 1) config.MaxDensity = 1;
                if (config.MaxDensity > 100) config.MaxDensity = 100; // cap to something reasonable
            }
            else if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) {
                // Alt modifies brush radius
                config.BrushRadius += wheel;
                if (config.BrushRadius < 1.0f) config.BrushRadius = 1.0f;
                if (config.BrushRadius > 100.0f) config.BrushRadius = 100.0f;
            }
        }


        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mousePos = GetMousePosition();
            //float hue = fmodf(time_seconds * hueCycleSpeed, 360.0f);
            //Color color = HsvToColor(hue, 1.0f, 1.0f);

            float hueSand = 45.0f; // yellow-tan hue
            Color color = ShadeCycle(hueSand, (float)GetTime());

            SpawnFountain(mousePos, 1, color, width);

            config.HoldDelayTimer = config.HoldDelay;
            config.MouseHoldTime = 0.0f;
        } else if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            if (config.HoldDelayTimer > 0.0f) {
                config.HoldDelayTimer -= GetFrameTime();
            }
            else {
                config.MouseHoldTime += GetFrameTime();
                int density = 1 + (int)std::min((float)config.MaxDensity, config.MouseHoldTime * config.DensityRampRate);

                Vector2 mousePos = GetMousePosition();
                //float hue = fmodf(time_seconds * hueCycleSpeed, 360.0f);
                //Color color = HsvToColor(hue, 1.0f, 1.0f);
                float hueSand = 45.0f; // yellow-tan hue
                Color color = ShadeCycle(hueSand, (float)GetTime());

                SpawnFountain(mousePos, density, color, width);
            }
        }
        else {
            config.MouseHoldTime = 0.0f;
            config.HoldDelayTimer = 0.0f;
        }

        UpdateGrains(width, height);
    }

    void Draw() override {
        DrawGrains();
    }

    void DrawUIOverlay() override {
        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)
            || IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) {
            DrawRectangle(10, 10, 220, 115, Color{ 0, 0, 0, 150 });
            DrawText("Mouse Wheel: Change Brush Size", 20, 20, 10, LIGHTGRAY);
            DrawText("Ctrl + Wheel: Change Max Density", 20, 35, 10, LIGHTGRAY);
            DrawText("Alt + Wheel: Change Brush Size", 20, 50, 10, LIGHTGRAY);
            DrawText(TextFormat("Brush Size: %.1f", config.BrushRadius), 20, 65, 10, YELLOW);
            DrawText(TextFormat("Max Density: %d", config.MaxDensity), 20, 80, 10, YELLOW);
            DrawText(std::string("FPS: " + std::to_string(GetFPS())).c_str(), 20, 95, 10, GREEN);
        }
    }
};