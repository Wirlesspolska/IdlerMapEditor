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

#ifndef RME_TILESET_CREATURE_H_
#define RME_TILESET_CREATURE_H_

#include "palette_common.h"
#include <wx/longlong.h>

// New class for seamless grid view of creature sprites with direct rendering
class CreatureSeamlessGridPanel : public wxScrolledWindow {
public:
	CreatureSeamlessGridPanel(wxWindow* parent);
	virtual ~CreatureSeamlessGridPanel();

	void Clear();
	void LoadCreatures(const BrushVector& brushlist);
	Brush* GetSelectedBrush() const;
	bool SelectBrush(const Brush* brush);
	void EnsureVisible(const Brush* brush);
	void SelectIndex(int index);
	
	// Getter for sprite size
	int GetSpriteSize() const { return sprite_size; }
	
	// Drawing handlers
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnScroll(wxScrollWinEvent& event);
	void OnMouseClick(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);
	void OnTimer(wxTimerEvent& event);

	// Creature brushes in this panel
	BrushVector creatures;

	// Friend class declaration to allow access to protected members
	friend class CreaturePalettePanel;

protected:
	void RecalculateGrid();
	int GetSpriteIndexAt(int x, int y) const;
	int GetCreatureNaturalSize(CreatureType* ctype) const;
	void DrawCreature(wxDC& dc, int x, int y, CreatureType* ctype, bool selected = false);
	void DrawItemsToPanel(wxDC& dc);
	void UpdateViewableItems();
	void StartProgressiveLoading();

	int columns;
	int sprite_size;
	int selected_index;
	int hover_index;
	wxBitmap* buffer;
	std::map<size_t, int> sprite_dimensions; // Maps creature index to natural size
	
	// Viewport and loading management
	int first_visible_row;
	int last_visible_row;
	int visible_rows_margin;
	int total_rows;
	bool need_full_redraw;
	
	// Progressive loading properties
	bool use_progressive_loading;
	bool is_large_tileset;
	int loading_step;
	int max_loading_steps;
	wxTimer* loading_timer;
	static const int LARGE_TILESET_THRESHOLD = 200;

	DECLARE_EVENT_TABLE();
};

// Original class for grid view of creature sprites with padding
class CreatureSpritePanel : public wxScrolledWindow {
public:
	CreatureSpritePanel(wxWindow* parent);
	virtual ~CreatureSpritePanel();

	void Clear();
	void LoadCreatures(const BrushVector& brushlist);
	Brush* GetSelectedBrush() const;
	bool SelectBrush(const Brush* brush);
	void EnsureVisible(const Brush* brush);
	void SelectIndex(int index);
	
	// Getter for sprite size
	int GetSpriteSize() const;
	
	// Drawing handlers
	void OnPaint(wxPaintEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnScroll(wxScrollWinEvent& event);
	void OnMouseClick(wxMouseEvent& event);
	void OnMouseMove(wxMouseEvent& event);

	// Creature brushes in this panel
	BrushVector creatures;

	// Friend class declaration to allow access to protected members
	friend class CreaturePalettePanel;

protected:
	void RecalculateGrid();
	int GetSpriteIndexAt(int x, int y) const;
	void DrawSprite(wxDC& dc, int x, int y, CreatureType* ctype, bool selected = false);

	int columns;
	int sprite_size;
	int padding;
	int selected_index;
	int hover_index;
	wxBitmap* buffer;

	DECLARE_EVENT_TABLE();
};

class CreaturePalettePanel : public PalettePanel {
public:
	CreaturePalettePanel(wxWindow* parent, wxWindowID id = wxID_ANY);
	virtual ~CreaturePalettePanel();

	PaletteType GetType() const;

	// Select the first brush
	void SelectFirstBrush();
	// Returns the currently selected brush (first brush if panel is not loaded)
	Brush* GetSelectedBrush() const;
	// Returns the currently selected brush size
	int GetSelectedBrushSize() const;
	// Select the brush in the parameter, this only changes the look of the panel
	bool SelectBrush(const Brush* whatbrush);

	// Updates the palette window to use the current brush size
	void OnUpdateBrushSize(BrushShape shape, int size);
	// Called when this page is displayed
	void OnSwitchIn();
	// Called sometimes?
	void OnUpdate();

protected:
	void SelectTileset(size_t index);
	void SelectCreature(size_t index);
	void SelectCreature(std::string name);
	// Switch between list view and sprite view modes
	void SetViewMode(bool use_sprites);
	// Set view style (regular grid vs seamless grid)
	void SetViewStyle(bool use_seamless);
	// Set large sprite mode (64x64 vs 32x32)
	void SetLargeSpriteMode(bool use_large);
	// Set zoom level for sprites
	void SetZoomLevel(int zoom_factor);

public:
	// Event handling
	void OnChangeSpawnTime(wxSpinEvent& event);
	void OnChangeSpawnSize(wxSpinEvent& event);

	void OnTilesetChange(wxCommandEvent& event);
	void OnListBoxChange(wxCommandEvent& event);
	void OnClickCreatureBrushButton(wxCommandEvent& event);
	void OnClickSpawnBrushButton(wxCommandEvent& event);
	void OnClickLoadNPCsButton(wxCommandEvent& event);
	void OnClickLoadMonstersButton(wxCommandEvent& event);
	void OnClickPurgeCreaturesButton(wxCommandEvent& event);
	void OnClickSearchButton(wxCommandEvent& event);
	void OnSearchFieldText(wxCommandEvent& event);
	void OnSearchFieldFocus(wxFocusEvent& event);
	void OnSearchFieldKillFocus(wxFocusEvent& event);
	void OnSearchFieldKeyDown(wxKeyEvent& event);
	void OnClickViewToggle(wxCommandEvent& event);
	void OnClickViewStyleToggle(wxCommandEvent& event);
	void OnClickLargeSpritesToggle(wxCommandEvent& event);
	void OnClickZoomButton(wxCommandEvent& event);
	void OnSpriteSelected(wxCommandEvent& event);

protected:
	void SelectCreatureBrush();
	void SelectSpawnBrush();
	bool LoadNPCsFromFolder(const wxString& folder);
	bool LoadMonstersFromFolder(const wxString& folder);
	bool PurgeCreaturePalettes();
	void FilterCreatures(const wxString& search_text);
	void PopulateAllCreatureBrushes(const TilesetCategory* tsc);
	BrushVector ComputeVisibleBrushes(const wxString& search_text) const;
	void RebuildCreatureList(const BrushVector& brushes);
	void UpdateCreatureDisplay();
	void SyncCreatureListSelection(const Brush* brush);
	void SetSelectedCreatureBrush(const Brush* brush);
	void ActivateSelectedCreatureBrush();
	void OnCreatureListMouseDown(wxMouseEvent& event);

	BrushVector all_creature_brushes;
	const Brush* selected_creature_brush;

	wxChoice* tileset_choice;
	SortableListBox* creature_list;
	CreatureSpritePanel* sprite_panel;
	CreatureSeamlessGridPanel* seamless_panel;
	wxToggleButton* view_toggle;
	wxToggleButton* view_style_toggle;
	wxToggleButton* large_sprites_toggle;
	wxButton* zoom_button;
	wxSizer* view_sizer;
	bool use_sprite_view;
	bool use_seamless_view;
	bool use_large_sprites;
	int zoom_factor;
	bool handling_event;

	wxTextCtrl* search_field;
	wxButton* search_button;
	wxButton* load_npcs_button;
	wxButton* load_monsters_button;
	wxButton* purge_creatures_button;
	
	wxSpinCtrl* creature_spawntime_spin;
	wxSpinCtrl* spawn_size_spin;
	wxToggleButton* creature_brush_button;
	wxToggleButton* spawn_brush_button;

private:
	static wxLongLong last_search_click;  // For tracking double-clicks on search button

	DECLARE_EVENT_TABLE();
};

#endif
