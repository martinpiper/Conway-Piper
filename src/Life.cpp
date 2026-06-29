// Life processing
#include <stdlib.h>

bool rule3D = true;
int ruleCellDiesFewerThan = 5;
int ruleCellLivesFewerThan = 7;
int ruleCellDiesMoreThan = 7;
// Gives a range for when a cell grows
int ruleCellGrowsMoreThan = 5;
int ruleCellGrowsFewerThan = 7;

static bool* willDie = nullptr;
static bool* willGrow = nullptr;
static int sWidth = 0;
static int sHeight = 0;
static int sDepth = 0;

static void setWillDie(bool flag, int w, int h, int d)
{
	willDie[w + (sWidth * h) + (sWidth * sHeight * d)] = flag;
}

static void setWillGrow(bool flag, int w, int h, int d)
{
	willGrow[w + (sWidth * h) + (sWidth * sHeight * d)] = flag;
}

void Life_Tick(int *cells, const int width, const int height, const int depth)
{
	sWidth = width;
	sHeight = height;
	sDepth = depth;

	if (!willDie)
	{
		willDie = new bool[width * height * depth];
	}
	if (!willGrow)
	{
		willGrow = new bool[width * height * depth];
	}

	// Process any state
	// Calculate what to do next
	for (int i = 1; i < width - 1; i++)
	{
		for (int j = 1; j < height - 1; j++)
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

						if (cells[(i + di) + width * (j + dj)] > 0)
						{
							neighbours++;
						}
					}
				}
				if (cells[i + width * j] > 0)
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

								if (cells[(i + di) + width * (j + dj) + width * height * (k + dk)] > 0)
								{
									neighbours++;
								}
							}
						}
					}
					if (cells[i + width * j + width * height * k] > 0)
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

	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			for (int k = 0; k < depth; k++)
			{
				if (willGrow[i + width * j + width * height * k])
				{
					cells[i + width * j + width * height * k] = 2;
					//							cells[i + width * j + width * height * k] = rand() % 9 + 1;
					willGrow[i + width * j + width * height * k] = false;
				}
				else if (willDie[i + width * j + width * height * k])
				{
					cells[i + width * j + width * height * k] = 0;
					willDie[i + width * j + width * height * k] = false;
				}
				else if (cells[i + width * j + width * height * k] > 0)
				{
					cells[i + width * j + width * height * k] = cells[i + width * j + width * height * k] + 1;
					if (cells[i + width * j + width * height * k] >= 9)
					{
						//								cells[i + width * j + width * height * k] = rand() % 9 + 1;
						cells[i + width * j + width * height * k] = 1;
					}
				}
			}
		}
	}
}
