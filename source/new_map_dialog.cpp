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
#include "new_map_dialog.h"
#include "settings.h"
#include "gui.h"

BEGIN_EVENT_TABLE(NewMapDialog, wxDialog)
	EVT_BUTTON(NEW_MAP_DIALOG_BROWSE, NewMapDialog::OnBrowse)
	EVT_BUTTON(wxID_OK, NewMapDialog::OnClickOK)
	EVT_BUTTON(wxID_CANCEL, NewMapDialog::OnClickCancel)
END_EVENT_TABLE()

NewMapDialog::NewMapDialog(wxWindow* parent) :
	wxDialog(parent, wxID_ANY, "New Map", wxDefaultPosition, wxDefaultSize),
	path_text(nullptr),
	browse_button(nullptr),
	version_choice(nullptr) {
	wxBoxSizer* topsizer = newd wxBoxSizer(wxVERTICAL);

	wxFlexGridSizer* form = newd wxFlexGridSizer(2, 5, 10);
	form->AddGrowableCol(1);

	form->Add(newd wxStaticText(this, wxID_ANY, "Map location:"), 0, wxALIGN_CENTER_VERTICAL);

	wxBoxSizer* path_sizer = newd wxBoxSizer(wxHORIZONTAL);
	path_text = newd wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, FROM_DIP(this, wxSize(320, -1)));
	browse_button = newd wxButton(this, NEW_MAP_DIALOG_BROWSE, "Browse...", wxDefaultPosition, FROM_DIP(this, wxSize(80, -1)));
	path_sizer->Add(path_text, 1, wxEXPAND | wxRIGHT, 5);
	path_sizer->Add(browse_button, 0);
	form->Add(path_sizer, 0, wxEXPAND);

	form->Add(newd wxStaticText(this, wxID_ANY, "Force version:"), 0, wxALIGN_CENTER_VERTICAL);
	version_choice = newd wxChoice(this, wxID_ANY);
	form->Add(version_choice, 0, wxEXPAND);

	topsizer->Add(form, 0, wxEXPAND | wxALL, 10);
	topsizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

	PopulateVersions();

	wxString default_path;
	std::string recent = g_settings.getString(Config::RECENT_EDITED_MAP_PATH);
	if (!recent.empty()) {
		wxFileName recent_fn(wxstr(recent));
		default_path = recent_fn.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR) + "Untitled.otbm";
	} else {
		default_path = "Untitled.otbm";
	}
	path_text->SetValue(default_path);

	SetSizerAndFit(topsizer);
	CentreOnParent();
	path_text->SetFocus();
}

void NewMapDialog::PopulateVersions() {
	version_choice->Clear();
	version_ids.clear();

	ClientVersionList versions = ClientVersion::getAllVisible();
	ClientVersionID preferred = ClientVersionID(g_settings.getInteger(Config::DEFAULT_CLIENT_VERSION));
	if (preferred == CLIENT_VERSION_NONE && ClientVersion::getLatestVersion()) {
		preferred = ClientVersion::getLatestVersion()->getID();
	}

	int selection = 0;
	int index = 0;
	for (ClientVersion* version : versions) {
		if (!version->isVisible()) {
			continue;
		}
		version_choice->Append(wxstr(version->getName()));
		version_ids.push_back(version->getID());
		if (version->getID() == preferred) {
			selection = index;
		}
		++index;
	}

	if (version_choice->GetCount() > 0) {
		version_choice->SetSelection(selection);
	}
}

wxString NewMapDialog::GetMapPath() const {
	return path_text->GetValue().Trim(true).Trim(false);
}

ClientVersionID NewMapDialog::GetSelectedVersion() const {
	int sel = version_choice->GetSelection();
	if (sel == wxNOT_FOUND || sel < 0 || static_cast<size_t>(sel) >= version_ids.size()) {
		if (ClientVersion::getLatestVersion()) {
			return ClientVersion::getLatestVersion()->getID();
		}
		return CLIENT_VERSION_NONE;
	}
	return version_ids[sel];
}

void NewMapDialog::OnBrowse(wxCommandEvent& WXUNUSED(event)) {
	wxString wildcard = g_settings.getInteger(Config::USE_OTGZ) != 0 ? MAP_SAVE_FILE_WILDCARD_OTGZ : MAP_SAVE_FILE_WILDCARD;
	wxFileName current(GetMapPath());
	wxString dir = current.GetPath();
	wxString name = current.GetFullName();
	if (name.empty()) {
		name = "Untitled.otbm";
	}

	wxFileDialog dialog(this, "New map location", dir, name, wildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (dialog.ShowModal() == wxID_OK) {
		path_text->SetValue(dialog.GetPath());
	}
}

void NewMapDialog::OnClickOK(wxCommandEvent& WXUNUSED(event)) {
	wxString path = GetMapPath();
	if (path.empty()) {
		g_gui.PopupDialog(this, "Error", "Please choose a location for the new map.", wxOK);
		return;
	}

	wxFileName fn(path);
	if (!fn.HasExt()) {
		fn.SetExt("otbm");
		path_text->SetValue(fn.GetFullPath());
	}

	if (version_choice->GetCount() == 0 || version_choice->GetSelection() == wxNOT_FOUND) {
		g_gui.PopupDialog(this, "Error", "No client versions are available. Check clients.xml.", wxOK);
		return;
	}

	EndModal(wxID_OK);
}

void NewMapDialog::OnClickCancel(wxCommandEvent& WXUNUSED(event)) {
	EndModal(wxID_CANCEL);
}
