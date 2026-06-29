#pragma once

class Life
{
public:
	Life();
	virtual ~Life();
	bool rule3D = true;
	int ruleCellDiesFewerThan = 5;
	int ruleCellLivesFewerThan = 7;
	int ruleCellDiesMoreThan = 7;
	// Gives a range for when a cell grows
	int ruleCellGrowsMoreThan = 5;
	int ruleCellGrowsFewerThan = 7;

	void Init(const int width, const int height, const int depth, const int maxLifetime);
	void Tick(int* cells);

private:
	bool* willDie = nullptr;
	bool* willGrow = nullptr;
	int sWidth = 0;
	int sHeight = 0;
	int sDepth = 0;
	int lifetime = 0;

	void setWillDie(bool flag, int w, int h, int d);
	void setWillGrow(bool flag, int w, int h, int d);
};
