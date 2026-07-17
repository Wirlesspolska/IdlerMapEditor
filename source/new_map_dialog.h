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

#ifndef RME_NEW_MAP_DIALOG_H_
#define RME_NEW_MAP_DIALOG_H_

#include "main.h"
#include "client_version.h"

class NewMapDialog : public wxDialog {
public:
	NewMapDialog(wxWindow* parent);
	~NewMapDialog() = default;

	wxString GetMapPath() const;
	ClientVersionID GetSelectedVersion() const;

protected:
	void OnBrowse(wxCommandEvent& event);
	void OnClickOK(wxCommandEvent& event);
	void OnClickCancel(wxCommandEvent& event);

private:
	void PopulateVersions();

	wxTextCtrl* path_text;
	wxButton* browse_button;
	wxChoice* version_choice;
	std::vector<ClientVersionID> version_ids;

	DECLARE_EVENT_TABLE()
};

enum {
	NEW_MAP_DIALOG_BROWSE = 12000,
};

#endif
