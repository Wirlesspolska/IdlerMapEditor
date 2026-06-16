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

#include <wx/grid.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>

#include "tile.h"
#include "item.h"
#include "complexitem.h"
#include "town.h"
#include "house.h"
#include "map.h"
#include "editor.h"
#include "creature.h"
#include "materials.h"
#include "tileset.h"

#include "gui.h"
#include "application.h"
#include "add_item_window.h"
#include "container_properties_window.h"
#include "find_item_window.h"
#include "palette_window.h"
#include "palette_brushlist.h"
#include "brush.h"
#include "raw_brush.h"

// Forward declarations
class PaletteWindow;
class BrushPalettePanel;

// ============================================================================
// Add Item Window

static constexpr int OUTFIT_COLOR_MAX = 133;

BEGIN_EVENT_TABLE(AddItemWindow, wxDialog)
EVT_BUTTON(wxID_OK, AddItemWindow::OnClickOK)
EVT_BUTTON(wxID_CANCEL, AddItemWindow::OnClickCancel)
EVT_CHECKBOX(wxID_ANY, AddItemWindow::OnRangeToggle)
EVT_SPINCTRL(wxID_ANY, AddItemWindow::OnRangeFieldChange)
END_EVENT_TABLE()

AddItemWindow::AddItemWindow(wxWindow* win_parent, TilesetCategoryType categoryType, Tileset* tilesetItem, wxPoint pos) :
	ObjectPropertiesWindowBase(win_parent, "Add a Item", pos),
	item_id(0),
	tileset_item(tilesetItem),
	category_type(categoryType),
	item_id_field(nullptr),
	item_id_label(nullptr),
	item_name_label(nullptr),
	item_button(nullptr),
	tileset_choice(nullptr),
	range_checkbox(nullptr),
	range_start_field(nullptr),
	range_end_field(nullptr),
	range_start_label(nullptr),
	range_end_label(nullptr),
	range_info_label(nullptr) {
	wxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);
	wxString description = "Add a Item";

	wxSizer* boxsizer = newd wxStaticBoxSizer(wxVERTICAL, this, description);

	wxFlexGridSizer* subsizer = newd wxFlexGridSizer(2, 10, 10);
	subsizer->AddGrowableCol(1);

	uint16_t itemId = 0;
	std::string itemName = "None";
	item_id_label = newd wxStaticText(this, wxID_ANY, "ID " + i2ws(itemId));
	subsizer->Add(item_id_label);
	item_name_label = newd wxStaticText(this, wxID_ANY, "\"" + wxstr(itemName) + "\"");
	subsizer->Add(item_name_label);

	subsizer->Add(newd wxStaticText(this, wxID_ANY, "Item"), wxSizerFlags(1).CenterVertical());
	item_button = newd DCButton(this, wxID_ANY, wxDefaultPosition, DC_BTN_TOGGLE, RENDER_SIZE_32x32, 0);
	subsizer->Add(item_button);

	subsizer->Add(newd wxStaticText(this, wxID_ANY, "Item Id"));
	item_id_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(itemId), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, 100000);
	item_id_field->Connect(wxEVT_COMMAND_SPINCTRL_UPDATED, wxCommandEventHandler(AddItemWindow::OnChangeItemId), NULL, this);
	subsizer->Add(item_id_field, wxSizerFlags(1).Expand());
	
	// Add range selection checkbox
	subsizer->Add(newd wxStaticText(this, wxID_ANY, "Add multiple items"));
	wxBoxSizer* range_checkbox_sizer = newd wxBoxSizer(wxHORIZONTAL);
	range_checkbox = newd wxCheckBox(this, wxID_ANY, "Add a range of items");
	range_checkbox_sizer->Add(range_checkbox, 1, wxEXPAND | wxRIGHT, 5);
	
	// Add a "Quick Range" button
	wxButton* quick_range_button = newd wxButton(this, wxID_ANY, "Quick +10 Range", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	quick_range_button->Connect(wxEVT_BUTTON, wxCommandEventHandler(AddItemWindow::OnQuickRange), NULL, this);
	quick_range_button->SetToolTip("Quickly set up a range from current ID to current ID + 10");
	range_checkbox_sizer->Add(quick_range_button, 0, wxALIGN_CENTER_VERTICAL);
	
	subsizer->Add(range_checkbox_sizer, wxSizerFlags(1).Expand());
	
	// Add range selection fields (initially hidden)
	range_start_label = newd wxStaticText(this, wxID_ANY, "Range start");
	subsizer->Add(range_start_label);
	range_start_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(itemId), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, 100000);
	range_start_field->Connect(wxEVT_SPINCTRL, wxSpinEventHandler(AddItemWindow::OnRangeFieldChange), NULL, this);
	subsizer->Add(range_start_field, wxSizerFlags(1).Expand());
	
	range_end_label = newd wxStaticText(this, wxID_ANY, "Range end");
	subsizer->Add(range_end_label);
	
	// Create a horizontal sizer for the end field and button
	wxBoxSizer* end_field_sizer = newd wxBoxSizer(wxHORIZONTAL);
	range_end_field = newd wxSpinCtrl(this, wxID_ANY, i2ws(itemId), wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 100, 100000);
	range_end_field->Connect(wxEVT_SPINCTRL, wxSpinEventHandler(AddItemWindow::OnRangeFieldChange), NULL, this);
	end_field_sizer->Add(range_end_field, 1, wxEXPAND | wxRIGHT, 5);
	
	// Add a "Use Current" button
	wxButton* use_current_button = newd wxButton(this, wxID_ANY, "Use Current Item", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	use_current_button->Connect(wxEVT_BUTTON, wxCommandEventHandler(AddItemWindow::OnUseCurrentItem), NULL, this);
	end_field_sizer->Add(use_current_button, 0, wxALIGN_CENTER_VERTICAL);
	
	subsizer->Add(end_field_sizer, wxSizerFlags(1).Expand());
	
	// Info label
	subsizer->Add(newd wxStaticText(this, wxID_ANY, "Status"));
	range_info_label = newd wxStaticText(this, wxID_ANY, "");
	subsizer->Add(range_info_label);
	
	// Initially hide range controls
	UpdateRangeFields(false);
	
	// Add tileset selection dropdown
	subsizer->Add(newd wxStaticText(this, wxID_ANY, "Tileset"));
	tileset_choice = newd wxChoice(this, wxID_ANY);
	
	// Populate the choice with all available tilesets
	int currentSelection = 0;
	int index = 0;
	for (TilesetContainer::iterator iter = g_materials.tilesets.begin(); iter != g_materials.tilesets.end(); ++iter) {
		if (iter->second->getCategory(category_type)) {
			tileset_choice->Append(wxstr(iter->first));
			// Set current selection to the provided tileset if it matches
			if (tileset_item && iter->second == tileset_item) {
				currentSelection = index;
			}
			index++;
		}
	}
	
	// Select the current tileset if available
	if (tileset_choice->GetCount() > 0) {
		tileset_choice->SetSelection(currentSelection);
	}
	
	subsizer->Add(tileset_choice, wxSizerFlags(1).Expand());

	// Add "Quick Selection Range" button that will use selected brushes from palette
	wxButton* quick_selection_button = newd wxButton(this, wxID_ANY, "Add Selection from Palette", 
		wxDefaultPosition, wxDefaultSize, 0);
	quick_selection_button->Connect(wxEVT_BUTTON, wxCommandEventHandler(AddItemWindow::OnQuickRange), NULL, this);
	quick_selection_button->SetToolTip("Add all items currently selected in the palette to the tileset");
	subsizer->Add(newd wxStaticText(this, wxID_ANY, "Quick Add"));
	subsizer->Add(quick_selection_button, wxSizerFlags(1).Expand());

	boxsizer->Add(subsizer, wxSizerFlags(1).Expand());

	topsizer->Add(boxsizer, wxSizerFlags(0).Expand().Border(wxLEFT | wxRIGHT, 20));

	wxSizer* subsizer_ = newd wxBoxSizer(wxHORIZONTAL);
	subsizer_->Add(newd wxButton(this, wxID_OK, "Add"), wxSizerFlags(1).Center().Border(wxTOP | wxBOTTOM, 10));
	subsizer_->Add(newd wxButton(this, wxID_CANCEL, "Cancel"), wxSizerFlags(1).Center().Border(wxTOP | wxBOTTOM, 10));
	topsizer->Add(subsizer_, wxSizerFlags(0).Center().Border(wxLEFT | wxRIGHT, 20));

	SetSizerAndFit(topsizer);
	Centre(wxBOTH);

	item_button->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(AddItemWindow::OnItemClicked), NULL, this);
}

void AddItemWindow::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	// Get the selected tileset name from the dropdown
	wxString tilesetName;
	if (tileset_choice && tileset_choice->GetSelection() != wxNOT_FOUND) {
		tilesetName = tileset_choice->GetString(tileset_choice->GetSelection());
	}
	
	// If no tileset is selected, use the initial tileset
	if (tilesetName.IsEmpty() && tileset_item) {
		tilesetName = wxstr(tileset_item->name);
	}
	
	// Make sure we have a valid tileset name
	if (tilesetName.IsEmpty()) {
		g_gui.PopupDialog("Error", "No tileset selected", wxOK);
		return;
	}
	
	std::string stdTilesetName = nstr(tilesetName);
	
	// Check if we're in range mode
	if (range_checkbox && range_checkbox->GetValue() && 
		range_start_field && range_end_field) {
		
		int start = range_start_field->GetValue();
		int end = range_end_field->GetValue();
		
		// Validate range
		if (start > end) {
			g_gui.PopupDialog("Error", "Invalid range: start ID is greater than end ID", wxOK);
			return;
		}
		
		// Add all items in range
		int added_count = 0;
		for (int id = start; id <= end; id++) {
			const ItemType& it = g_items.getItemType(id);
			if (it.id != 0) {
				g_materials.addToTileset(stdTilesetName, it.id, category_type);
				added_count++;
			}
		}
		
		// Save changes
		g_materials.modify();
		
		// Show success message
		if (added_count > 0) {
			g_gui.PopupDialog("Items Added", 
				wxString::Format("Successfully added %d items (IDs %d-%d) to tileset '%s'", 
				added_count, start, end, tilesetName), wxOK);
			EndModal(1);
		} else {
			g_gui.PopupDialog("Error", "No valid items found in the specified range", wxOK);
		}
	} 
	// Single item mode
	else {
		const ItemType& it = g_items.getItemType(item_id_field->GetValue());
		if (it.id != 0) {
			g_materials.addToTileset(stdTilesetName, it.id, category_type);
			g_materials.modify();
			g_gui.PopupDialog("Item Added", 
				wxString::Format("'%s' (ID: %d) has been added to tileset '%s'", 
				wxString(it.name.c_str()), it.id, tilesetName), wxOK);
			EndModal(1);
		} else {
			g_gui.PopupDialog("Error", "You need to select an item", wxOK);
		}
	}
}

void AddItemWindow::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	// Just close this window
	EndModal(0);
}

void AddItemWindow::OnChangeItemId(wxCommandEvent& WXUNUSED(event)) {
	uint16_t itemId = item_id_field->GetValue();
	ItemType& it = g_items[itemId];
	if (it.id != 0) {
		item_id_label->SetLabelText("ID " + i2ws(it.id));
		item_name_label->SetLabelText("\"" + wxstr(it.name) + "\"");

		item_button->SetSprite(it.clientID);
		
		// Update range start field if range mode is enabled
		if (range_checkbox && range_checkbox->GetValue() && range_start_field) {
			range_start_field->SetValue(itemId);
			UpdateRangeInfo();
		}
	} else {
		item_id_field->SetValue(100);
	}
}

void AddItemWindow::OnItemClicked(wxMouseEvent& WXUNUSED(event)) {
	// Get the GUI instance to access the current palette and selected brush
	GUI* gui = &g_gui;
	if (!gui) return;
	
	// Get the selected brush from the current palette
	Brush* brush = gui->GetCurrentBrush();
	if (brush) {
		uint16_t id = 0;
		
		// Try to get the ID from the brush
		if (brush->isRaw()) {
			RAWBrush* raw_brush = brush->asRaw();
			if (raw_brush) {
				id = raw_brush->getItemID();
			}
		}
		
		// If we got a valid ID, set it
		if (id > 0) {
			SetItemIdToItemButton(id);
			return;
		}
	}
	
	// Fall back to the FindItemDialog if needed
	FindItemDialog dialog(this, "Item");
	if (dialog.ShowModal() == wxID_OK) {
		uint16_t id = dialog.getResultID();
		SetItemIdToItemButton(id);
	}
	dialog.Destroy();
}

void AddItemWindow::SetItemIdToItemButton(uint16_t id) {
	if (!item_button) {
		return;
	}

	if (id != 0) {
		const ItemType& it = g_items.getItemType(id);
		if (it.id != 0) {
			item_id_field->SetValue(it.id);
			item_id_label->SetLabelText("ID " + i2ws(it.id));
			item_name_label->SetLabelText("\"" + wxstr(it.name) + "\"");

			item_button->SetSprite(it.clientID);
			return;
		}
	}

	item_button->SetSprite(0);
}

void AddItemWindow::UpdateRangeFields(bool show) {
	if (range_start_label) range_start_label->Show(show);
	if (range_start_field) range_start_field->Show(show);
	if (range_end_label) range_end_label->Show(show);
	if (range_end_field) range_end_field->Show(show);
	if (range_info_label) range_info_label->Show(show);
	
	// Set range fields values if showing
	if (show && item_id_field) {
		int current_id = item_id_field->GetValue();
		if (range_start_field) range_start_field->SetValue(current_id);
		if (range_end_field) range_end_field->SetValue(current_id + 10); // Default to 10 item range
		
		// Update info label
		UpdateRangeInfo();
	}
	
	Layout();
	Fit();
}

void AddItemWindow::UpdateRangeInfo() {
	if (!range_info_label || !range_start_field || !range_end_field) 
		return;
	
	int start = range_start_field->GetValue();
	int end = range_end_field->GetValue();
	
	if (start > end) {
		range_info_label->SetLabel(wxString::Format("Invalid range (start > end)"));
		range_info_label->SetForegroundColour(*wxRED);
	} else {
		int count = end - start + 1;
		range_info_label->SetLabel(wxString::Format("%d items will be added", count));
		range_info_label->SetForegroundColour(*wxBLACK);
	}
}

void AddItemWindow::OnRangeToggle(wxCommandEvent& event) {
	if (range_checkbox) {
		bool checked = range_checkbox->GetValue();
		UpdateRangeFields(checked);
	}
}

void AddItemWindow::OnRangeFieldChange(wxSpinEvent& event) {
	UpdateRangeInfo();
	
	// If the event is from the end field, update the sprite preview
	if (event.GetId() == range_end_field->GetId()) {
		uint16_t id = range_end_field->GetValue();
		const ItemType& it = g_items.getItemType(id);
		if (it.id != 0) {
			item_id_field->SetValue(id);
			item_id_label->SetLabelText("ID " + i2ws(it.id));
			item_name_label->SetLabelText("\"" + wxstr(it.name) + "\"");
			item_button->SetSprite(it.clientID);
		}
	}
}

void AddItemWindow::OnUseCurrentItem(wxCommandEvent& event) {
	// Get the current item ID from the field instead of the button
	if (item_id_field) {
		uint16_t id = item_id_field->GetValue();
		if (id != 0) {
			range_end_field->SetValue(id);
			UpdateRangeInfo();
		}
	}
}

void AddItemWindow::OnQuickRange(wxCommandEvent& WXUNUSED(event)) {
	// For now, just set a range of +10 items from the current item ID
	if (item_id_field) {
		uint16_t id = item_id_field->GetValue();
		if (id != 0) {
			// Enable range mode
			if (range_checkbox) {
				range_checkbox->SetValue(true);
				UpdateRangeFields(true);
			}
			
			// Set range start to current ID
			if (range_start_field) {
				range_start_field->SetValue(id);
			}
			
			// Set range end to current ID + 10
			if (range_end_field) {
				range_end_field->SetValue(id + 10);
			}
			
			// Update range info
			UpdateRangeInfo();
		}
	}
}
