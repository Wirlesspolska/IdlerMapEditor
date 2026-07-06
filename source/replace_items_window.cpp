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
#include "replace_items_window.h"
#include "find_item_window.h"
#include "graphics.h"
#include "gui.h"
#include "artprovider.h"
#include "items.h"
#include "brush.h"
#include "ground_brush.h"
#include "wall_brush.h"
#include "doodad_brush.h"
#include "string_utils.h"
#include <wx/dir.h>
#include <wx/tokenzr.h>
#include <algorithm>

/*
Current Task:
--------------
1. Enhance the ReplaceItemsDialog UI by expanding the window vertically to better display additional controls.
2. Retain the current border dropdown selection functionality that shows preview images for border choices.
3. Add new wall selection functionality:
   - Utilize data from the walls.xml file (e.g., data/760/walls.xml) to populate wall choices.
   - Create a dedicated wall selection section similar to borders.
   - Provide two input boxes for walls:
   withIds and ReplaceIds just like borders

   - Add an "Add Walls" button analogous to the "Add Border Items" button for adding a wall replacement rule.
4. Ensure that preview functionality works for wall selections as it does for borders.
5. Update event handlers and sizer layouts accordingly to accommodate and properly layout these new controls.
6. Reference external files:
   - Use walls.xml (located at data/760/walls.xml) as the source of wall data.
   - Display previews and handle selections based on wall properties defined in this XML.

Overall, the modifications should maintain a consistent UI style while providing expanded functionality for wall replacements, parallel to how border replacements are managed.
*/


ReplaceItemsButton::ReplaceItemsButton(wxWindow* parent) :
	DCButton(parent, wxID_ANY, wxDefaultPosition, DC_BTN_TOGGLE, RENDER_SIZE_32x32, 0),
	m_id(0) {
	////
}

ItemGroup_t ReplaceItemsButton::GetGroup() const {
	if (m_id != 0) {
		const ItemType& it = g_items.getItemType(m_id);
		if (it.id != 0) {
			return it.group;
		}
	}
	return ITEM_GROUP_NONE;
}

void ReplaceItemsButton::SetItemId(uint16_t id) {
	if (m_id == id) {
		return;
	}

	m_id = id;

	if (m_id != 0) {
		const ItemType& it = g_items.getItemType(m_id);
		if (it.id != 0) {
			SetSprite(it.clientID);
			return;
		}
	}

	SetSprite(0);
}

// ============================================================================
// ReplaceItemsListBox

ReplaceItemsListBox::ReplaceItemsListBox(wxWindow* parent) :
	wxVListBox(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLB_SINGLE) {
	m_arrow_bitmap = wxArtProvider::GetBitmap(ART_POSITION_GO, wxART_TOOLBAR, wxSize(16, 16));
	m_flag_bitmap = wxArtProvider::GetBitmap(ART_PZ_BRUSH, wxART_TOOLBAR, wxSize(16, 16));
}

bool ReplaceItemsListBox::AddItem(const ReplacingItem& item) {
	if (item.replaceId == 0 || item.withId == 0 || item.replaceId == item.withId) {
		return false;
	}

	m_items.push_back(item);
	SetItemCount(m_items.size());
	Refresh();

	return true;
}

void ReplaceItemsListBox::MarkAsComplete(const ReplacingItem& item, uint32_t total) {
	auto it = std::find(m_items.begin(), m_items.end(), item);
	if (it != m_items.end()) {
		it->total = total;
		it->complete = true;
		Refresh();
	}
}

void ReplaceItemsListBox::RemoveSelected() {
	if (m_items.empty()) {
		return;
	}

	const int index = GetSelection();
	if (index == wxNOT_FOUND) {
		return;
	}

	m_items.erase(m_items.begin() + index);
	SetItemCount(GetItemCount() - 1);
	Refresh();
}

bool ReplaceItemsListBox::CanAdd(uint16_t replaceId, uint16_t withId) const {
	if (replaceId == 0 || withId == 0 || replaceId == withId) {
		return false;
	}

	for (const ReplacingItem& item : m_items) {
		if (replaceId == item.replaceId) {
			return false;
		}
	}
	return true;
}

void ReplaceItemsListBox::OnDrawItem(wxDC& dc, const wxRect& rect, size_t index) const {
	ASSERT(index < m_items.size());

	const ReplacingItem& item = m_items.at(index);
	const ItemType& type1 = g_items.getItemType(item.replaceId);
	Sprite* sprite1 = g_gui.gfx.getSprite(type1.clientID);
	const ItemType& type2 = g_items.getItemType(item.withId);
	Sprite* sprite2 = g_gui.gfx.getSprite(type2.clientID);

	if (sprite1 && sprite2) {
		int x = rect.GetX();
		int y = rect.GetY();
		sprite1->DrawTo(&dc, SPRITE_SIZE_32x32, x + 4, y + 4, rect.GetWidth(), rect.GetHeight());
		dc.DrawBitmap(m_arrow_bitmap, x + 38, y + 10, true);
		sprite2->DrawTo(&dc, SPRITE_SIZE_32x32, x + 56, y + 4, rect.GetWidth(), rect.GetHeight());
		dc.DrawText(wxString::Format("Replace: %d With: %d", item.replaceId, item.withId), x + 104, y + 10);

		if (item.complete) {
			x = rect.GetWidth() - 100;
			dc.DrawBitmap(m_flag_bitmap, x + 70, y + 10, true);
			dc.DrawText(wxString::Format("Total: %d", item.total), x, y + 10);
		}
	}

	if (IsSelected(index)) {
		if (HasFocus()) {
			dc.SetTextForeground(wxColor(0xFF, 0xFF, 0xFF));
		} else {
			dc.SetTextForeground(wxColor(0x00, 0x00, 0xFF));
		}
	} else {
		dc.SetTextForeground(wxColor(0x00, 0x00, 0x00));
	}
}

wxCoord ReplaceItemsListBox::OnMeasureItem(size_t WXUNUSED(index)) const {
	return 40;
}

void ReplaceItemsListBox::Clear() {
	m_items.clear();
	SetItemCount(0);
	Refresh();
	Update();
}

void ReplaceItemsListBox::SetItems(const std::vector<ReplacingItem>& items) {
	m_items = items;
	SetItemCount(m_items.size());
	Refresh();
}

// ============================================================================
// ReplaceItemsDialog

ReplaceItemsDialog::ReplaceItemsDialog(wxWindow* parent, bool selectionOnly) :
	wxDialog(parent, wxID_ANY, (selectionOnly ? "Replace Items on Selection" : "Replace Items"),
		wxDefaultPosition, wxSize(900, 560), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
	selectionOnly(selectionOnly),
	preview_source_(PreviewSource::Manual) {
	const int pad = 6;

	wxBoxSizer* root_sizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

	// Replacement list
	list = new ReplaceItemsListBox(this);
	list->SetMinSize(wxSize(-1, 140));
	main_sizer->Add(list, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, pad);

	progress = new wxGauge(this, wxID_ANY, 100, wxDefaultPosition, wxSize(-1, 18));
	main_sizer->Add(progress, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, pad);

	// Manual item replace row
	{
		wxStaticBoxSizer* items_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "Replace Items");

		wxBoxSizer* row = new wxBoxSizer(wxHORIZONTAL);

		wxBoxSizer* replace_column = new wxBoxSizer(wxVERTICAL);
		replace_button = new ReplaceItemsButton(this);
		replace_column->Add(replace_button, 0, wxALIGN_CENTER_HORIZONTAL);
		replace_range_input = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(120, -1));
		replace_range_input->SetToolTip("Enter IDs or ranges separated by commas (e.g., 100-105,200)");
		replace_column->Add(replace_range_input, 0, wxEXPAND | wxTOP, 4);
		row->Add(replace_column, 0, wxALIGN_CENTER_VERTICAL);

		arrow_bitmap = new wxStaticBitmap(this, wxID_ANY, wxArtProvider::GetBitmap(wxART_GO_FORWARD));
		row->Add(arrow_bitmap, 0, wxALIGN_CENTER_VERTICAL | wxLEFT | wxRIGHT, 8);

		wxBoxSizer* with_column = new wxBoxSizer(wxVERTICAL);
		with_button = new ReplaceItemsButton(this);
		with_column->Add(with_button, 0, wxALIGN_CENTER_HORIZONTAL);
		with_range_input = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(120, -1));
		with_range_input->SetToolTip("Enter IDs or ranges separated by commas (e.g., 200-205,300)");
		with_column->Add(with_range_input, 0, wxEXPAND | wxTOP, 4);
		row->Add(with_column, 0, wxALIGN_CENTER_VERTICAL);

		add_button = new wxButton(this, wxID_ANY, "Add");
		row->Add(add_button, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);

		items_sizer->Add(row, 0, wxEXPAND | wxALL, pad);
		main_sizer->Add(items_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, pad);
	}

	// Border replace
	{
		wxStaticBoxSizer* border_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "Replace Borders");

		wxBoxSizer* border_selection_sizer = new wxBoxSizer(wxHORIZONTAL);
		border_from_choice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
		border_to_choice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
		border_from_choice->SetMinSize(wxSize(180, -1));
		border_to_choice->SetMinSize(wxSize(180, -1));
		border_selection_sizer->Add(border_from_choice, 1, wxEXPAND | wxRIGHT, pad);
		border_selection_sizer->Add(border_to_choice, 1, wxEXPAND);
		border_sizer->Add(border_selection_sizer, 0, wxEXPAND | wxALL, pad);

		add_border_button = new wxButton(this, wxID_ANY, "Add Border Items");
		border_sizer->Add(add_border_button, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM, pad);

		main_sizer->Add(border_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, pad);
	}

	// Wall replace
	{
		wxStaticBoxSizer* wall_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "Replace Walls");

		wxBoxSizer* wall_selection_sizer = new wxBoxSizer(wxHORIZONTAL);
		wall_from_choice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
		wall_to_choice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
		wall_orientation_choice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(90, -1));
		wall_from_choice->SetMinSize(wxSize(150, -1));
		wall_to_choice->SetMinSize(wxSize(150, -1));

		wall_orientation_choice->Append("All");
		wall_orientation_choice->Append("Horizontal");
		wall_orientation_choice->Append("Vertical");
		wall_orientation_choice->Append("Corner");
		wall_orientation_choice->Append("Pole");
		wall_orientation_choice->SetSelection(0);

		wall_selection_sizer->Add(wall_from_choice, 1, wxEXPAND | wxRIGHT, pad);
		wall_selection_sizer->Add(wall_to_choice, 1, wxEXPAND | wxRIGHT, pad);
		wall_selection_sizer->Add(wall_orientation_choice, 0, wxEXPAND);
		wall_sizer->Add(wall_selection_sizer, 0, wxEXPAND | wxALL, pad);

		add_wall_button = new wxButton(this, wxID_ANY, "Add Wall Items");
		wall_sizer->Add(add_wall_button, 0, wxALIGN_LEFT | wxLEFT | wxRIGHT | wxBOTTOM, pad);

		main_sizer->Add(wall_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, pad);
	}

	// Presets
	{
		wxStaticBoxSizer* preset_sizer = new wxStaticBoxSizer(wxHORIZONTAL, this, "Presets");
		preset_choice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
		preset_choice->SetMinSize(wxSize(140, -1));
		preset_sizer->Add(preset_choice, 1, wxEXPAND | wxRIGHT, pad);

		load_preset_button = new wxButton(this, wxID_ANY, wxT("Load"));
		add_preset_button = new wxButton(this, wxID_ANY, wxT("Add Preset"));
		remove_preset_button = new wxButton(this, wxID_ANY, wxT("Remove Preset"));
		preset_sizer->Add(load_preset_button, 0, wxRIGHT, pad);
		preset_sizer->Add(add_preset_button, 0, wxRIGHT, pad);
		preset_sizer->Add(remove_preset_button, 0);

		main_sizer->Add(preset_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, pad);
	}

	// Bottom action bar
	{
		wxBoxSizer* buttons_sizer = new wxBoxSizer(wxHORIZONTAL);

		remove_button = new wxButton(this, wxID_ANY, wxT("Remove"));
		remove_button->Enable(false);
		execute_button = new wxButton(this, wxID_ANY, wxT("Execute"));
		execute_button->Enable(false);
		buttons_sizer->Add(remove_button, 0, wxRIGHT, pad);
		buttons_sizer->Add(execute_button, 0);

		buttons_sizer->AddStretchSpacer();

		swap_checkbox = new wxCheckBox(this, wxID_ANY, "Swap Items");
		swap_checkbox->SetToolTip("When checked, items will be swapped instead of just replaced");
		buttons_sizer->Add(swap_checkbox, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, pad);

		close_button = new wxButton(this, wxID_ANY, wxT("Close"));
		buttons_sizer->Add(close_button, 0);

		main_sizer->Add(buttons_sizer, 0, wxEXPAND | wxALL, pad);
	}

	root_sizer->Add(main_sizer, 1, wxEXPAND | wxALL, pad);

	{
		wxStaticBoxSizer* preview_sizer = new wxStaticBoxSizer(wxVERTICAL, this, "Preview");
		preview_list = new ReplaceItemsListBox(this);
		preview_list->SetMinSize(wxSize(300, 200));
		preview_sizer->Add(preview_list, 1, wxEXPAND | wxALL, pad);
		root_sizer->Add(preview_sizer, 1, wxEXPAND | wxTOP | wxBOTTOM | wxRIGHT, pad);
	}

	SetSizer(root_sizer);
	Layout();
	SetMinSize(wxSize(820, 480));
	Centre(wxBOTH);

	// Connect Events
	list->Connect(wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler(ReplaceItemsDialog::OnListSelected), NULL, this);
	replace_button->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(ReplaceItemsDialog::OnReplaceItemClicked), NULL, this);
	with_button->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(ReplaceItemsDialog::OnWithItemClicked), NULL, this);
	add_button->Connect(wxEVT_BUTTON, wxCommandEventHandler(ReplaceItemsDialog::OnAddButtonClicked), NULL, this);
	remove_button->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ReplaceItemsDialog::OnRemoveButtonClicked), NULL, this);
	execute_button->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ReplaceItemsDialog::OnExecuteButtonClicked), NULL, this);
	close_button->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ReplaceItemsDialog::OnCancelButtonClicked), NULL, this);
	preset_choice->Connect(wxEVT_CHOICE, wxCommandEventHandler(ReplaceItemsDialog::OnPresetSelect), NULL, this);
	add_preset_button->Connect(wxEVT_BUTTON, wxCommandEventHandler(ReplaceItemsDialog::OnAddPreset), NULL, this);
	remove_preset_button->Connect(wxEVT_BUTTON, wxCommandEventHandler(ReplaceItemsDialog::OnRemovePreset), NULL, this);
	load_preset_button->Connect(wxEVT_BUTTON, wxCommandEventHandler(ReplaceItemsDialog::OnLoadPreset), NULL, this);
	swap_checkbox->Connect(wxEVT_CHECKBOX, wxCommandEventHandler(ReplaceItemsDialog::OnSwapCheckboxClicked), NULL, this);
	border_from_choice->Connect(wxEVT_CHOICE, wxCommandEventHandler(ReplaceItemsDialog::OnBorderFromSelect), NULL, this);
	border_to_choice->Connect(wxEVT_CHOICE, wxCommandEventHandler(ReplaceItemsDialog::OnBorderToSelect), NULL, this);
	add_border_button->Connect(wxEVT_BUTTON, wxCommandEventHandler(ReplaceItemsDialog::OnAddBorderItems), NULL, this);
	add_wall_button->Connect(wxEVT_BUTTON, wxCommandEventHandler(ReplaceItemsDialog::OnAddWallItems), NULL, this);
	wall_from_choice->Connect(wxEVT_CHOICE, wxCommandEventHandler(ReplaceItemsDialog::OnWallFromSelect), NULL, this);
	wall_to_choice->Connect(wxEVT_CHOICE, wxCommandEventHandler(ReplaceItemsDialog::OnWallToSelect), NULL, this);
	wall_orientation_choice->Connect(wxEVT_CHOICE, wxCommandEventHandler(ReplaceItemsDialog::OnWallOrientationSelect), NULL, this);
	replace_range_input->Bind(wxEVT_TEXT, &ReplaceItemsDialog::OnIdInput, this);
	with_range_input->Bind(wxEVT_TEXT, &ReplaceItemsDialog::OnIdInput, this);

	// Load initial data
	RefreshPresetList();
	LoadBorderChoices();
	LoadWallChoices();
	add_border_button->Enable(false);
	add_wall_button->Enable(false);
	UpdatePreviewList();
	UpdateWidgets();
}

ReplaceItemsDialog::~ReplaceItemsDialog() {
	// Disconnect Events
	list->Disconnect(wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler(ReplaceItemsDialog::OnListSelected), NULL, this);
	replace_button->Disconnect(wxEVT_LEFT_DOWN, wxMouseEventHandler(ReplaceItemsDialog::OnReplaceItemClicked), NULL, this);
	with_button->Disconnect(wxEVT_LEFT_DOWN, wxMouseEventHandler(ReplaceItemsDialog::OnWithItemClicked), NULL, this);
	add_button->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(ReplaceItemsDialog::OnAddButtonClicked), NULL, this);
	remove_button->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ReplaceItemsDialog::OnRemoveButtonClicked), NULL, this);
	execute_button->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ReplaceItemsDialog::OnExecuteButtonClicked), NULL, this);
	close_button->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ReplaceItemsDialog::OnCancelButtonClicked), NULL, this);
	preset_choice->Disconnect(wxEVT_CHOICE, wxCommandEventHandler(ReplaceItemsDialog::OnPresetSelect), NULL, this);
	add_preset_button->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(ReplaceItemsDialog::OnAddPreset), NULL, this);
	remove_preset_button->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(ReplaceItemsDialog::OnRemovePreset), NULL, this);
	load_preset_button->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(ReplaceItemsDialog::OnLoadPreset), NULL, this);
	swap_checkbox->Disconnect(wxEVT_CHECKBOX, wxCommandEventHandler(ReplaceItemsDialog::OnSwapCheckboxClicked), NULL, this);
	border_from_choice->Disconnect(wxEVT_CHOICE, wxCommandEventHandler(ReplaceItemsDialog::OnBorderFromSelect), NULL, this);
	border_to_choice->Disconnect(wxEVT_CHOICE, wxCommandEventHandler(ReplaceItemsDialog::OnBorderToSelect), NULL, this);
	add_border_button->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(ReplaceItemsDialog::OnAddBorderItems), NULL, this);
	add_wall_button->Disconnect(wxEVT_BUTTON, wxCommandEventHandler(ReplaceItemsDialog::OnAddWallItems), NULL, this);
	wall_from_choice->Disconnect(wxEVT_CHOICE, wxCommandEventHandler(ReplaceItemsDialog::OnWallFromSelect), NULL, this);
	wall_to_choice->Disconnect(wxEVT_CHOICE, wxCommandEventHandler(ReplaceItemsDialog::OnWallToSelect), NULL, this);
	wall_orientation_choice->Disconnect(wxEVT_CHOICE, wxCommandEventHandler(ReplaceItemsDialog::OnWallOrientationSelect), NULL, this);
}

void ReplaceItemsDialog::SetPreviewSource(PreviewSource source) {
	preview_source_ = source;
	UpdatePreviewList();
}

void ReplaceItemsDialog::UpdatePreviewList() {
	std::vector<ReplacingItem> items;
	switch (preview_source_) {
		case PreviewSource::Border:
			items = BuildBorderPreviewItems();
			break;
		case PreviewSource::Wall:
			items = BuildWallPreviewItems();
			break;
		case PreviewSource::Manual:
		default:
			items = BuildManualPreviewItems();
			break;
	}

	preview_list->SetItems(items);

	if (!items.empty()) {
		replace_button->SetItemId(items.front().replaceId);
		with_button->SetItemId(items.front().withId);
	} else if (preview_source_ != PreviewSource::Manual) {
		replace_button->SetItemId(0);
		with_button->SetItemId(0);
	}

	add_border_button->Enable(preview_source_ == PreviewSource::Border && !items.empty());
	add_wall_button->Enable(preview_source_ == PreviewSource::Wall && !items.empty());
	UpdateAddButtonState();
}

void ReplaceItemsDialog::AddPreviewItemsToList() {
	for (const ReplacingItem& item : preview_list->GetItems()) {
		if (list->CanAdd(item.replaceId, item.withId)) {
			ReplacingItem copy = item;
			copy.complete = false;
			copy.total = 0;
			list->AddItem(copy);
		}
	}
	UpdateWidgets();
	list->Refresh();
}

std::vector<ReplacingItem> ReplaceItemsDialog::BuildBorderPreviewItems() const {
	std::vector<ReplacingItem> items;
	const int fromIdx = border_from_choice->GetSelection();
	const int toIdx = border_to_choice->GetSelection();
	if (fromIdx <= 0 || toIdx <= 0) {
		return items;
	}

	const int fromXmlIdx = GetBorderXmlIndex(fromIdx);
	const int toXmlIdx = GetBorderXmlIndex(toIdx);
	if (fromXmlIdx < 0 || toXmlIdx < 0) {
		return items;
	}

	wxString dataDir = GetDataDirectoryForVersion(g_gui.GetCurrentVersion().getName());
	if (dataDir.IsEmpty()) {
		return items;
	}

	wxString bordersPath = g_gui.GetDataDirectory() + "/" + dataDir + "/borders.xml";
	pugi::xml_document doc;
	if (!doc.load_file(bordersPath.mb_str())) {
		return items;
	}

	std::map<std::string, uint16_t> fromItems;
	std::map<std::string, uint16_t> toItems;

	int currentBorder = 0;
	for (pugi::xml_node borderNode = doc.child("materials").child("border"); borderNode; borderNode = borderNode.next_sibling("border")) {
		if (currentBorder == fromXmlIdx || currentBorder == toXmlIdx) {
			for (pugi::xml_node itemNode = borderNode.child("borderitem"); itemNode; itemNode = itemNode.next_sibling("borderitem")) {
				std::string edge = itemNode.attribute("edge").value();
				uint16_t itemId = itemNode.attribute("item").as_uint();

				if (currentBorder == fromXmlIdx) {
					fromItems[edge] = itemId;
				} else {
					toItems[edge] = itemId;
				}
			}
		}
		currentBorder++;
	}

	for (const auto& pair : fromItems) {
		auto toIt = toItems.find(pair.first);
		if (toIt != toItems.end() && pair.second != 0 && toIt->second != 0 && pair.second != toIt->second) {
			ReplacingItem item;
			item.replaceId = pair.second;
			item.withId = toIt->second;
			items.push_back(item);
		}
	}

	return items;
}

std::vector<ReplacingItem> ReplaceItemsDialog::BuildWallPreviewItems() const {
	std::vector<ReplacingItem> items;
	const int fromIdx = wall_from_choice->GetSelection();
	const int toIdx = wall_to_choice->GetSelection();
	const int fromXmlIdx = GetWallXmlIndex(fromIdx);
	const int toXmlIdx = GetWallXmlIndex(toIdx);
	if (fromXmlIdx < 0 || toXmlIdx < 0) {
		return items;
	}

	wxString dataDir = GetDataDirectoryForVersion(g_gui.GetCurrentVersion().getName());
	if (dataDir.IsEmpty()) {
		return items;
	}

	wxString wallsPath = g_gui.GetDataDirectory() + "/" + dataDir + "/walls.xml";
	pugi::xml_document doc;
	if (!doc.load_file(wallsPath.mb_str())) {
		return items;
	}

	const wxString orientation = wall_orientation_choice->GetStringSelection().Lower();

	pugi::xml_node fromBrush, toBrush;
	int currentWall = 0;
	for (pugi::xml_node brushNode = doc.child("materials").child("brush");
		brushNode; brushNode = brushNode.next_sibling("brush")) {

		if (std::string(brushNode.attribute("type").value()) == "wall") {
			if (currentWall == fromXmlIdx) {
				fromBrush = brushNode;
			} else if (currentWall == toXmlIdx) {
				toBrush = brushNode;
			}

			if (fromBrush && toBrush) {
				break;
			}
			currentWall++;
		}
	}

	if (!fromBrush || !toBrush) {
		return items;
	}

	auto addPair = [&items](uint16_t fromId, uint16_t toId) {
		if (fromId != 0 && toId != 0 && fromId != toId) {
			ReplacingItem item;
			item.replaceId = fromId;
			item.withId = toId;
			items.push_back(item);
		}
	};

	for (pugi::xml_node fromWallNode = fromBrush.child("wall");
		fromWallNode; fromWallNode = fromWallNode.next_sibling("wall")) {

		const std::string wallType = fromWallNode.attribute("type").value();
		if (orientation != "all" && orientation != wallType) {
			continue;
		}

		pugi::xml_node toWallNode = toBrush.find_child_by_attribute("wall", "type", wallType.c_str());
		if (!toWallNode) {
			continue;
		}

		for (pugi::xml_node fromItem = fromWallNode.child("item");
			fromItem; fromItem = fromItem.next_sibling("item")) {

			const uint16_t fromItemId = fromItem.attribute("id").as_uint();
			std::vector<pugi::xml_node> availableItems;
			for (pugi::xml_node toItem = toWallNode.child("item"); toItem; toItem = toItem.next_sibling("item")) {
				availableItems.push_back(toItem);
			}

			if (!availableItems.empty()) {
				int fromPos = 0;
				for (pugi::xml_node tempItem = fromWallNode.child("item"); tempItem && tempItem != fromItem; tempItem = tempItem.next_sibling("item")) {
					fromPos++;
				}
				const size_t targetPos = std::min(fromPos, static_cast<int>(availableItems.size()) - 1);
				addPair(fromItemId, availableItems[targetPos].attribute("id").as_uint());
			}
		}

		for (pugi::xml_node fromDoor = fromWallNode.child("door");
			fromDoor; fromDoor = fromDoor.next_sibling("door")) {

			const std::string doorType = fromDoor.attribute("type").value();
			const bool isOpen = fromDoor.attribute("open").as_bool();
			const bool isLocked = fromDoor.attribute("locked").as_bool();

			pugi::xml_node toDoor = toWallNode.child("door");
			pugi::xml_node fallbackDoor;
			while (toDoor) {
				if (toDoor.attribute("type").value() == doorType &&
					toDoor.attribute("open").as_bool() == isOpen &&
					toDoor.attribute("locked").as_bool() == isLocked) {
					break;
				}
				if (!fallbackDoor) {
					fallbackDoor = toDoor;
				}
				toDoor = toDoor.next_sibling("door");
			}

			if (!toDoor) {
				toDoor = fallbackDoor;
			}

			if (toDoor) {
				addPair(fromDoor.attribute("id").as_uint(), toDoor.attribute("id").as_uint());
			}
		}
	}

	return items;
}

std::vector<ReplacingItem> ReplaceItemsDialog::BuildManualPreviewItems() const {
	std::vector<ReplacingItem> items;
	const wxString replaceStr = replace_range_input->GetValue().Trim();
	const wxString withStr = with_range_input->GetValue().Trim();
	const uint16_t replaceButtonId = replace_button->GetItemId();
	const uint16_t withButtonId = with_button->GetItemId();

	if (replaceStr.IsEmpty() && withStr.IsEmpty()) {
		if (replaceButtonId != 0 && withButtonId != 0 && replaceButtonId != withButtonId) {
			ReplacingItem item;
			item.replaceId = replaceButtonId;
			item.withId = withButtonId;
			items.push_back(item);
		}
		return items;
	}

	std::vector<uint16_t> replaceIds = FlattenRanges(ParseRangeString(replaceStr));
	if (replaceIds.empty() && replaceButtonId != 0) {
		replaceIds.push_back(replaceButtonId);
	}

	std::vector<uint16_t> withIds = FlattenRanges(ParseRangeString(withStr));
	if (withIds.empty() && withButtonId != 0) {
		withIds.push_back(withButtonId);
	}

	if (replaceIds.empty() || withIds.empty()) {
		return items;
	}

	for (size_t i = 0; i < replaceIds.size(); ++i) {
		const uint16_t fromId = replaceIds[i];
		const uint16_t toId = withIds[std::min(i, withIds.size() - 1)];
		if (fromId != 0 && toId != 0 && fromId != toId) {
			ReplacingItem item;
			item.replaceId = fromId;
			item.withId = toId;
			items.push_back(item);
		}
	}

	return items;
}

void ReplaceItemsDialog::UpdateWidgets() {
	UpdateAddButtonState();
	remove_button->Enable(list->GetCount() != 0 && list->GetSelection() != wxNOT_FOUND);
	execute_button->Enable(list->GetCount() != 0);
}

void ReplaceItemsDialog::OnListSelected(wxCommandEvent& WXUNUSED(event)) {
	remove_button->Enable(list->GetCount() != 0 && list->GetSelection() != wxNOT_FOUND);
}

void ReplaceItemsDialog::OnReplaceItemClicked(wxMouseEvent& WXUNUSED(event)) {
	OutputDebugStringA("ReplaceItemsDialog::OnReplaceItemClicked called\n");
	
	const Brush* brush = g_gui.GetCurrentBrush();
	uint16_t id = getActualItemIdFromBrush(brush);

	if (id != 0) {
		replace_button->SetItemId(id);
		SetPreviewSource(PreviewSource::Manual);
		UpdateWidgets();
		OutputDebugStringA(wxString::Format("Final Replace Item ID set: %d\n", id).c_str());
	} else {
		OutputDebugStringA("ReplaceItemsDialog::OnReplaceItemClicked: Could not resolve item ID from brush\n");
	}
}

void ReplaceItemsDialog::OnWithItemClicked(wxMouseEvent& WXUNUSED(event)) {
	OutputDebugStringA("ReplaceItemsDialog::OnWithItemClicked called\n");

	if (replace_button->GetItemId() == 0) {
		OutputDebugStringA("ReplaceItemsDialog::OnWithItemClicked: Replace button has no item selected\n");
		return;
	}

	const Brush* brush = g_gui.GetCurrentBrush();
	uint16_t id = getActualItemIdFromBrush(brush);

	if (id != 0) {
		with_button->SetItemId(id);
		SetPreviewSource(PreviewSource::Manual);
		UpdateWidgets();
		OutputDebugStringA(wxString::Format("Final With Item ID set: %d\n", id).c_str());
	} else {
		OutputDebugStringA("ReplaceItemsDialog::OnWithItemClicked: Could not resolve item ID from brush\n");
	}
}

void ReplaceItemsDialog::OnAddButtonClicked(wxCommandEvent& WXUNUSED(event)) {
	SetPreviewSource(PreviewSource::Manual);

	if (preview_list->GetCount() == 0) {
		wxMessageBox("Please select items to replace!", "Error", wxOK | wxICON_ERROR);
		return;
	}

	AddPreviewItemsToList();

	replace_button->SetItemId(0);
	with_button->SetItemId(0);
	replace_range_input->SetValue("");
	with_range_input->SetValue("");
	UpdatePreviewList();
}

void ReplaceItemsDialog::OnRemoveButtonClicked(wxCommandEvent& WXUNUSED(event)) {
	list->RemoveSelected();
	UpdateWidgets();
}

void ReplaceItemsDialog::OnExecuteButtonClicked(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		return;
	}

	const auto& items = list->GetItems();
	if (items.empty()) {
		return;
	}

	replace_button->Enable(false);
	with_button->Enable(false);
	add_button->Enable(false);
	remove_button->Enable(false);
	execute_button->Enable(false);
	close_button->Enable(false);
	progress->SetValue(0);

	MapTab* tab = dynamic_cast<MapTab*>(GetParent());
	if (!tab) {
		return;
	}

	Editor* editor = tab->GetEditor();
	bool isReversed = swap_checkbox->GetValue();

	int done = 0;
	for (const ReplacingItem& info : items) {
		// If reversed, swap the IDs for the search
		uint16_t searchId = isReversed ? info.withId : info.replaceId;
		uint16_t replaceWithId = isReversed ? info.replaceId : info.withId;
		
		ItemFinder finder(searchId, (uint32_t)g_settings.getInteger(Config::REPLACE_SIZE));

		// search on map
		foreach_ItemOnMap(editor->map, finder, selectionOnly);

		uint32_t total = 0;
		std::vector<std::pair<Tile*, Item*>>& result = finder.result;

		if (!result.empty()) {
			Action* action = editor->actionQueue->createAction(ACTION_REPLACE_ITEMS);
			for (std::vector<std::pair<Tile*, Item*>>::const_iterator rit = result.begin(); rit != result.end(); ++rit) {
				Tile* new_tile = rit->first->deepCopy(editor->map);
				int index = rit->first->getIndexOf(rit->second);
				ASSERT(index != wxNOT_FOUND);
				Item* item = new_tile->getItemAt(index);
				ASSERT(item && item->getID() == rit->second->getID());
				transformItem(item, replaceWithId, new_tile);
				action->addChange(new Change(new_tile));
				total++;
			}
			editor->actionQueue->addAction(action);
		}

		done++;
		const int value = static_cast<int>((done / items.size()) * 100);
		progress->SetValue(std::clamp<int>(value, 0, 100));
		list->MarkAsComplete(info, total);
	}

	// Re-enable all buttons
	replace_button->Enable(true);
	with_button->Enable(true);
	add_button->Enable(false); // Stays disabled until valid items are selected
	remove_button->Enable(false); // Stays disabled until an item is selected in list
	execute_button->Enable(list->GetCount() != 0);
	close_button->Enable(true);
	UpdateWidgets();

	tab->Refresh();
}

void ReplaceItemsDialog::OnCancelButtonClicked(wxCommandEvent& WXUNUSED(event)) {
	Close();
}

void ReplaceItemsDialog::OnSwapCheckboxClicked(wxCommandEvent& WXUNUSED(event)) {
	if (arrow_bitmap) {
		// Get the original bitmap from the art provider
		wxBitmap original = wxArtProvider::GetBitmap(wxART_GO_FORWARD);
		
		// Create a rotated version using wxImage for the 180-degree rotation
		wxImage img = original.ConvertToImage();
		img = swap_checkbox->GetValue() ? 
			img.Rotate180() :  // Flip it completely when checked
			original.ConvertToImage();  // Use original when unchecked
			
		// Update the bitmap
		arrow_bitmap->SetBitmap(wxBitmap(img));
	}
}

void ReplaceItemsDialog::RefreshPresetList() {
	wxString path = g_gui.GetDataDirectory() + "\\replace_presets\\";
	if (!wxDirExists(path)) {
		wxMkdir(path);
	}
	
	preset_choice->Clear();
	wxDir dir(path);
	if (!dir.IsOpened()) return;
	
	wxString filename;
	bool cont = dir.GetFirst(&filename, "*.xml", wxDIR_FILES);
	while (cont) {
		preset_choice->Append(filename.BeforeLast('.'));
		cont = dir.GetNext(&filename);
	}
	
	remove_preset_button->Enable(preset_choice->GetCount() > 0);
}

void ReplaceItemsDialog::OnPresetSelect(wxCommandEvent& WXUNUSED(event)) {
	int sel = preset_choice->GetSelection();
	if (sel != wxNOT_FOUND) {
		LoadPresetFromXML(preset_choice->GetString(sel));
	}
}

void ReplaceItemsDialog::OnAddPreset(wxCommandEvent& WXUNUSED(event)) {
	wxString name = wxGetTextFromUser("Enter preset name:", "Save Replace Items Preset");
	if (!name.empty()) {
		SavePresetToXML(name);
		RefreshPresetList();
		// Select the newly added preset
		int idx = preset_choice->FindString(name);
		if (idx != wxNOT_FOUND) {
			preset_choice->SetSelection(idx);
		}
	}
}

void ReplaceItemsDialog::OnRemovePreset(wxCommandEvent& WXUNUSED(event)) {
	int sel = preset_choice->GetSelection();
	if (sel != wxNOT_FOUND) {
		wxString name = preset_choice->GetString(sel);
		if (wxMessageBox(wxString::Format("Are you sure you want to delete the preset '%s'?", name), 
						"Confirm Delete", wxYES_NO | wxNO_DEFAULT) == wxYES) {
			wxString path = g_gui.GetDataDirectory() + "\\replace_presets\\" + name + ".xml";
			wxRemoveFile(path);
			RefreshPresetList();
		}
	}
}

void ReplaceItemsDialog::SavePresetToXML(const wxString& name) {
	wxString path = g_gui.GetDataDirectory() + "\\replace_presets\\";
	if (!wxDirExists(path)) {
		wxMkdir(path);
	}

	pugi::xml_document doc;
	pugi::xml_node root = doc.append_child("replace_items");
	
	const auto& items = list->GetItems();
	for (const ReplacingItem& item : items) {
		pugi::xml_node replace_node = root.append_child("replace");
		replace_node.append_attribute("replaceId") = item.replaceId;
		replace_node.append_attribute("withId") = item.withId;
	}
	
	doc.save_file((path + name + ".xml").mb_str());
}

void ReplaceItemsDialog::LoadPresetFromXML(const wxString& name) {
	wxString path = g_gui.GetDataDirectory() + "\\replace_presets\\" + name + ".xml";
	
	pugi::xml_document doc;
	if (!doc.load_file(path.mb_str())) {
		return;
	}

	// Reset everything first
	list->Clear();    // Clear the list (which will clear internal m_items)
	replace_button->SetItemId(0);
	with_button->SetItemId(0);
	progress->SetValue(0);
	
	pugi::xml_node root = doc.child("replace_items");
	for (pugi::xml_node replace_node = root.child("replace"); replace_node; replace_node = replace_node.next_sibling("replace")) {
		ReplacingItem item;
		item.replaceId = replace_node.attribute("replaceId").as_uint();
		item.withId = replace_node.attribute("withId").as_uint();
		item.complete = false;
		item.total = 0;
		if (item.replaceId != 0 && item.withId != 0) {  // Validate items before adding
			list->AddItem(item);
		}
	}
	
	UpdateWidgets();
	list->Refresh();
	list->Update();
}

void ReplaceItemsDialog::OnLoadPreset(wxCommandEvent& WXUNUSED(event)) {
	int sel = preset_choice->GetSelection();
	if (sel != wxNOT_FOUND) {
		LoadPresetFromXML(preset_choice->GetString(sel));
	}
}

uint16_t ReplaceItemsDialog::getActualItemIdFromBrush(const Brush* brush) const {
	if (!brush) {
		OutputDebugStringA("getActualItemIdFromBrush: No brush provided\n");
		return 0;
	}

	uint16_t id = 0;

	if (brush->isRaw()) {
		RAWBrush* raw = static_cast<RAWBrush*>(const_cast<Brush*>(brush));
		id = raw->getItemID();
		OutputDebugStringA(wxString::Format("RAW brush item ID: %d\n", id).c_str());
	} else if (brush->isGround()) {
		GroundBrush* gb = const_cast<Brush*>(brush)->asGround();
		if (gb) {
			// Try to get the actual item ID through the ground brush's properties
			if (gb->getID() != 0) {
				// First try to find a matching RAW brush
				for (const auto& brushPair : g_brushes.getMap()) {
					if (brushPair.second && brushPair.second->isRaw()) {
						RAWBrush* raw = static_cast<RAWBrush*>(brushPair.second);
						const ItemType& rawType = g_items.getItemType(raw->getItemID());
						if (rawType.brush && rawType.brush->getID() == gb->getID()) {
							id = raw->getItemID();
							OutputDebugStringA(wxString::Format("Found matching RAW brush ID: %d for ground brush\n", id).c_str());
							break;
						}
					}
				}

				// If no RAW brush found, try to get through item database
				if (id == 0) {
					const ItemType& type = g_items.getItemType(gb->getID());
					if (type.id != 0) {
						id = type.id;
						OutputDebugStringA(wxString::Format("Found item type ID: %d for ground brush\n", id).c_str());
					}
				}
			}
		}
	} else {
		// For other brush types, try to find the corresponding item ID
		const ItemType& type = g_items.getItemType(brush->getID());
		if (type.id != 0) {
			id = type.id;
			OutputDebugStringA(wxString::Format("Found item type ID: %d for brush\n", id).c_str());
		}
	}

	if (id == 0) {
		OutputDebugStringA("Could not resolve actual item ID from brush\n");
	}

	return id;
}

void ReplaceItemsDialog::LoadBorderChoices() {
	border_from_choice->Clear();
	border_to_choice->Clear();
	border_xml_indices_.clear();
	
	wxString dataDir = GetDataDirectoryForVersion(g_gui.GetCurrentVersion().getName());
	if(dataDir.IsEmpty()) return;
	
	std::map<int, wxString> borderNames;
	wxString groundsPath = g_gui.GetDataDirectory() + "/" + dataDir + "/grounds.xml";
	pugi::xml_document groundsDoc;
	if(groundsDoc.load_file(groundsPath.mb_str())) {
		for(pugi::xml_node brushNode = groundsDoc.child("materials").child("brush"); 
			brushNode; brushNode = brushNode.next_sibling("brush")) {
			
			for(pugi::xml_node borderNode = brushNode.child("border"); 
				borderNode; borderNode = borderNode.next_sibling("border")) {
				
				int borderId = borderNode.attribute("id").as_int();
				wxString brushName = wxString(brushNode.attribute("name").value());
				if(!brushName.IsEmpty()) {
					borderNames[borderId] = brushName;
				}
			}
		}
	}
	
	wxString bordersPath = g_gui.GetDataDirectory() + "/" + dataDir + "/borders.xml";
	
	border_from_choice->Append("Select border to replace...");
	border_to_choice->Append("Select border to replace with...");
	border_from_choice->SetSelection(0);
	border_to_choice->SetSelection(0);
	
	struct BorderEntry {
		wxString displayText;
		int itemCount;
		int xmlIndex;
	};
	std::vector<BorderEntry> entries;
	
	pugi::xml_document doc;
	if(doc.load_file(bordersPath.mb_str())) {
		int xmlIndex = 0;
		for(pugi::xml_node borderNode = doc.child("materials").child("border"); 
			borderNode; borderNode = borderNode.next_sibling("border")) {
			
			int borderId = borderNode.attribute("id").as_int();
			wxString name = borderNames[borderId];
			
			int itemCount = 0;
			for(pugi::xml_node itemNode = borderNode.child("borderitem"); 
				itemNode; itemNode = itemNode.next_sibling("borderitem")) {
				itemCount++;
			}
			
			BorderEntry entry;
			if(!name.IsEmpty()) {
				entry.displayText = wxString::Format("%s [%d] (%d items)", name, borderId, itemCount);
			} else {
				entry.displayText = wxString::Format("Border %d (%d items)", borderId, itemCount);
			}
			entry.itemCount = itemCount;
			entry.xmlIndex = xmlIndex;
			entries.push_back(entry);
			xmlIndex++;
		}
	}
	
	std::sort(entries.begin(), entries.end(), [](const BorderEntry& a, const BorderEntry& b) {
		if (a.itemCount != b.itemCount) {
			return a.itemCount > b.itemCount;
		}
		return a.displayText < b.displayText;
	});
	
	for (const BorderEntry& entry : entries) {
		border_from_choice->Append(entry.displayText);
		border_to_choice->Append(entry.displayText);
		border_xml_indices_.push_back(entry.xmlIndex);
	}
}

void ReplaceItemsDialog::OnAddBorderItems(wxCommandEvent& WXUNUSED(event)) {
	if (preview_source_ != PreviewSource::Border || preview_list->GetCount() == 0) {
		wxMessageBox("Please select both border types!", "Error", wxOK | wxICON_ERROR);
		return;
	}

	AddPreviewItemsToList();
}

void ReplaceItemsDialog::OnBorderFromSelect(wxCommandEvent& WXUNUSED(event)) {
	SetPreviewSource(PreviewSource::Border);
}

void ReplaceItemsDialog::OnBorderToSelect(wxCommandEvent& WXUNUSED(event)) {
	SetPreviewSource(PreviewSource::Border);
}

// Helper function to avoid code duplication
wxString ReplaceItemsDialog::GetDataDirectoryForVersion(const wxString& versionStr) const {
	pugi::xml_document clientsDoc;
	wxString clientsPath = g_gui.GetDataDirectory() + "/clients.xml";
	if(clientsDoc.load_file(clientsPath.mb_str())) {
		for(pugi::xml_node clientNode = clientsDoc.child("client_config").child("clients").child("client"); 
			clientNode; clientNode = clientNode.next_sibling("client")) {
			if(versionStr == clientNode.attribute("name").value()) {
				return wxString(clientNode.attribute("data_directory").value());
			}
		}
	}
	return wxString();
}

void ReplaceItemsDialog::LoadWallChoices() {
	wall_from_choice->Clear();
	wall_to_choice->Clear();
	wall_xml_indices_.clear();
	
	wall_from_choice->Append("Select wall...");
	wall_to_choice->Append("Select wall...");
	
	wxString dataDir = GetDataDirectoryForVersion(g_gui.GetCurrentVersion().getName());
	if(dataDir.IsEmpty()) return;
	
	struct WallEntry {
		wxString displayText;
		int variationCount;
		int xmlIndex;
	};
	std::vector<WallEntry> entries;
	
	wxString wallsPath = g_gui.GetDataDirectory() + "/" + dataDir + "/walls.xml";
	pugi::xml_document doc;
	if(doc.load_file(wallsPath.mb_str())) {
		int xmlIndex = 0;
		for(pugi::xml_node brushNode = doc.child("materials").child("brush"); 
			brushNode; brushNode = brushNode.next_sibling("brush")) {
			
			if(std::string(brushNode.attribute("type").value()) == "wall") {
				wxString name = brushNode.attribute("name").value();
				uint16_t serverId = brushNode.attribute("server_lookid").as_uint();
				
				int totalVariations = 0;
				for(pugi::xml_node wallNode = brushNode.child("wall"); 
					wallNode; wallNode = wallNode.next_sibling("wall")) {
					for(pugi::xml_node itemNode = wallNode.child("item"); 
						itemNode; itemNode = itemNode.next_sibling("item")) {
						if(itemNode.attribute("chance").as_int() > 0) {
							totalVariations++;
						}
					}
					for(pugi::xml_node doorNode = wallNode.child("door"); 
						doorNode; doorNode = doorNode.next_sibling("door")) {
						totalVariations++;
					}
				}
				
				WallEntry entry;
				entry.displayText = wxString::Format("%s [%d] (%d variations)", name, serverId, totalVariations);
				entry.variationCount = totalVariations;
				entry.xmlIndex = xmlIndex;
				entries.push_back(entry);
				xmlIndex++;
			}
		}
	}
	
	std::sort(entries.begin(), entries.end(), [](const WallEntry& a, const WallEntry& b) {
		if (a.variationCount != b.variationCount) {
			return a.variationCount > b.variationCount;
		}
		return a.displayText < b.displayText;
	});
	
	for (const WallEntry& entry : entries) {
		wall_from_choice->Append(entry.displayText);
		wall_to_choice->Append(entry.displayText);
		wall_xml_indices_.push_back(entry.xmlIndex);
	}
}

void ReplaceItemsDialog::OnWallFromSelect(wxCommandEvent& WXUNUSED(event)) {
	SetPreviewSource(PreviewSource::Wall);
}

void ReplaceItemsDialog::OnWallToSelect(wxCommandEvent& WXUNUSED(event)) {
	SetPreviewSource(PreviewSource::Wall);
}

void ReplaceItemsDialog::OnWallOrientationSelect(wxCommandEvent& WXUNUSED(event)) {
	if (preview_source_ == PreviewSource::Wall ||
		wall_from_choice->GetSelection() > 0 || wall_to_choice->GetSelection() > 0) {
		SetPreviewSource(PreviewSource::Wall);
	}
}

void ReplaceItemsDialog::OnAddWallItems(wxCommandEvent& WXUNUSED(event)) {
	if (preview_source_ != PreviewSource::Wall || preview_list->GetCount() == 0) {
		wxMessageBox("Please select both wall types!", "Error", wxOK | wxICON_ERROR);
		return;
	}

	AddPreviewItemsToList();
}

void ReplaceItemsDialog::AddWallVariations(uint16_t fromId, uint16_t toId) {
	if (fromId == 0 || toId == 0 || fromId == toId) return;

	wxString dataDir = GetDataDirectoryForVersion(g_gui.GetCurrentVersion().getName());
	if(dataDir.IsEmpty()) return;
	
	wxString wallsPath = g_gui.GetDataDirectory() + "/" + dataDir + "/walls.xml";
	pugi::xml_document doc;
	if(!doc.load_file(wallsPath.mb_str())) return;

	// Find source and target wall brushes
	pugi::xml_node fromBrush, toBrush;
	for(pugi::xml_node brushNode = doc.child("materials").child("brush"); 
		brushNode; brushNode = brushNode.next_sibling("brush")) {
		
		if(std::string(brushNode.attribute("type").value()) == "wall") {
			uint16_t serverId = brushNode.attribute("server_lookid").as_uint();
			if(serverId == fromId) fromBrush = brushNode;
			else if(serverId == toId) toBrush = brushNode;
		}
	}

	if(!fromBrush || !toBrush) return;

	// Process each wall orientation
	for(pugi::xml_node fromWallNode = fromBrush.child("wall"); 
		fromWallNode; fromWallNode = fromWallNode.next_sibling("wall")) {
		
		std::string wallType = fromWallNode.attribute("type").value();
		pugi::xml_node toWallNode = toBrush.find_child_by_attribute("wall", "type", wallType.c_str());
		
		if(!toWallNode) continue;

		// Handle regular wall items first
		for(pugi::xml_node fromItem = fromWallNode.child("item"); 
			fromItem; fromItem = fromItem.next_sibling("item")) {
			
			if(fromItem.attribute("chance").as_int() > 0) {
				uint16_t fromItemId = fromItem.attribute("id").as_uint();
				
				// Get first valid item from target wall
				pugi::xml_node toItem = toWallNode.child("item");
				while(toItem && toItem.attribute("chance").as_int() == 0) {
					toItem = toItem.next_sibling("item");
				}
				
				if(toItem) {
					AddReplacingItem(fromItemId, toItem.attribute("id").as_uint());
				}
			}
		}

		// Handle doors with type matching
		for(pugi::xml_node fromDoor = fromWallNode.child("door"); 
			fromDoor; fromDoor = fromDoor.next_sibling("door")) {
			
			std::string doorType = fromDoor.attribute("type").value();
			bool isOpen = fromDoor.attribute("open").as_bool();
			bool isLocked = fromDoor.attribute("locked").as_bool();
			
			// Try to find matching door in target wall
			pugi::xml_node toDoor = toWallNode.child("door");
			pugi::xml_node fallbackDoor;
			
			while(toDoor) {
				if(toDoor.attribute("type").value() == doorType &&
				   toDoor.attribute("open").as_bool() == isOpen &&
				   toDoor.attribute("locked").as_bool() == isLocked) {
					break;
				}
				// Keep track of first door as fallback
				if(!fallbackDoor) fallbackDoor = toDoor;
				toDoor = toDoor.next_sibling("door");
			}
			
			// If no exact match found, use fallback door
			if(!toDoor) toDoor = fallbackDoor;
			
			if(toDoor) {
				AddReplacingItem(fromDoor.attribute("id").as_uint(), 
							   toDoor.attribute("id").as_uint());
			}
		}
	}
}

// Helper method to add items to the replace list
void ReplaceItemsDialog::AddReplacingItem(uint16_t fromId, uint16_t toId) {
	ReplacingItem item;
	item.replaceId = fromId;
	item.withId = toId;
	item.complete = false;
	item.total = 0;
	
	if(list->CanAdd(item.replaceId, item.withId)) {
		list->AddItem(item);
	}
}

void ReplaceItemsDialog::OnIdInput(wxCommandEvent& event) {
	wxTextCtrl* input = dynamic_cast<wxTextCtrl*>(event.GetEventObject());
	if (!input) {
		return;
	}

	const wxString value = input->GetValue().Trim();
	if (!value.IsEmpty() && !value.Contains("-") && !value.Contains(",")) {
		long id = 0;
		if (value.ToLong(&id) && id > 0 && id <= 65535) {
			if (input == replace_range_input) {
				replace_button->SetItemId(static_cast<uint16_t>(id));
			} else if (input == with_range_input) {
				with_button->SetItemId(static_cast<uint16_t>(id));
			}
		}
	}

	SetPreviewSource(PreviewSource::Manual);
}

void ReplaceItemsDialog::UpdateAddButtonState() {
	if (preview_source_ == PreviewSource::Manual) {
		add_button->Enable(!BuildManualPreviewItems().empty());
		return;
	}

	add_button->Enable(false);
}

int ReplaceItemsDialog::GetBorderXmlIndex(int choiceIdx) const {
	if (choiceIdx <= 0 || choiceIdx > static_cast<int>(border_xml_indices_.size())) {
		return -1;
	}
	return border_xml_indices_[choiceIdx - 1];
}

int ReplaceItemsDialog::GetWallXmlIndex(int choiceIdx) const {
	if (choiceIdx <= 0 || choiceIdx > static_cast<int>(wall_xml_indices_.size())) {
		return -1;
	}
	return wall_xml_indices_[choiceIdx - 1];
}

std::vector<uint16_t> ReplaceItemsDialog::FlattenRanges(const std::vector<std::pair<uint16_t, uint16_t>>& ranges) {
	std::vector<uint16_t> ids;
	for (const auto& range : ranges) {
		for (uint16_t id = range.first; id <= range.second; ++id) {
			ids.push_back(id);
		}
	}
	return ids;
}

std::vector<std::pair<uint16_t, uint16_t>> ReplaceItemsDialog::ParseRangeString(const wxString& input) const {
	wxString trimmed = input;
	trimmed.Trim();
	return parseIdRangesString(nstr(trimmed));
}

