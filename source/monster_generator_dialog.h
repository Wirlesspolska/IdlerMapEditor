#ifndef RME_MONSTER_GENERATOR_DIALOG_H_
#define RME_MONSTER_GENERATOR_DIALOG_H_

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/listctrl.h>

class MonsterGeneratorDialog : public wxDialog {
public:
    MonsterGeneratorDialog(wxWindow* parent);
    virtual ~MonsterGeneratorDialog();

protected:
    // Event handlers
    void OnSave(wxCommandEvent& event);
    void OnLoad(wxCommandEvent& event);
    void OnOutfitChange(wxSpinEvent& event);
    void OnAddAttack(wxCommandEvent& event);
    void OnAddDefense(wxCommandEvent& event);
    void OnAddLoot(wxCommandEvent& event);

private:
    void CreateGUI();
    void UpdatePreview();
    void SaveMonsterFile();
    void LoadMonsterFile();

    // GUI elements
    wxNotebook* notebook;
    
    // Basic info page
    wxPanel* basicPanel;
    wxTextCtrl* nameCtrl;
    wxTextCtrl* descriptionCtrl;
    wxChoice* raceChoice;
    wxSpinCtrl* experienceCtrl;
    wxSpinCtrl* healthCtrl;
    wxSpinCtrl* speedCtrl;
    wxSpinCtrl* armorCtrl;
    wxSpinCtrl* defenseCtrl;

    // Outfit page
    wxPanel* outfitPanel;
    wxSpinCtrl* lookTypeCtrl;
    wxSpinCtrl* headCtrl;
    wxSpinCtrl* bodyCtrl;
    wxSpinCtrl* legsCtrl;
    wxSpinCtrl* feetCtrl;
    wxSpinCtrl* addonsCtrl;
    wxStaticBitmap* outfitPreview;

    // Attacks page
    wxPanel* attacksPanel;
    wxListCtrl* attacksList;
    
    // Defenses page
    wxPanel* defensesPanel;
    wxListCtrl* defensesList;

    // Immunities page
    wxPanel* immunitiesPanel;
    wxCheckBox* holyCheck;
    wxCheckBox* deathCheck;
    wxCheckBox* fireCheck;
    wxCheckBox* energyCheck;
    wxCheckBox* poisonCheck;
    wxCheckBox* iceCheck;
    wxCheckBox* physicalCheck;

    // Loot page
    wxPanel* lootPanel;
    wxListCtrl* lootList;

    DECLARE_EVENT_TABLE()
};

#endif // RME_MONSTER_GENERATOR_DIALOG_H_ 