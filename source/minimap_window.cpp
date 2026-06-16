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
/*
CURRENT SITUATION:
1. Minimap refresh issues:
   - Updates when changing floors (minimap_window.cpp:93-101)
   - Does not update properly after paste operations (copybuffer.cpp:232-257)
   - No caching mechanism for faster reloading
   - No binary file storage for minimap data

IMPROVEMENT TASKS:
1. Paste Operation Minimap Update:
   - Add minimap update calls in copybuffer.cpp after paste operations
   - Track modified positions during paste for efficient updates
   - Reference: copybuffer.cpp:232-257

2. Minimap Caching:
   - Implement block-based caching system (already started in MinimapBlock struct)
   - Add floor-specific caching
   - Optimize block size for better performance (currently 256)
   - Reference: minimap_window.cpp:53-55

3. Binary File Storage:
   - Create binary file format for minimap data
   - Store color information and floor data
   - Implement save/load functions
   - Cache binary data for quick loading

4. Drawing Optimization:
   - Batch rendering operations
   - Implement dirty region tracking
   - Use hardware acceleration where possible
   - Reference: minimap_window.cpp:290-339

RELEVANT CODE SECTIONS:
- Minimap window core: minimap_window.cpp:82-177
- Copy buffer paste: copybuffer.cpp:232-257
- Selection handling: selection.cpp:1-365
*/
#include "main.h"

#include "graphics.h"
#include "editor.h"
#include "map.h"

#include "gui.h"
#include "map_display.h"
#include "minimap_window.h"

#include <thread>
#include <mutex>
#include <atomic>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/dir.h>
#include <wx/ffile.h>
#include <wx/sizer.h>
#include <wx/statline.h>

BEGIN_EVENT_TABLE(MinimapWindow, wxPanel)
	EVT_ERASE_BACKGROUND(MinimapWindow::OnEraseBackground)
	EVT_KEY_DOWN(MinimapWindow::OnKey)
	EVT_SIZE(MinimapWindow::OnSize)
	EVT_CLOSE(MinimapWindow::OnClose)
	EVT_TIMER(ID_MINIMAP_UPDATE, MinimapWindow::OnDelayedUpdate)
	EVT_TIMER(ID_RESIZE_TIMER, MinimapWindow::OnResizeTimer)

END_EVENT_TABLE()

MinimapCanvasPanel::MinimapCanvasPanel(MinimapWindow* owner, wxWindow* parent) :
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE),
	owner_(owner) {
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	SetBackgroundColour(*wxBLACK);
	Bind(wxEVT_PAINT, &MinimapCanvasPanel::OnPaint, this);
	Bind(wxEVT_LEFT_DOWN, &MinimapCanvasPanel::OnMouseClick, this);
}

void MinimapCanvasPanel::OnPaint(wxPaintEvent& WXUNUSED(event)) {
	wxBufferedPaintDC dc(this);
	owner_->PaintMap(dc);
}

void MinimapCanvasPanel::OnMouseClick(wxMouseEvent& event) {
	owner_->OnMapClick(event);
}

MinimapWindow::MinimapWindow(wxWindow* parent) : 
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(205, 130), wxFULL_REPAINT_ON_RESIZE),
	update_timer(this),
	thread_running(false),
	needs_update(true),
	last_center_x(0),
	last_center_y(0),
	last_floor(0),
	last_start_x(0),
	last_start_y(0),
	is_resizing(false),
	empty_tile_atlas_initialized(false)
{
	for (int i = 0; i < 256; ++i) {
		pens[i] = newd wxPen(wxColor(minimap_color[i].red, minimap_color[i].green, minimap_color[i].blue));
	}
	
	// Initialize the update timer
	update_timer.SetOwner(this, ID_MINIMAP_UPDATE);
	
	// Initialize the resize timer
	resize_timer.SetOwner(this, ID_RESIZE_TIMER);
	
	// Floor initialization fix
	if (g_gui.IsEditorOpen()) {
		minimap_floor = g_gui.GetCurrentFloor();
	} else {
		minimap_floor = 7; // Safe default
	}
	
	CreateHeaderPanel();

	map_canvas_ = new MinimapCanvasPanel(this, this);
	map_canvas_->SetBackgroundColour(*wxBLACK);

	wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);
	rootSizer->Add(header_panel_, 0, wxEXPAND);
	rootSizer->Add(map_canvas_, 1, wxEXPAND);
	SetSizer(rootSizer);
	UpdateHeaderInfo();

	StartRenderThread();
	
	// Schedule initial loading after a short delay
	update_timer.Start(100, true); // Start a one-shot timer for 100ms
}

MinimapWindow::~MinimapWindow() {
	StopRenderThread();
	for (int i = 0; i < 256; ++i) {
		delete pens[i];
	}
}

void MinimapWindow::CreateHeaderPanel() {
	const wxColour headerBg(45, 45, 48);
	const wxColour headerFg(220, 220, 220);
	const wxColour btnBg(58, 58, 64);

	header_panel_ = new wxPanel(this, wxID_ANY);
	header_panel_->SetBackgroundColour(headerBg);
	header_panel_->SetMinSize(wxSize(-1, 36));

	position_label_ = new wxStaticText(header_panel_, wxID_ANY, wxT("Position: —"));
	position_label_->SetForegroundColour(headerFg);

	floor_label_ = new wxStaticText(header_panel_, wxID_ANY, wxT("Z: 7"));
	floor_label_->SetForegroundColour(headerFg);

	btn_down_ = new wxButton(header_panel_, wxID_ANY, wxT("-"), wxDefaultPosition, wxSize(26, 26), wxBORDER_SIMPLE);
	btn_up_ = new wxButton(header_panel_, wxID_ANY, wxT("+"), wxDefaultPosition, wxSize(26, 26), wxBORDER_SIMPLE);
	btn_cache_ = new wxButton(header_panel_, wxID_ANY, wxT("Cache"), wxDefaultPosition, wxSize(56, 26), wxBORDER_SIMPLE);

	auto styleButton = [&](wxButton* button) {
		button->SetBackgroundColour(btnBg);
		button->SetForegroundColour(headerFg);
	};

	styleButton(btn_down_);
	styleButton(btn_up_);
	styleButton(btn_cache_);

	btn_up_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ChangeFloor(1); });
	btn_down_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ChangeFloor(-1); });
	btn_cache_->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { StartCacheCurrentFloor(); });

	minimap_waypoint_combo = new wxComboBox(header_panel_, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(120, 24));
	add_minimap_waypoint_btn = new wxButton(header_panel_, wxID_ANY, wxT("+"), wxDefaultPosition, wxSize(26, 26), wxBORDER_SIMPLE);
	save_minimap_waypoints_btn = new wxButton(header_panel_, wxID_ANY, wxT("Save"), wxDefaultPosition, wxSize(48, 26), wxBORDER_SIMPLE);
	load_minimap_waypoints_btn = new wxButton(header_panel_, wxID_ANY, wxT("Load"), wxDefaultPosition, wxSize(48, 26), wxBORDER_SIMPLE);
	minimap_waypoint_combo->Bind(wxEVT_COMBOBOX, &MinimapWindow::OnMinimapWaypointSelected, this);
	add_minimap_waypoint_btn->Bind(wxEVT_BUTTON, &MinimapWindow::OnAddMinimapWaypoint, this);
	save_minimap_waypoints_btn->Bind(wxEVT_BUTTON, &MinimapWindow::OnSaveMinimapWaypoints, this);
	load_minimap_waypoints_btn->Bind(wxEVT_BUTTON, &MinimapWindow::OnLoadMinimapWaypoints, this);

	styleButton(add_minimap_waypoint_btn);
	styleButton(save_minimap_waypoints_btn);
	styleButton(load_minimap_waypoints_btn);

	save_cache_checkbox = new wxCheckBox(header_panel_, wxID_ANY, wxT("Save cache"), wxDefaultPosition, wxDefaultSize);
	save_cache_checkbox->SetForegroundColour(headerFg);
	save_cache_checkbox->SetValue(false);
	save_cache_checkbox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
		save_cache_to_disk = save_cache_checkbox->GetValue();
	});

	UpdateMinimapWaypointCombo();

	wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);
	row->Add(position_label_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
	row->AddStretchSpacer(1);
	row->Add(btn_down_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 2);
	row->Add(btn_up_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	row->Add(floor_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	row->Add(new wxStaticLine(header_panel_, wxID_ANY, wxDefaultPosition, wxSize(1, 20), wxLI_VERTICAL), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	row->Add(btn_cache_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	row->Add(minimap_waypoint_combo, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	row->Add(add_minimap_waypoint_btn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	row->Add(save_minimap_waypoints_btn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 4);
	row->Add(load_minimap_waypoints_btn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	row->Add(save_cache_checkbox, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	header_panel_->SetSizer(row);
}

void MinimapWindow::UpdateHeaderInfo() {
	if (!position_label_ || !floor_label_) {
		return;
	}

	floor_label_->SetLabel(wxString::Format(wxT("Z: %d"), minimap_floor));

	if (!g_gui.IsEditorOpen()) {
		position_label_->SetLabel(wxT("Position: —"));
		return;
	}

	MapCanvas* canvas = g_gui.GetCurrentMapTab()->GetCanvas();
	int centerX = 0;
	int centerY = 0;
	canvas->GetScreenCenter(&centerX, &centerY);
	position_label_->SetLabel(wxString::Format(wxT("Position: %d, %d"), centerX, centerY));
}

void MinimapWindow::ChangeFloor(int delta) {
	int newFloor = minimap_floor + delta;
	if (newFloor < 0) {
		newFloor = 0;
	} else if (newFloor > 15) {
		newFloor = 15;
	}

	if (newFloor == minimap_floor) {
		return;
	}

	minimap_floor = newFloor;
	needs_update = true;
	RefreshMap();
}

void MinimapWindow::RefreshMap() {
	UpdateHeaderInfo();
	if (map_canvas_) {
		map_canvas_->Refresh();
	}
}

void MinimapWindow::StartRenderThread() {
	thread_running = true;
	render_thread = std::thread(&MinimapWindow::RenderThreadFunction, this);
}

void MinimapWindow::StopRenderThread() {
	thread_running = false;
	if(render_thread.joinable()) {
		render_thread.join();
	}
}

void MinimapWindow::RenderThreadFunction() {
	while(thread_running) {
		if(needs_update && g_gui.IsEditorOpen()) {
			Editor& editor = *g_gui.GetCurrentEditor();
			MapCanvas* canvas = g_gui.GetCurrentMapTab()->GetCanvas();
			
			int center_x, center_y;
			canvas->GetScreenCenter(&center_x, &center_y);
			int floor = g_gui.GetCurrentFloor();
			
			// Force update if floor changed
			if(floor != last_floor) {
				// Clear the buffer when floor changes
				std::lock_guard<std::mutex> lock(buffer_mutex);
				buffer = wxBitmap(GetSize().GetWidth(), GetSize().GetHeight());
				
				// Clear block cache
				std::lock_guard<std::mutex> blockLock(m_mutex);
				m_blocks.clear();
			}
			
			// Always update if floor changed or position changed
			if(center_x != last_center_x || 
			   center_y != last_center_y || 
			   floor != last_floor) {
				
				int window_width = GetSize().GetWidth();
				int window_height = GetSize().GetHeight();
				
				// Create temporary bitmap
				wxBitmap temp_buffer(window_width, window_height);
				wxMemoryDC dc(temp_buffer);
				
				dc.SetBackground(*wxBLACK_BRUSH);
				dc.Clear();
				
				// CRITICAL FIX: Prevent division by zero in minimap rendering
				if (window_width <= 0 || window_height <= 0) {
					char debug_msg[256];
					sprintf(debug_msg, "DEBUG DRAG: CRITICAL ERROR! Minimap window dimensions are zero: width=%d, height=%d\n", 
						window_width, window_height);
					OutputDebugStringA(debug_msg);
					return; // Exit early to prevent division by zero
				}
				
				int start_x = center_x - window_width / 2;
				int start_y = center_y - window_height / 2;
				
				// Batch drawing by color
				std::vector<std::vector<wxPoint>> colorPoints(256);
				
				for(int y = 0; y < window_height; ++y) {
					for(int x = 0; x < window_width; ++x) {
						int map_x = start_x + x;
						int map_y = start_y + y;
						
						if(map_x >= 0 && map_y >= 0 && 
						   map_x < editor.map.getWidth() && 
						   map_y < editor.map.getHeight()) {
							
							Tile* tile = editor.map.getTile(map_x, map_y, floor);
							if(tile) {
								uint8_t color = tile->getMiniMapColor();
								if(color) {
									colorPoints[color].push_back(wxPoint(x, y));
								}
							}
						}
					}
				}
				
				// Draw points by color
				for(int color = 0; color < 256; ++color) {
					if(!colorPoints[color].empty()) {
						dc.SetPen(*pens[color]);
						for(const wxPoint& pt : colorPoints[color]) {
							dc.DrawPoint(pt.x, pt.y);
						}
					}
				}
				
				// Update buffer safely
				{
					std::lock_guard<std::mutex> lock(buffer_mutex);
					buffer = temp_buffer;
				}
				
				// Store current state
				last_center_x = center_x;
				last_center_y = center_y;
				last_floor = floor;
				
				// Request refresh of the window
				wxCommandEvent evt(wxEVT_COMMAND_BUTTON_CLICKED);
				evt.SetId(ID_MINIMAP_UPDATE);
				wxPostEvent(this, evt);
			}
			
			needs_update = false;
		}
		
		// Sleep to prevent excessive CPU usage
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}

void MinimapWindow::OnSize(wxSizeEvent& event) {
	// Get the new size
	wxSize newSize = event.GetSize();
	
	// If we're already at this size, skip
	if (newSize == GetSize()) {
		event.Skip();
		return;
	}
	
	// Mark that we're resizing
	is_resizing = true;
	
	// Stop any previous resize timer
	if (resize_timer.IsRunning()) {
		resize_timer.Stop();
	}
	
	// Create a new buffer at the new size
	{
		std::lock_guard<std::mutex> lock(buffer_mutex);
		buffer = wxBitmap(newSize.GetWidth(), newSize.GetHeight());
	}
	
	// Clear the block cache since we're changing size
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_blocks.clear();
	}
	
	// Start the resize timer (will fire when resize is complete)
	resize_timer.Start(50, true); // Reduced to 50ms for faster response
	
	// Skip the event to allow default handling
	event.Skip();
}

void MinimapWindow::OnClose(wxCloseEvent& event) {
	// Hide the window instead of destroying it
	// This allows the minimap to be reopened in the same position
	if (wxWindow::GetParent()) {
		g_gui.HideMinimap();
		event.Veto(); // Prevent the window from being destroyed
	} else {
		event.Skip(); // Allow default handling if no parent
	}
}

void MinimapWindow::OnDelayedUpdate(wxTimerEvent& event) {
	if (g_gui.IsEditorOpen()) {
		// If editor is already open, load the minimap
		if (event.GetId() == ID_MINIMAP_UPDATE) {
			InitialLoad();
		}
	}
	
	needs_update = true;
}

void MinimapWindow::DelayedUpdate() {
	update_timer.Start(100, true);  // 100ms single-shot timer
}

void MinimapWindow::OnResizeTimer(wxTimerEvent& event) {
	// Resizing has stopped
	is_resizing = false;
	
	// Force a complete redraw with the new size
	needs_update = true;
	
	// Request a full refresh
	RefreshMap();
	
	// Trigger an immediate update
	if (g_gui.IsEditorOpen()) {
		Editor& editor = *g_gui.GetCurrentEditor();
		MapCanvas* canvas = g_gui.GetCurrentMapTab()->GetCanvas();
		
		int center_x, center_y;
		canvas->GetScreenCenter(&center_x, &center_y);
		
		// Force update of the render thread
		last_center_x = center_x;
		last_center_y = center_y;
		last_floor = g_gui.GetCurrentFloor();
	}
}

void MinimapWindow::PaintMap(wxDC& dc) {
	dc.SetBackground(*wxBLACK_BRUSH);
	dc.Clear();
	
	if (!g_gui.IsEditorOpen() || !map_canvas_) return;
	
	Editor& editor = *g_gui.GetCurrentEditor();
	MapCanvas* canvas = g_gui.GetCurrentMapTab()->GetCanvas();
	
	int centerX, centerY;
	canvas->GetScreenCenter(&centerX, &centerY);
	int floor = minimap_floor;
	
	last_center_x = centerX;
	last_center_y = centerY;
	last_floor = floor;

	int windowWidth = map_canvas_->GetSize().GetWidth();
	int windowHeight = map_canvas_->GetSize().GetHeight();
	if (windowWidth <= 0 || windowHeight <= 0) {
		return;
	}
	
	int padding = 10;
	int startX = std::max(0, centerX - (windowWidth / 2) - padding);
	int startY = std::max(0, centerY - (windowHeight / 2) - padding);
	int endX = std::min(editor.map.getWidth(), startX + windowWidth + padding * 2);
	int endY = std::min(editor.map.getHeight(), startY + windowHeight + padding * 2);
	int blockStartX = startX / BLOCK_SIZE;
	int blockEndX = (endX + BLOCK_SIZE - 1) / BLOCK_SIZE;
	int blockStartY = startY / BLOCK_SIZE;
	int blockEndY = (endY + BLOCK_SIZE - 1) / BLOCK_SIZE;
	for (int by = blockStartY; by < blockEndY; ++by) {
		for (int bx = blockStartX; bx < blockEndX; ++bx) {
			BlockKey key{bx, by, floor};
			wxBitmap* bmp = nullptr;
			auto it = block_cache.find(key);
			if (it != block_cache.end()) {
				bmp = &it->second;
			} else if (IsBlockFilled(bx, by, floor)) {
				wxBitmap rendered = RenderBlock(bx, by, floor);
				block_cache[key] = rendered;
				bmp = &block_cache[key];
			}
			if (bmp) {
				int drawX = bx * BLOCK_SIZE - startX;
				int drawY = by * BLOCK_SIZE - startY;
				dc.DrawBitmap(*bmp, drawX, drawY, false);
			}
		}
	}
	
	dc.SetPen(wxPen(wxColour(255, 0, 0), 2));
	int centerDrawX = windowWidth / 2;
	int centerDrawY = windowHeight / 2;
	dc.DrawLine(centerDrawX - 5, centerDrawY, centerDrawX + 5, centerDrawY);
	dc.DrawLine(centerDrawX, centerDrawY - 5, centerDrawX, centerDrawY + 5);
}

void MinimapWindow::StartCacheCurrentFloor() {
	wxString msg = wxString::Format("Caching minimap floor %d...", minimap_floor);
	g_gui.CreateLoadBar(msg, true);
	const bool completed = CacheFilledBlocksForFloor(minimap_floor);
	if (completed && save_cache_to_disk) {
		SaveBlockCacheToDisk(minimap_floor);
	}
	g_gui.DestroyLoadBar();
	needs_update = true;
	RefreshMap();
}

void MinimapWindow::BatchCacheFloor(int floor) {
	if (!g_gui.IsEditorOpen()) return;
	Editor& editor = *g_gui.GetCurrentEditor();
	int mapWidth = editor.map.getWidth();
	int mapHeight = editor.map.getHeight();
	int totalRows = mapHeight;
	int doneRows = 0;
	
	// For each row, update the minimap cache and progress bar
	for (int y = 0; y < mapHeight; ++y) {
		for (int x = 0; x < mapWidth; ++x) {
			Tile* tile = editor.map.getTile(x, y, floor);
			// Optionally, update cache here if you have a block-based cache
			// For now, just access the tile to simulate caching
			(void)tile;
		}
		doneRows++;
		int percent = int((doneRows / (double)totalRows) * 100.0);
		g_gui.SetLoadDone(percent, wxString::Format("Caching row %d/%d", doneRows, totalRows));
		wxYield(); // Keep UI responsive
	}
	needs_update = true;
	RefreshMap();
}

void MinimapWindow::OnMapClick(wxMouseEvent& event) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	Editor& editor = *g_gui.GetCurrentEditor();
	MapCanvas* canvas = g_gui.GetCurrentMapTab()->GetCanvas();
	
	int centerX, centerY;
	canvas->GetScreenCenter(&centerX, &centerY);
	
	int windowWidth = map_canvas_ ? map_canvas_->GetSize().GetWidth() : 0;
	int windowHeight = map_canvas_ ? map_canvas_->GetSize().GetHeight() : 0;
	if (windowWidth <= 0 || windowHeight <= 0) {
		return;
	}
	
	int clickX = event.GetX();
	int clickY = event.GetY();
	int mapX = centerX - (windowWidth / 2) + clickX;
	int mapY = centerY - (windowHeight / 2) + clickY;
	
	g_gui.SetScreenCenterPosition(Position(mapX, mapY, minimap_floor));
	RefreshMap();
	g_gui.RefreshView();
}

void MinimapWindow::OnKey(wxKeyEvent& event) {
	if (g_gui.GetCurrentTab() != nullptr) {
		g_gui.GetCurrentMapTab()->GetEventHandler()->AddPendingEvent(event);
	}
}

MinimapWindow::BlockPtr MinimapWindow::getBlock(int x, int y) {
	std::lock_guard<std::mutex> lock(m_mutex);
	uint32_t index = getBlockIndex(x, y);
	
	auto it = m_blocks.find(index);
	if (it == m_blocks.end()) {
		auto block = std::make_shared<MinimapBlock>();
		m_blocks[index] = block;
		return block;
	}
	return it->second;
}

void MinimapWindow::updateBlock(BlockPtr block, int startX, int startY, int floor) {
	Editor& editor = *g_gui.GetCurrentEditor();
	
	// Always update if the block's floor doesn't match current floor
	if (!block->needsUpdate && block->floor != floor) {
		block->needsUpdate = true;
	}
	
	if (!block->needsUpdate) return;
	
	wxBitmap bitmap(BLOCK_SIZE, BLOCK_SIZE);
	wxMemoryDC dc(bitmap);
	dc.SetBackground(*wxBLACK_BRUSH);
	dc.Clear();
	
	// Store the floor this block was rendered for
	block->floor = floor;
	
	// Batch drawing by color like OTClient
	std::vector<std::vector<wxPoint>> colorPoints(256);
	
	for (int y = 0; y < BLOCK_SIZE; ++y) {
		for (int x = 0; x < BLOCK_SIZE; ++x) {
			int mapX = startX + x;
			int mapY = startY + y;
			
			Tile* tile = editor.map.getTile(mapX, mapY, floor);
			if (tile) {
				uint8_t color = tile->getMiniMapColor();
				if (color != 255) {  // Not transparent
					colorPoints[color].push_back(wxPoint(x, y));
				}
			}
		}
	}
	
	// Draw all points of same color at once
	for (int color = 0; color < 256; ++color) {
		if (!colorPoints[color].empty()) {
			dc.SetPen(*pens[color]);
			for (const wxPoint& pt : colorPoints[color]) {
				dc.DrawPoint(pt);
			}
		}
	}
	
	block->bitmap = bitmap;
	block->needsUpdate = false;
	block->wasSeen = true;
}

void MinimapWindow::ClearCache() {
	RefreshMap();
}

void MinimapWindow::UpdateDrawnTiles(const PositionVector& positions) {
	(void)positions;
	RefreshMap();
}

void MinimapWindow::PrepareCacheOnMapLoad() {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	minimap_floor = g_gui.GetCurrentFloor();
	ClearBlockCache();

	for (int floor = 0; floor <= 15; ++floor) {
		LoadBlockCacheFromDisk(floor);
	}

	if (block_cache.empty()) {
		g_gui.CreateLoadBar("Building minimap cache...", true);
		const bool completed = CacheFilledBlocksForFloor(minimap_floor);
		if (completed && save_cache_to_disk) {
			SaveBlockCacheToDisk(minimap_floor);
		}
		g_gui.DestroyLoadBar();
	}

	needs_update = true;
	RefreshMap();
}

void MinimapWindow::PreCacheEntireMap() {
	PrepareCacheOnMapLoad();
}

void MinimapWindow::InitialLoad() {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	std::lock_guard<std::mutex> lock(m_mutex);
	m_blocks.clear();
	
	needs_update = true;
	RefreshMap();
}

void MinimapWindow::UpdateMinimapWaypointCombo() {
	if (!minimap_waypoint_combo) return;
	minimap_waypoint_combo->Clear();
	for (const auto& wp : minimap_waypoints) {
		minimap_waypoint_combo->Append(wp.name);
	}
	if (selected_minimap_waypoint_idx >= 0 && selected_minimap_waypoint_idx < (int)minimap_waypoints.size()) {
		minimap_waypoint_combo->SetSelection(selected_minimap_waypoint_idx);
	}
}

void MinimapWindow::OnMinimapWaypointSelected(wxCommandEvent& event) {
	int idx = minimap_waypoint_combo->GetSelection();
	if (idx >= 0 && idx < (int)minimap_waypoints.size()) {
		TeleportToMinimapWaypoint(idx);
	}
}

void MinimapWindow::OnAddMinimapWaypoint(wxCommandEvent& event) {
	// Prompt for name
	wxString name = wxGetTextFromUser("Enter waypoint name:", "Add Minimap Waypoint");
	if (name.IsEmpty()) return;
	// Use current minimap position
	int centerX = last_center_x;
	int centerY = last_center_y;
	int floor = minimap_floor;
	minimap_waypoints.emplace_back(name, Position(centerX, centerY, floor));
	selected_minimap_waypoint_idx = minimap_waypoints.size() - 1;
	UpdateMinimapWaypointCombo();
}

void MinimapWindow::TeleportToMinimapWaypoint(int idx) {
	if (idx < 0 || idx >= (int)minimap_waypoints.size()) return;
	const MinimapWaypoint& wp = minimap_waypoints[idx];
	minimap_floor = wp.pos.z;
	needs_update = true;
	RefreshMap();
	// Optionally, also move the main view:
	if (g_gui.IsEditorOpen()) {
		g_gui.GetCurrentMapTab()->SetScreenCenterPosition(wp.pos);
	}
}

void MinimapWindow::SaveMinimapWaypointsToXML() {
	wxString dataDir = g_gui.GetDataDirectory();
	wxString filePath = dataDir + wxFileName::GetPathSeparator() + "minimap_waypoints.xml";
	wxXmlDocument doc;
	wxXmlNode* root = new wxXmlNode(wxXML_ELEMENT_NODE, "minimap_waypoints");
	for (const auto& wp : minimap_waypoints) {
		wxXmlNode* waypoint = new wxXmlNode(wxXML_ELEMENT_NODE, "waypoint");
		waypoint->AddAttribute("name", wp.name);
		waypoint->AddAttribute("x", wxString::Format("%d", wp.pos.x));
		waypoint->AddAttribute("y", wxString::Format("%d", wp.pos.y));
		waypoint->AddAttribute("floor", wxString::Format("%d", wp.pos.z));
		root->AddChild(waypoint);
	}
	doc.SetRoot(root);
	doc.Save(filePath);
}

void MinimapWindow::LoadMinimapWaypointsFromXML() {
	wxString dataDir = g_gui.GetDataDirectory();
	wxString filePath = dataDir + wxFileName::GetPathSeparator() + "minimap_waypoints.xml";
	minimap_waypoints.clear();
	wxXmlDocument doc;
	if (doc.Load(filePath)) {
		wxXmlNode* root = doc.GetRoot();
		if (root->GetName() == "minimap_waypoints") {
			wxXmlNode* waypoint = root->GetChildren();
			while (waypoint) {
				if (waypoint->GetName() == "waypoint") {
					wxString name = waypoint->GetAttribute("name");
					int x = 0, y = 0, floor = 0;
					waypoint->GetAttribute("x").ToInt(&x);
					waypoint->GetAttribute("y").ToInt(&y);
					waypoint->GetAttribute("floor").ToInt(&floor);
					minimap_waypoints.emplace_back(name, Position(x, y, floor));
				}
				waypoint = waypoint->GetNext();
			}
			selected_minimap_waypoint_idx = 0;
			UpdateMinimapWaypointCombo();
		}
	}
}

void MinimapWindow::OnSaveMinimapWaypoints(wxCommandEvent& event) {
	SaveMinimapWaypointsToXML();
}

void MinimapWindow::OnLoadMinimapWaypoints(wxCommandEvent& event) {
	LoadMinimapWaypointsFromXML();
}

bool MinimapWindow::CacheFilledBlocksForFloor(int floor) {
	if (!g_gui.IsEditorOpen()) return false;
	Editor& editor = *g_gui.GetCurrentEditor();
	int mapWidth = editor.map.getWidth();
	int mapHeight = editor.map.getHeight();
	int numBlocksX = (mapWidth + BLOCK_SIZE - 1) / BLOCK_SIZE;
	int numBlocksY = (mapHeight + BLOCK_SIZE - 1) / BLOCK_SIZE;
	int totalBlocks = numBlocksX * numBlocksY;
	int doneBlocks = 0;
	RemoveBlockCacheForFloor(floor);
	for (int by = 0; by < numBlocksY; ++by) {
		for (int bx = 0; bx < numBlocksX; ++bx) {
			if (IsBlockFilled(bx, by, floor)) {
				wxBitmap bmp = RenderBlock(bx, by, floor);
				block_cache[{bx, by, floor}] = bmp;
			}
			doneBlocks++;
			int percent = int((doneBlocks / (double)totalBlocks) * 100.0);
			if (g_gui.SetLoadDone(percent, wxString::Format("Caching block %d/%d", doneBlocks, totalBlocks))) {
				return false;
			}
			wxYield();
		}
	}
	return true;
}

void MinimapWindow::SaveBlockCacheToDisk(int floor) {
	wxString dataDir = g_gui.GetDataDirectory();
	wxString mapName = GetCurrentMapName();
	wxString cacheDir = dataDir + wxFileName::GetPathSeparator() + "cachedmaps" + wxFileName::GetPathSeparator() + mapName;
	wxFileName::Mkdir(cacheDir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
	for (const auto& pair : block_cache) {
		if (pair.first.z != floor) continue;
		wxString fileName = wxString::Format("block_%d_%d_%d.bin", pair.first.bx, pair.first.by, pair.first.z);
		wxString filePath = cacheDir + wxFileName::GetPathSeparator() + fileName;
		wxFFile file(filePath, "wb");
		if (!file.IsOpened()) continue;
		wxImage img = pair.second.ConvertToImage();
		for (int y = 0; y < BLOCK_SIZE; ++y) {
			for (int x = 0; x < BLOCK_SIZE; ++x) {
				unsigned char r = img.GetRed(x, y);
				unsigned char g = img.GetGreen(x, y);
				unsigned char b = img.GetBlue(x, y);
				// Find closest minimap color index
				uint8_t best = 0;
				int bestDist = 256*256*3;
				for (int i = 0; i < 256; ++i) {
					int dr = int(minimap_color[i].red) - r;
					int dg = int(minimap_color[i].green) - g;
					int db = int(minimap_color[i].blue) - b;
					int dist = dr*dr + dg*dg + db*db;
					if (dist < bestDist) {
						bestDist = dist;
						best = i;
					}
				}
				file.Write(&best, 1);
			}
		}
		file.Close();
	}
}

void MinimapWindow::LoadBlockCacheFromDisk(int floor) {
	wxString dataDir = g_gui.GetDataDirectory();
	wxString mapName = GetCurrentMapName();
	wxString cacheDir = dataDir + wxFileName::GetPathSeparator() + "cachedmaps" + wxFileName::GetPathSeparator() + mapName;
	if (!wxDir::Exists(cacheDir)) {
		return;
	}
	wxDir dir(cacheDir);
	wxString filename;
	bool cont = dir.GetFirst(&filename, wxString::Format("block_*_%d.bin", floor), wxDIR_FILES);
	while (cont) {
		wxFileName fn(cacheDir, filename);
		wxFFile file(fn.GetFullPath(), "rb");
		if (!file.IsOpened()) { cont = dir.GetNext(&filename); continue; }
		std::vector<uint8_t> buffer(BLOCK_SIZE * BLOCK_SIZE);
		if (file.Read(buffer.data(), buffer.size()) == buffer.size()) {
			int bx = 0, by = 0, z = 0;
			if (sscanf(filename.mb_str(), "block_%d_%d_%d.bin", &bx, &by, &z) == 3) {
				wxImage img(BLOCK_SIZE, BLOCK_SIZE);
				for (int y = 0; y < BLOCK_SIZE; ++y) {
					for (int x = 0; x < BLOCK_SIZE; ++x) {
						uint8_t idx = buffer[y * BLOCK_SIZE + x];
						img.SetRGB(x, y, minimap_color[idx].red, minimap_color[idx].green, minimap_color[idx].blue);
					}
				}
				block_cache[{bx, by, z}] = wxBitmap(img);
			}
		}
		file.Close();
		cont = dir.GetNext(&filename);
	}
}

void MinimapWindow::ClearBlockCache() {
	block_cache.clear();
}

void MinimapWindow::RemoveBlockCacheForFloor(int floor) {
	for (auto it = block_cache.begin(); it != block_cache.end(); ) {
		if (it->first.z == floor) {
			it = block_cache.erase(it);
		} else {
			++it;
		}
	}
}

bool MinimapWindow::IsBlockFilled(int bx, int by, int floor) {
	if (!g_gui.IsEditorOpen()) return false;
	Editor& editor = *g_gui.GetCurrentEditor();
	int startX = bx * BLOCK_SIZE;
	int startY = by * BLOCK_SIZE;
	for (int y = 0; y < BLOCK_SIZE; ++y) {
		for (int x = 0; x < BLOCK_SIZE; ++x) {
			Tile* tile = editor.map.getTile(startX + x, startY + y, floor);
			if (tile && tile->getMiniMapColor()) {
				return true;
			}
		}
	}
	return false;
}

wxBitmap MinimapWindow::RenderBlock(int bx, int by, int floor) {
	if (!g_gui.IsEditorOpen()) return wxBitmap(BLOCK_SIZE, BLOCK_SIZE);
	Editor& editor = *g_gui.GetCurrentEditor();
	wxBitmap bitmap(BLOCK_SIZE, BLOCK_SIZE);
	wxMemoryDC dc(bitmap);
	dc.SetBackground(*wxBLACK_BRUSH);
	dc.Clear();
	int startX = bx * BLOCK_SIZE;
	int startY = by * BLOCK_SIZE;
	for (int y = 0; y < BLOCK_SIZE; ++y) {
		for (int x = 0; x < BLOCK_SIZE; ++x) {
			Tile* tile = editor.map.getTile(startX + x, startY + y, floor);
			if (tile) {
				uint8_t color = tile->getMiniMapColor();
				if (color) {
					dc.SetPen(*pens[color]);
					dc.DrawPoint(x, y);
				}
			}
		}
	}
	return bitmap;
}

wxString MinimapWindow::GetCurrentMapName() const {
	if (!g_gui.IsEditorOpen()) return "unnamed";
	Editor* editor = g_gui.GetCurrentEditor();
	if (!editor) return "unnamed";
	wxString mapName = editor->map.getName();
	if (mapName.IsEmpty()) mapName = "unnamed";
	return mapName;
}

void MinimapWindow::SetMinimapFloor(int floor) {
	if (minimap_floor != floor) {
		minimap_floor = floor;
		needs_update = true;
		RefreshMap();
	}
}