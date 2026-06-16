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
#include "map_summary_window.h"
#include "gui.h"
#include "map.h"
#include "tile.h"
#include "item.h"
#include "items.h"
#include "string_utils.h"
#include <algorithm>
#include <map>

BEGIN_EVENT_TABLE(MapSummaryWindow, wxPanel)
EVT_LISTBOX(wxID_ANY, MapSummaryWindow::OnClickResult)
EVT_BUTTON(wxID_FILE, MapSummaryWindow::OnClickExport)
EVT_BUTTON(wxID_CLEAR, MapSummaryWindow::OnClickClear)
EVT_BUTTON(wxID_REFRESH, MapSummaryWindow::OnClickSummarize)
EVT_TEXT(wxID_FIND, MapSummaryWindow::OnFilterText)
EVT_BUTTON(wxID_SORT_ASCENDING, MapSummaryWindow::OnClickSort)
END_EVENT_TABLE()

MapSummaryWindow::MapSummaryWindow(wxWindow* parent) :
	wxPanel(parent, wxID_ANY) {
	
	wxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);

	// Filter section
	wxSizer* filter_sizer = new wxBoxSizer(wxHORIZONTAL);
	filter_sizer->Add(new wxStaticText(this, wxID_ANY, "Filter:"), 0, wxALIGN_CENTER_VERTICAL | wxALL, 5);
	filter_text = new wxTextCtrl(this, wxID_FIND, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	filter_sizer->Add(filter_text, 1, wxEXPAND | wxALL, 5);
	main_sizer->Add(filter_sizer, 0, wxEXPAND | wxALL, 5);

	// List
	result_list = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxSize(300, 400), 0, nullptr, wxLB_SINGLE | wxLB_ALWAYS_SB);
	main_sizer->Add(result_list, 1, wxEXPAND | wxALL, 5);

	// Buttons
	wxSizer* button_sizer = new wxBoxSizer(wxHORIZONTAL);
	
	summarize_button = new wxButton(this, wxID_REFRESH, "Summarize Map");
	button_sizer->Add(summarize_button, 0, wxALL, 5);
	
	// Sort buttons
	sort_count_button = new wxButton(this, wxID_SORT_ASCENDING, "Sort by Count");
	sort_id_button = new wxButton(this, wxID_SORT_ASCENDING + 1, "Sort by ID");
	sort_name_button = new wxButton(this, wxID_SORT_ASCENDING + 2, "Sort by Name");
	
	button_sizer->Add(sort_count_button, 0, wxALL, 5);
	button_sizer->Add(sort_id_button, 0, wxALL, 5);
	button_sizer->Add(sort_name_button, 0, wxALL, 5);
	
	button_sizer->Add(new wxButton(this, wxID_FILE, "Export"), 0, wxALL, 5);
	button_sizer->Add(new wxButton(this, wxID_CLEAR, "Clear"), 0, wxALL, 5);
	
	main_sizer->Add(button_sizer, 0, wxCENTER | wxALL, 5);
	
	SetSizerAndFit(main_sizer);
	
	// Bind sort button events
	Bind(wxEVT_BUTTON, &MapSummaryWindow::OnClickSort, this, wxID_SORT_ASCENDING);
	Bind(wxEVT_BUTTON, &MapSummaryWindow::OnClickSort, this, wxID_SORT_ASCENDING + 1);
	Bind(wxEVT_BUTTON, &MapSummaryWindow::OnClickSort, this, wxID_SORT_ASCENDING + 2);
}

MapSummaryWindow::~MapSummaryWindow() {
	Clear();
}

void MapSummaryWindow::Clear() {
	result_list->Clear();
	item_summaries.clear();
}

void MapSummaryWindow::SummarizeMap(Map& map) {
	Clear();
	
	// Count all items on the map
	std::map<uint16_t, uint32_t> item_counts;
	
	g_gui.CreateLoadBar("Summarizing map items...");
	
	uint64_t total_tiles = map.getTileCount();
	uint64_t processed = 0;
	
	for (MapIterator mit = map.begin(); mit != map.end(); ++mit) {
		Tile* tile = (*mit)->get();
		if (!tile) {
			processed++;
			continue;
		}
		
		// Update progress bar every 1000 tiles
		if (processed % 1000 == 0) {
			g_gui.SetLoadDone((uint32_t)(100 * processed / total_tiles));
		}
		
		// Count ground items
		if (tile->ground) {
			item_counts[tile->ground->getID()]++;
		}
		
		// Count items on tile
		for (Item* item : tile->items) {
			if (item) {
				item_counts[item->getID()]++;
				
				// Count items in containers recursively
				if (Container* container = dynamic_cast<Container*>(item)) {
					CountContainerItems(container, item_counts);
				}
			}
		}
		
		processed++;
	}
	
	g_gui.DestroyLoadBar();
	
	// Convert to our internal format and add to list
	for (const auto& pair : item_counts) {
		uint16_t itemId = pair.first;
		uint32_t count = pair.second;
		
		ItemType& itemType = g_items[itemId];
		wxString itemName = wxstr(itemType.name);
		if (itemName.empty()) {
			itemName = wxString::Format("Unknown Item %d", itemId);
		}
		
		item_summaries.emplace_back(itemId, itemName, count);
	}
	
	// Sort by count (descending) by default
	SortByCount();
	RefreshList();
	
	wxString statusMsg = wxString::Format("Map summarized: %zu unique item types found", item_summaries.size());
	g_gui.SetStatusText(statusMsg);
}

void MapSummaryWindow::CountContainerItems(Container* container, std::map<uint16_t, uint32_t>& item_counts) {
	const ItemVector& items = container->getVector();
	for (Item* item : items) {
		if (item) {
			item_counts[item->getID()]++;
			
			// Recursively count items in nested containers
			if (Container* nested_container = dynamic_cast<Container*>(item)) {
				CountContainerItems(nested_container, item_counts);
			}
		}
	}
}

void MapSummaryWindow::AddItemCount(uint16_t itemId, const wxString& itemName, uint32_t count) {
	item_summaries.emplace_back(itemId, itemName, count);
	RefreshList();
}

void MapSummaryWindow::SetFilter(const wxString& filter) {
	current_filter = filter.Lower();
	RefreshList();
}

void MapSummaryWindow::RefreshList() {
	result_list->Clear();
	
	for (const auto& summary : item_summaries) {
		// Apply filter if set
		if (!current_filter.empty()) {
			wxString itemNameLower = summary.itemName.Lower();
			wxString itemIdStr = wxString::Format("%d", summary.itemId);
			
			if (itemNameLower.Find(current_filter) == wxNOT_FOUND && 
				itemIdStr.Find(current_filter) == wxNOT_FOUND) {
				continue;
			}
		}
		
		wxString display_text = wxString::Format("%s [ID: %d] - Count: %u", 
			summary.itemName, summary.itemId, summary.count);
		result_list->Append(display_text);
	}
}

void MapSummaryWindow::SortByCount() {
	std::sort(item_summaries.begin(), item_summaries.end(), 
		[](const ItemSummary& a, const ItemSummary& b) {
			return a.count > b.count; // Descending order
		});
}

void MapSummaryWindow::SortByID() {
	std::sort(item_summaries.begin(), item_summaries.end(), 
		[](const ItemSummary& a, const ItemSummary& b) {
			return a.itemId < b.itemId; // Ascending order
		});
}

void MapSummaryWindow::SortByName() {
	std::sort(item_summaries.begin(), item_summaries.end(), 
		[](const ItemSummary& a, const ItemSummary& b) {
			return a.itemName.CmpNoCase(b.itemName) < 0; // Case-insensitive ascending
		});
}

void MapSummaryWindow::OnClickResult(wxCommandEvent& event) {
	// Could implement going to first occurrence of selected item
	int selection = result_list->GetSelection();
	if (selection != wxNOT_FOUND) {
		// For now, just show a message with the selected item info
		wxString selected_text = result_list->GetString(selection);
		g_gui.SetStatusText("Selected: " + selected_text);
	}
}

void MapSummaryWindow::OnClickExport(wxCommandEvent& WXUNUSED(event)) {
	wxFileDialog dialog(this, "Save map summary...", "", "", "Text Documents (*.txt) | *.txt", wxFD_SAVE);
	if (dialog.ShowModal() == wxID_OK) {
		wxFile file(dialog.GetPath(), wxFile::write);
		if (file.IsOpened()) {
			file.Write("Map Item Summary\n");
			file.Write("Generated by Remere's Map Editor " + __RME_VERSION__ + "\n");
			file.Write("======================================\n\n");
			
			for (const auto& summary : item_summaries) {
				// Apply filter if set
				if (!current_filter.empty()) {
					wxString itemNameLower = summary.itemName.Lower();
					wxString itemIdStr = wxString::Format("%d", summary.itemId);
					
					if (itemNameLower.Find(current_filter) == wxNOT_FOUND && 
						itemIdStr.Find(current_filter) == wxNOT_FOUND) {
						continue;
					}
				}
				
				wxString line = wxString::Format("%s [ID: %d] - Count: %u\n", 
					summary.itemName, summary.itemId, summary.count);
				file.Write(line);
			}
			
			file.Close();
			g_gui.SetStatusText("Map summary exported successfully");
		}
	}
}

void MapSummaryWindow::OnClickClear(wxCommandEvent& WXUNUSED(event)) {
	Clear();
	g_gui.SetStatusText("Map summary cleared");
}

void MapSummaryWindow::OnClickSummarize(wxCommandEvent& WXUNUSED(event)) {
	if (!g_gui.IsEditorOpen()) {
		g_gui.PopupDialog("Error", "No map is currently open!", wxOK | wxICON_ERROR);
		return;
	}
	
	Map& current_map = g_gui.GetCurrentMap();
	SummarizeMap(current_map);
}

void MapSummaryWindow::OnFilterText(wxCommandEvent& WXUNUSED(event)) {
	SetFilter(filter_text->GetValue());
}

void MapSummaryWindow::OnClickSort(wxCommandEvent& event) {
	int id = event.GetId();
	
	if (id == wxID_SORT_ASCENDING) {
		// Sort by count
		SortByCount();
	} else if (id == wxID_SORT_ASCENDING + 1) {
		// Sort by ID
		SortByID();
	} else if (id == wxID_SORT_ASCENDING + 2) {
		// Sort by name
		SortByName();
	}
	
	RefreshList(); 
} 
