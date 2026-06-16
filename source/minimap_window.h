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

#ifndef RME_MINIMAP_WINDOW_H_
#define RME_MINIMAP_WINDOW_H_

#include "position.h"
#include <wx/panel.h>
#include <memory>
#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <wx/timer.h>
#include <wx/pen.h>
#include <vector>
#include <wx/combobox.h>
#include <wx/button.h>
#include <wx/xml/xml.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>

class MinimapCanvasPanel;

class MinimapWindow : public wxPanel {
public:
	enum {
		ID_MINIMAP_UPDATE = 45000,  // Choose a number that won't conflict with other IDs
		ID_RESIZE_TIMER = 45001     // Timer ID for resize completion detection
	};

	MinimapWindow(wxWindow* parent);
	virtual ~MinimapWindow();

	void OnEraseBackground(wxEraseEvent&) { }
	void OnMapClick(wxMouseEvent& event);
	void OnSize(wxSizeEvent&);
	void OnClose(wxCloseEvent&);
	void OnResizeTimer(wxTimerEvent&);

	void DelayedUpdate();
	void OnDelayedUpdate(wxTimerEvent& event);
	void OnKey(wxKeyEvent& event);

	void ClearCache();
	void RefreshMap();
	void PaintMap(wxDC& dc);

	// Build or load minimap cache when a map is opened.
	void PrepareCacheOnMapLoad();

	// Pre-cache method for building the entire minimap at load time
	void PreCacheEntireMap();

	// Method to initialize minimap loading progressively on startup
	void InitialLoad();

	void UpdateDrawnTiles(const PositionVector& positions);

	static const int BLOCK_SIZE = 256;  // 256 IS most optimal 512 is too laggy and 64 is too small
	
	struct MinimapBlock {
		wxBitmap bitmap;
		bool needsUpdate = true;
		bool wasSeen = false;
		int floor = -1;
		
		MinimapBlock() : needsUpdate(true), wasSeen(false) {}
	};
	
	using BlockPtr = std::shared_ptr<MinimapBlock>;
	using BlockMap = std::map<uint32_t, BlockPtr>;

	bool needs_update;

	void MarkBlockForUpdate(int x, int y) {
		if (auto block = getBlock(x, y)) {
			block->needsUpdate = true;
		}
	}

	// Minimap waypoint support
	struct MinimapWaypoint {
		wxString name;
		Position pos;
		MinimapWaypoint(const wxString& n, const Position& p) : name(n), pos(p) {}
	};
	std::vector<MinimapWaypoint> minimap_waypoints;
	int selected_minimap_waypoint_idx = -1;
	wxComboBox* minimap_waypoint_combo = nullptr;
	wxButton* add_minimap_waypoint_btn = nullptr;

	void UpdateMinimapWaypointCombo();
	void OnMinimapWaypointSelected(wxCommandEvent& event);
	void OnAddMinimapWaypoint(wxCommandEvent& event);
	void TeleportToMinimapWaypoint(int idx);

	// Save/Load buttons for minimap waypoints
	wxButton* save_minimap_waypoints_btn = nullptr;
	wxButton* load_minimap_waypoints_btn = nullptr;

	void SaveMinimapWaypointsToXML();
	void LoadMinimapWaypointsFromXML();
	void OnSaveMinimapWaypoints(wxCommandEvent& event);
	void OnLoadMinimapWaypoints(wxCommandEvent& event);
	void SetMinimapFloor(int floor);

private:
	BlockMap m_blocks;
	std::mutex m_mutex;
	
	// Empty tile atlas for faster rendering
	wxBitmap empty_tile_atlas;
	bool empty_tile_atlas_initialized;

	// Minimap floor (separate from editor floor)
	int minimap_floor;

	wxPanel* header_panel_ = nullptr;
	MinimapCanvasPanel* map_canvas_ = nullptr;
	wxStaticText* position_label_ = nullptr;
	wxStaticText* floor_label_ = nullptr;
	wxButton* btn_up_ = nullptr;
	wxButton* btn_down_ = nullptr;
	wxButton* btn_cache_ = nullptr;

	// Helper methods
	uint32_t getBlockIndex(int x, int y) const {
		return ((x / BLOCK_SIZE) * (65536 / BLOCK_SIZE)) + (y / BLOCK_SIZE);
	}
	
	wxPoint getBlockOffset(int x, int y) {
		return wxPoint(x - x % BLOCK_SIZE, y - y % BLOCK_SIZE);
	}
	
	BlockPtr getBlock(int x, int y);
	void updateBlock(BlockPtr block, int startX, int startY, int floor);

	wxBitmap buffer;
	std::mutex buffer_mutex;
	std::thread render_thread;
	std::atomic<bool> thread_running;
	
	// Window resizing handling
	bool is_resizing;
	wxTimer resize_timer;
	
	void RenderThreadFunction();
	void StartRenderThread();
	void StopRenderThread();
	
	// Store last known state to detect changes
	int last_center_x;
	int last_center_y;
	int last_floor;

	wxPen* pens[256];
	wxTimer update_timer;
	int last_start_x;
	int last_start_y;

	// Helper methods
	void InitializeEmptyTileAtlas();
	void DrawEmptyTileAtlas(wxDC& dc, int startX, int startY, int width, int height);

	void CreateHeaderPanel();
	void UpdateHeaderInfo();
	void ChangeFloor(int delta);
	void StartCacheCurrentFloor();
	void BatchCacheFloor(int floor);

	// Block-based cache
	struct BlockKey {
		int bx, by, z;
		bool operator<(const BlockKey& other) const {
			if (z != other.z) return z < other.z;
			if (bx != other.bx) return bx < other.bx;
			return by < other.by;
		}
	};
	std::map<BlockKey, wxBitmap> block_cache;

	// UI: Save cache to disk checkbox
	wxCheckBox* save_cache_checkbox = nullptr;
	bool save_cache_to_disk = false;

	// Block cache logic
	bool CacheFilledBlocksForFloor(int floor);
	void SaveBlockCacheToDisk(int floor);
	void LoadBlockCacheFromDisk(int floor);
	void ClearBlockCache();
	void RemoveBlockCacheForFloor(int floor);
	bool IsBlockFilled(int bx, int by, int floor);
	wxBitmap RenderBlock(int bx, int by, int floor);
	wxString GetCurrentMapName() const;

	friend class MinimapCanvasPanel;

	DECLARE_EVENT_TABLE()
};

class MinimapCanvasPanel : public wxPanel {
public:
	MinimapCanvasPanel(MinimapWindow* owner, wxWindow* parent);

private:
	void OnPaint(wxPaintEvent& event);
	void OnMouseClick(wxMouseEvent& event);

	MinimapWindow* owner_;
};

#endif
