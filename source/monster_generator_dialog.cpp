#include "monster_generator_dialog.h"
#include "sprites.h"
#include "outfit.h"
#include "creatures.h"
#include "ext/pugixml.hpp"

BEGIN_EVENT_TABLE(MonsterGeneratorDialog, wxDialog)
    EVT_BUTTON(wxID_SAVE, MonsterGeneratorDialog::OnSave)
    EVT_BUTTON(wxID_OPEN, MonsterGeneratorDialog::OnLoad)
    EVT_SPINCTRL(SPIN_OUTFIT_TYPE, MonsterGeneratorDialog::OnOutfitChange)
    EVT_BUTTON(BUTTON_ADD_ATTACK, MonsterGeneratorDialog::OnAddAttack)
    EVT_BUTTON(BUTTON_ADD_DEFENSE, MonsterGeneratorDialog::OnAddDefense)
    EVT_BUTTON(BUTTON_ADD_LOOT, MonsterGeneratorDialog::OnAddLoot)
END_EVENT_TABLE()

MonsterGeneratorDialog::MonsterGeneratorDialog(wxWindow* parent) 
    : wxDialog(parent, wxID_ANY, "Monster Generator", wxDefaultPosition, wxSize(800, 600))
{
    CreateGUI();
}

MonsterGeneratorDialog::~MonsterGeneratorDialog()
{
}

void MonsterGeneratorDialog::CreateGUI()
{
    // Create main sizer
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Create notebook
    notebook = new wxNotebook(this, wxID_ANY);

    // Basic Info Page
    basicPanel = new wxPanel(notebook);
    wxBoxSizer* basicSizer = new wxBoxSizer(wxVERTICAL);

    // Name
    wxBoxSizer* nameSizer = new wxBoxSizer(wxHORIZONTAL);
    nameSizer->Add(new wxStaticText(basicPanel, wxID_ANY, "Name:"), 0, wxALL, 5);
    nameCtrl = new wxTextCtrl(basicPanel, wxID_ANY);
    nameSizer->Add(nameCtrl, 1, wxEXPAND | wxALL, 5);
    basicSizer->Add(nameSizer, 0, wxEXPAND);

    // Description
    wxBoxSizer* descSizer = new wxBoxSizer(wxHORIZONTAL);
    descSizer->Add(new wxStaticText(basicPanel, wxID_ANY, "Description:"), 0, wxALL, 5);
    descriptionCtrl = new wxTextCtrl(basicPanel, wxID_ANY);
    descSizer->Add(descriptionCtrl, 1, wxEXPAND | wxALL, 5);
    basicSizer->Add(descSizer, 0, wxEXPAND);

    // Race
    wxBoxSizer* raceSizer = new wxBoxSizer(wxHORIZONTAL);
    raceSizer->Add(new wxStaticText(basicPanel, wxID_ANY, "Race:"), 0, wxALL, 5);
    wxArrayString races;
    races.Add("blood");
    races.Add("venom");
    races.Add("undead");
    races.Add("fire");
    races.Add("energy");
    raceChoice = new wxChoice(basicPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, races);
    raceSizer->Add(raceChoice, 1, wxEXPAND | wxALL, 5);
    basicSizer->Add(raceSizer, 0, wxEXPAND);

    // Stats
    wxFlexGridSizer* statsSizer = new wxFlexGridSizer(2, 5, 5);
    
    statsSizer->Add(new wxStaticText(basicPanel, wxID_ANY, "Experience:"));
    experienceCtrl = new wxSpinCtrl(basicPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 999999999);
    statsSizer->Add(experienceCtrl);

    statsSizer->Add(new wxStaticText(basicPanel, wxID_ANY, "Health:"));
    healthCtrl = new wxSpinCtrl(basicPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 999999999);
    statsSizer->Add(healthCtrl);

    statsSizer->Add(new wxStaticText(basicPanel, wxID_ANY, "Speed:"));
    speedCtrl = new wxSpinCtrl(basicPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 9999);
    statsSizer->Add(speedCtrl);

    basicSizer->Add(statsSizer, 0, wxALL, 5);
    basicPanel->SetSizer(basicSizer);

    // Outfit Page
    outfitPanel = new wxPanel(notebook);
    wxBoxSizer* outfitSizer = new wxBoxSizer(wxVERTICAL);

    wxFlexGridSizer* outfitGridSizer = new wxFlexGridSizer(2, 5, 5);
    
    outfitGridSizer->Add(new wxStaticText(outfitPanel, wxID_ANY, "Look Type:"));
    lookTypeCtrl = new wxSpinCtrl(outfitPanel, SPIN_OUTFIT_TYPE, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 999);
    outfitGridSizer->Add(lookTypeCtrl);

    outfitGridSizer->Add(new wxStaticText(outfitPanel, wxID_ANY, "Head:"));
    headCtrl = new wxSpinCtrl(outfitPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255);
    outfitGridSizer->Add(headCtrl);

    outfitGridSizer->Add(new wxStaticText(outfitPanel, wxID_ANY, "Body:"));
    bodyCtrl = new wxSpinCtrl(outfitPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 255);
    outfitGridSizer->Add(bodyCtrl);

    outfitSizer->Add(outfitGridSizer, 0, wxALL, 5);

    // Outfit preview
    outfitPreview = new wxStaticBitmap(outfitPanel, wxID_ANY, wxNullBitmap, wxDefaultPosition, wxSize(64, 64));
    outfitSizer->Add(outfitPreview, 0, wxALL | wxCENTER, 5);

    outfitPanel->SetSizer(outfitSizer);

    // Add pages to notebook
    notebook->AddPage(basicPanel, "Basic Info");
    notebook->AddPage(outfitPanel, "Outfit");
    
    // Add notebook to main sizer
    mainSizer->Add(notebook, 1, wxEXPAND | wxALL, 5);

    // Buttons
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(new wxButton(this, wxID_SAVE, "Save"), 0, wxALL, 5);
    buttonSizer->Add(new wxButton(this, wxID_OPEN, "Load"), 0, wxALL, 5);
    buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Close"), 0, wxALL, 5);
    mainSizer->Add(buttonSizer, 0, wxALIGN_RIGHT | wxALL, 5);

    SetSizer(mainSizer);
    Layout();
}

void MonsterGeneratorDialog::UpdatePreview()
{
    // Get the outfit values
    int type = lookTypeCtrl->GetValue();
    int head = headCtrl->GetValue();
    int body = bodyCtrl->GetValue();
    int legs = legsCtrl->GetValue();
    int feet = feetCtrl->GetValue();
    int addons = addonsCtrl->GetValue();

    // Create outfit object
    Outfit outfit;
    outfit.lookType = type;
    outfit.lookHead = head;
    outfit.lookBody = body;
    outfit.lookLegs = legs;
    outfit.lookFeet = feet;
    outfit.lookAddons = addons;

    // Get the sprite and convert to wxBitmap
    if(g_sprites.GetOutfitSprite(outfit)) {
        wxBitmap bmp = wxBitmap(g_sprites.GetOutfitSprite(outfit)->getImage());
        outfitPreview->SetBitmap(bmp);
    }
}

void MonsterGeneratorDialog::OnOutfitChange(wxSpinEvent& event)
{
    UpdatePreview();
}

void MonsterGeneratorDialog::SaveMonsterFile()
{
    wxFileDialog saveFileDialog(this, "Save Monster File", "", "",
                              "XML files (*.xml)|*.xml", wxFD_SAVE|wxFD_OVERWRITE_PROMPT);
    if (saveFileDialog.ShowModal() == wxID_CANCEL)
        return;

    pugi::xml_document doc;
    pugi::xml_node monster = doc.append_child("monster");

    // Basic attributes
    monster.append_attribute("name") = nameCtrl->GetValue();
    monster.append_attribute("nameDescription") = descriptionCtrl->GetValue();
    monster.append_attribute("race") = raceChoice->GetStringSelection();
    monster.append_attribute("experience") = experienceCtrl->GetValue();
    monster.append_attribute("speed") = speedCtrl->GetValue();

    // Health
    pugi::xml_node health = monster.append_child("health");
    health.append_attribute("now") = healthCtrl->GetValue();
    health.append_attribute("max") = healthCtrl->GetValue();

    // Look
    pugi::xml_node look = monster.append_child("look");
    look.append_attribute("type") = lookTypeCtrl->GetValue();
    look.append_attribute("head") = headCtrl->GetValue();
    look.append_attribute("body") = bodyCtrl->GetValue();
    look.append_attribute("legs") = legsCtrl->GetValue();
    look.append_attribute("feet") = feetCtrl->GetValue();
    look.append_attribute("addons") = addonsCtrl->GetValue();

    // Save the file
    doc.save_file(saveFileDialog.GetPath().mb_str());
}

void MonsterGeneratorDialog::OnSave(wxCommandEvent& event)
{
    SaveMonsterFile();
}

void MonsterGeneratorDialog::OnLoad(wxCommandEvent& event)
{
    wxFileDialog openFileDialog(this, "Open Monster File", "", "",
                              "XML files (*.xml)|*.xml", wxFD_OPEN|wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;

    pugi::xml_document doc;
    if(!doc.load_file(openFileDialog.GetPath().mb_str())) {
        wxMessageBox("Failed to load monster file!", "Error", wxICON_ERROR);
        return;
    }

    pugi::xml_node monster = doc.child("monster");
    if(!monster) {
        wxMessageBox("Invalid monster file format!", "Error", wxICON_ERROR);
        return;
    }

    // Load basic attributes
    nameCtrl->SetValue(monster.attribute("name").as_string());
    descriptionCtrl->SetValue(monster.attribute("nameDescription").as_string());
    raceChoice->SetStringSelection(monster.attribute("race").as_string());
    experienceCtrl->SetValue(monster.attribute("experience").as_int());
    speedCtrl->SetValue(monster.attribute("speed").as_int());

    // Load health
    if(pugi::xml_node health = monster.child("health")) {
        healthCtrl->SetValue(health.attribute("max").as_int());
    }

    // Load look
    if(pugi::xml_node look = monster.child("look")) {
        lookTypeCtrl->SetValue(look.attribute("type").as_int());
        headCtrl->SetValue(look.attribute("head").as_int());
        bodyCtrl->SetValue(look.attribute("body").as_int());
        legsCtrl->SetValue(look.attribute("legs").as_int());
        feetCtrl->SetValue(look.attribute("feet").as_int());
        addonsCtrl->SetValue(look.attribute("addons").as_int());
        UpdatePreview();
    }
} 