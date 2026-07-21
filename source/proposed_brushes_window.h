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

#ifndef RME_PROPOSED_BRUSHES_WINDOW_H_
#define RME_PROPOSED_BRUSHES_WINDOW_H_

#include "main.h"
#include "palette_common.h"

#include <wx/panel.h>
#include <wx/scrolwin.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <wx/button.h>

#include <vector>
#include <set>

class Brush;
class BrushButton;
class ItemType;

enum ProposedBrushCategory {
	PROPOSED_BORDER = 0,
	PROPOSED_FLAGS,
	PROPOSED_COLOR
};

struct ProposedBrushEntry {
	Brush* brush;
	ProposedBrushCategory category;
	int score;
	int hotkey; // 1-9 when bound, else 0

	ProposedBrushEntry(Brush* b, ProposedBrushCategory cat, int s) :
		brush(b), category(cat), score(s), hotkey(0) { }
};

class ProposedBrushesPanel : public wxScrolledWindow {
public:
	ProposedBrushesPanel(wxWindow* parent);
	~ProposedBrushesPanel();

	void SetProposals(const std::vector<ProposedBrushEntry>& entries);
	void ClearProposals();
	void RefreshDisplay();
	Brush* GetBrushAtHotkey(int digit) const; // digit 1-9
	const std::vector<ProposedBrushEntry>& GetEntries() const {
		return entries;
	}

	void OnBrushClick(wxCommandEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnPaint(wxPaintEvent& event);
	void OnEraseBackground(wxEraseEvent& event);

private:
	wxString GetCategoryName(ProposedBrushCategory category) const;

	std::vector<ProposedBrushEntry> entries;
	std::vector<BrushButton*> brush_buttons;
	std::vector<wxStaticText*> section_labels;
	wxBoxSizer* main_sizer;

	DECLARE_EVENT_TABLE()
};

class ProposedBrushesWindow : public wxPanel {
public:
	ProposedBrushesWindow(wxWindow* parent);
	~ProposedBrushesWindow();

	void RebuildFromCurrentBrush();
	void RebuildFromBrush(Brush* brush);
	bool SelectHotkey(int digit); // 1-9, returns true if handled
	bool AreHotkeysEnabled() const;

	void OnRebuildButton(wxCommandEvent& event);
	void OnHotkeyCheckbox(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);
	void OnPaint(wxPaintEvent& event);
	void OnEraseBackground(wxEraseEvent& event);

private:
	ItemType* ResolveSeedItem(Brush* brush) const;
	uint8_t GetMiniMapColor(const ItemType& type) const;
	int ScoreFlagSimilarity(const ItemType& seed, const ItemType& candidate) const;
	void BuildProposals(Brush* brush);

	ProposedBrushesPanel* panel;
	wxStaticText* status_label;
	wxStaticText* seed_label;
	wxCheckBox* hotkey_checkbox;
	wxButton* rebuild_button;
	bool rebuilding;

	DECLARE_EVENT_TABLE()
};

#endif
