#pragma once
#include "raylib_win32.h"
#include "Helper.h"
#include "Simulation.h"
#include "Config.h"

extern ConfigManager configManager;

class Snowflake {
public:
    Snowflake(int px, int py, Color c, int idx, int sz = 1)
        : x(px), y(py), gridIndex(idx), velocity{ 0,0 }, color(c), size(sz) {
    }

    void Draw() const {
        if (size <= 1) {
            DrawPixel(x, y, Fade(color, alpha));
        }
        else {
            DrawCircle(x, y, (float)size * 0.5f, Fade(color, alpha));
        }
    }

    int x, y;
    int gridIndex;
    Vector2 velocity;
    Color color;
    int size;

    float alpha = 1.0f;
    double landedTime = -1.0;
    double fadeStartTime = -1.0;

    // individuality
    float gravity = 0.0f;
    float windFactor = 1.0f;
    float driftX = 0.0f;
};

class SnowSimulation : public ISimulation {
private:
    std::vector<uint8_t> occupancy;
    std::vector<Snowflake> dynamicFlakes;
    std::vector<Snowflake> staticFlakes;
    //RenderTexture2D staticLayer;

    float spawnTimer = 0.0f;
    float gustTimer = 0.0f;
    float windForce = 0.0f;
    float targetWindForce = 0.0f;

public:
	SnowSimulationConfig config;

    SnowSimulation() {
        width = GetScreenWidth();
        height = GetScreenHeight();

        occupancy.resize(width * height, 0);
        //staticLayer = LoadRenderTexture(width, height);

        //BeginTextureMode(staticLayer);
        //ClearBackground(BLANK);
        //EndTextureMode();

        WindowTitle = "Snow Simulation - F2: Toggle Click-Through, Ctrl+Y: Toggle Topmost";
        SetWindowTitle(WindowTitle.c_str());

		config = configManager.GetConfig().SnowSimConfig;
    }

    ~SnowSimulation() {
        //UnloadRenderTexture(staticLayer);
    }

    void Update() override {
		width = GetScreenWidth();
		height = GetScreenHeight();

        int taskbar_height = height;
        if (configManager.GetConfig().TaskbarAware)
            taskbar_height -= configManager.GetTaskbarHeight();
        else taskbar_height -= 1;

        float dt = GetFrameTime();
        double now = GetTime();
        spawnTimer += dt;
        gustTimer += dt;

        // Smooth wind gust
        if (gustTimer > GetRandomValue(2, 5)) {
            targetWindForce = (GetRandomValue(-50, 50) / 100.0f);
            gustTimer = 0;
        }
        windForce += (targetWindForce - windForce) * 0.5f * dt;

        // Spawn flakes
        if (spawnTimer > config.SpawnInterval) {
            spawnTimer = 0.0f;
            int px = GetRandomValue(0, width - 1);
            int py = 0;
            int size = GetRandomValue(config.MinFlakeSize, config.MaxFlakeSize);

            Snowflake flake(px, py, WHITE, py * width + px, size);
            float baseFall = 0.3f + (0.6f / size);
            flake.gravity = baseFall * (GetRandomValue(80, 120) / 100.0f);
            flake.windFactor = GetRandomValue(50, 150) / 100.0f;
            flake.driftX = (GetRandomValue(-100, 100) / 100.0f) * 0.3f;
            flake.velocity = { flake.driftX, flake.gravity };

            dynamicFlakes.push_back(flake);
        }

        // Update dynamics
        std::vector<Snowflake> stillDynamic;
        Vector2 mousePos = GetCursorPosition();

        for (auto& f : dynamicFlakes) {
            // Physics
            f.velocity.x += (f.driftX * 0.1f) * dt;
            f.velocity.x += (windForce * f.windFactor) * dt;
            f.velocity.y += f.gravity * dt;

            // Mouse avoidance
            float dx = (float)f.x - mousePos.x;
            float dy = (float)f.y - mousePos.y;
            float distSq = dx * dx + dy * dy;

            if (distSq < config.MouseAvoidRadius * config.MouseAvoidRadius && distSq > 1.0f) {
                float dist = sqrtf(distSq);
                float factor = (config.MouseAvoidRadius - dist) / config.MouseAvoidRadius;
                f.velocity.x += (dx / dist) * config.MouseAvoidStrength * dt * factor;
                if (dy < 0) {
                    f.velocity.y += (dy / dist) * (config.MouseAvoidStrength * 0.2f) * dt * factor;
                }
            }

            int newX = f.x + (int)roundf(f.velocity.x);
            int newY = f.y + (int)roundf(f.velocity.y);

            bool landed = false;

            if (newY >= taskbar_height) {
                landed = true;
            }
            else {
                int idx = newY * width + newX;
                if (idx >= 0 && idx < occupancy.size() && occupancy[idx] != 0) {
                    landed = true;
                }
            }

            if (!landed) {
                // Out of bounds
                if (newX < 0 || newX >= width || newY < 0 || newY >= height) {
                    f.landedTime = now;
                    f.fadeStartTime = now; // fade immediately
                    f.alpha = 0.0f;
                    staticFlakes.push_back(f);
                    continue;
                }
                f.x = newX; f.y = newY;
                f.gridIndex = f.y * width + f.x;
                stillDynamic.push_back(f);
            }
            else {
                int idx = f.y * width + f.x;
                if (idx >= 0 && idx < occupancy.size()) {
                    occupancy[idx] = 1;
                }
                f.landedTime = now;
                f.fadeStartTime = now + config.FadeDelay;
                staticFlakes.push_back(f);
            }
        }
        dynamicFlakes = stillDynamic;

        // Fade static flakes
        for (auto& f : staticFlakes) {
            if (f.fadeStartTime > 0 && now > f.fadeStartTime) {
                f.alpha -= dt * config.FadeSpeed;
            }
        }
        staticFlakes.erase(
            std::remove_if(staticFlakes.begin(), staticFlakes.end(),
                [&](const Snowflake& f) {
                    if (f.alpha <= 0.0f) {
                        occupancy[f.gridIndex] = 0; // clear occupancy only when removing
                        return true;
                    }
                    return false;
                }),
            staticFlakes.end()
        );
    }

    void Draw() override {
        for (auto& f : staticFlakes) f.Draw();
        for (auto& f : dynamicFlakes) f.Draw();
    }

    void DrawUIOverlay() override {
        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)
            || IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) {
            DrawRectangle(10, 10, 280, 135, Color{ 0, 0, 0, 150 });
            DrawText(TextFormat("Dynamic flakes: %d", dynamicFlakes.size()), 20, 20, 10, LIGHTGRAY);
            DrawText(TextFormat("Static flakes: %d", staticFlakes.size()), 20, 35, 10, LIGHTGRAY);
            DrawText(TextFormat("MinSize: %d, MaxSize: %d", config.MinFlakeSize, config.MaxFlakeSize), 20, 50, 10, YELLOW);
            DrawText(TextFormat("SpawnInterval: %.3f", config.SpawnInterval), 20, 65, 10, YELLOW);
            DrawText(TextFormat("FadeDelay: %.0fs", config.FadeDelay), 20, 80, 10, YELLOW);
            DrawText(std::string("FPS: " + std::to_string(GetFPS())).c_str(), 20, 95, 10, GREEN);
        }
    }
};