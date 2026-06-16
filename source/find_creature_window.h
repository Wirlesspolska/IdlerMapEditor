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

#ifndef RME_FIND_CREATURE_WINDOW_H_
#define RME_FIND_CREATURE_WINDOW_H_

#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/timer.h>

class FindCreatureListBox;

class FindCreatureDialog : public wxDialog {
public:
    FindCreatureDialog(wxWindow* parent, const wxString& title);
    virtual ~FindCreatureDialog();

    wxString getResultName() const { return result_name; }

    // Public methods
    void RefreshContentsInternal();
    
protected:
    void OnText(wxCommandEvent& event);
    void OnInputTimer(wxTimerEvent& event);
    void OnClickOK(wxCommandEvent& event);
    void OnClickCancel(wxCommandEvent& event);
    void OnRefreshClick(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

    wxTextCtrl* name_text_input;
    wxTimer input_timer;
    wxCheckBox* search_monsters;
    wxCheckBox* search_npcs;
    wxCheckBox* auto_refresh;

    FindCreatureListBox* creatures_list;
    wxStdDialogButtonSizer* buttons_box_sizer;
    wxButton* ok_button;
    wxButton* cancel_button;
    wxButton* refresh_button;
    wxString result_name;

    DECLARE_EVENT_TABLE()
};

#endif // RME_FIND_CREATURE_WINDOW_H_ 