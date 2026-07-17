//////////////////////////////////////////////////////////////////////
// This file is part of Remere's Map Editor
//////////////////////////////////////////////////////////////////////
// Remere's Map Editor is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Remere's Map Editor is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////

#ifndef RME_VIEWPORT_Z_H_
#define RME_VIEWPORT_Z_H_

#include "definitions.h"
#include "settings.h"

#include <algorithm>

// Viewport Z layout preference (Config::VIEWPORT_MODE).
// Classic keeps the original Tibia 0..15 model (ground = 7).
// Void uses the extended 0..255 model:
//   70      = ground / sea floor
//   71..255 = underground (±2 awareness)
//   0..69   = surface / sky (±6 toward sky & ground)

enum ViewportMode {
	VIEWPORT_CLASSIC = 0,
	VIEWPORT_VOID = 1,
};

namespace ViewportZ {
	constexpr int VOID_GROUND_LAYER = 70;
	constexpr int VOID_SURFACE_AWARENESS = 6;
	constexpr int UNDERGROUND_AWARENESS = 2;

	inline int GetMode() {
		return g_settings.getInteger(Config::VIEWPORT_MODE);
	}

	inline bool IsVoid() {
		return GetMode() == VIEWPORT_VOID;
	}

	inline int GetGroundLayer() {
		return IsVoid() ? VOID_GROUND_LAYER : GROUND_LAYER;
	}

	inline int GetUndergroundStart() {
		return GetGroundLayer() + 1;
	}

	inline bool IsUnderground(int z) {
		return z > GetGroundLayer();
	}

	inline bool IsSurfaceOrSky(int z) {
		return z <= GetGroundLayer();
	}

	// Isometric screen offset for a floor relative to the ground plane.
	inline int GetFloorPixelOffset(int map_z) {
		if (IsUnderground(map_z)) {
			return 0;
		}
		return TileSize * (GetGroundLayer() - map_z);
	}

	// Pixel offset used while drawing map_z relative to the focused floor.
	inline int GetDrawPixelOffset(int map_z, int focus_floor) {
		if (IsSurfaceOrSky(map_z)) {
			return TileSize * (GetGroundLayer() - map_z);
		}
		return TileSize * (focus_floor - map_z);
	}

	// Compute which Z range the drawer should iterate.
	// shade_z is the focused floor (where the shade overlay is drawn).
	// Tile drawing uses map_z >= tile_end_z (same as classic end_z).
	inline void GetDrawFloorRange(int floor, bool show_all_floors, int& start_z, int& tile_end_z, int& superend_z, int& shade_z) {
		const int ground = GetGroundLayer();
		shade_z = floor;

		if (!show_all_floors) {
			start_z = floor;
			tile_end_z = floor;
			superend_z = floor;
			return;
		}

		if (IsSurfaceOrSky(floor)) {
			if (IsVoid()) {
				start_z = std::min(ground, floor + VOID_SURFACE_AWARENESS);
				tile_end_z = std::max(0, floor - VOID_SURFACE_AWARENESS);
				superend_z = tile_end_z;
			} else {
				start_z = ground;
				tile_end_z = floor;
				superend_z = 0;
			}
		} else {
			start_z = std::min(MAP_MAX_LAYER, floor + UNDERGROUND_AWARENESS);
			if (IsVoid()) {
				tile_end_z = std::max(GetUndergroundStart(), floor - UNDERGROUND_AWARENESS);
				superend_z = tile_end_z;
			} else {
				tile_end_z = floor;
				superend_z = GetUndergroundStart();
			}
		}
	}
}

#endif
