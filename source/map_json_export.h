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

#ifndef RME_MAP_JSON_EXPORT_H_
#define RME_MAP_JSON_EXPORT_H_

#include "position.h"

#include <string>

class Map;
class Editor;
class CopyBuffer;

struct MapJsonExportOptions {
	bool includeEmptyTiles = false;
	bool includeSpawns = true;
	bool includeHouses = true;
	bool relativeCoords = true;
	// Collapse runs of identical plain-ground tiles into from/to rectangles.
	bool simplifyUniformGround = true;
	// Only emit fills when this fraction of scanned cells on a floor are that ground (0-100).
	int simplifyMinCoveragePercent = 50;
	int chunkSize = 128;
	// Optional client/items VERSION (spr/dat). Empty name skips writing these fields.
	std::string clientVersion;
	int clientVersionId = 0;
};

class MapJsonExporter {
public:
	static constexpr const char* FORMAT_NAME = "rme-map-patch";
	static constexpr int FORMAT_VERSION = 1;

	// Export a rectangular region (inclusive from/to) to a single JSON file.
	static bool exportRegion(Map& map, const Position& from, const Position& to, const FileName& path, const MapJsonExportOptions& options, bool showProgress = true);

	// Export a rectangular region to an in-memory JSON string (no disk I/O).
	static bool exportRegionToString(Map& map, const Position& from, const Position& to, const MapJsonExportOptions& options, std::string& outJson, bool showProgress = false);

	// Export the AABB of the current selection as a patch.
	static bool exportSelection(Editor& editor, const FileName& path, const MapJsonExportOptions& options);

	// Export selection AABB to an in-memory JSON string.
	static bool exportSelectionToString(Editor& editor, const MapJsonExportOptions& options, std::string& outJson);

	// Export the whole map as chunked JSON files into a directory.
	static bool exportMapBatched(Map& map, const FileName& directory, const wxString& baseName, const MapJsonExportOptions& options);

	static uint64_t estimateTileCount(const Position& from, const Position& to);
	static void normalizeBounds(Position& from, Position& to);
};

struct MapJsonImportOptions {
	int offsetX = 0;
	int offsetY = 0;
	int offsetZ = 0;
	bool merge = true; // merge into existing tiles (like paste merge)
	bool importSpawns = true;
	bool importHouses = true;
};

class MapJsonImporter {
public:
	// Import an rme-map-patch JSON file into the editor (undoable).
	static bool importFile(Editor& editor, const FileName& path, const MapJsonImportOptions& options, wxString& error);

	// Parse rme-map-patch JSON text into the copy buffer for paste-hand placement.
	static bool importToCopyBuffer(CopyBuffer& buffer, const std::string& jsonText, wxString& error);
};

#endif
