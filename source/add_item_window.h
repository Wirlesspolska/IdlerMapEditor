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

#ifndef RME_ADD_ITEM_WINDOW_H_
#define RME_ADD_ITEM_WINDOW_H_

#include "main.h"

#include "common_windows.h"

class ContainerItemButton;
class ContainerItemPopupMenu;

class AddItemWindow : public ObjectPropertiesWindowBase {
public:
	AddItemWindow(wxWindow* parent, TilesetCategoryType categoryType, Tileset* tilesetItem, wxPoint = wxDefaultPosition);

	void OnItemClicked(wxMouseEvent& event);
	void SetItemIdToItemButton(uint16_t id);
	void OnChangeItemId(wxCommandEvent& WXUNUSED(event));
	void OnRangeToggle(wxCommandEvent& WXUNUSED(event));
	void OnRangeFieldChange(wxSpinEvent& event);
	void OnUseCurrentItem(wxCommandEvent& event);
	void OnQuickRange(wxCommandEvent& event);
	void UpdateRangeFields(bool show);
	void UpdateRangeInfo();

	void OnClickOK(wxCommandEvent&);
	void OnClickCancel(wxCommandEvent&);

	wxChoice* tileset_choice;

protected:
	int item_id;

	wxSpinCtrl* item_id_field;
	wxStaticText* item_id_label;
	wxStaticText* item_name_label;
	DCButton* item_button;
	
	// Range selection controls
	wxCheckBox* range_checkbox;
	wxSpinCtrl* range_start_field;
	wxSpinCtrl* range_end_field;
	wxStaticText* range_start_label;
	wxStaticText* range_end_label;
	wxStaticText* range_info_label;

	TilesetCategoryType category_type;
	Tileset* tileset_item;

	DECLARE_EVENT_TABLE();
};

#endif
