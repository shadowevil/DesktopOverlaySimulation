#pragma once
#include "raylib_win32.h"
#include "Config.h"
#include "Helper.h"
#include "Simulation.h"
#include <vector>
#include <random>
#include <cmath>

extern ConfigManager configManager;
extern std::mt19937 gen;
extern std::uniform_real_distribution<float> dist01;

// ======================================================
// Spark
// ======================================================
struct Spark {
    float x, y;
    float vx, vy;
    float life;
    float r, g, b;

    static constexpr int TRAIL_MAX = 10; // shorter trail for performance
    Vector2 trail[TRAIL_MAX];
    int trailCount = 0;
    int trailHead = 0;

    Spark(float sx, float sy) {
        x = sx; y = sy;
        float angle = dist01(gen) * 2.0f * PI;
        float speed = dist01(gen) * 4.0f + 1.0f;
        vx = cosf(angle) * speed;
        vy = sinf(angle) * speed;
        life = dist01(gen) * 1.0f + 1.0f;
        r = dist01(gen);
        g = dist01(gen);
        b = dist01(gen);

        // Initialize trail properly
        for (int i = 0; i < TRAIL_MAX; i++) {
            trail[i] = { x, y };
        }
        trailCount = 1;
        trailHead = 0;
    }

    void AddTrail() {
        trail[trailHead] = { x, y };
        trailHead = (trailHead + 1) % TRAIL_MAX;
        if (trailCount < TRAIL_MAX) trailCount++;
    }

    bool Update(float dt) {
        AddTrail();

        vy += 1.5f * dt; // gravity
        x += vx;
        y += vy;
        life -= dt;

        return life > 0.0f;
    }

    void Draw() const {
        // Only draw if we have at least 2 valid trail points
        if (trailCount > 1) {
            int idx = trailHead;
            for (int i = 1; i < trailCount; i++) {
                int prev = (idx + TRAIL_MAX - 1) % TRAIL_MAX;
                float alpha = (float)i / trailCount;
                Color tc = {
                    (unsigned char)(r * 255),
                    (unsigned char)(g * 255),
                    (unsigned char)(b * 255),
                    (unsigned char)(alpha * 180)
                };
                DrawLineV(trail[prev], trail[idx], tc);
                idx = (idx + 1) % TRAIL_MAX;
            }
        }

        // Core spark
        Color c = { 255, 255, 255, (unsigned char)(Clamp(life * 255, 0.0f, 255.0f)) };
        DrawCircle((int)x, (int)y, 2, c);
    }
};

// ======================================================
// Firework
// ======================================================
struct Firework {
    float x, y;
    float tx, ty;
    float vx, vy;
    bool exploded = false;
    bool popping = false;
    float popTimer = 0.0f;

    std::vector<Spark> sparks; // sparks owned by this firework

    Firework(int startX, int startY, int targetX, int targetY) {
        x = (float)startX; y = (float)startY;
        tx = (float)targetX; ty = (float)targetY;

        float dx = tx - x, dy = ty - y;
        float t = 60.0f; // "time" to reach target
        vx = dx / t;
        float g = 0.15f;
        vy = (dy - 0.5f * g * t * t) / t;
    }

    bool Update(float dt) {
        if (!exploded && !popping) {
            vy += 0.15f;
            x += vx; y += vy;

            if ((vx >= 0 && x >= tx) || (vx <= 0 && x <= tx)) {
                if ((vy >= 0 && y >= ty) || (vy <= 0 && y <= ty)) {
                    popping = true;
                    popTimer = 0.2f;
                }
            }
        }
        else if (popping) {
            popTimer -= dt;
            if (popTimer <= 0.0f) DoExplode();
        }
        else {
            for (size_t i = 0; i < sparks.size();) {
                if (!sparks[i].Update(dt)) {
                    sparks[i] = sparks.back();
                    sparks.pop_back();
                }
                else {
                    ++i;
                }
            }
            if (sparks.empty()) return true; // finished
        }
        return false;
    }

    void Draw() const {
        if (!exploded && !popping) {
            DrawCircle((int)x, (int)y, 2, YELLOW);
        }
        else if (popping) {
            float progress = 1.0f - (popTimer / 0.2f);
            float radius = 30.0f * progress;

            Color c1 = { 255,255,255,(unsigned char)(255 * (1.0f - progress)) };
            Color c2 = { 255,255,0,(unsigned char)(200 * (1.0f - progress)) };
            Color c3 = { 255,200,50,(unsigned char)(120 * (1.0f - progress)) };
            DrawCircle((int)x, (int)y, radius * 0.4f, c1);
            DrawCircle((int)x, (int)y, radius * 0.7f, c2);
            DrawCircle((int)x, (int)y, radius, c3);
        }
        else {
            for (auto const& s : sparks) s.Draw();
        }
    }

private:
    void DoExplode() {
        popping = false; exploded = true;
        int count = (int)(dist01(gen) * 25 + 25); // 25–50 sparks
        sparks.reserve(count);
        for (int i = 0; i < count; i++) sparks.emplace_back(x, y);
    }
};

// ======================================================
// Spawner
// ======================================================
class Spawner {
public:
    void TrySpawn(std::vector<Firework>& fireworks, int width, int height) {
        if (IsMouseButtonPressedGlobal(MOUSE_LEFT_BUTTON)) {
            Vector2 mouse = GetCursorPosition();
            fireworks.emplace_back(width / 2, height, (int)mouse.x, (int)mouse.y);
            return;
        }
        if (dist01(gen) < 0.01f) {
            Vector2 mouse = GetCursorPosition();
            fireworks.emplace_back(width / 2, height, (int)mouse.x, (int)mouse.y);
        }
    }
};

// ======================================================
// FireworksSimulation
// ======================================================
class FireworksSimulation : public ISimulation {
public:
    FireworksSimulation() {
        width = GetScreenWidth();
        height = GetScreenHeight();
        WindowTitle = "Fireworks Simulation - F2: Toggle Click-Through, Ctrl+Y: Toggle Topmost";
        SetWindowTitle(WindowTitle.c_str());
    }

    void Update() override {
        float dt = GetFrameTime();
        spawner.TrySpawn(fireworks, width, height);

        for (size_t i = 0; i < fireworks.size();) {
            if (fireworks[i].Update(dt))
                fireworks[i] = fireworks.back(), fireworks.pop_back();
            else
                ++i;
        }
    }

    void Draw() override {
        for (auto& fw : fireworks)
            fw.Draw();
    }

    void DrawUIOverlay() override {}

private:
    std::vector<Firework> fireworks;
    Spawner spawner;
};