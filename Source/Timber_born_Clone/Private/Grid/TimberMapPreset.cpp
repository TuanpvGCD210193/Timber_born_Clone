#include "Timber_born_Clone/Public/Grid/TimberMapPreset.h"

UTimberMapPreset::UTimberMapPreset()
{
	MapName = TEXT("Custom Hand-Crafted Map");
	Description = TEXT("Bản đồ được thiết kế và nướng bằng Timber Level Design Tool.");
	GridSizeX = 48;
	GridSizeY = 24;
	GridSizeZ = 16;
	CellSize = 100.0f;
}

