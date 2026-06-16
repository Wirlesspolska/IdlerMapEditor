#ifndef RME_GROUND_VALIDATION_DIALOG_H_
#define RME_GROUND_VALIDATION_DIALOG_H_

#include <wx/dialog.h>

class GroundValidationDialog : public wxDialog {
public:
    GroundValidationDialog(wxWindow* parent);
    ~GroundValidationDialog();

    bool shouldValidateGroundStack() const { return validateGroundStack; }
    bool shouldGenerateEmptySurroundedGrounds() const { return generateEmptySurroundedGrounds; }
    bool shouldRemoveDuplicateGrounds() const { return removeDuplicateGrounds; }

protected:
    void OnClickOK(wxCommandEvent& event);
    void OnClickCancel(wxCommandEvent& event);

    wxCheckBox* validateGroundStackBox;
    wxCheckBox* generateEmptySurroundedGroundsBox;
    wxCheckBox* removeDuplicateGroundsBox;

    bool validateGroundStack;
    bool generateEmptySurroundedGrounds;
    bool removeDuplicateGrounds;

    DECLARE_EVENT_TABLE()
};

#endif // RME_GROUND_VALIDATION_DIALOG_H_ 