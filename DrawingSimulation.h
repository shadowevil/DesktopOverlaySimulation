#pragma once
#include "raylib_win32.h"
#include "Helper.h"
#include "Simulation.h"
#include "Config.h"
#include <vector>
#include <string>
#include <algorithm>

extern ConfigManager configManager;

struct Stroke {
    std::vector<Vector2> points;
    Color color;
    int brushSize;
    bool highlighter;
};

class DrawingSimulation : public ISimulation {
private:
    DrawingSimulationConfig cfg;
    std::vector<Stroke> strokes;
    bool drawing = false;

    int brushSize;
    int colorIndex = 0;

    Config* config = nullptr;
    RenderTexture2D canvas;
    bool canvasInitialized = false;

public:
    DrawingSimulation(const DrawingSimulationConfig& configOverride = {})
        : cfg(configOverride) {
        width = GetScreenWidth();
        height = GetScreenHeight();
        config = configManager.GetConfig();

        brushSize = cfg.defaultBrushSize;

        colorIndex = 0;
    }

    ~DrawingSimulation() {
        if (canvasInitialized) {
            UnloadRenderTexture(canvas);
        }
    }

    void InitCanvas() {
        if (!canvasInitialized) {
            canvas = LoadRenderTexture(width, height);
            canvasInitialized = true;
            SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);

            BeginTextureMode(canvas);
            ClearBackground({ 0,0,0,0 });
            EndTextureMode();
        }
    }

    Color CurrentColor() const {
        return cfg.presetColors[colorIndex];
    }

    void CycleColor(int dir) {
        int n = (int)cfg.presetColors.size();
        if (n == 0) return;
        colorIndex = (colorIndex + dir + n) % n;
    }

    void DrawStrokeSegment(const Stroke& stroke, size_t index, RenderTexture2D& canvas) {
        if (index == 0 || index >= stroke.points.size()) return;

        Vector2 a = stroke.points[index - 1];
        Vector2 b = stroke.points[index];

        BeginTextureMode(canvas);

        Color col = stroke.highlighter
            ? Fade(stroke.color, cfg.highlighterAlpha)
            : stroke.color;

        float dx = b.x - a.x;
        float dy = b.y - a.y;
        float dist = sqrtf(dx * dx + dy * dy);

        float step = stroke.brushSize * 0.25f;
        int steps = (int)(dist / step);
        if (steps < 1) steps = 1;

        for (int i = 0; i <= steps; i++) {
            float t = i / (float)steps;
            Vector2 p = { a.x + dx * t, a.y + dy * t };
            DrawCircleV(p, (float)stroke.brushSize, col);
        }

        EndTextureMode();
    }

    void DrawStrokeEndCap(const Stroke& stroke, RenderTexture2D& canvas) {
        if (stroke.points.empty()) return;
        Vector2 end = stroke.points.back();

        BeginTextureMode(canvas);

        Color col = stroke.highlighter
            ? Fade(stroke.color, cfg.highlighterAlpha)
            : stroke.color;

        DrawCircleV(end, (float)stroke.brushSize, col);

        EndTextureMode();
    }

    void Update() override {
        InitCanvas();

        bool highlighter = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        Color currentColor = CurrentColor();

        // Ctrl+Scroll to cycle colors
        if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))) {
            int wheel = (int)GetMouseWheelMove();
            if (wheel != 0) {
                CycleColor(wheel > 0 ? 1 : -1);
            }
        }

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            if (!drawing) {
                drawing = true;
                strokes.push_back({ {}, currentColor, brushSize, highlighter });
            }

            Vector2 mousePos = GetMousePosition();
            strokes.back().points.push_back(mousePos);

            DrawStrokeSegment(strokes.back(), strokes.back().points.size() - 1, canvas);
        }
        else {
            if (drawing) {
                drawing = false;
                if (!strokes.empty()) {
                    DrawStrokeEndCap(strokes.back(), canvas);
                }
            }
        }

        if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_C)) {
            strokes.clear();
            BeginTextureMode(canvas);
            ClearBackground({ 0,0,0,0 });
            EndTextureMode();
        }

        if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP))
            brushSize = std::min(brushSize + 1, cfg.maxBrushSize);
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN))
            brushSize = std::max(brushSize - 1, cfg.minBrushSize);

		if (IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) {
			int wheel = (int)GetMouseWheelMove();
			if (wheel != 0) {
				brushSize += (wheel > 0 ? 1 : -1);
				if (brushSize < cfg.minBrushSize) brushSize = cfg.minBrushSize;
				if (brushSize > cfg.maxBrushSize) brushSize = cfg.maxBrushSize;
			}
		}
    }

    void Draw() override {
        InitCanvas();

        DrawTextureRec(
            canvas.texture,
            { 0, 0, (float)canvas.texture.width, -(float)canvas.texture.height },
            { 0, 0 },
            WHITE
        );

        if (!config->MousePassthrough) {
            Vector2 mousePos = GetMousePosition();
            bool highlighter = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

            // Brush preview
            if (highlighter) {
                DrawCircleV(mousePos, (float)brushSize, Fade(CurrentColor(), cfg.highlighterAlpha));
                DrawCircleLinesV(mousePos, (float)brushSize, Fade(CurrentColor(), 0.6f));
            }
            else {
                DrawCircleLinesV(mousePos, (float)brushSize, Fade(CurrentColor(), 0.6f));
            }

			if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL))
            {
                // Color palette preview (prev, current, next)
                int n = (int)cfg.presetColors.size();
                if (n > 0) {
                    int prevIdx = (colorIndex - 1 + n) % n;
                    int nextIdx = (colorIndex + 1) % n;

                    // Clamp preview bubble size
                    float radius = 10;

                    float spacing = radius * 2.5f;
                    Vector2 basePos = { (float)mousePos.x, (float)mousePos.y - ((radius * 1.5f) + brushSize + 2.0f) };

                    // Previous (left)
                    DrawCircleV({ basePos.x - spacing, basePos.y }, radius, cfg.presetColors[prevIdx]);
                    // Current (center, larger + outline)
                    float currentRadius = radius * 1.2f;
                    DrawCircleV(basePos, currentRadius, cfg.presetColors[colorIndex]);
                    DrawCircleLines((int)basePos.x, (int)basePos.y, currentRadius + 2, WHITE);
                    // Next (right)
                    DrawCircleV({ basePos.x + spacing, basePos.y }, radius, cfg.presetColors[nextIdx]);
                }
            }

            HideCursor();
        }
        else {
            ShowCursor();
        }
    }

    void DrawUIOverlay() override {
        if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)
            || IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) {
            DrawRectangle(10, 10, 320, 120, Color{ 0,0,0,150 });
            DrawText(TextFormat("Strokes: %d", (int)strokes.size()), 20, 20, 10, LIGHTGRAY);
            DrawText(TextFormat("Current Brush Size: %d", brushSize), 20, 35, 10, YELLOW);
            DrawText(TextFormat("Highlighter Alpha: %.2f", cfg.highlighterAlpha), 20, 50, 10, LIGHTGRAY);
            DrawText("Ctrl+Scroll to change color", 20, 65, 10, LIGHTGRAY);
            DrawText("Hold Shift for highlighter mode", 20, 80, 10, LIGHTGRAY);
            DrawText(std::string("FPS: " + std::to_string(GetFPS())).c_str(), 20, 95, 10, GREEN);
        }
    }
};