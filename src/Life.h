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

	void setCell(int* cells, int stage, int w, int h, int d)
	{
		cells[w + (sWidth * h) + (sWidth * sHeight * d)] = stage;
	}

	int getCell(int* cells, int w, int h, int d)
	{
		return cells[w + (sWidth * h) + (sWidth * sHeight * d)];
	}

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
