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

#include "main.h"

#include "map_json_export.h"

#include "editor.h"
#include "map.h"
#include "tile.h"
#include "item.h"
#include "complexitem.h"
#include "creature.h"
#include "spawn.h"
#include "house.h"
#include "gui.h"
#include "json.h"
#include "action.h"
#include "copybuffer.h"
#include "basemap.h"

#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>

namespace {

json::mObject serializePosition(const Position& pos) {
	json::mObject obj;
	obj["x"] = json::mValue(pos.x);
	obj["y"] = json::mValue(pos.y);
	obj["z"] = json::mValue(pos.z);
	return obj;
}

Position toExportPosition(const Position& absolute, const Position& origin, bool relative) {
	if (!relative) {
		return absolute;
	}
	return Position(absolute.x - origin.x, absolute.y - origin.y, absolute.z - origin.z);
}

json::mObject serializeItem(const Item* item) {
	json::mObject obj;
	if (!item) {
		return obj;
	}

	obj["id"] = json::mValue(static_cast<int>(item->getID()));

	if (item->hasSubtype()) {
		obj["subtype"] = json::mValue(static_cast<int>(item->getSubtype()));
	}

	const uint16_t aid = item->getActionID();
	if (aid != 0) {
		obj["aid"] = json::mValue(static_cast<int>(aid));
	}

	const uint16_t uid = item->getUniqueID();
	if (uid != 0) {
		obj["uid"] = json::mValue(static_cast<int>(uid));
	}

	const uint16_t tier = item->getTier();
	if (tier != 0) {
		obj["tier"] = json::mValue(static_cast<int>(tier));
	}

	const std::string text = item->getText();
	if (!text.empty()) {
		obj["text"] = json::mValue(text);
	}

	const std::string desc = item->getDescription();
	if (!desc.empty()) {
		obj["desc"] = json::mValue(desc);
	}

	// Remaining custom attributes (skip ones already written above)
	ItemAttributeMap attrs = item->getAttributes();
	if (!attrs.empty()) {
		json::mObject attrObj;
		for (ItemAttributeMap::const_iterator it = attrs.begin(); it != attrs.end(); ++it) {
			const std::string& key = it->first;
			if (key == "aid" || key == "uid" || key == "tier" || key == "text" || key == "desc") {
				continue;
			}

			const ItemAttribute& attr = it->second;
			if (const std::string* s = attr.getString()) {
				attrObj[key] = json::mValue(*s);
			} else if (const int32_t* i = attr.getInteger()) {
				attrObj[key] = json::mValue(static_cast<int>(*i));
			} else if (const double* f = attr.getFloat()) {
				attrObj[key] = json::mValue(*f);
			} else if (const bool* b = attr.getBoolean()) {
				attrObj[key] = json::mValue(*b);
			}
		}
		if (!attrObj.empty()) {
			obj["attributes"] = json::mValue(attrObj);
		}
	}

	if (const Teleport* teleport = dynamic_cast<const Teleport*>(item)) {
		obj["type"] = json::mValue("teleport");
		obj["destination"] = json::mValue(serializePosition(teleport->getDestination()));
	} else if (const Door* door = dynamic_cast<const Door*>(item)) {
		obj["type"] = json::mValue("door");
		obj["doorid"] = json::mValue(static_cast<int>(door->getDoorID()));
	} else if (const Depot* depot = dynamic_cast<const Depot*>(item)) {
		obj["type"] = json::mValue("depot");
		obj["depotid"] = json::mValue(static_cast<int>(depot->getDepotID()));
	} else if (Podium* podium = dynamic_cast<Podium*>(const_cast<Item*>(item))) {
		obj["type"] = json::mValue("podium");
		const Outfit& outfit = podium->getOutfit();
		json::mObject outfitObj;
		outfitObj["looktype"] = json::mValue(outfit.lookType);
		outfitObj["lookitem"] = json::mValue(outfit.lookItem);
		outfitObj["lookmount"] = json::mValue(outfit.lookMount);
		outfitObj["lookaddon"] = json::mValue(outfit.lookAddon);
		outfitObj["lookhead"] = json::mValue(outfit.lookHead);
		outfitObj["lookbody"] = json::mValue(outfit.lookBody);
		outfitObj["looklegs"] = json::mValue(outfit.lookLegs);
		outfitObj["lookfeet"] = json::mValue(outfit.lookFeet);
		outfitObj["lookmounthead"] = json::mValue(outfit.lookMountHead);
		outfitObj["lookmountbody"] = json::mValue(outfit.lookMountBody);
		outfitObj["lookmountlegs"] = json::mValue(outfit.lookMountLegs);
		outfitObj["lookmountfeet"] = json::mValue(outfit.lookMountFeet);
		obj["outfit"] = json::mValue(outfitObj);
		obj["direction"] = json::mValue(static_cast<int>(podium->getDirection()));
		obj["show_outfit"] = json::mValue(podium->hasShowOutfit());
		obj["show_mount"] = json::mValue(podium->hasShowMount());
		obj["show_platform"] = json::mValue(podium->hasShowPlatform());
	} else if (Container* container = dynamic_cast<Container*>(const_cast<Item*>(item))) {
		obj["type"] = json::mValue("container");
		json::mArray contents;
		const ItemVector& vec = container->getVector();
		for (Item* content : vec) {
			if (!content || content->isMetaItem()) {
				continue;
			}
			contents.push_back(serializeItem(content));
		}
		if (!contents.empty()) {
			obj["items"] = json::mValue(contents);
		}
	}

	return obj;
}

json::mObject serializeTile(const Tile* tile, const Position& origin, bool relativeCoords, bool includeSpawns) {
	json::mObject obj;
	const Position abs = tile->getPosition();
	const Position pos = toExportPosition(abs, origin, relativeCoords);
	obj["x"] = json::mValue(pos.x);
	obj["y"] = json::mValue(pos.y);
	obj["z"] = json::mValue(pos.z);

	if (tile->isHouseTile()) {
		obj["house_id"] = json::mValue(static_cast<int>(tile->getHouseID()));
	}

	const uint16_t mapFlags = tile->getMapFlags();
	if (mapFlags != 0) {
		obj["flags"] = json::mValue(static_cast<int>(mapFlags));
	}

	const std::vector<uint16_t>& zones = tile->getZoneIds();
	if (!zones.empty()) {
		json::mArray zoneArr;
		for (uint16_t zoneId : zones) {
			zoneArr.push_back(json::mValue(static_cast<int>(zoneId)));
		}
		obj["zones"] = json::mValue(zoneArr);
	}

	if (const HouseExitList* exits = tile->getHouseExits()) {
		if (!exits->empty()) {
			json::mArray exitArr;
			for (uint32_t houseId : *exits) {
				exitArr.push_back(json::mValue(static_cast<int>(houseId)));
			}
			obj["house_exits"] = json::mValue(exitArr);
		}
	}

	if (tile->ground && !tile->ground->isMetaItem()) {
		obj["ground"] = json::mValue(serializeItem(tile->ground));
	}

	json::mArray items;
	for (Item* item : tile->items) {
		if (!item || item->isMetaItem()) {
			continue;
		}
		items.push_back(serializeItem(item));
	}
	if (!items.empty()) {
		obj["items"] = json::mValue(items);
	}

	if (includeSpawns) {
		if (tile->spawn) {
			json::mObject spawnObj;
			spawnObj["radius"] = json::mValue(tile->spawn->getSize());
			obj["spawn"] = json::mValue(spawnObj);
		}
		if (tile->creature) {
			json::mObject creatureObj;
			creatureObj["name"] = json::mValue(tile->creature->getName());
			creatureObj["spawntime"] = json::mValue(tile->creature->getSpawnTime());
			creatureObj["direction"] = json::mValue(static_cast<int>(tile->creature->getDirection()));
			obj["creature"] = json::mValue(creatureObj);
		}
	}

	return obj;
}

bool tileIntersectsRegion(const Position& pos, const Position& from, const Position& to) {
	return pos.x >= from.x && pos.x <= to.x
		&& pos.y >= from.y && pos.y <= to.y
		&& pos.z >= from.z && pos.z <= to.z;
}

json::mArray serializeHouses(Map& map, const Position& from, const Position& to, const Position& origin, bool relativeCoords) {
	json::mArray houses;
	std::set<uint32_t> seen;

	for (int z = from.z; z <= to.z; ++z) {
		for (int y = from.y; y <= to.y; ++y) {
			for (int x = from.x; x <= to.x; ++x) {
				Tile* tile = map.getTile(x, y, z);
				if (!tile || !tile->isHouseTile()) {
					continue;
				}
				seen.insert(tile->getHouseID());
			}
		}
	}

	for (uint32_t houseId : seen) {
		House* house = map.houses.getHouse(houseId);
		if (!house) {
			continue;
		}

		json::mObject houseObj;
		houseObj["id"] = json::mValue(static_cast<int>(house->getID()));
		houseObj["name"] = json::mValue(house->name);
		houseObj["townid"] = json::mValue(static_cast<int>(house->townid));
		houseObj["rent"] = json::mValue(house->rent);
		houseObj["guildhall"] = json::mValue(house->guildhall);

		const Position exit = house->getExit();
		if (exit != Position()) {
			houseObj["exit"] = json::mValue(serializePosition(toExportPosition(exit, origin, relativeCoords)));
		}

		json::mArray tileArr;
		const PositionList& tiles = house->getTiles();
		for (PositionList::const_iterator it = tiles.begin(); it != tiles.end(); ++it) {
			if (!tileIntersectsRegion(*it, from, to)) {
				continue;
			}
			tileArr.push_back(serializePosition(toExportPosition(*it, origin, relativeCoords)));
		}
		if (!tileArr.empty()) {
			houseObj["tiles"] = json::mValue(tileArr);
		}

		houses.push_back(houseObj);
	}

	return houses;
}

// Plain ground-only tile: eligible for rectangle fill compression.
bool isSimpleGroundTile(const Tile* tile, uint16_t& groundId) {
	if (!tile || !tile->ground || tile->ground->isMetaItem()) {
		return false;
	}
	if (!tile->items.empty()) {
		return false;
	}
	if (tile->spawn || tile->creature) {
		return false;
	}
	if (tile->isHouseTile()) {
		return false;
	}
	if (tile->getMapFlags() != 0) {
		return false;
	}
	if (!tile->getZoneIds().empty()) {
		return false;
	}
	if (const HouseExitList* exits = tile->getHouseExits()) {
		if (!exits->empty()) {
			return false;
		}
	}
	if (tile->ground->getActionID() != 0 || tile->ground->getUniqueID() != 0 || tile->ground->getTier() != 0) {
		return false;
	}
	if (!tile->ground->getText().empty() || !tile->ground->getDescription().empty()) {
		return false;
	}
	groundId = tile->ground->getID();
	return groundId != 0;
}

struct GroundFillRect {
	int x1, y1, x2, y2, z;
	uint16_t groundId;
};

// Greedy maximal rectangles over a ground-id grid (0 = not fillable).
void findGroundFillRects(const std::vector<uint16_t>& grid, int width, int height, int z, std::vector<GroundFillRect>& out) {
	std::vector<char> visited(static_cast<size_t>(width * height), 0);

	auto at = [&](int x, int y) -> uint16_t {
		return grid[static_cast<size_t>(y * width + x)];
	};

	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			const size_t idx = static_cast<size_t>(y * width + x);
			if (visited[idx]) {
				continue;
			}
			const uint16_t groundId = at(x, y);
			if (groundId == 0) {
				visited[idx] = 1;
				continue;
			}

			int maxW = 1;
			while (x + maxW < width && !visited[static_cast<size_t>(y * width + x + maxW)] && at(x + maxW, y) == groundId) {
				++maxW;
			}

			int maxH = 1;
			bool canGrow = true;
			while (canGrow && y + maxH < height) {
				for (int dx = 0; dx < maxW; ++dx) {
					const size_t nidx = static_cast<size_t>((y + maxH) * width + x + dx);
					if (visited[nidx] || at(x + dx, y + maxH) != groundId) {
						canGrow = false;
						break;
					}
				}
				if (canGrow) {
					++maxH;
				}
			}

			for (int dy = 0; dy < maxH; ++dy) {
				for (int dx = 0; dx < maxW; ++dx) {
					visited[static_cast<size_t>((y + dy) * width + x + dx)] = 1;
				}
			}

			GroundFillRect rect;
			rect.x1 = x;
			rect.y1 = y;
			rect.x2 = x + maxW - 1;
			rect.y2 = y + maxH - 1;
			rect.z = z;
			rect.groundId = groundId;
			out.push_back(rect);
		}
	}
}

bool buildPatch(Map& map, const Position& from, const Position& to, const MapJsonExportOptions& options, json::mObject& root, bool showProgress, double progressStart, double progressEnd) {
	Position boundsFrom = from;
	Position boundsTo = to;
	MapJsonExporter::normalizeBounds(boundsFrom, boundsTo);

	const Position origin = boundsFrom;
	const int width = boundsTo.x - boundsFrom.x + 1;
	const int height = boundsTo.y - boundsFrom.y + 1;

	root.clear();
	root["format"] = json::mValue(std::string(MapJsonExporter::FORMAT_NAME));
	root["version"] = json::mValue(MapJsonExporter::FORMAT_VERSION);
	root["coords"] = json::mValue(std::string(options.relativeCoords ? "relative" : "absolute"));
	if (!options.clientVersion.empty()) {
		root["clientVersion"] = json::mValue(options.clientVersion);
		root["clientVersionId"] = json::mValue(options.clientVersionId);
	}
	root["from"] = json::mValue(serializePosition(boundsFrom));
	root["to"] = json::mValue(serializePosition(boundsTo));
	root["origin"] = json::mValue(serializePosition(origin));

	json::mObject sizeObj;
	sizeObj["width"] = json::mValue(width);
	sizeObj["height"] = json::mValue(height);
	sizeObj["floors"] = json::mValue(boundsTo.z - boundsFrom.z + 1);
	root["size"] = json::mValue(sizeObj);

	json::mArray tiles;
	json::mArray fills;
	const uint64_t total = MapJsonExporter::estimateTileCount(boundsFrom, boundsTo);
	uint64_t iterated = 0;
	bool anySimplified = false;

	for (int z = boundsFrom.z; z <= boundsTo.z; ++z) {
		std::vector<uint16_t> groundGrid;
		if (options.simplifyUniformGround) {
			groundGrid.assign(static_cast<size_t>(width * height), 0);
		}

		uint64_t simpleCount = 0;
		uint64_t cellCount = 0;

		for (int y = boundsFrom.y; y <= boundsTo.y; ++y) {
			for (int x = boundsFrom.x; x <= boundsTo.x; ++x) {
				++iterated;
				++cellCount;
				if (showProgress && total > 0 && iterated % 8192 == 0) {
					const double ratio = static_cast<double>(iterated) / static_cast<double>(total);
					g_gui.SetLoadDone(static_cast<int>(progressStart + ratio * (progressEnd - progressStart)));
				}

				Tile* tile = map.getTile(x, y, z);
				uint16_t groundId = 0;
				const bool simple = tile && isSimpleGroundTile(tile, groundId);

				if (options.simplifyUniformGround && simple) {
					const int lx = x - boundsFrom.x;
					const int ly = y - boundsFrom.y;
					groundGrid[static_cast<size_t>(ly * width + lx)] = groundId;
					++simpleCount;
					continue;
				}

				if (!tile) {
					if (options.includeEmptyTiles) {
						json::mObject emptyTile;
						const Position abs(x, y, z);
						const Position pos = toExportPosition(abs, origin, options.relativeCoords);
						emptyTile["x"] = json::mValue(pos.x);
						emptyTile["y"] = json::mValue(pos.y);
						emptyTile["z"] = json::mValue(pos.z);
						tiles.push_back(emptyTile);
					}
					continue;
				}

				if (!options.includeEmptyTiles && tile->empty() && !tile->spawn && !tile->creature && !tile->isHouseTile() && tile->getMapFlags() == 0 && tile->getZoneIds().empty()) {
					continue;
				}

				tiles.push_back(serializeTile(tile, origin, options.relativeCoords, options.includeSpawns));
			}
		}

		if (!options.simplifyUniformGround || simpleCount == 0) {
			continue;
		}

		const int minCoverage = std::max(1, std::min(100, options.simplifyMinCoveragePercent));
		const double coverage = cellCount > 0 ? (static_cast<double>(simpleCount) * 100.0) / static_cast<double>(cellCount) : 0.0;
		if (coverage < static_cast<double>(minCoverage)) {
			// Not enough uniform ground — dump simple tiles individually.
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x) {
					const uint16_t gid = groundGrid[static_cast<size_t>(y * width + x)];
					if (gid == 0) {
						continue;
					}
					Tile* tile = map.getTile(boundsFrom.x + x, boundsFrom.y + y, z);
					if (tile) {
						tiles.push_back(serializeTile(tile, origin, options.relativeCoords, options.includeSpawns));
					}
				}
			}
			continue;
		}

		std::vector<GroundFillRect> rects;
		findGroundFillRects(groundGrid, width, height, z, rects);

		// Prefer larger rectangles; drop 1x1 fills (they are cheaper as tiles).
		for (const GroundFillRect& rect : rects) {
			const int rw = rect.x2 - rect.x1 + 1;
			const int rh = rect.y2 - rect.y1 + 1;
			const int area = rw * rh;

			if (area <= 1) {
				Tile* tile = map.getTile(boundsFrom.x + rect.x1, boundsFrom.y + rect.y1, z);
				if (tile) {
					tiles.push_back(serializeTile(tile, origin, options.relativeCoords, options.includeSpawns));
				}
				continue;
			}

			json::mObject fill;
			const Position absFrom(boundsFrom.x + rect.x1, boundsFrom.y + rect.y1, z);
			const Position absTo(boundsFrom.x + rect.x2, boundsFrom.y + rect.y2, z);
			fill["from"] = json::mValue(serializePosition(toExportPosition(absFrom, origin, options.relativeCoords)));
			fill["to"] = json::mValue(serializePosition(toExportPosition(absTo, origin, options.relativeCoords)));
			fill["z"] = json::mValue(options.relativeCoords ? (z - origin.z) : z);
			json::mObject groundObj;
			groundObj["id"] = json::mValue(static_cast<int>(rect.groundId));
			fill["ground"] = json::mValue(groundObj);
			fill["area"] = json::mValue(area);
			fills.push_back(fill);
			anySimplified = true;
		}
	}

	if (anySimplified) {
		root["simplified"] = json::mValue(true);
		root["fills"] = json::mValue(fills);
	} else {
		root["simplified"] = json::mValue(false);
	}

	root["tiles"] = json::mValue(tiles);

	if (options.includeHouses) {
		json::mArray houses = serializeHouses(map, boundsFrom, boundsTo, origin, options.relativeCoords);
		if (!houses.empty()) {
			root["houses"] = json::mValue(houses);
		}
	}

	return true;
}

bool writePatch(Map& map, const Position& from, const Position& to, const FileName& path, const MapJsonExportOptions& options, bool showProgress, double progressStart, double progressEnd) {
	json::mObject root;
	if (!buildPatch(map, from, to, options, root, showProgress, progressStart, progressEnd)) {
		return false;
	}

	std::ofstream file(nstr(path.GetFullPath()).c_str(), std::ios::out | std::ios::trunc);
	if (!file.is_open()) {
		return false;
	}

	json::write_formatted(root, file);
	file.close();
	return true;
}

} // namespace

void MapJsonExporter::normalizeBounds(Position& from, Position& to) {
	if (from.x > to.x) {
		std::swap(from.x, to.x);
	}
	if (from.y > to.y) {
		std::swap(from.y, to.y);
	}
	if (from.z > to.z) {
		std::swap(from.z, to.z);
	}
}

uint64_t MapJsonExporter::estimateTileCount(const Position& from, const Position& to) {
	Position a = from;
	Position b = to;
	normalizeBounds(a, b);
	const uint64_t w = static_cast<uint64_t>(b.x - a.x + 1);
	const uint64_t h = static_cast<uint64_t>(b.y - a.y + 1);
	const uint64_t d = static_cast<uint64_t>(b.z - a.z + 1);
	return w * h * d;
}

bool MapJsonExporter::exportRegion(Map& map, const Position& from, const Position& to, const FileName& path, const MapJsonExportOptions& options, bool showProgress) {
	if (showProgress) {
		g_gui.CreateLoadBar("Exporting map patch to JSON");
	}

	bool ok = false;
	try {
		ok = writePatch(map, from, to, path, options, showProgress, 0.0, 100.0);
	} catch (std::bad_alloc&) {
		ok = false;
		if (showProgress) {
			g_gui.DestroyLoadBar();
		}
		g_gui.PopupDialog("Error", "There is not enough memory available to complete the operation.", wxOK);
		return false;
	}

	if (showProgress) {
		g_gui.DestroyLoadBar();
	}
	return ok;
}

bool MapJsonExporter::exportRegionToString(Map& map, const Position& from, const Position& to, const MapJsonExportOptions& options, std::string& outJson, bool showProgress) {
	if (showProgress) {
		g_gui.CreateLoadBar("Exporting map patch to JSON");
	}

	bool ok = false;
	try {
		json::mObject root;
		ok = buildPatch(map, from, to, options, root, showProgress, 0.0, 100.0);
		if (ok) {
			outJson = json::write_formatted(json::mValue(root));
		}
	} catch (std::bad_alloc&) {
		ok = false;
		if (showProgress) {
			g_gui.DestroyLoadBar();
		}
		g_gui.PopupDialog("Error", "There is not enough memory available to complete the operation.", wxOK);
		return false;
	}

	if (showProgress) {
		g_gui.DestroyLoadBar();
	}
	return ok;
}

bool MapJsonExporter::exportSelection(Editor& editor, const FileName& path, const MapJsonExportOptions& options) {
	if (!editor.hasSelection()) {
		return false;
	}

	Position from = editor.selection.minPosition();
	Position to = editor.selection.maxPosition();
	return exportRegion(editor.map, from, to, path, options, true);
}

bool MapJsonExporter::exportSelectionToString(Editor& editor, const MapJsonExportOptions& options, std::string& outJson) {
	if (!editor.hasSelection()) {
		return false;
	}

	Position from = editor.selection.minPosition();
	Position to = editor.selection.maxPosition();
	return exportRegionToString(editor.map, from, to, options, outJson, false);
}

bool MapJsonExporter::exportMapBatched(Map& map, const FileName& directory, const wxString& baseName, const MapJsonExportOptions& options) {
	if (!directory.Exists() || !directory.IsDirWritable()) {
		return false;
	}

	int chunkSize = options.chunkSize;
	if (chunkSize < 16) {
		chunkSize = 16;
	}
	if (chunkSize > 1024) {
		chunkSize = 1024;
	}

	const int mapWidth = map.getWidth();
	const int mapHeight = map.getHeight();
	if (mapWidth <= 0 || mapHeight <= 0) {
		return false;
	}

	g_gui.CreateLoadBar("Exporting map to JSON (batched)");

	// Discover non-empty chunks from existing tiles (avoids scanning empty floors).
	struct ChunkKey {
		int x;
		int y;
		int z;
		bool operator<(const ChunkKey& other) const {
			if (z != other.z) {
				return z < other.z;
			}
			if (y != other.y) {
				return y < other.y;
			}
			return x < other.x;
		}
	};

	std::set<ChunkKey> chunks;
	try {
		g_gui.SetLoadDone(5);
		for (MapIterator it = map.begin(); it != map.end(); ++it) {
			TileLocation* location = *it;
			if (!location || !location->get()) {
				continue;
			}
			Tile* tile = location->get();
			if (!options.includeEmptyTiles && tile->empty() && !tile->spawn && !tile->creature && !tile->isHouseTile() && tile->getMapFlags() == 0 && tile->getZoneIds().empty()) {
				continue;
			}
			const Position pos = tile->getPosition();
			ChunkKey key;
			key.x = (pos.x / chunkSize) * chunkSize;
			key.y = (pos.y / chunkSize) * chunkSize;
			key.z = pos.z;
			chunks.insert(key);
		}

		if (chunks.empty()) {
			g_gui.DestroyLoadBar();
			g_gui.SetStatusText("No tiles to export.");
			return true;
		}

		int written = 0;
		const size_t totalChunks = chunks.size();
		size_t chunkIndex = 0;

		for (const ChunkKey& key : chunks) {
			++chunkIndex;
			g_gui.SetLoadDone(static_cast<int>(5.0 + (static_cast<double>(chunkIndex) / static_cast<double>(totalChunks)) * 95.0));

			const int x1 = key.x;
			const int y1 = key.y;
			const int z = key.z;
			const int x2 = std::min(x1 + chunkSize - 1, mapWidth - 1);
			const int y2 = std::min(y1 + chunkSize - 1, mapHeight - 1);

			wxString fileName = baseName + wxString::Format("_%d_%d_%d.json", x1, y1, z);
			FileName file(fileName);
			file.Normalize(wxPATH_NORM_ALL, directory.GetFullPath());

			if (!writePatch(map, Position(x1, y1, z), Position(x2, y2, z), file, options, false, 0.0, 100.0)) {
				g_gui.DestroyLoadBar();
				return false;
			}
			++written;
		}

		g_gui.DestroyLoadBar();
		g_gui.SetStatusText(wxString::Format("Exported %d JSON map chunks.", written));
		return true;
	} catch (std::bad_alloc&) {
		g_gui.DestroyLoadBar();
		g_gui.PopupDialog("Error", "There is not enough memory available to complete the operation.", wxOK);
		return false;
	}
}

namespace {

bool jsonHas(const json::mObject& obj, const char* key) {
	return obj.find(key) != obj.end();
}

int jsonInt(const json::mObject& obj, const char* key, int fallback = 0) {
	json::mObject::const_iterator it = obj.find(key);
	if (it == obj.end()) {
		return fallback;
	}
	const json::mValue& value = it->second;
	if (value.type() == json::int_type) {
		return value.get_int();
	}
	if (value.type() == json::real_type) {
		return static_cast<int>(value.get_real());
	}
	return fallback;
}

std::string jsonStr(const json::mObject& obj, const char* key, const std::string& fallback = std::string()) {
	json::mObject::const_iterator it = obj.find(key);
	if (it == obj.end() || it->second.type() != json::str_type) {
		return fallback;
	}
	return it->second.get_str();
}

bool jsonBool(const json::mObject& obj, const char* key, bool fallback = false) {
	json::mObject::const_iterator it = obj.find(key);
	if (it == obj.end() || it->second.type() != json::bool_type) {
		return fallback;
	}
	return it->second.get_bool();
}

bool parsePosition(const json::mValue& value, Position& out) {
	if (value.type() != json::obj_type) {
		return false;
	}
	const json::mObject& obj = value.get_obj();
	out.x = jsonInt(obj, "x");
	out.y = jsonInt(obj, "y");
	out.z = jsonInt(obj, "z");
	return true;
}

void applyItemAttributes(Item* item, const json::mObject& obj) {
	if (!item) {
		return;
	}
	if (jsonHas(obj, "aid")) {
		item->setActionID(static_cast<uint16_t>(jsonInt(obj, "aid")));
	}
	if (jsonHas(obj, "uid")) {
		item->setUniqueID(static_cast<uint16_t>(jsonInt(obj, "uid")));
	}
	if (jsonHas(obj, "tier")) {
		item->setTier(static_cast<uint16_t>(jsonInt(obj, "tier")));
	}
	if (jsonHas(obj, "text")) {
		item->setText(jsonStr(obj, "text"));
	}
	if (jsonHas(obj, "desc")) {
		item->setDescription(jsonStr(obj, "desc"));
	}
	if (jsonHas(obj, "attributes") && obj.find("attributes")->second.type() == json::obj_type) {
		const json::mObject& attrs = obj.find("attributes")->second.get_obj();
		for (json::mObject::const_iterator it = attrs.begin(); it != attrs.end(); ++it) {
			const std::string& key = it->first;
			const json::mValue& value = it->second;
			if (value.type() == json::str_type) {
				item->setAttribute(key, value.get_str());
			} else if (value.type() == json::int_type) {
				item->setAttribute(key, value.get_int());
			} else if (value.type() == json::real_type) {
				item->setAttribute(key, value.get_real());
			} else if (value.type() == json::bool_type) {
				item->setAttribute(key, value.get_bool());
			}
		}
	}
}

Item* createItemFromJson(const json::mObject& obj) {
	const int id = jsonInt(obj, "id", 0);
	if (id <= 0 || !g_items.typeExists(id)) {
		return nullptr;
	}

	uint16_t subtype = 0xFFFF;
	if (jsonHas(obj, "subtype")) {
		subtype = static_cast<uint16_t>(jsonInt(obj, "subtype"));
	}

	Item* item = Item::Create(static_cast<uint16_t>(id), subtype);
	if (!item) {
		return nullptr;
	}

	applyItemAttributes(item, obj);

	const std::string type = jsonStr(obj, "type");
	if (type == "teleport" || jsonHas(obj, "destination")) {
		if (Teleport* teleport = dynamic_cast<Teleport*>(item)) {
			Position dest;
			if (jsonHas(obj, "destination") && parsePosition(obj.find("destination")->second, dest)) {
				teleport->setDestination(dest);
			}
		}
	} else if (type == "door" || jsonHas(obj, "doorid")) {
		if (Door* door = dynamic_cast<Door*>(item)) {
			door->setDoorID(static_cast<uint8_t>(jsonInt(obj, "doorid")));
		}
	} else if (type == "depot" || jsonHas(obj, "depotid")) {
		if (Depot* depot = dynamic_cast<Depot*>(item)) {
			depot->setDepotID(static_cast<uint8_t>(jsonInt(obj, "depotid")));
		}
	} else if (type == "podium" || jsonHas(obj, "outfit")) {
		if (Podium* podium = dynamic_cast<Podium*>(item)) {
			if (jsonHas(obj, "outfit") && obj.find("outfit")->second.type() == json::obj_type) {
				const json::mObject& outfitObj = obj.find("outfit")->second.get_obj();
				Outfit outfit = podium->getOutfit();
				outfit.lookType = jsonInt(outfitObj, "looktype");
				outfit.lookItem = jsonInt(outfitObj, "lookitem");
				outfit.lookMount = jsonInt(outfitObj, "lookmount");
				outfit.lookAddon = jsonInt(outfitObj, "lookaddon");
				outfit.lookHead = jsonInt(outfitObj, "lookhead");
				outfit.lookBody = jsonInt(outfitObj, "lookbody");
				outfit.lookLegs = jsonInt(outfitObj, "looklegs");
				outfit.lookFeet = jsonInt(outfitObj, "lookfeet");
				outfit.lookMountHead = jsonInt(outfitObj, "lookmounthead");
				outfit.lookMountBody = jsonInt(outfitObj, "lookmountbody");
				outfit.lookMountLegs = jsonInt(outfitObj, "lookmountlegs");
				outfit.lookMountFeet = jsonInt(outfitObj, "lookmountfeet");
				podium->setOutfit(outfit);
			}
			podium->setDirection(static_cast<uint8_t>(jsonInt(obj, "direction")));
			if (jsonHas(obj, "show_outfit")) {
				podium->setShowOutfit(jsonBool(obj, "show_outfit", true));
			}
			if (jsonHas(obj, "show_mount")) {
				podium->setShowMount(jsonBool(obj, "show_mount", true));
			}
			if (jsonHas(obj, "show_platform")) {
				podium->setShowPlatform(jsonBool(obj, "show_platform", true));
			}
		}
	} else if (type == "container" || jsonHas(obj, "items")) {
		if (Container* container = dynamic_cast<Container*>(item)) {
			if (jsonHas(obj, "items") && obj.find("items")->second.type() == json::array_type) {
				const json::mArray& contents = obj.find("items")->second.get_array();
				for (json::mArray::const_iterator it = contents.begin(); it != contents.end(); ++it) {
					if (it->type() != json::obj_type) {
						continue;
					}
					Item* child = createItemFromJson(it->get_obj());
					if (child) {
						container->getVector().push_back(child);
					}
				}
			}
		}
	}

	return item;
}

Position resolveImportPosition(const Position& stored, bool relative, const Position& offset) {
	if (relative) {
		return Position(stored.x + offset.x, stored.y + offset.y, stored.z + offset.z);
	}
	return Position(stored.x + offset.x, stored.y + offset.y, stored.z + offset.z);
}

Tile* buildTileFromJson(BaseMap& map, const Position& absPos, const json::mObject& tileObj, const MapJsonImportOptions& options) {
	TileLocation* location = map.createTileL(absPos);
	Tile* tile = map.allocator(location);

	if (options.importHouses && jsonHas(tileObj, "house_id")) {
		tile->setHouseID(static_cast<uint32_t>(jsonInt(tileObj, "house_id")));
	}

	if (jsonHas(tileObj, "flags")) {
		tile->setMapFlags(static_cast<uint16_t>(jsonInt(tileObj, "flags")));
	}

	if (jsonHas(tileObj, "zones") && tileObj.find("zones")->second.type() == json::array_type) {
		const json::mArray& zones = tileObj.find("zones")->second.get_array();
		for (json::mArray::const_iterator it = zones.begin(); it != zones.end(); ++it) {
			if (it->type() == json::int_type) {
				tile->addZoneId(static_cast<uint16_t>(it->get_int()));
			}
		}
	}

	if (jsonHas(tileObj, "ground") && tileObj.find("ground")->second.type() == json::obj_type) {
		Item* ground = createItemFromJson(tileObj.find("ground")->second.get_obj());
		if (ground) {
			tile->addItem(ground);
		}
	}

	if (jsonHas(tileObj, "items") && tileObj.find("items")->second.type() == json::array_type) {
		const json::mArray& items = tileObj.find("items")->second.get_array();
		for (json::mArray::const_iterator it = items.begin(); it != items.end(); ++it) {
			if (it->type() != json::obj_type) {
				continue;
			}
			Item* item = createItemFromJson(it->get_obj());
			if (item) {
				tile->addItem(item);
			}
		}
	}

	if (options.importSpawns) {
		if (jsonHas(tileObj, "spawn") && tileObj.find("spawn")->second.type() == json::obj_type) {
			const json::mObject& spawnObj = tileObj.find("spawn")->second.get_obj();
			tile->spawn = newd Spawn(jsonInt(spawnObj, "radius", 3));
		}
		if (jsonHas(tileObj, "creature") && tileObj.find("creature")->second.type() == json::obj_type) {
			const json::mObject& creatureObj = tileObj.find("creature")->second.get_obj();
			const std::string name = jsonStr(creatureObj, "name");
			if (!name.empty()) {
				Creature* creature = newd Creature(name);
				creature->setSpawnTime(jsonInt(creatureObj, "spawntime", 60));
				creature->setDirection(static_cast<Direction>(jsonInt(creatureObj, "direction", SOUTH)));
				tile->creature = creature;
			}
		}
	}

	return tile;
}

void placeBuiltTile(Editor& editor, Action* action, Tile* built, const Position& absPos, bool merge) {
	if (!built) {
		return;
	}

	TileLocation* location = editor.map.createTileL(absPos);
	Tile* oldTile = location->get();
	Tile* newTile = nullptr;

	if (merge && oldTile) {
		newTile = oldTile->deepCopy(editor.map);
		newTile->merge(built);
		delete built;
	} else if (merge && !oldTile && !built->ground) {
		newTile = built;
	} else if (merge && !oldTile) {
		newTile = built;
	} else {
		// Replace
		newTile = built;
	}

	newTile->setLocation(location);
	action->addChange(newd Change(newTile));
}

} // namespace

bool MapJsonImporter::importFile(Editor& editor, const FileName& path, const MapJsonImportOptions& options, wxString& error) {
	std::ifstream file(nstr(path.GetFullPath()).c_str(), std::ios::in);
	if (!file.is_open()) {
		error = "Could not open JSON file.";
		return false;
	}

	json::mValue rootValue;
	if (!json::read(file, rootValue) || rootValue.type() != json::obj_type) {
		error = "Invalid JSON (expected object root).";
		return false;
	}

	const json::mObject& root = rootValue.get_obj();
	const std::string format = jsonStr(root, "format");
	if (!format.empty() && format != MapJsonExporter::FORMAT_NAME) {
		error = wxString::Format("Unsupported format '%s' (expected %s).", format, MapJsonExporter::FORMAT_NAME);
		return false;
	}

	const bool relative = jsonStr(root, "coords", "relative") != "absolute";
	const Position offset(options.offsetX, options.offsetY, options.offsetZ);

	g_gui.CreateLoadBar("Importing map JSON");

	BatchAction* batch = editor.actionQueue->createBatch(ACTION_PASTE_TILES);
	Action* action = editor.actionQueue->createAction(batch);
	int placed = 0;
	int actionChanges = 0;
	const int flushEvery = 2048;

	auto flushAction = [&]() {
		if (actionChanges > 0) {
			batch->addAndCommitAction(action);
			action = editor.actionQueue->createAction(batch);
			actionChanges = 0;
		}
	};

	try {
		// Simplified fills first (uniform ground rectangles)
		if (jsonHas(root, "fills") && root.find("fills")->second.type() == json::array_type) {
			const json::mArray& fills = root.find("fills")->second.get_array();
			for (json::mArray::const_iterator fit = fills.begin(); fit != fills.end(); ++fit) {
				if (fit->type() != json::obj_type) {
					continue;
				}
				const json::mObject& fill = fit->get_obj();
				Position fromPos;
				Position toPos;
				if (!jsonHas(fill, "from") || !jsonHas(fill, "to") || !parsePosition(fill.find("from")->second, fromPos) || !parsePosition(fill.find("to")->second, toPos)) {
					continue;
				}

				uint16_t groundId = 0;
				if (jsonHas(fill, "ground") && fill.find("ground")->second.type() == json::obj_type) {
					groundId = static_cast<uint16_t>(jsonInt(fill.find("ground")->second.get_obj(), "id"));
				}
				if (groundId == 0 || !g_items.typeExists(groundId)) {
					continue;
				}

				MapJsonExporter::normalizeBounds(fromPos, toPos);
				for (int z = fromPos.z; z <= toPos.z; ++z) {
					for (int y = fromPos.y; y <= toPos.y; ++y) {
						for (int x = fromPos.x; x <= toPos.x; ++x) {
							const Position abs = resolveImportPosition(Position(x, y, z), relative, offset);
							if (!abs.isValid()) {
								continue;
							}

							TileLocation* location = editor.map.createTileL(abs);
							Tile* built = editor.map.allocator(location);
							Item* ground = Item::Create(groundId);
							if (ground) {
								built->addItem(ground);
							}
							placeBuiltTile(editor, action, built, abs, options.merge);
							++placed;
							++actionChanges;
							if (actionChanges >= flushEvery) {
								flushAction();
								g_gui.SetLoadDone(std::min(95, placed / 100));
							}
						}
					}
				}
			}
		}

		if (jsonHas(root, "tiles") && root.find("tiles")->second.type() == json::array_type) {
			const json::mArray& tiles = root.find("tiles")->second.get_array();
			const size_t tileCount = tiles.size();
			size_t index = 0;
			for (json::mArray::const_iterator tit = tiles.begin(); tit != tiles.end(); ++tit, ++index) {
				if (tit->type() != json::obj_type) {
					continue;
				}
				const json::mObject& tileObj = tit->get_obj();
				Position stored(jsonInt(tileObj, "x"), jsonInt(tileObj, "y"), jsonInt(tileObj, "z"));
				const Position abs = resolveImportPosition(stored, relative, offset);
				if (!abs.isValid()) {
					continue;
				}

				Tile* built = buildTileFromJson(editor.map, abs, tileObj, options);
				placeBuiltTile(editor, action, built, abs, options.merge);
				++placed;
				++actionChanges;
				if (actionChanges >= flushEvery) {
					flushAction();
				}
				if (tileCount > 0 && index % 512 == 0) {
					g_gui.SetLoadDone(std::min(95, static_cast<int>((index * 100) / tileCount)));
				}
			}
		}

		flushAction();
		editor.addBatch(batch);
	} catch (std::exception& e) {
		g_gui.DestroyLoadBar();
		error = wxString::Format("Import failed: %s", e.what());
		return false;
	} catch (...) {
		g_gui.DestroyLoadBar();
		error = "Import failed due to an unknown error.";
		return false;
	}

	g_gui.DestroyLoadBar();
	g_gui.RefreshView();
	g_gui.SetStatusText(wxString::Format("Imported %d tiles from JSON.", placed));
	return placed > 0 || (jsonHas(root, "fills") || jsonHas(root, "tiles"));
}

bool MapJsonImporter::importToCopyBuffer(CopyBuffer& buffer, const std::string& jsonText, wxString& error) {
	std::istringstream stream(jsonText);
	json::mValue rootValue;
	if (!json::read(stream, rootValue) || rootValue.type() != json::obj_type) {
		error = "Invalid JSON (expected object root).";
		return false;
	}

	const json::mObject& root = rootValue.get_obj();
	const std::string format = jsonStr(root, "format");
	if (!format.empty() && format != MapJsonExporter::FORMAT_NAME) {
		error = wxString::Format("Unsupported format '%s' (expected %s).", format, MapJsonExporter::FORMAT_NAME);
		return false;
	}

	const bool relative = jsonStr(root, "coords", "relative") != "absolute";
	const Position offset(0, 0, 0);
	MapJsonImportOptions options;
	options.merge = false;
	options.importSpawns = true;
	options.importHouses = true;

	BaseMap* tiles = newd BaseMap();
	Position minPos(0xFFFF, 0xFFFF, 0xF);
	int placed = 0;

	auto touchMin = [&](const Position& abs) {
		if (abs.x < minPos.x) {
			minPos.x = abs.x;
		}
		if (abs.y < minPos.y) {
			minPos.y = abs.y;
		}
		if (abs.z < minPos.z) {
			minPos.z = abs.z;
		}
	};

	auto placeBufferTile = [&](Tile* built, const Position& abs) {
		if (!built || !abs.isValid()) {
			delete built;
			return;
		}
		built->setLocation(tiles->createTileL(abs));
		tiles->setTile(built);
		touchMin(abs);
		++placed;
	};

	try {
		if (jsonHas(root, "fills") && root.find("fills")->second.type() == json::array_type) {
			const json::mArray& fills = root.find("fills")->second.get_array();
			for (json::mArray::const_iterator fit = fills.begin(); fit != fills.end(); ++fit) {
				if (fit->type() != json::obj_type) {
					continue;
				}
				const json::mObject& fill = fit->get_obj();
				Position fromPos;
				Position toPos;
				if (!jsonHas(fill, "from") || !jsonHas(fill, "to") || !parsePosition(fill.find("from")->second, fromPos) || !parsePosition(fill.find("to")->second, toPos)) {
					continue;
				}

				uint16_t groundId = 0;
				if (jsonHas(fill, "ground") && fill.find("ground")->second.type() == json::obj_type) {
					groundId = static_cast<uint16_t>(jsonInt(fill.find("ground")->second.get_obj(), "id"));
				}
				if (groundId == 0 || !g_items.typeExists(groundId)) {
					continue;
				}

				MapJsonExporter::normalizeBounds(fromPos, toPos);
				for (int z = fromPos.z; z <= toPos.z; ++z) {
					for (int y = fromPos.y; y <= toPos.y; ++y) {
						for (int x = fromPos.x; x <= toPos.x; ++x) {
							const Position abs = resolveImportPosition(Position(x, y, z), relative, offset);
							TileLocation* location = tiles->createTileL(abs);
							Tile* built = tiles->allocator(location);
							Item* ground = Item::Create(groundId);
							if (ground) {
								built->addItem(ground);
							}
							placeBufferTile(built, abs);
						}
					}
				}
			}
		}

		if (jsonHas(root, "tiles") && root.find("tiles")->second.type() == json::array_type) {
			const json::mArray& tileArr = root.find("tiles")->second.get_array();
			for (json::mArray::const_iterator tit = tileArr.begin(); tit != tileArr.end(); ++tit) {
				if (tit->type() != json::obj_type) {
					continue;
				}
				const json::mObject& tileObj = tit->get_obj();
				Position stored(jsonInt(tileObj, "x"), jsonInt(tileObj, "y"), jsonInt(tileObj, "z"));
				const Position abs = resolveImportPosition(stored, relative, offset);
				Tile* built = buildTileFromJson(*tiles, abs, tileObj, options);
				placeBufferTile(built, abs);
			}
		}
	} catch (std::exception& e) {
		delete tiles;
		error = wxString::Format("Import failed: %s", e.what());
		return false;
	} catch (...) {
		delete tiles;
		error = "Import failed due to an unknown error.";
		return false;
	}

	if (placed == 0) {
		delete tiles;
		error = "Patch contained no placeable tiles.";
		return false;
	}

	if (minPos.x == 0xFFFF) {
		minPos = Position(0, 0, 0);
	}

	buffer.assignTiles(tiles, minPos);
	return true;
}

