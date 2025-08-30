#pragma once
#include "raylib_win32.h"
#include "json.hpp"
using json = nlohmann::json;

#include <fstream>
#include <filesystem>

const std::filesystem::path config_file_path = "config.json";

enum class ActiveSimulation {
	Sand = 1,
	Snow = 2
};

struct SandSimulationConfig {
	float BrushRadius = 10.0f;
	int   MaxDensity = 30;

	float HueCycleSpeed = 2.0f; // degrees per second (360 / 60 = 6)
	float DensityRampRate = 40.0f;
	float MouseHoldTime = 0.0f;
	float HoldDelayTimer = 0.0f;
	float HoldDelay = 0.15f;

	float Gravity = 0.05f;
	float MaxFallSpeed = 5.0f;
	float AirResistance = 0.99f;

	float SettleThreshold = 5.0f; // seconds
};

struct SnowSimulationConfig {
	int MinFlakeSize = 1;
	int MaxFlakeSize = 6;
	float SpawnInterval = 0.01f;       // seconds between spawns
	float FadeDelay = 180.0f;          // seconds before fade starts
	float FadeSpeed = 0.05f;           // alpha drop per second
	float MouseAvoidRadius = 75.0f;         // mouse avoid radius
	float MouseAvoidStrength = 6.0f;        // mouse avoidance force
};

struct Config {
	int ActiveMonitor = -1; // -1 means current monitor
	bool MousePassthrough = true;
	bool TopMost = true;
	bool TaskbarAware = true;
	int TargetFPS = 60;
	ActiveSimulation ActiveSim = ActiveSimulation::Sand;
	SnowSimulationConfig SnowSimConfig = {};
	SandSimulationConfig SandSimConfig = {};
};

class ConfigManager {
public:
	ConfigManager() {
		LoadConfig();
	}

	int GetTaskbarHeight() {
		if(taskbar_height <= 0)
			taskbar_height = ::GetTaskbarHeight(config.ActiveMonitor);

		return taskbar_height;
	}


	Config GetConfig() const { return config; }

	void SaveConfig() {
		json j;
		j["ActiveMonitor"] = config.ActiveMonitor;
		j["MousePassthrough"] = config.MousePassthrough;
		j["TopMost"] = config.TopMost;
		j["TaskbarAware"] = config.TaskbarAware;
		j["TargetFPS"] = config.TargetFPS;
		j["ActiveSim"] = static_cast<int>(config.ActiveSim);
		j["SnowSimConfig"]["MinFlakeSize"] = config.SnowSimConfig.MinFlakeSize;
		j["SnowSimConfig"]["MaxFlakeSize"] = config.SnowSimConfig.MaxFlakeSize;
		j["SnowSimConfig"]["SpawnInterval"] = config.SnowSimConfig.SpawnInterval;
		j["SnowSimConfig"]["FadeDelay"] = config.SnowSimConfig.FadeDelay;
		j["SnowSimConfig"]["FadeSpeed"] = config.SnowSimConfig.FadeSpeed;
		j["SnowSimConfig"]["MouseAvoidRadius"] = config.SnowSimConfig.MouseAvoidRadius;
		j["SnowSimConfig"]["MouseAvoidStrength"] = config.SnowSimConfig.MouseAvoidStrength;
		j["SandSimConfig"]["BrushRadius"] = config.SandSimConfig.BrushRadius;
		j["SandSimConfig"]["MaxDensity"] = config.SandSimConfig.MaxDensity;
		j["SandSimConfig"]["HueCycleSpeed"] = config.SandSimConfig.HueCycleSpeed;
		j["SandSimConfig"]["DensityRampRate"] = config.SandSimConfig.DensityRampRate;
		j["SandSimConfig"]["MouseHoldTime"] = config.SandSimConfig.MouseHoldTime;
		j["SandSimConfig"]["HoldDelayTimer"] = config.SandSimConfig.HoldDelayTimer;
		j["SandSimConfig"]["HoldDelay"] = config.SandSimConfig.HoldDelay;
		j["SandSimConfig"]["Gravity"] = config.SandSimConfig.Gravity;
		j["SandSimConfig"]["MaxFallSpeed"] = config.SandSimConfig.MaxFallSpeed;
		j["SandSimConfig"]["AirResistance"] = config.SandSimConfig.AirResistance;
		j["SandSimConfig"]["SettleThreshold"] = config.SandSimConfig.SettleThreshold;

		std::ofstream file(config_file_path);
		if (file.is_open()) {
			file << j.dump(4);
			file.close();
		}
	}

	void LoadConfig() {
		if (std::filesystem::exists(config_file_path)) {
			std::ifstream file(config_file_path);
			if (file.is_open()) {
				json j;
				file >> j;
				config.ActiveMonitor = j.value("ActiveMonitor", -1);
				config.MousePassthrough = j.value("MousePassthrough", true);
				config.TopMost = j.value("TopMost", true);
				config.TaskbarAware = j.value("TaskbarAware", true);
				config.TargetFPS = j.value("TargetFPS", 60);
				config.ActiveSim = static_cast<ActiveSimulation>(j.value("ActiveSim", 1));
				config.SnowSimConfig.MinFlakeSize = j["SnowSimConfig"].value("MinFlakeSize", 1);
				config.SnowSimConfig.MaxFlakeSize = j["SnowSimConfig"].value("MaxFlakeSize", 6);
				config.SnowSimConfig.SpawnInterval = j["SnowSimConfig"].value("SpawnInterval", 0.01f);
				config.SnowSimConfig.FadeDelay = j["SnowSimConfig"].value("FadeDelay", 180.0f);
				config.SnowSimConfig.FadeSpeed = j["SnowSimConfig"].value("FadeSpeed", 0.05f);
				config.SnowSimConfig.MouseAvoidRadius = j["SnowSimConfig"].value("MouseAvoidRadius", 75.0f);
				config.SnowSimConfig.MouseAvoidStrength = j["SnowSimConfig"].value("MouseAvoidStrength", 6.0f);
				config.SandSimConfig.BrushRadius = j["SandSimConfig"].value("BrushRadius", 10.0f);
				config.SandSimConfig.MaxDensity = j["SandSimConfig"].value("MaxDensity", 30);
				config.SandSimConfig.HueCycleSpeed = j["SandSimConfig"].value("HueCycleSpeed", 2.0f);
				config.SandSimConfig.DensityRampRate = j["SandSimConfig"].value("DensityRampRate", 40.0f);
				config.SandSimConfig.MouseHoldTime = j["SandSimConfig"].value("MouseHoldTime", 0.0f);
				config.SandSimConfig.HoldDelayTimer = j["SandSimConfig"].value("HoldDelayTimer", 0.0f);
				config.SandSimConfig.HoldDelay = j["SandSimConfig"].value("HoldDelay", 0.15f);
				config.SandSimConfig.Gravity = j["SandSimConfig"].value("Gravity", 0.05f);
				config.SandSimConfig.MaxFallSpeed = j["SandSimConfig"].value("MaxFallSpeed", 5.0f);
				config.SandSimConfig.AirResistance = j["SandSimConfig"].value("AirResistance", 0.99f);
				config.SandSimConfig.SettleThreshold = j["SandSimConfig"].value("SettleThreshold", 5.0f);

				file.close();
			}
		}
		else {
			// Save default config if file doesn't exist
			SaveConfig();
			LoadConfig();
		}
	}

private:
	Config config;
	int taskbar_height = 0;
};