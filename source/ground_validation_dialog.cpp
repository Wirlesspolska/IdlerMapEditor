#include "main.h"
#include "ground_validation_dialog.h"
#include "common_windows.h"
#include "gui.h"

BEGIN_EVENT_TABLE(GroundValidationDialog, wxDialog)
    EVT_BUTTON(wxID_OK, GroundValidationDialog::OnClickOK)
    EVT_BUTTON(wxID_CANCEL, GroundValidationDialog::OnClickCancel)
END_EVENT_TABLE()

GroundValidationDialog::GroundValidationDialog(wxWindow* parent) :
    wxDialog(parent, wxID_ANY, "Ground Tile Validation", wxDefaultPosition, wxSize(300, 200)),
    validateGroundStack(false),
    generateEmptySurroundedGrounds(false),
    removeDuplicateGrounds(false)
{
    wxBoxSizer* topsizer = new wxBoxSizer(wxVERTICAL);
    
    // Create checkboxes
    validateGroundStackBox = new wxCheckBox(this, wxID_ANY, "Validate ground stack order");
    validateGroundStackBox->SetToolTip("Move ground tiles to bottom of stack if they are above other items");
    
    generateEmptySurroundedGroundsBox = new wxCheckBox(this, wxID_ANY, "Generate empty surrounded grounds");
    generateEmptySurroundedGroundsBox->SetToolTip("Fill empty ground tiles that are surrounded by other ground tiles");
    
    removeDuplicateGroundsBox = new wxCheckBox(this, wxID_ANY, "Remove duplicate grounds");
    removeDuplicateGroundsBox->SetToolTip("Remove duplicate ground tiles from the same position");

    // Add checkboxes to sizer
    topsizer->Add(validateGroundStackBox, 0, wxALL | wxEXPAND, 5);
    topsizer->Add(generateEmptySurroundedGroundsBox, 0, wxALL | wxEXPAND, 5);
    topsizer->Add(removeDuplicateGroundsBox, 0, wxALL | wxEXPAND, 5);

    // Add warning text
    wxStaticText* warning = new wxStaticText(this, wxID_ANY, 
        "Warning: This operation cannot be undone!\nPlease save your map before proceeding.");
    warning->SetForegroundColour(*wxRED);
    topsizer->Add(warning, 0, wxALL | wxALIGN_CENTER, 10);

    // Add buttons
    wxStdDialogButtonSizer* buttonSizer = new wxStdDialogButtonSizer();
    buttonSizer->AddButton(new wxButton(this, wxID_OK, "Validate"));
    buttonSizer->AddButton(new wxButton(this, wxID_CANCEL, "Cancel"));
    buttonSizer->Realize();
    topsizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 5);

    SetSizer(topsizer);
    Centre();
}

GroundValidationDialog::~GroundValidationDialog() {
    // Nothing to clean up
}

void GroundValidationDialog::OnClickOK(wxCommandEvent& event) {
    validateGroundStack = validateGroundStackBox->GetValue();
    generateEmptySurroundedGrounds = generateEmptySurroundedGroundsBox->GetValue();
    removeDuplicateGrounds = removeDuplicateGroundsBox->GetValue();

    if (!validateGroundStack && !generateEmptySurroundedGrounds && !removeDuplicateGrounds) {
        g_gui.PopupDialog(this, "Error", "Please select at least one validation option!", wxOK);
        return;
    }

    EndModal(wxID_OK);
}

void GroundValidationDialog::OnClickCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
} 