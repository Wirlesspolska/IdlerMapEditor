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

#include "proposed_brushes_window.h"
#include "gui.h"
#include "brush.h"
#include "raw_brush.h"
#include "ground_brush.h"
#include "items.h"
#include "graphics.h"
#include "settings.h"
#include "dcbutton.h"

#include <algorithm>

namespace {
	constexpr size_t MAX_BORDER_PROPOSALS = 64;
	constexpr size_t MAX_FLAG_PROPOSALS = 48;
	constexpr size_t MAX_COLOR_PROPOSALS = 32;
	constexpr int MIN_FLAG_SCORE = 40;
}

enum {
	PROPOSED_REBUILD_ID = 21000,
	PROPOSED_HOTKEY_CHECKBOX_ID
};

BEGIN_EVENT_TABLE(ProposedBrushesPanel, wxScrolledWindow)
EVT_SIZE(ProposedBrushesPanel::OnSize)
EVT_PAINT(ProposedBrushesPanel::OnPaint)
EVT_ERASE_BACKGROUND(ProposedBrushesPanel::OnEraseBackground)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(ProposedBrushesWindow, wxPanel)
EVT_CLOSE(ProposedBrushesWindow::OnClose)
EVT_BUTTON(PROPOSED_REBUILD_ID, ProposedBrushesWindow::OnRebuildButton)
EVT_CHECKBOX(PROPOSED_HOTKEY_CHECKBOX_ID, ProposedBrushesWindow::OnHotkeyCheckbox)
EVT_PAINT(ProposedBrushesWindow::OnPaint)
EVT_ERASE_BACKGROUND(ProposedBrushesWindow::OnEraseBackground)
END_EVENT_TABLE()

//=============================================================================
// ProposedBrushesPanel
//=============================================================================

ProposedBrushesPanel::ProposedBrushesPanel(wxWindow* parent) :
	wxScrolledWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHSCROLL | wxVSCROLL) {
	SetBackgroundStyle(wxBG_STYLE_SYSTEM);
	SetBackgroundColour(*wxWHITE);
	main_sizer = newd wxBoxSizer(wxVERTICAL);
	SetSizer(main_sizer);
	ClearBackground();
}

ProposedBrushesPanel::~ProposedBrushesPanel() {
	for (BrushButton* button : brush_buttons) {
		button->Destroy();
	}
	brush_buttons.clear();
}

void ProposedBrushesPanel::SetProposals(const std::vector<ProposedBrushEntry>& newEntries) {
	entries = newEntries;
	RefreshDisplay();
}

void ProposedBrushesPanel::ClearProposals() {
	entries.clear();
	RefreshDisplay();
}

Brush* ProposedBrushesPanel::GetBrushAtHotkey(int digit) const {
	if (digit < 1 || digit > 9) {
		return nullptr;
	}
	for (const ProposedBrushEntry& entry : entries) {
		if (entry.hotkey == digit) {
			return entry.brush;
		}
	}
	return nullptr;
}

wxString ProposedBrushesPanel::GetCategoryName(ProposedBrushCategory category) const {
	switch (category) {
		case PROPOSED_BORDER:
			return "Borders (borders.xml)";
		case PROPOSED_FLAGS:
			return "Similar Flags";
		case PROPOSED_COLOR:
			return "Similar Minimap Color";
		default:
			return "Other";
	}
}

void ProposedBrushesPanel::RefreshDisplay() {
	for (BrushButton* button : brush_buttons) {
		main_sizer->Detach(button);
		button->Destroy();
	}
	brush_buttons.clear();

	for (wxStaticText* label : section_labels) {
		main_sizer->Detach(label);
		label->Destroy();
	}
	section_labels.clear();
	main_sizer->Clear(true);

	if (entries.empty()) {
		wxStaticText* empty = newd wxStaticText(this, wxID_ANY, "No proposals yet.\nSelect a brush and click Rebuild.");
		empty->SetForegroundColour(wxColour(120, 120, 120));
		main_sizer->Add(empty, 0, wxALL, 8);
		Layout();
		FitInside();
		return;
	}

	std::map<ProposedBrushCategory, std::vector<const ProposedBrushEntry*>> grouped;
	std::vector<ProposedBrushCategory> order;
	order.push_back(PROPOSED_BORDER);
	order.push_back(PROPOSED_FLAGS);
	order.push_back(PROPOSED_COLOR);

	for (const ProposedBrushEntry& entry : entries) {
		grouped[entry.category].push_back(&entry);
	}

	for (ProposedBrushCategory category : order) {
		const auto& list = grouped[category];
		if (list.empty()) {
			continue;
		}

		wxString label_text = GetCategoryName(category);
		wxStaticText* section_label = newd wxStaticText(this, wxID_ANY, label_text);
		wxFont font = section_label->GetFont();
		font.SetWeight(wxFONTWEIGHT_BOLD);
		font.SetPointSize(font.GetPointSize() - 1);
		section_label->SetFont(font);
		section_label->SetForegroundColour(wxColour(100, 100, 100));
		main_sizer->Add(section_label, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 3);
		section_labels.push_back(section_label);

		wxPanel* line = newd wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, 1));
		line->SetBackgroundColour(wxColour(200, 200, 200));
		main_sizer->Add(line, 0, wxEXPAND | wxLEFT | wxRIGHT, 5);

		const int brushes_per_row = std::max(1, GetClientSize().GetWidth() / 38);
		wxFlexGridSizer* grid_sizer = newd wxFlexGridSizer(0, brushes_per_row, 2, 2);

		for (const ProposedBrushEntry* entry : list) {
			BrushButton* button = newd BrushButton(this, entry->brush, RENDER_SIZE_32x32);
			wxString tooltip;
			if (entry->brush->isRaw()) {
				RAWBrush* raw = entry->brush->asRaw();
				tooltip = wxString::Format("%s [%d]", raw->getName(), raw->getItemID());
			} else {
				tooltip = wxstr(entry->brush->getName());
			}
			if (entry->hotkey > 0) {
				tooltip += wxString::Format("  [key %d]", entry->hotkey);
			}
			if (entry->score > 0) {
				tooltip += wxString::Format("  (score %d)", entry->score);
			}
			button->SetToolTip(tooltip);
			button->Bind(wxEVT_TOGGLEBUTTON, &ProposedBrushesPanel::OnBrushClick, this);
			grid_sizer->Add(button, 0, wxALL, 1);
			brush_buttons.push_back(button);
		}

		main_sizer->Add(grid_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 3);
		main_sizer->AddSpacer(4);
	}

	Layout();
	FitInside();
}

void ProposedBrushesPanel::OnBrushClick(wxCommandEvent& event) {
	BrushButton* clicked = static_cast<BrushButton*>(event.GetEventObject());
	if (!clicked) {
		return;
	}

	for (BrushButton* button : brush_buttons) {
		if (button != clicked) {
			button->SetValue(false);
		}
	}

	if (clicked->GetValue() && clicked->brush) {
		g_gui.SelectBrush(clicked->brush);
	}
}

void ProposedBrushesPanel::OnSize(wxSizeEvent& event) {
	RefreshDisplay();
	event.Skip();
}

void ProposedBrushesPanel::OnPaint(wxPaintEvent& WXUNUSED(event)) {
	wxPaintDC dc(this);
	dc.SetBackground(*wxWHITE_BRUSH);
	dc.Clear();
}

void ProposedBrushesPanel::OnEraseBackground(wxEraseEvent& WXUNUSED(event)) {
	////
}

//=============================================================================
// ProposedBrushesWindow
//=============================================================================

ProposedBrushesWindow::ProposedBrushesWindow(wxWindow* parent) :
	wxPanel(parent, wxID_ANY),
	rebuilding(false) {
	SetBackgroundColour(*wxWHITE);

	wxBoxSizer* main_sizer = newd wxBoxSizer(wxVERTICAL);

	wxStaticText* title = newd wxStaticText(this, wxID_ANY, "Proposed Brush");
	wxFont font = title->GetFont();
	font.SetWeight(wxFONTWEIGHT_BOLD);
	title->SetFont(font);
	main_sizer->Add(title, 0, wxALL | wxALIGN_CENTER, 5);

	seed_label = newd wxStaticText(this, wxID_ANY, "Seed: (none)");
	seed_label->SetForegroundColour(wxColour(80, 80, 80));
	main_sizer->Add(seed_label, 0, wxLEFT | wxRIGHT | wxEXPAND, 6);

	status_label = newd wxStaticText(this, wxID_ANY, "Idle — select a brush, then Rebuild.");
	status_label->SetForegroundColour(wxColour(0, 90, 160));
	main_sizer->Add(status_label, 0, wxALL | wxEXPAND, 6);

	panel = newd ProposedBrushesPanel(this);
	main_sizer->Add(panel, 1, wxEXPAND | wxALL, 2);

	hotkey_checkbox = newd wxCheckBox(this, PROPOSED_HOTKEY_CHECKBOX_ID, "Bind 1-9 to first 9 proposals");
	hotkey_checkbox->SetValue(g_settings.getBoolean(Config::PROPOSED_BRUSH_HOTKEYS));
	hotkey_checkbox->SetToolTip("When enabled, number keys 1-9 select the first nine proposals instead of stored brush hotkeys.");
	main_sizer->Add(hotkey_checkbox, 0, wxALL | wxEXPAND, 4);

	rebuild_button = newd wxButton(this, PROPOSED_REBUILD_ID, "Rebuild");
	main_sizer->Add(rebuild_button, 0, wxALL | wxALIGN_CENTER, 4);

	SetSizer(main_sizer);
	SetMinSize(wxSize(160, 320));
}

ProposedBrushesWindow::~ProposedBrushesWindow() = default;

bool ProposedBrushesWindow::AreHotkeysEnabled() const {
	return hotkey_checkbox && hotkey_checkbox->GetValue();
}

bool ProposedBrushesWindow::SelectHotkey(int digit) {
	if (!AreHotkeysEnabled() || !panel) {
		return false;
	}
	Brush* brush = panel->GetBrushAtHotkey(digit);
	if (!brush) {
		return false;
	}
	g_gui.SelectBrush(brush);
	g_gui.SetStatusText(wxString::Format("Proposed brush hotkey %d", digit));
	return true;
}

void ProposedBrushesWindow::OnRebuildButton(wxCommandEvent& WXUNUSED(event)) {
	RebuildFromCurrentBrush();
}

void ProposedBrushesWindow::OnHotkeyCheckbox(wxCommandEvent& event) {
	g_settings.setInteger(Config::PROPOSED_BRUSH_HOTKEYS, event.IsChecked() ? 1 : 0);
	RebuildFromCurrentBrush();
}

void ProposedBrushesWindow::OnClose(wxCloseEvent& event) {
	if (g_gui.aui_manager) {
		g_gui.aui_manager->GetPane(this).Show(false);
		g_gui.aui_manager->Update();
	}
	event.Veto();
}

void ProposedBrushesWindow::OnPaint(wxPaintEvent& WXUNUSED(event)) {
	wxPaintDC dc(this);
	dc.SetBackground(*wxWHITE_BRUSH);
	dc.Clear();
}

void ProposedBrushesWindow::OnEraseBackground(wxEraseEvent& WXUNUSED(event)) {
	////
}

void ProposedBrushesWindow::RebuildFromCurrentBrush() {
	RebuildFromBrush(g_gui.GetCurrentBrush());
}

ItemType* ProposedBrushesWindow::ResolveSeedItem(Brush* brush) const {
	if (!brush) {
		return nullptr;
	}

	if (brush->isRaw()) {
		RAWBrush* raw = brush->asRaw();
		if (raw && g_items.typeExists(raw->getItemID())) {
			return &g_items[raw->getItemID()];
		}
		return nullptr;
	}

	if (brush->isGround()) {
		GroundBrush* ground = brush->asGround();
		uint16_t id = ground ? ground->getDefaultGroundItemId() : 0;
		if (id != 0 && g_items.typeExists(id)) {
			return &g_items[id];
		}
	}

	const int lookId = brush->getLookID();
	if (lookId > 0 && g_items.typeExists(lookId)) {
		return &g_items[lookId];
	}

	return nullptr;
}

uint8_t ProposedBrushesWindow::GetMiniMapColor(const ItemType& type) const {
	if (!type.sprite) {
		return 0;
	}
	return type.sprite->getMiniMapColor();
}

int ProposedBrushesWindow::ScoreFlagSimilarity(const ItemType& seed, const ItemType& candidate) const {
	int score = 0;

	if (seed.group == candidate.group && seed.group != ITEM_GROUP_NONE) {
		score += 50;
	}
	if (seed.type == candidate.type && seed.type != ITEM_TYPE_NONE) {
		score += 30;
	}
	if (seed.border_group != 0 && seed.border_group == candidate.border_group) {
		score += 45;
	}
	if (seed.isBorder == candidate.isBorder) {
		score += 8;
	}
	if (seed.isWall == candidate.isWall) {
		score += 10;
	}
	if (seed.isOptionalBorder == candidate.isOptionalBorder && seed.isOptionalBorder) {
		score += 15;
	}

	auto matchBool = [&](bool a, bool b, int weight) {
		if (a == b) {
			score += weight;
		}
	};

	matchBool(seed.unpassable, candidate.unpassable, 8);
	matchBool(seed.blockMissiles, candidate.blockMissiles, 6);
	matchBool(seed.blockPathfinder, candidate.blockPathfinder, 6);
	matchBool(seed.pickupable, candidate.pickupable, 8);
	matchBool(seed.moveable, candidate.moveable, 4);
	matchBool(seed.stackable, candidate.stackable, 6);
	matchBool(seed.rotable, candidate.rotable, 3);
	matchBool(seed.hasElevation, candidate.hasElevation, 5);
	matchBool(seed.isHangable, candidate.isHangable, 6);
	matchBool(seed.hookEast, candidate.hookEast, 4);
	matchBool(seed.hookSouth, candidate.hookSouth, 4);
	matchBool(seed.floorChange, candidate.floorChange, 12);
	matchBool(seed.floorChangeDown, candidate.floorChangeDown, 6);
	matchBool(seed.floorChangeNorth, candidate.floorChangeNorth, 4);
	matchBool(seed.floorChangeSouth, candidate.floorChangeSouth, 4);
	matchBool(seed.floorChangeEast, candidate.floorChangeEast, 4);
	matchBool(seed.floorChangeWest, candidate.floorChangeWest, 4);
	matchBool(seed.isTable, candidate.isTable, 10);
	matchBool(seed.isCarpet, candidate.isCarpet, 10);
	matchBool(seed.isBrushDoor, candidate.isBrushDoor, 12);

	if (seed.ground_equivalent != 0 && seed.ground_equivalent == candidate.id) {
		score += 40;
	}
	if (candidate.ground_equivalent != 0 && candidate.ground_equivalent == seed.id) {
		score += 40;
	}

	return score;
}

void ProposedBrushesWindow::BuildProposals(Brush* brush) {
	std::vector<ProposedBrushEntry> proposals;
	std::set<Brush*> seen;

	auto addBrush = [&](Brush* candidate, ProposedBrushCategory category, int score) {
		if (!candidate || candidate == brush || candidate == g_gui.eraser) {
			return;
		}
		if (candidate->isCreature() || candidate->isHouse() || candidate->isWaypoint()) {
			return;
		}
		if (!seen.insert(candidate).second) {
			return;
		}
		proposals.push_back(ProposedBrushEntry(candidate, category, score));
	};

	auto addItemId = [&](uint16_t itemId, ProposedBrushCategory category, int score) {
		if (!g_items.typeExists(itemId)) {
			return;
		}
		ItemType& type = g_items[itemId];
		if (type.raw_brush) {
			addBrush(type.raw_brush, category, score);
		}
		if (type.brush && type.brush != type.raw_brush) {
			addBrush(type.brush, category, score + 5);
		}
		if (type.doodad_brush) {
			addBrush(type.doodad_brush, category, score);
		}
	};

	ItemType* seed = ResolveSeedItem(brush);
	std::vector<uint16_t> borderIds;

	if (brush && brush->isGround()) {
		GroundBrush* ground = brush->asGround();
		if (ground) {
			ground->getRelatedBorderItemIds(borderIds);
		}
	}

	if (seed) {
		std::vector<AutoBorder*> matchingBorders;
		g_brushes.collectBordersContaining(seed->id, matchingBorders);
		for (AutoBorder* border : matchingBorders) {
			for (int i = 0; i < 13; ++i) {
				if (border->tiles[i] != 0) {
					borderIds.push_back(static_cast<uint16_t>(border->tiles[i]));
				}
			}
		}

		if (seed->border_group != 0) {
			const Brushes::BorderMap& allBorders = g_brushes.getBorders();
			for (Brushes::BorderMap::const_iterator it = allBorders.begin(); it != allBorders.end(); ++it) {
				AutoBorder* border = it->second;
				if (!border || border->group != seed->border_group) {
					continue;
				}
				for (int i = 0; i < 13; ++i) {
					if (border->tiles[i] != 0) {
						borderIds.push_back(static_cast<uint16_t>(border->tiles[i]));
					}
				}
			}
		}

		if (seed->ground_equivalent != 0) {
			addItemId(seed->ground_equivalent, PROPOSED_BORDER, 100);
		}
	}

	std::sort(borderIds.begin(), borderIds.end());
	borderIds.erase(std::unique(borderIds.begin(), borderIds.end()), borderIds.end());

	size_t borderCount = 0;
	for (uint16_t id : borderIds) {
		if (borderCount >= MAX_BORDER_PROPOSALS) {
			break;
		}
		const size_t before = proposals.size();
		addItemId(id, PROPOSED_BORDER, 120);
		if (proposals.size() > before) {
			++borderCount;
		}
	}

	if (seed) {
		const uint8_t seedColor = GetMiniMapColor(*seed);
		std::vector<std::pair<int, uint16_t>> flagHits;
		std::vector<std::pair<int, uint16_t>> colorHits;

		const uint16_t maxId = g_items.getMaxID();
		for (uint16_t id = 100; id <= maxId; ++id) {
			if (!g_items.typeExists(id) || id == seed->id) {
				continue;
			}
			ItemType& candidate = g_items[id];
			if (!candidate.raw_brush && !candidate.brush) {
				continue;
			}
			if (candidate.isMetaItem()) {
				continue;
			}

			const int flagScore = ScoreFlagSimilarity(*seed, candidate);
			if (flagScore >= MIN_FLAG_SCORE) {
				flagHits.push_back(std::make_pair(flagScore, id));
			}

			if (seedColor != 0) {
				const uint8_t color = GetMiniMapColor(candidate);
				if (color == seedColor) {
					colorHits.push_back(std::make_pair(flagScore + 20, id));
				}
			}
		}

		std::sort(flagHits.begin(), flagHits.end(), [](const std::pair<int, uint16_t>& a, const std::pair<int, uint16_t>& b) {
			return a.first > b.first;
		});
		std::sort(colorHits.begin(), colorHits.end(), [](const std::pair<int, uint16_t>& a, const std::pair<int, uint16_t>& b) {
			return a.first > b.first;
		});

		size_t flagCount = 0;
		for (const auto& hit : flagHits) {
			if (flagCount >= MAX_FLAG_PROPOSALS) {
				break;
			}
			const size_t before = proposals.size();
			addItemId(hit.second, PROPOSED_FLAGS, hit.first);
			if (proposals.size() > before) {
				++flagCount;
			}
		}

		size_t colorCount = 0;
		for (const auto& hit : colorHits) {
			if (colorCount >= MAX_COLOR_PROPOSALS) {
				break;
			}
			const size_t before = proposals.size();
			addItemId(hit.second, PROPOSED_COLOR, hit.first);
			if (proposals.size() > before) {
				++colorCount;
			}
		}
	}

	// Assign 1-9 hotkeys across display order (borders, flags, color)
	if (AreHotkeysEnabled()) {
		int nextKey = 1;
		auto assignKeys = [&](ProposedBrushCategory category) {
			for (ProposedBrushEntry& entry : proposals) {
				if (nextKey > 9) {
					return;
				}
				if (entry.category == category) {
					entry.hotkey = nextKey++;
				}
			}
		};
		assignKeys(PROPOSED_BORDER);
		assignKeys(PROPOSED_FLAGS);
		assignKeys(PROPOSED_COLOR);
	}

	panel->SetProposals(proposals);

	wxString seedName = "(none)";
	if (brush) {
		seedName = wxstr(brush->getName());
		if (seed) {
			seedName += wxString::Format(" [%d]", seed->id);
		}
	}
	seed_label->SetLabel("Seed: " + seedName);
	status_label->SetLabel(wxString::Format("Ready — %zu recommendations", proposals.size()));
	status_label->SetForegroundColour(wxColour(0, 120, 40));
	Layout();
}

void ProposedBrushesWindow::RebuildFromBrush(Brush* brush) {
	if (rebuilding) {
		return;
	}
	rebuilding = true;

	status_label->SetLabel("Analyzing brush flags / borders / colors...");
	status_label->SetForegroundColour(wxColour(160, 100, 0));
	seed_label->SetLabel("Seed: building...");
	Update();
	wxYield();

	try {
		BuildProposals(brush);
	} catch (...) {
		status_label->SetLabel("Failed to build proposals.");
		status_label->SetForegroundColour(*wxRED);
		panel->ClearProposals();
	}

	rebuilding = false;
}
