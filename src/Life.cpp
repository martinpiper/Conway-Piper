// Life processing
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>

#include "Life.h"

Life::Life()
{
}

Life::~Life()
{
	free(willDie);
	free(willGrow);
}

void Life::Init(const int width, const int height, const int depth, const int maxLifetime)
{
	sWidth = width;
	sHeight = height;
	sDepth = depth;

	willDie = (bool*) calloc(width * height * depth, sizeof(bool));
	willGrow = (bool*) calloc(width * height * depth, sizeof(bool));

	lifetime = maxLifetime;
}

void Life::setWillDie(bool flag, int w, int h, int d)
{
	willDie[w + (sWidth * h) + (sWidth * sHeight * d)] = flag;
}

void Life::setWillGrow(bool flag, int w, int h, int d)
{
	willGrow[w + (sWidth * h) + (sWidth * sHeight * d)] = flag;
}

void Life::Tick(int *cells)
{
	int depth = sDepth;
	if (!rule3D)
	{
		depth = 1;
	}

	// Process any state
	// Calculate what to do next
	for (int i = 1; i < sWidth - 1; i++)
	{
		for (int j = 1; j < sHeight - 1; j++)
		{
			if (!rule3D)
			{
				// 2D rules processing
				int neighbours = 0;
				for (int di = -1; di <= 1; di++)
				{
					for (int dj = -1; dj <= 1; dj++)
					{
						if (di == 0 && dj == 0)
						{
							continue;
						}

						if (cells[(i + di) + sWidth * (j + dj)] > 0)
						{
							neighbours++;
						}
					}
				}
				if (cells[i + sWidth * j] > 0)
				{
					// Any live cell...
					if (neighbours < ruleCellDiesFewerThan)
					{
						setWillDie(true, i, j, 0);
					}
					else if (neighbours < ruleCellLivesFewerThan)
					{
						// Do nothing
					}
					else if (neighbours > ruleCellDiesMoreThan)
					{
						setWillDie(true, i, j, 0);
					}
				}
				else
				{
					// Any dead cell...
					if (neighbours > ruleCellGrowsMoreThan && neighbours < ruleCellGrowsFewerThan)
					{
						setWillGrow(true, i, j, 0);
					}
				}
			}
			else
			{
				// 3D rules processing
				for (int k = 1; k < depth - 1; k++)
				{
					int neighbours = 0;
					for (int di = -1; di <= 1; di++)
					{
						for (int dj = -1; dj <= 1; dj++)
						{
							for (int dk = -1; dk <= 1; dk++)
							{
								if (di == 0 && dj == 0 && dk == 0)
								{
									continue;
								}

								if (cells[(i + di) + sWidth * (j + dj) + sWidth * sHeight * (k + dk)] > 0)
								{
									neighbours++;
								}
							}
						}
					}
					if (cells[i + sWidth * j + sWidth * sHeight * k] > 0)
					{
						// Any live cell...
						if (neighbours < ruleCellDiesFewerThan)
						{
							setWillDie(true, i, j, k);
						}
						else if (neighbours < ruleCellLivesFewerThan)
						{
							// Do nothing
						}
						else if (neighbours > ruleCellDiesMoreThan)
						{
							setWillDie(true, i, j, k);
						}
					}
					else
					{
						// Any dead cell...
						if (neighbours > ruleCellGrowsMoreThan && neighbours < ruleCellGrowsFewerThan)
						{
							setWillGrow(true, i, j, k);
						}
					}
				}
			}
		}
	}

	for (int i = 0; i < sWidth; i++)
	{
		for (int j = 0; j < sHeight; j++)
		{
			for (int k = 0; k < depth; k++)
			{
				if (willGrow[i + sWidth * j + sWidth * sHeight * k])
				{
					cells[i + sWidth * j + sWidth * sHeight * k] = 2;
//					cells[i + width * j + width * height * k] = rand() % lifetime + 1;
					willGrow[i + sWidth * j + sWidth * sHeight * k] = false;
				}
				else if (willDie[i + sWidth * j + sWidth * sHeight * k])
				{
					cells[i + sWidth * j + sWidth * sHeight * k] = 0;
					willDie[i + sWidth * j + sWidth * sHeight * k] = false;
				}
				else if (cells[i + sWidth * j + sWidth * sHeight * k] > 0)
				{
					cells[i + sWidth * j + sWidth * sHeight * k] = cells[i + sWidth * j + sWidth * sHeight * k] + 1;
					if (cells[i + sWidth * j + sWidth * sHeight * k] >= lifetime)
					{
//						cells[i + width * j + width * height * k] = rand() % lifetime + 1;
						cells[i + sWidth * j + sWidth * sHeight * k] = 1;
					}
				}
			}
		}
	}
}

void Life::Clear(int* cells)
{
	memset(cells, 0, sizeof(int) * sWidth * sHeight * sDepth);
	memset(willDie, 0, sizeof(bool) * sWidth * sHeight * sDepth);
	memset(willGrow, 0, sizeof(bool) * sWidth * sHeight * sDepth);
}

std::string Life::Save(int* cells, const bool withOriginCheck, const bool justPositions)
{
	std::string ret;

	// Figure out a minimum origin to use
	int mx = sWidth, my = sHeight, mz = sDepth;
	bool found = false;
	if (withOriginCheck)
	{
		for (int x = 0; x < sWidth; x++)
		{
			for (int y = 0; y < sHeight; y++)
			{
				for (int z = 0; z < sDepth; z++)
				{
					int cell = getCell(cells, x, y, z);
					if (cell)
					{
						mx = std::min(mx, x);
						my = std::min(my, y);
						mz = std::min(mz, z);
						found = true;
					}
				}
			}
		}
	}
	// If the entire cell grid is full then reset the origin...
	if (!found)
	{
		mx = 0;
		my = 0;
		mz = 0;
	}
	for (int x = mx; x < sWidth; x++)
	{
		for (int y = my; y < sHeight; y++)
		{
			for (int z = mz; z < sDepth; z++)
			{
				int cell = getCell(cells, x, y, z);
				if (cell)
				{
					char cellBuffer[128];
					if (justPositions)
					{
						sprintf(cellBuffer, "%d,%d,%d\n", x - mx, y - my, z - mz);
					}
					else
					{
						sprintf(cellBuffer, "%d/%d,%d,%d\n", cell, x - mx, y - my, z - mz);
					}
					ret += cellBuffer;
				}
			}
		}
	}

	return ret;
}

void Life::generateShapeFromSeed(int* cells, const uint64_t seed, const int w, const int h, const int x, const int y, const int z)
{
	uint64_t bits = seed;
	for (int xp = 0; xp < w; xp++)
	{
		for (int yp = 0; yp < h; yp++)
		{
			if (bits & 1)
			{
				// Double depth
				// Store cells in the middle of the grid
				setCell(cells, 2, x + xp, y + yp, z);
			}
			bits = bits >> 1;
		}
	}

}
