#pragma once
#include <string>

class ISimulation
{
public:
	ISimulation() : width(800), height(600), WindowTitle("Simulation") {}

	int width;
	int height;
	std::string WindowTitle;

	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual void DrawUIOverlay() = 0;
};