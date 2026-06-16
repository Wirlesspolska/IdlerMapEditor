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

#ifndef RME_MAP_SUMMARY_WINDOW_H_
#define RME_MAP_SUMMARY_WINDOW_H_

#include "main.h"
#include <map>
#include <vector>

// Forward declarations
class Map;
class Container;

class MapSummaryWindow : public wxPanel {
public:
	MapSummaryWindow(wxWindow* parent);
	virtual ~MapSummaryWindow();

	void Clear();
	void SummarizeMap(Map& map);
	void AddItemCount(uint16_t itemId, const wxString& itemName, uint32_t count);
	void SetFilter(const wxString& filter);

	// Event handlers
	void OnClickResult(wxCommandEvent& event);
	void OnClickExport(wxCommandEvent& event);
	void OnClickClear(wxCommandEvent& event);
	void OnClickSummarize(wxCommandEvent& event);
	void OnFilterText(wxCommandEvent& event);

protected:
	struct ItemSummary {
		uint16_t itemId;
		wxString itemName;
		uint32_t count;
		
		ItemSummary(uint16_t id, const wxString& name, uint32_t c) 
			: itemId(id), itemName(name), count(c) {}
	};

	void RefreshList();
	void SortByCount();
	void SortByID();
	void SortByName();
	void CountContainerItems(Container* container, std::map<uint16_t, uint32_t>& item_counts);
	void OnClickSort(wxCommandEvent& event);

	wxListBox* result_list;
	wxTextCtrl* filter_text;
	wxButton* summarize_button;
	wxButton* sort_count_button;
	wxButton* sort_id_button;
	wxButton* sort_name_button;
	
	std::vector<ItemSummary> item_summaries;
	wxString current_filter;

	DECLARE_EVENT_TABLE()
};

#endif // RME_MAP_SUMMARY_WINDOW_H_ 