#pragma once

extern bool rule3D;
extern int ruleCellDiesFewerThan;
extern int ruleCellLivesFewerThan;
extern int ruleCellDiesMoreThan;
extern int ruleCellGrowsMoreThan;
extern int ruleCellGrowsFewerThan;

extern void Life_Tick(int* cells, const int width, const int height, const int depth);
