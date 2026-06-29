// Life processing
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
