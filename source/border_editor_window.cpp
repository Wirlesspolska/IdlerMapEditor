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
#include "border_editor_window.h"
#include "browse_tile_window.h"
#include "find_item_window.h"
#include "common_windows.h"
#include "graphics.h"
#include "gui.h"
#include "artprovider.h"
#include "items.h"
#include "brush.h"
#include "ground_brush.h"
#include "materials.h"
#include "tileset.h"
#include "dcbutton.h"
#include <wx/sizer.h>
#include <wx/gbsizer.h>
#include <wx/statline.h>
#include <wx/tglbtn.h>
#include <wx/dcbuffer.h>
#include <wx/filename.h>
#include <wx/filepicker.h>
#include <wx/textdlg.h>
#include <algorithm>
#define BORDER_PREVIEW_SIZE 256
#define BORDER_GRID_CELL_SIZE 96
#define ID_BORDER_GRID_SELECT wxID_HIGHEST + 1
#define ID_GROUND_ITEM_LIST wxID_HIGHEST + 2
#define ID_GRID_VIEW_NOTEBOOK wxID_HIGHEST + 3
#define ID_SERVER_LOOK_PICKER wxID_HIGHEST + 4
#define ID_GROUND_ITEM_PICKER wxID_HIGHEST + 5
#define ID_BORDER_ITEM_PICKER wxID_HIGHEST + 6
#define ID_CREATE_TILESET wxID_HIGHEST + 7
#define ID_TEST_BRUSH wxID_HIGHEST + 8
#define ID_ZORDER_CHOICE wxID_HIGHEST + 9

const BorderGridCell BorderGridPanel::s_edgeCells[9] = {
    BorderGridCell(EDGE_CNW, EDGE_DNW),
    BorderGridCell(EDGE_N, EDGE_NONE),
    BorderGridCell(EDGE_CNE, EDGE_DNE),
    BorderGridCell(EDGE_W, EDGE_NONE),
    BorderGridCell(EDGE_NONE, EDGE_NONE),
    BorderGridCell(EDGE_E, EDGE_NONE),
    BorderGridCell(EDGE_CSW, EDGE_DSW),
    BorderGridCell(EDGE_S, EDGE_NONE),
    BorderGridCell(EDGE_CSE, EDGE_DSE),
};

// Utility functions for edge string/position conversion
BorderEdgePosition edgeStringToPosition(const std::string& edgeStr) {
    if (edgeStr == "n") return EDGE_N;
    if (edgeStr == "e") return EDGE_E;
    if (edgeStr == "s") return EDGE_S;
    if (edgeStr == "w") return EDGE_W;
    if (edgeStr == "cnw") return EDGE_CNW;
    if (edgeStr == "cne") return EDGE_CNE;
    if (edgeStr == "cse") return EDGE_CSE;
    if (edgeStr == "csw") return EDGE_CSW;
    if (edgeStr == "dnw") return EDGE_DNW;
    if (edgeStr == "dne") return EDGE_DNE;
    if (edgeStr == "dse") return EDGE_DSE;
    if (edgeStr == "dsw") return EDGE_DSW;
    return EDGE_NONE;
}

std::string edgePositionToString(BorderEdgePosition pos) {
    switch (pos) {
        case EDGE_N: return "n";
        case EDGE_E: return "e";
        case EDGE_S: return "s";
        case EDGE_W: return "w";
        case EDGE_CNW: return "cnw";
        case EDGE_CNE: return "cne";
        case EDGE_CSE: return "cse";
        case EDGE_CSW: return "csw";
        case EDGE_DNW: return "dnw";
        case EDGE_DNE: return "dne";
        case EDGE_DSE: return "dse";
        case EDGE_DSW: return "dsw";
        default: return "";
    }
}

// Add a helper function at the top of the file to get item ID from brush
uint16_t GetItemIDFromBrush(Brush* brush) {
    if (!brush) {
        wxLogDebug("GetItemIDFromBrush: Brush is null");
        OutputDebugStringA("GetItemIDFromBrush: Brush is null\n");
        return 0;
    }
    
    uint16_t id = 0;
    
    wxLogDebug("GetItemIDFromBrush: Checking brush type: %s", wxString(brush->getName()).c_str());
    OutputDebugStringA(wxString::Format("GetItemIDFromBrush: Checking brush type: %s\n", wxString(brush->getName()).c_str()).mb_str());
    
    // First prioritize RAW brush - this is the most direct approach
    if (brush->isRaw()) {
        RAWBrush* rawBrush = brush->asRaw();
        if (rawBrush) {
            id = rawBrush->getItemID();
            wxLogDebug("GetItemIDFromBrush: Found RAW brush ID: %d", id);
            OutputDebugStringA(wxString::Format("GetItemIDFromBrush: Found RAW brush ID: %d\n", id).mb_str());
            if (id > 0) {
                return id;
            }
        }
    } 
    
    // Then try getID which sometimes works directly
    id = brush->getID();
    if (id > 0) {
        wxLogDebug("GetItemIDFromBrush: Got ID from brush->getID(): %d", id);
        OutputDebugStringA(wxString::Format("GetItemIDFromBrush: Got ID from brush->getID(): %d\n", id).mb_str());
        return id;
    }
    
    // Try getLookID which works for most other brush types
    id = brush->getLookID();
    if (id > 0) {
        wxLogDebug("GetItemIDFromBrush: Got ID from getLookID(): %d", id);
        OutputDebugStringA(wxString::Format("GetItemIDFromBrush: Got ID from getLookID(): %d\n", id).mb_str());
        return id;
    }
    
    // Try specific brush type methods - when all else fails
    if (brush->isGround()) {
        wxLogDebug("GetItemIDFromBrush: Detected Ground brush");
        OutputDebugStringA("GetItemIDFromBrush: Detected Ground brush\n");
        GroundBrush* groundBrush = brush->asGround();
        if (groundBrush) {
            // For ground brush, id is usually the server_lookid from grounds.xml
            // Try to find something else
            wxLogDebug("GetItemIDFromBrush: Failed to get ID for Ground brush");
            OutputDebugStringA("GetItemIDFromBrush: Failed to get ID for Ground brush\n");
        }
    }
    else if (brush->isWall()) {
        wxLogDebug("GetItemIDFromBrush: Detected Wall brush");
        OutputDebugStringA("GetItemIDFromBrush: Detected Wall brush\n");
        WallBrush* wallBrush = brush->asWall();
        if (wallBrush) {
            wxLogDebug("GetItemIDFromBrush: Failed to get ID for Wall brush");
            OutputDebugStringA("GetItemIDFromBrush: Failed to get ID for Wall brush\n");
        }
    }
    else if (brush->isDoodad()) {
        wxLogDebug("GetItemIDFromBrush: Detected Doodad brush");
        OutputDebugStringA("GetItemIDFromBrush: Detected Doodad brush\n");
        DoodadBrush* doodadBrush = brush->asDoodad();
        if (doodadBrush) {
            wxLogDebug("GetItemIDFromBrush: Failed to get ID for Doodad brush");
            OutputDebugStringA("GetItemIDFromBrush: Failed to get ID for Doodad brush\n");
        }
    }
    
    if (id == 0) {
        wxLogDebug("GetItemIDFromBrush: Failed to get item ID from brush %s", wxString(brush->getName()).c_str());
        OutputDebugStringA(wxString::Format("GetItemIDFromBrush: Failed to get item ID from brush %s\n", wxString(brush->getName()).c_str()).mb_str());
    }
    
    return id;
}

// Event table for BorderEditorDialog
BEGIN_EVENT_TABLE(BorderEditorDialog, wxDialog)
    EVT_BUTTON(wxID_ADD, BorderEditorDialog::OnAddItem)
    EVT_BUTTON(wxID_CLEAR, BorderEditorDialog::OnClear)
    EVT_BUTTON(wxID_SAVE, BorderEditorDialog::OnSave)
    EVT_BUTTON(wxID_CLOSE, BorderEditorDialog::OnClose)
    EVT_BUTTON(wxID_FIND, BorderEditorDialog::OnBrowse)
    EVT_COMBOBOX(wxID_ANY, BorderEditorDialog::OnLoadBorder)
    EVT_NOTEBOOK_PAGE_CHANGED(wxID_ANY, BorderEditorDialog::OnPageChanged)
    EVT_BUTTON(wxID_ADD + 100, BorderEditorDialog::OnAddGroundItem)
    EVT_BUTTON(wxID_REMOVE, BorderEditorDialog::OnRemoveGroundItem)
    EVT_BUTTON(wxID_FIND + 100, BorderEditorDialog::OnGroundBrowse)
    EVT_COMBOBOX(wxID_ANY + 100, BorderEditorDialog::OnLoadGroundBrush)
    EVT_NOTEBOOK_PAGE_CHANGED(ID_GRID_VIEW_NOTEBOOK, BorderEditorDialog::OnGridViewChanged)
    EVT_BUTTON(ID_CREATE_TILESET, BorderEditorDialog::OnCreateTileset)
    EVT_BUTTON(ID_TEST_BRUSH, BorderEditorDialog::OnTestBrush)
    EVT_CHOICE(ID_ZORDER_CHOICE, BorderEditorDialog::OnZOrderChoice)
END_EVENT_TABLE()

// Event table for BorderItemButton
BEGIN_EVENT_TABLE(BorderItemButton, wxButton)
    EVT_PAINT(BorderItemButton::OnPaint)
END_EVENT_TABLE()

// Event table for BorderGridPanel
BEGIN_EVENT_TABLE(BorderGridPanel, wxPanel)
    EVT_PAINT(BorderGridPanel::OnPaint)
    EVT_LEFT_UP(BorderGridPanel::OnMouseClick)
    EVT_LEFT_DOWN(BorderGridPanel::OnMouseDown)
END_EVENT_TABLE()

// Event table for BorderPreviewPanel
BEGIN_EVENT_TABLE(BorderPreviewPanel, wxPanel)
    EVT_PAINT(BorderPreviewPanel::OnPaint)
END_EVENT_TABLE()

BorderEditorDialog::BorderEditorDialog(wxWindow* parent, const wxString& title) :
    wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxSize(920, 720),
        wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
    m_nextBorderId(1),
    m_activeTab(0),
    m_borderItemPicker(nullptr),
    m_gridViewNotebook(nullptr),
    m_serverLookPicker(nullptr),
    m_zOrderChoice(nullptr),
    m_groundItemPicker(nullptr),
    m_newTilesetButton(nullptr) {
    
    CreateGUIControls();
    LoadExistingBorders();
    LoadExistingGroundBrushes();
    LoadTilesets();
    LoadDetectedZOrders();
    AutoLoadDemoBorder();
    
    SetMinSize(wxSize(820, 640));
    CenterOnParent();
}

BorderEditorDialog::~BorderEditorDialog() {
    // Nothing to destroy manually
}

void BorderEditorDialog::CreateGUIControls() {
    wxBoxSizer* topSizer = new wxBoxSizer(wxVERTICAL);
    
    // Common properties - more compact horizontal layout
    wxStaticBoxSizer* commonPropertiesSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Common Properties");
    
    wxBoxSizer* commonPropertiesHorizSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Name field
    wxBoxSizer* nameSizer = new wxBoxSizer(wxVERTICAL);
    nameSizer->Add(new wxStaticText(this, wxID_ANY, "Name:"), 0);
    m_nameCtrl = new wxTextCtrl(this, wxID_ANY);
    m_nameCtrl->SetToolTip("Descriptive name for the border/brush");
    nameSizer->Add(m_nameCtrl, 0, wxEXPAND | wxTOP, 2);
    commonPropertiesHorizSizer->Add(nameSizer, 1, wxEXPAND | wxRIGHT, 10);
    
    // ID field
    wxBoxSizer* idSizer = new wxBoxSizer(wxVERTICAL);
    idSizer->Add(new wxStaticText(this, wxID_ANY, "ID:"), 0);
    m_idCtrl = new wxSpinCtrl(this, wxID_ANY, "1", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 1000);
    m_idCtrl->SetToolTip("Unique identifier for this border/brush");
    idSizer->Add(m_idCtrl, 0, wxEXPAND | wxTOP, 2);
    commonPropertiesHorizSizer->Add(idSizer, 0, wxEXPAND);
    
    commonPropertiesSizer->Add(commonPropertiesHorizSizer, 0, wxEXPAND | wxALL, 5);
    topSizer->Add(commonPropertiesSizer, 0, wxEXPAND | wxALL, 5);
    
    // Create notebook with Border and Ground tabs
    m_notebook = new wxNotebook(this, wxID_ANY);
    
    // ========== BORDER TAB ==========
    m_borderPanel = new wxPanel(m_notebook);
    wxBoxSizer* borderSizer = new wxBoxSizer(wxVERTICAL);
    
    // Border Properties - more compact layout
    wxStaticBoxSizer* borderPropertiesSizer = new wxStaticBoxSizer(wxVERTICAL, m_borderPanel, "Border Properties");
    
    // Two-column horizontal layout
    wxBoxSizer* borderPropsHorizSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left column - Group and Type
    wxBoxSizer* leftColSizer = new wxBoxSizer(wxVERTICAL);
    
    // Border Group
    wxBoxSizer* groupSizer = new wxBoxSizer(wxVERTICAL);
    groupSizer->Add(new wxStaticText(m_borderPanel, wxID_ANY, "Group:"), 0);
    m_groupCtrl = new wxSpinCtrl(m_borderPanel, wxID_ANY, "0", wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 1000);
    m_groupCtrl->SetToolTip("Optional group identifier (0 = no group)");
    groupSizer->Add(m_groupCtrl, 0, wxEXPAND | wxTOP, 2);
    leftColSizer->Add(groupSizer, 0, wxEXPAND | wxBOTTOM, 5);
    
    // Border Type
    wxBoxSizer* typeSizer = new wxBoxSizer(wxVERTICAL);
    typeSizer->Add(new wxStaticText(m_borderPanel, wxID_ANY, "Type:"), 0);
    wxBoxSizer* checkboxSizer = new wxBoxSizer(wxHORIZONTAL);
    m_isOptionalCheck = new wxCheckBox(m_borderPanel, wxID_ANY, "Optional");
    m_isOptionalCheck->SetToolTip("Marks this border as optional");
    m_isGroundCheck = new wxCheckBox(m_borderPanel, wxID_ANY, "Ground");
    m_isGroundCheck->SetToolTip("Marks this border as a ground border");
    checkboxSizer->Add(m_isOptionalCheck, 0, wxRIGHT, 10);
    checkboxSizer->Add(m_isGroundCheck, 0);
    typeSizer->Add(checkboxSizer, 0, wxEXPAND | wxTOP, 2);
    leftColSizer->Add(typeSizer, 0, wxEXPAND);
    
    borderPropsHorizSizer->Add(leftColSizer, 1, wxEXPAND | wxRIGHT, 10);
    
    // Right column - Load Existing
    wxBoxSizer* rightColSizer = new wxBoxSizer(wxVERTICAL);
    rightColSizer->Add(new wxStaticText(m_borderPanel, wxID_ANY, "Load Existing:"), 0);
    m_existingBordersCombo = new wxComboBox(m_borderPanel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY | wxCB_DROPDOWN);
    m_existingBordersCombo->SetToolTip("Load an existing border as template");
    rightColSizer->Add(m_existingBordersCombo, 0, wxEXPAND | wxTOP, 2);
    
    borderPropsHorizSizer->Add(rightColSizer, 1, wxEXPAND);
    
    borderPropertiesSizer->Add(borderPropsHorizSizer, 0, wxEXPAND | wxALL, 5);
    borderSizer->Add(borderPropertiesSizer, 0, wxEXPAND | wxALL, 5);
    
    // Border content area with grid and preview
    wxBoxSizer* borderContentSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left side - Grid Editor with view mode tabs
    wxStaticBoxSizer* gridSizer = new wxStaticBoxSizer(wxVERTICAL, m_borderPanel, "Border Grid");
    m_gridViewNotebook = new wxNotebook(m_borderPanel, ID_GRID_VIEW_NOTEBOOK);
    m_gridViewNotebook->AddPage(new wxPanel(m_gridViewNotebook), "9-Tile Edges");
    m_gridViewNotebook->AddPage(new wxPanel(m_gridViewNotebook), "Map Preview");
    m_gridViewNotebook->AddPage(new wxPanel(m_gridViewNotebook), "Outer");
    m_gridViewNotebook->AddPage(new wxPanel(m_gridViewNotebook), "Inner");
    gridSizer->Add(m_gridViewNotebook, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 5);

    m_gridPanel = new BorderGridPanel(m_borderPanel);
    m_gridPanel->SetMinSize(wxSize(320, 320));
    gridSizer->Add(m_gridPanel, 1, wxEXPAND | wxALL, 5);
    
    wxStaticText* instructions = new wxStaticText(m_borderPanel, wxID_ANY, 
        "Click a grid cell to assign the current brush/item. Corner cells hold two edge types (corner + diagonal).");
    instructions->SetForegroundColour(*wxBLUE);
    gridSizer->Add(instructions, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    // Current selected item controls with sprite preview
    wxBoxSizer* itemSizer = new wxBoxSizer(wxHORIZONTAL);
    m_borderItemPicker = new BorderItemPicker(m_borderPanel, ID_BORDER_ITEM_PICKER);
    m_borderItemPicker->SetToolTip("Click to pick from the current palette brush (RAW items work best)");
    itemSizer->Add(m_borderItemPicker, 0, wxRIGHT, 5);
    itemSizer->Add(new wxStaticText(m_borderPanel, wxID_ANY, "Item ID:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    m_itemIdCtrl = new wxSpinCtrl(m_borderPanel, wxID_ANY, "0", wxDefaultPosition, wxSize(90, -1), wxSP_ARROW_KEYS, 0, 65535);
    m_itemIdCtrl->SetToolTip("Enter an item ID manually or click the sprite to use the current brush");
    itemSizer->Add(m_itemIdCtrl, 0, wxRIGHT, 5);
    wxButton* browseButton = new wxButton(m_borderPanel, wxID_FIND, "Browse...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    browseButton->SetToolTip("Browse for an item");
    itemSizer->Add(browseButton, 0, wxRIGHT, 5);
    wxButton* addButton = new wxButton(m_borderPanel, wxID_ADD, "Apply", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    addButton->SetToolTip("Apply the item ID to the selected grid cell");
    itemSizer->Add(addButton, 0);
    gridSizer->Add(itemSizer, 0, wxEXPAND | wxALL, 5);
    
    m_borderItemPicker->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(BorderEditorDialog::OnBorderItemPickerClick), nullptr, this);
    
    borderContentSizer->Add(gridSizer, 3, wxEXPAND | wxALL, 5);
    
    // Right side - Preview Panel
    wxStaticBoxSizer* previewSizer = new wxStaticBoxSizer(wxVERTICAL, m_borderPanel, "Preview");
    m_previewPanel = new BorderPreviewPanel(m_borderPanel);
    previewSizer->Add(m_previewPanel, 1, wxEXPAND | wxALL, 5);
    borderContentSizer->Add(previewSizer, 2, wxEXPAND | wxALL, 5);
    
    // Add content sizer to main border sizer
    borderSizer->Add(borderContentSizer, 1, wxEXPAND | wxALL, 5);
    
    // Bottom buttons for border tab
    wxBoxSizer* borderButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    borderButtonSizer->Add(new wxButton(m_borderPanel, wxID_CLEAR, "Clear"), 0, wxRIGHT, 5);
    borderButtonSizer->Add(new wxButton(m_borderPanel, wxID_SAVE, "Save Border"), 0, wxRIGHT, 5);
    borderButtonSizer->Add(new wxButton(m_borderPanel, ID_TEST_BRUSH, "Test on Map"), 0, wxRIGHT, 5);
    borderButtonSizer->AddStretchSpacer(1);
    borderButtonSizer->Add(new wxButton(m_borderPanel, wxID_CLOSE, "Close"), 0);
    
    borderSizer->Add(borderButtonSizer, 0, wxEXPAND | wxALL, 5);
    
    m_borderPanel->SetSizer(borderSizer);
    
    // ========== GROUND TAB ==========
    m_groundPanel = new wxPanel(m_notebook);
    wxBoxSizer* groundSizer = new wxBoxSizer(wxVERTICAL);
    
    // Ground Brush Properties - more compact layout
    wxStaticBoxSizer* groundPropertiesSizer = new wxStaticBoxSizer(wxVERTICAL, m_groundPanel, "Ground Brush Properties");
    
    // Two rows of two columns each
    wxBoxSizer* topRowSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Tileset selector
    wxBoxSizer* tilesetSizer = new wxBoxSizer(wxVERTICAL);
    tilesetSizer->Add(new wxStaticText(m_groundPanel, wxID_ANY, "Tileset:"), 0);
    wxBoxSizer* tilesetRow = new wxBoxSizer(wxHORIZONTAL);
    m_tilesetChoice = new wxChoice(m_groundPanel, wxID_ANY);
    m_tilesetChoice->SetToolTip("Select tileset to add this brush to");
    tilesetRow->Add(m_tilesetChoice, 1, wxEXPAND | wxRIGHT, 5);
    m_newTilesetButton = new wxButton(m_groundPanel, ID_CREATE_TILESET, "New...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    m_newTilesetButton->SetToolTip("Create a new tileset and append it to tilesets.xml");
    tilesetRow->Add(m_newTilesetButton, 0);
    tilesetSizer->Add(tilesetRow, 0, wxEXPAND | wxTOP, 2);
    topRowSizer->Add(tilesetSizer, 1, wxEXPAND | wxRIGHT, 10);
    
    // Server Look ID with sprite preview
    wxBoxSizer* serverIdSizer = new wxBoxSizer(wxVERTICAL);
    serverIdSizer->Add(new wxStaticText(m_groundPanel, wxID_ANY, "Server Look ID:"), 0);
    wxBoxSizer* serverRow = new wxBoxSizer(wxHORIZONTAL);
    m_serverLookPicker = new BorderItemPicker(m_groundPanel, ID_SERVER_LOOK_PICKER);
    m_serverLookPicker->SetToolTip("Click to pick from the current palette brush");
    serverRow->Add(m_serverLookPicker, 0, wxRIGHT, 5);
    m_serverLookIdCtrl = new wxSpinCtrl(m_groundPanel, wxID_ANY, "0", wxDefaultPosition, wxSize(90, -1), wxSP_ARROW_KEYS, 0, 65535);
    m_serverLookIdCtrl->SetToolTip("Server-side item ID used as the brush look");
    serverRow->Add(m_serverLookIdCtrl, 0, wxEXPAND);
    serverIdSizer->Add(serverRow, 0, wxEXPAND | wxTOP, 2);
    topRowSizer->Add(serverIdSizer, 1, wxEXPAND);
    
    m_serverLookPicker->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(BorderEditorDialog::OnServerLookPickerClick), nullptr, this);
    
    groundPropertiesSizer->Add(topRowSizer, 0, wxEXPAND | wxALL, 5);
    
    // Second row
    wxBoxSizer* bottomRowSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Z-Order from detected values or custom
    wxBoxSizer* zOrderSizer = new wxBoxSizer(wxVERTICAL);
    zOrderSizer->Add(new wxStaticText(m_groundPanel, wxID_ANY, "Z-Order:"), 0);
    wxBoxSizer* zOrderRow = new wxBoxSizer(wxHORIZONTAL);
    m_zOrderChoice = new wxChoice(m_groundPanel, ID_ZORDER_CHOICE);
    m_zOrderChoice->SetToolTip("Pick a z-order used by existing ground brushes or choose Custom");
    zOrderRow->Add(m_zOrderChoice, 1, wxEXPAND | wxRIGHT, 5);
    m_zOrderCtrl = new wxSpinCtrl(m_groundPanel, wxID_ANY, "0", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 0, 10000);
    m_zOrderCtrl->SetToolTip("Custom z-order value");
    zOrderRow->Add(m_zOrderCtrl, 0);
    zOrderSizer->Add(zOrderRow, 0, wxEXPAND | wxTOP, 2);
    bottomRowSizer->Add(zOrderSizer, 1, wxEXPAND | wxRIGHT, 10);
    
    // Existing ground brushes dropdown
    wxBoxSizer* existingSizer = new wxBoxSizer(wxVERTICAL);
    existingSizer->Add(new wxStaticText(m_groundPanel, wxID_ANY, "Load Existing:"), 0);
    m_existingGroundBrushesCombo = new wxComboBox(m_groundPanel, wxID_ANY + 100, "", wxDefaultPosition, wxDefaultSize, 0, nullptr, wxCB_READONLY | wxCB_DROPDOWN);
    m_existingGroundBrushesCombo->SetToolTip("Load an existing ground brush as template");
    existingSizer->Add(m_existingGroundBrushesCombo, 0, wxEXPAND | wxTOP, 2);
    bottomRowSizer->Add(existingSizer, 1, wxEXPAND);
    
    groundPropertiesSizer->Add(bottomRowSizer, 0, wxEXPAND | wxALL, 5);
    
    groundSizer->Add(groundPropertiesSizer, 0, wxEXPAND | wxALL, 5);
    
    // Ground Items
    wxStaticBoxSizer* groundItemsSizer = new wxStaticBoxSizer(wxVERTICAL, m_groundPanel, "Ground Items");
    
    // List of ground items - set a smaller height
    m_groundItemsList = new wxListBox(m_groundPanel, ID_GROUND_ITEM_LIST, wxDefaultPosition, wxSize(-1, 100), 0, nullptr, wxLB_SINGLE);
    groundItemsSizer->Add(m_groundItemsList, 0, wxEXPAND | wxALL, 5);
    
    // Controls for adding/removing ground items
    wxBoxSizer* groundItemRowSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Left side - item ID and chance
    wxBoxSizer* itemDetailsSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Item ID input
    wxBoxSizer* itemIdSizer = new wxBoxSizer(wxVERTICAL);
    itemIdSizer->Add(new wxStaticText(m_groundPanel, wxID_ANY, "Item ID:"), 0);
    m_groundItemIdCtrl = new wxSpinCtrl(m_groundPanel, wxID_ANY, "0", wxDefaultPosition, wxSize(80, -1), wxSP_ARROW_KEYS, 0, 65535);
    m_groundItemIdCtrl->SetToolTip("ID of the item to add");
    itemIdSizer->Add(m_groundItemIdCtrl, 0, wxEXPAND | wxTOP, 2);
    itemDetailsSizer->Add(itemIdSizer, 0, wxEXPAND | wxRIGHT, 5);
    
    // Chance input
    wxBoxSizer* chanceSizer = new wxBoxSizer(wxVERTICAL);
    chanceSizer->Add(new wxStaticText(m_groundPanel, wxID_ANY, "Chance:"), 0);
    m_groundItemChanceCtrl = new wxSpinCtrl(m_groundPanel, wxID_ANY, "10", wxDefaultPosition, wxSize(60, -1), wxSP_ARROW_KEYS, 1, 10000);
    m_groundItemChanceCtrl->SetToolTip("Chance of this item appearing");
    chanceSizer->Add(m_groundItemChanceCtrl, 0, wxEXPAND | wxTOP, 2);
    itemDetailsSizer->Add(chanceSizer, 0, wxEXPAND);
    
    groundItemRowSizer->Add(itemDetailsSizer, 1, wxEXPAND | wxRIGHT, 10);
    
    // Right side - buttons
    wxBoxSizer* itemButtonsSizer = new wxBoxSizer(wxVERTICAL);
    itemButtonsSizer->AddStretchSpacer();
    
    wxBoxSizer* buttonsSizer = new wxBoxSizer(wxHORIZONTAL);
    wxButton* groundBrowseButton = new wxButton(m_groundPanel, wxID_FIND + 100, "Browse...", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    groundBrowseButton->SetToolTip("Browse for an item");
    buttonsSizer->Add(groundBrowseButton, 0, wxRIGHT, 5);
    
    wxButton* addGroundItemButton = new wxButton(m_groundPanel, wxID_ADD + 100, "Add", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    addGroundItemButton->SetToolTip("Add this item to the list");
    buttonsSizer->Add(addGroundItemButton, 0, wxRIGHT, 5);
    
    wxButton* removeGroundItemButton = new wxButton(m_groundPanel, wxID_REMOVE, "Remove", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    removeGroundItemButton->SetToolTip("Remove the selected item");
    buttonsSizer->Add(removeGroundItemButton, 0);
    
    itemButtonsSizer->Add(buttonsSizer, 0, wxEXPAND);
    groundItemRowSizer->Add(itemButtonsSizer, 0, wxEXPAND);
    
    groundItemsSizer->Add(groundItemRowSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    groundSizer->Add(groundItemsSizer, 0, wxEXPAND | wxALL, 5); // Changed from 1 to 0 to not expand
    
    // Grid and border selection for ground tab
    wxStaticBoxSizer* groundBorderSizer = new wxStaticBoxSizer(wxVERTICAL, m_groundPanel, "Border for Ground Brush");
    
    // First row - Border alignment and 'to none' option
    wxBoxSizer* borderRow1 = new wxBoxSizer(wxHORIZONTAL);
    
    // Border alignment
    wxBoxSizer* alignSizer = new wxBoxSizer(wxVERTICAL);
    alignSizer->Add(new wxStaticText(m_groundPanel, wxID_ANY, "Border Alignment:"), 0);
    wxArrayString alignOptions;
    alignOptions.Add("outer");
    alignOptions.Add("inner");
    m_borderAlignmentChoice = new wxChoice(m_groundPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize, alignOptions);
    m_borderAlignmentChoice->SetSelection(0); // Default to "outer"
    m_borderAlignmentChoice->SetToolTip("Alignment type for the border");
    alignSizer->Add(m_borderAlignmentChoice, 0, wxEXPAND | wxTOP, 2);
    borderRow1->Add(alignSizer, 1, wxEXPAND | wxRIGHT, 10);
    
    // Border options (checkboxes)
    wxBoxSizer* optionsSizer = new wxBoxSizer(wxVERTICAL);
    optionsSizer->Add(new wxStaticText(m_groundPanel, wxID_ANY, "Border Options:"), 0);
    wxBoxSizer* checksSizer = new wxBoxSizer(wxHORIZONTAL);
    m_includeToNoneCheck = new wxCheckBox(m_groundPanel, wxID_ANY, "To None");
    m_includeToNoneCheck->SetValue(true); // Default to checked
    m_includeToNoneCheck->SetToolTip("Adds additional border with 'to none' attribute");
    m_includeInnerCheck = new wxCheckBox(m_groundPanel, wxID_ANY, "Inner Border");
    m_includeInnerCheck->SetToolTip("Adds additional inner border with same ID");
    checksSizer->Add(m_includeToNoneCheck, 0, wxRIGHT, 10);
    checksSizer->Add(m_includeInnerCheck, 0);
    optionsSizer->Add(checksSizer, 0, wxEXPAND | wxTOP, 2);
    borderRow1->Add(optionsSizer, 1, wxEXPAND);
    
    groundBorderSizer->Add(borderRow1, 0, wxEXPAND | wxALL, 5);
    
    // Border ID notice (red text)
    wxBoxSizer* borderIdSizer = new wxBoxSizer(wxHORIZONTAL);
    wxStaticText* borderIdLabel = new wxStaticText(m_groundPanel, wxID_ANY, "Border ID:");
    borderIdSizer->Add(borderIdLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    wxStaticText* borderId = new wxStaticText(m_groundPanel, wxID_ANY, "Uses the ID specified in 'Common Properties' section");
    borderId->SetForegroundColour(*wxRED);
    borderIdSizer->Add(borderId, 1, wxALIGN_CENTER_VERTICAL);
    
    groundBorderSizer->Add(borderIdSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    // Grid use instruction - shorter text
    wxStaticText* gridInstructions = new wxStaticText(m_groundPanel, wxID_ANY, 
        "Use the grid in the Border tab to define borders for this ground brush.");
    gridInstructions->SetForegroundColour(*wxBLUE);
    groundBorderSizer->Add(gridInstructions, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
    
    groundSizer->Add(groundBorderSizer, 0, wxEXPAND | wxALL, 5);
    
    // Bottom buttons for ground tab
    wxBoxSizer* groundButtonSizer = new wxBoxSizer(wxHORIZONTAL);
    groundButtonSizer->Add(new wxButton(m_groundPanel, wxID_CLEAR, "Clear"), 0, wxRIGHT, 5);
    groundButtonSizer->Add(new wxButton(m_groundPanel, wxID_SAVE, "Save Ground"), 0, wxRIGHT, 5);
    groundButtonSizer->Add(new wxButton(m_groundPanel, ID_TEST_BRUSH, "Test on Map"), 0, wxRIGHT, 5);
    groundButtonSizer->AddStretchSpacer(1);
    groundButtonSizer->Add(new wxButton(m_groundPanel, wxID_CLOSE, "Close"), 0);
    
    groundSizer->Add(groundButtonSizer, 0, wxEXPAND | wxALL, 5);
    
    m_groundPanel->SetSizer(groundSizer);
    
    // Add tabs to notebook
    m_notebook->AddPage(m_borderPanel, "Border");
    m_notebook->AddPage(m_groundPanel, "Ground");
    
    topSizer->Add(m_notebook, 1, wxEXPAND | wxALL, 5);
    
    SetSizer(topSizer);
    Layout();
}

wxString BorderEditorDialog::GetVersionDataDirectory() const {
    pugi::xml_document clientsDoc;
    wxString clientsPath = g_gui.GetDataDirectory() + wxFileName::GetPathSeparator() + "clients.xml";
    if (clientsDoc.load_file(nstr(clientsPath).c_str())) {
        wxString versionString = g_gui.GetCurrentVersion().getName();
        for (pugi::xml_node clientNode = clientsDoc.child("client_config").child("clients").child("client");
             clientNode; clientNode = clientNode.next_sibling("client")) {
            if (versionString == wxString(clientNode.attribute("name").as_string())) {
                return wxString(clientNode.attribute("data_directory").as_string());
            }
        }
    }

    wxString versionString = g_gui.GetCurrentVersion().getName();
    std::string versionStr = std::string(versionString.mb_str());
    versionStr.erase(std::remove(versionStr.begin(), versionStr.end(), '.'), versionStr.end());
    if (versionStr.length() == 2) {
        versionStr += "0";
    } else if (versionStr == "1010") {
        versionStr = "10100";
    }
    return wxString(versionStr.c_str());
}

wxString BorderEditorDialog::GetVersionFilePath(const wxString& filename) const {
    wxString dataDir = GetVersionDataDirectory();
    if (dataDir.IsEmpty()) {
        return wxString();
    }
    return g_gui.GetDataDirectory() + wxFileName::GetPathSeparator() + dataDir + wxFileName::GetPathSeparator() + filename;
}

uint16_t BorderEditorDialog::GetItemIdFromCurrentBrush() const {
    return GetItemIDFromBrush(g_gui.GetCurrentBrush());
}

void BorderEditorDialog::ApplyItemIdFromBrush(uint16_t itemId, BorderItemPicker* picker, wxSpinCtrl* spinCtrl) {
    if (itemId == 0) {
        return;
    }
    if (picker) {
        picker->SetItemId(itemId);
    }
    if (spinCtrl) {
        spinCtrl->SetValue(itemId);
    }
}

void BorderEditorDialog::AutoLoadDemoBorder() {
    m_idCtrl->SetValue(m_nextBorderId);

    for (int i = 0; i < m_existingBordersCombo->GetCount(); ++i) {
        wxStringClientData* data = static_cast<wxStringClientData*>(m_existingBordersCombo->GetClientObject(i));
        if (data && wxAtoi(data->GetData()) == 2) {
            m_existingBordersCombo->SetSelection(i);
            LoadBorderById(2);
            return;
        }
    }
}

void BorderEditorDialog::OnGridViewChanged(wxBookCtrlEvent& event) {
    if (!m_gridPanel) {
        return;
    }

    switch (event.GetSelection()) {
        case 0: m_gridPanel->SetViewMode(GRID_VIEW_EDGES); break;
        case 1: m_gridPanel->SetViewMode(GRID_VIEW_MAP); break;
        case 2: m_gridPanel->SetViewMode(GRID_VIEW_OUTER); break;
        case 3: m_gridPanel->SetViewMode(GRID_VIEW_INNER); break;
        default: break;
    }
}

void BorderEditorDialog::OnBorderItemPickerClick(wxMouseEvent& WXUNUSED(event)) {
    ApplyItemIdFromBrush(GetItemIdFromCurrentBrush(), m_borderItemPicker, m_itemIdCtrl);
}

void BorderEditorDialog::OnServerLookPickerClick(wxMouseEvent& WXUNUSED(event)) {
    ApplyItemIdFromBrush(GetItemIdFromCurrentBrush(), m_serverLookPicker, m_serverLookIdCtrl);
}

void BorderEditorDialog::OnGroundItemPickerClick(wxMouseEvent& WXUNUSED(event)) {
    ApplyItemIdFromBrush(GetItemIdFromCurrentBrush(), m_groundItemPicker, m_groundItemIdCtrl);
}

void BorderEditorDialog::OnZOrderChoice(wxCommandEvent& event) {
    int selection = m_zOrderChoice->GetSelection();
    if (selection == wxNOT_FOUND) {
        return;
    }

    wxString label = m_zOrderChoice->GetString(selection);
    if (label == "Custom...") {
        m_zOrderCtrl->Enable(true);
        return;
    }

    long value = 0;
    if (label.BeforeFirst(' ').ToLong(&value)) {
        m_zOrderCtrl->SetValue(static_cast<int>(value));
        m_zOrderCtrl->Enable(false);
        return;
    }
    m_zOrderCtrl->Enable(true);
}

void BorderEditorDialog::LoadDetectedZOrders() {
    m_zOrderChoice->Clear();
    std::set<int> zOrders;

    wxString groundsFile = GetVersionFilePath("grounds.xml");
    if (!groundsFile.IsEmpty() && wxFileExists(groundsFile)) {
        pugi::xml_document doc;
        if (doc.load_file(nstr(groundsFile).c_str())) {
            pugi::xml_node materials = doc.child("materials");
            for (pugi::xml_node brushNode = materials.child("brush"); brushNode; brushNode = brushNode.next_sibling("brush")) {
                pugi::xml_attribute typeAttr = brushNode.attribute("type");
                if (!typeAttr || std::string(typeAttr.as_string()) != "ground") {
                    continue;
                }
                pugi::xml_attribute zOrderAttr = brushNode.attribute("z-order");
                if (zOrderAttr) {
                    zOrders.insert(zOrderAttr.as_int());
                }
            }
        }
    }

    for (int z : zOrders) {
        m_zOrderChoice->Append(wxString::Format("%d", z));
    }
    m_zOrderChoice->Append("Custom...");
    if (m_zOrderChoice->GetCount() > 1) {
        m_zOrderChoice->SetSelection(0);
        OnZOrderChoice(wxCommandEvent(wxEVT_CHOICE, ID_ZORDER_CHOICE));
    } else if (m_zOrderChoice->GetCount() == 1) {
        m_zOrderChoice->SetSelection(0);
        m_zOrderCtrl->Enable(true);
    }
}

void BorderEditorDialog::OnCreateTileset(wxCommandEvent& WXUNUSED(event)) {
    wxTextEntryDialog dlg(this, "Enter a name for the new tileset:", "Create Tileset");
    if (dlg.ShowModal() != wxID_OK) {
        return;
    }

    wxString name = dlg.GetValue().Trim();
    if (name.IsEmpty()) {
        wxMessageBox("Tileset name cannot be empty.", "Error", wxICON_ERROR);
        return;
    }

    if (m_tilesetChoice->FindString(name) != wxNOT_FOUND) {
        wxMessageBox("A tileset with that name already exists.", "Error", wxICON_ERROR);
        return;
    }

    wxString tilesetsFile = GetVersionFilePath("tilesets.xml");
    if (tilesetsFile.IsEmpty() || !wxFileExists(tilesetsFile)) {
        wxMessageBox("Cannot find tilesets.xml in the data directory.", "Error", wxICON_ERROR);
        return;
    }

    pugi::xml_document doc;
    if (!doc.load_file(nstr(tilesetsFile).c_str())) {
        wxMessageBox("Failed to load tilesets.xml.", "Error", wxICON_ERROR);
        return;
    }

    pugi::xml_node materials = doc.child("materials");
    if (!materials) {
        wxMessageBox("Invalid tilesets.xml: missing materials node.", "Error", wxICON_ERROR);
        return;
    }

    pugi::xml_node tilesetNode = materials.append_child("tileset");
    tilesetNode.append_attribute("name").set_value(nstr(name).c_str());
    tilesetNode.append_child("terrain");

    if (!doc.save_file(nstr(tilesetsFile).c_str())) {
        wxMessageBox("Failed to save tilesets.xml.", "Error", wxICON_ERROR);
        return;
    }

    AddTilesetToMemory(name);
    LoadTilesets();
    int idx = m_tilesetChoice->FindString(name);
    if (idx != wxNOT_FOUND) {
        m_tilesetChoice->SetSelection(idx);
    }
}

void BorderEditorDialog::AddTilesetToMemory(const wxString& name) {
    const std::string tilesetName = nstr(name);
    if (g_materials.tilesets.find(tilesetName) == g_materials.tilesets.end()) {
        Tileset* tileset = newd Tileset(g_brushes, tilesetName);
        g_materials.tilesets[tilesetName] = tileset;
        tileset->getCategory(TILESET_TERRAIN);
    }
}

void BorderEditorDialog::ReloadBorderInMemory(int borderId) {
    g_brushes.removeBorder(borderId);

    wxString bordersFile = GetVersionFilePath("borders.xml");
    if (bordersFile.IsEmpty() || !wxFileExists(bordersFile)) {
        return;
    }

    pugi::xml_document doc;
    if (!doc.load_file(nstr(bordersFile).c_str())) {
        return;
    }

    pugi::xml_node materials = doc.child("materials");
    for (pugi::xml_node borderNode = materials.child("border"); borderNode; borderNode = borderNode.next_sibling("border")) {
        pugi::xml_attribute idAttr = borderNode.attribute("id");
        if (idAttr && idAttr.as_int() == borderId) {
            wxArrayString warnings;
            g_brushes.unserializeBorder(borderNode, warnings);
            break;
        }
    }
}

void BorderEditorDialog::OnTestBrush(wxCommandEvent& WXUNUSED(event)) {
    wxString brushName = m_nameCtrl->GetValue().Trim();
    if (brushName.IsEmpty()) {
        wxMessageBox("Enter a brush/border name before testing.", "Test Brush", wxICON_INFORMATION);
        return;
    }

    if (m_activeTab == 0) {
        if (!ValidateBorder()) {
            return;
        }
        SaveBorder();
    } else {
        if (!ValidateGroundBrush()) {
            return;
        }
        SaveGroundBrush();
    }

    wxString error;
    wxArrayString warnings;
    if (!g_gui.LoadVersion(g_gui.GetCurrentVersionID(), error, warnings, true)) {
        wxMessageBox("Failed to reload editor data: " + error, "Test Brush", wxICON_ERROR);
        return;
    }

    Brush* brush = g_brushes.getBrush(nstr(brushName));
    if (!brush && m_activeTab == 0) {
        wxMessageBox("Border saved. Create a ground brush on the Ground tab to paint with it on the map.", "Test Brush", wxICON_INFORMATION);
        return;
    }

    if (brush) {
        g_gui.SelectBrush(brush);
        g_gui.SetStatusText("Testing brush: " + brushName);
        wxMessageBox("Brush selected on the map. Paint to verify borders, then return here to adjust.", "Test Brush", wxICON_INFORMATION);
    } else {
        wxMessageBox("Brush was saved but could not be selected. Try reloading the client data manually.", "Test Brush", wxICON_WARNING);
    }
}

void BorderEditorDialog::LoadExistingBorders() {
    m_existingBordersCombo->Clear();
    m_existingBordersCombo->Append("<Create New>");
    m_existingBordersCombo->SetSelection(0);

    wxString bordersFile = GetVersionFilePath("borders.xml");
    if (bordersFile.IsEmpty() || !wxFileExists(bordersFile)) {
        wxMessageBox("Cannot find borders.xml file in the data directory.", "Error", wxICON_ERROR);
        return;
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(nstr(bordersFile).c_str());
    if (!result) {
        wxMessageBox("Failed to load borders.xml: " + wxString(result.description()), "Error", wxICON_ERROR);
        return;
    }

    pugi::xml_node materials = doc.child("materials");
    if (!materials) {
        wxMessageBox("Invalid borders.xml file: missing 'materials' node", "Error", wxICON_ERROR);
        return;
    }

    int highestId = 0;

    for (pugi::xml_node borderNode = materials.child("border"); borderNode; borderNode = borderNode.next_sibling("border")) {
        pugi::xml_attribute idAttr = borderNode.attribute("id");
        if (!idAttr) {
            continue;
        }

        int id = idAttr.as_int();
        if (id > highestId) {
            highestId = id;
        }

        std::string description;
        pugi::xml_node commentNode = borderNode.previous_sibling();
        if (commentNode && commentNode.type() == pugi::node_comment) {
            description = commentNode.value();
            description.erase(0, description.find_first_not_of(" \t\n\r"));
            description.erase(description.find_last_not_of(" \t\n\r") + 1);
            if (description.substr(0, 4) == "<!--") {
                description.erase(0, 4);
                description.erase(0, description.find_first_not_of(" \t\n\r"));
            }
            if (description.length() >= 3 && description.substr(description.length() - 3) == "-->") {
                description.erase(description.length() - 3);
                description.erase(description.find_last_not_of(" \t\n\r") + 1);
            }
        }

        wxString label = wxString::Format("Border %d", id);
        if (!description.empty()) {
            label += wxString::Format(" (%s)", wxstr(description));
        }

        m_existingBordersCombo->Append(label, new wxStringClientData(wxString::Format("%d", id)));
    }

    m_nextBorderId = highestId + 1;
}

void BorderEditorDialog::LoadBorderById(int borderId) {
    wxString bordersFile = GetVersionFilePath("borders.xml");
    if (bordersFile.IsEmpty() || !wxFileExists(bordersFile)) {
        return;
    }

    pugi::xml_document doc;
    if (!doc.load_file(nstr(bordersFile).c_str())) {
        return;
    }

    ClearItems();

    pugi::xml_node materials = doc.child("materials");
    for (pugi::xml_node borderNode = materials.child("border"); borderNode; borderNode = borderNode.next_sibling("border")) {
        pugi::xml_attribute idAttr = borderNode.attribute("id");
        if (!idAttr || idAttr.as_int() != borderId) {
            continue;
        }

        m_idCtrl->SetValue(borderId);

        pugi::xml_attribute typeAttr = borderNode.attribute("type");
        m_isOptionalCheck->SetValue(typeAttr && std::string(typeAttr.as_string()) == "optional");

        pugi::xml_attribute groupAttr = borderNode.attribute("group");
        m_groupCtrl->SetValue(groupAttr ? groupAttr.as_int() : 0);

        pugi::xml_attribute groundAttr = borderNode.attribute("ground");
        m_isGroundCheck->SetValue(groundAttr && groundAttr.as_bool());

        pugi::xml_node commentNode = borderNode.previous_sibling();
        if (commentNode && commentNode.type() == pugi::node_comment) {
            std::string description = commentNode.value();
            description.erase(0, description.find_first_not_of(" \t\n\r"));
            description.erase(description.find_last_not_of(" \t\n\r") + 1);
            if (description.substr(0, 4) == "<!--") {
                description.erase(0, 4);
                description.erase(0, description.find_first_not_of(" \t\n\r"));
            }
            if (description.length() >= 3 && description.substr(description.length() - 3) == "-->") {
                description.erase(description.length() - 3);
                description.erase(description.find_last_not_of(" \t\n\r") + 1);
            }
            m_nameCtrl->SetValue(wxstr(description));
        } else {
            m_nameCtrl->SetValue(wxString::Format("Border %d", borderId));
        }

        for (pugi::xml_node itemNode = borderNode.child("borderitem"); itemNode; itemNode = itemNode.next_sibling("borderitem")) {
            pugi::xml_attribute edgeAttr = itemNode.attribute("edge");
            pugi::xml_attribute itemAttr = itemNode.attribute("item");
            if (!edgeAttr || !itemAttr) {
                continue;
            }

            BorderEdgePosition pos = edgeStringToPosition(edgeAttr.as_string());
            uint16_t itemId = itemAttr.as_uint();
            if (pos != EDGE_NONE && itemId > 0) {
                m_borderItems.push_back(BorderItem(pos, itemId));
                m_gridPanel->SetItemId(pos, itemId);
            }
        }
        break;
    }

    UpdatePreview();
}

void BorderEditorDialog::OnLoadBorder(wxCommandEvent& event) {
    int selection = m_existingBordersCombo->GetSelection();
    if (selection <= 0) {
        ClearItems();
        m_idCtrl->SetValue(m_nextBorderId);
        return;
    }

    wxStringClientData* data = static_cast<wxStringClientData*>(m_existingBordersCombo->GetClientObject(selection));
    if (!data) {
        return;
    }

    LoadBorderById(wxAtoi(data->GetData()));
    m_existingBordersCombo->SetSelection(selection);
}

void BorderEditorDialog::OnItemIdChanged(wxCommandEvent& event) {
    // This event handler would update the display when an item ID is entered manually
    // but we're handling this directly in OnAddItem instead
}

void BorderEditorDialog::OnBrowse(wxCommandEvent& event) {
    FindItemDialog dialog(this, "Select Border Item");
    if (dialog.ShowModal() == wxID_OK) {
        uint16_t itemId = dialog.getResultID();
        if (itemId > 0) {
            ApplyItemIdFromBrush(itemId, m_borderItemPicker, m_itemIdCtrl);
        }
    }
}

void BorderEditorDialog::OnPositionSelected(wxCommandEvent& event) {
    // Get the position from the event
    BorderEdgePosition pos = static_cast<BorderEdgePosition>(event.GetInt());
    wxLogDebug("OnPositionSelected: Position %s selected", wxstr(edgePositionToString(pos)).c_str());
    OutputDebugStringA(wxString::Format("BorderEditor: Position %s selected\n", wxstr(edgePositionToString(pos)).c_str()).mb_str());
    
    // Get the item ID from the current brush
    Brush* currentBrush = g_gui.GetCurrentBrush();
    if (!currentBrush) {
        wxLogDebug("OnPositionSelected: No current brush selected");
        OutputDebugStringA("BorderEditor: No current brush selected\n");
        wxMessageBox("Please select a brush or item first.", "No Brush Selected", wxICON_INFORMATION);
        return;
    }
    
    wxLogDebug("OnPositionSelected: Using brush: %s", wxString(currentBrush->getName()).c_str());
    OutputDebugStringA(wxString::Format("BorderEditor: Using brush: %s\n", wxString(currentBrush->getName()).c_str()).mb_str());
    
    // Try to get the item ID directly - check if it's a RAW brush first
    uint16_t itemId = 0;
    if (currentBrush->isRaw()) {
        RAWBrush* rawBrush = currentBrush->asRaw();
        if (rawBrush) {
            itemId = rawBrush->getItemID();
            wxLogDebug("OnPositionSelected: Got item ID %d directly from RAW brush", itemId);
            OutputDebugStringA(wxString::Format("BorderEditor: Got item ID %d directly from RAW brush\n", itemId).mb_str());
        } else {
            wxLogDebug("OnPositionSelected: Failed to cast to RAW brush");
            OutputDebugStringA("BorderEditor: Failed to cast to RAW brush\n");
        }
    } else {
        OutputDebugStringA(wxString::Format("BorderEditor: Current brush is NOT a RAW brush, is: %s\n", 
            currentBrush->isGround() ? "Ground" : 
            currentBrush->isWall() ? "Wall" : 
            currentBrush->isDoodad() ? "Doodad" : "Other").mb_str());
    }
    
    // If we didn't get an ID from the RAW brush method, try the generic method
    if (itemId == 0) {
        itemId = GetItemIDFromBrush(currentBrush);
        wxLogDebug("OnPositionSelected: Got item ID %d from GetItemIDFromBrush", itemId);
        OutputDebugStringA(wxString::Format("BorderEditor: Got item ID %d from GetItemIDFromBrush\n", itemId).mb_str());
    }
    
    if (itemId > 0) {
        // Update the item ID control - keeps the UI in sync with our selection
        if (m_itemIdCtrl) {
            m_itemIdCtrl->SetValue(itemId);
        }
        if (m_borderItemPicker) {
            m_borderItemPicker->SetItemId(itemId);
        }
        
        // Add or update the border item
        bool updated = false;
        for (size_t i = 0; i < m_borderItems.size(); i++) {
            if (m_borderItems[i].position == pos) {
                m_borderItems[i].itemId = itemId;
                updated = true;
                wxLogDebug("OnPositionSelected: Updated existing border item at position %s", wxstr(edgePositionToString(pos)).c_str());
                OutputDebugStringA(wxString::Format("BorderEditor: Updated existing border item at position %s\n", wxstr(edgePositionToString(pos)).c_str()).mb_str());
                break;
            }
        }
        
        if (!updated) {
            m_borderItems.push_back(BorderItem(pos, itemId));
            wxLogDebug("OnPositionSelected: Added new border item at position %s", wxstr(edgePositionToString(pos)).c_str());
            OutputDebugStringA(wxString::Format("BorderEditor: Added new border item at position %s\n", wxstr(edgePositionToString(pos)).c_str()).mb_str());
        }
        
        // Update the grid panel
        m_gridPanel->SetItemId(pos, itemId);
        wxLogDebug("OnPositionSelected: Set grid panel item ID for position %s to %d", wxstr(edgePositionToString(pos)).c_str(), itemId);
        OutputDebugStringA(wxString::Format("BorderEditor: Set grid panel item ID for position %s to %d\n", wxstr(edgePositionToString(pos)).c_str(), itemId).mb_str());
        
        // Update the preview
        UpdatePreview();
        
        // Log the addition
        wxLogDebug("Added border item at position %s with item ID %d", 
                  wxstr(edgePositionToString(pos)).c_str(), itemId);
        OutputDebugStringA(wxString::Format("BorderEditor: Successfully added border item at position %s with item ID %d\n", 
                  wxstr(edgePositionToString(pos)).c_str(), itemId).mb_str());
    } else {
        // If we couldn't get an item ID from the brush, check if there's a value in the item ID control
        itemId = m_itemIdCtrl->GetValue();
        
        if (itemId > 0) {
            // Use the value from the control to update/add the border item
            bool updated = false;
            for (size_t i = 0; i < m_borderItems.size(); i++) {
                if (m_borderItems[i].position == pos) {
                    m_borderItems[i].itemId = itemId;
                    updated = true;
                    break;
                }
            }
            
            if (!updated) {
                m_borderItems.push_back(BorderItem(pos, itemId));
            }
            
            // Update the grid panel
            m_gridPanel->SetItemId(pos, itemId);
            
            // Update the preview
            UpdatePreview();
            
            wxLogDebug("Used item ID %d from control for position %s", 
                       itemId, wxstr(edgePositionToString(pos)).c_str());
            OutputDebugStringA(wxString::Format("BorderEditor: Used item ID %d from control for position %s\n", 
                       itemId, wxstr(edgePositionToString(pos)).c_str()).mb_str());
        } else {
            wxLogDebug("No valid item ID found from current brush: %s", wxString(currentBrush->getName()).c_str());
            OutputDebugStringA(wxString::Format("BorderEditor: No valid item ID found from current brush: %s\n", wxString(currentBrush->getName()).c_str()).mb_str());
            wxMessageBox("Could not get a valid item ID from the current brush. Please select an item brush or use the Browse button to select an item manually.", "Invalid Brush", wxICON_INFORMATION);
        }
    }
}

void BorderEditorDialog::OnAddItem(wxCommandEvent& event) {
    // Get the currently selected position in the grid panel
    static BorderEdgePosition lastSelectedPos = EDGE_NONE;
    BorderEdgePosition selectedPos = m_gridPanel->GetSelectedPosition();
    
    // If no position is currently selected, use the last selected position
    if (selectedPos == EDGE_NONE) {
        selectedPos = lastSelectedPos;
    }
    
    if (selectedPos == EDGE_NONE) {
        wxMessageBox("Please select a position on the grid first by clicking on it.", "Error", wxICON_ERROR);
        return;
    }
    
    // Save this position for future use
    lastSelectedPos = selectedPos;
    
    // Get the item ID from the control (now using the class member)
    uint16_t itemId = m_itemIdCtrl->GetValue();
    
    if (itemId == 0) {
        wxMessageBox("Please enter a valid item ID or use the Browse button.", "Error", wxICON_ERROR);
        return;
    }
    
    // Add or update the border item
    bool updated = false;
    for (size_t i = 0; i < m_borderItems.size(); i++) {
        if (m_borderItems[i].position == selectedPos) {
            m_borderItems[i].itemId = itemId;
            updated = true;
            break;
        }
    }
    
    if (!updated) {
        m_borderItems.push_back(BorderItem(selectedPos, itemId));
    }
    
    // Update the grid panel
    m_gridPanel->SetItemId(selectedPos, itemId);
    
    // Update the preview
    UpdatePreview();
    
    // Log the addition for debugging
    wxLogDebug("Added item ID %d at position %s via Add button", 
               itemId, wxstr(edgePositionToString(selectedPos)).c_str());
}

void BorderEditorDialog::OnClear(wxCommandEvent& event) {
    if (m_activeTab == 0) {
        // Border tab
    ClearItems();
    } else {
        // Ground tab
        ClearGroundItems();
    }
}

void BorderEditorDialog::ClearItems() {
    m_borderItems.clear();
    m_gridPanel->Clear();
    m_previewPanel->Clear();
    
    // Reset controls to defaults
    m_idCtrl->SetValue(m_nextBorderId);
    m_nameCtrl->SetValue("");
    m_isOptionalCheck->SetValue(false);
    m_isGroundCheck->SetValue(false);
    m_groupCtrl->SetValue(0);
    
    // Set combo selection to "Create New"
    m_existingBordersCombo->SetSelection(0);
}

void BorderEditorDialog::UpdatePreview() {
    if (!m_groundItems.empty()) {
        m_gridPanel->SetCenterGroundItemId(m_groundItems.front().itemId);
    } else if (!m_borderItems.empty()) {
        m_gridPanel->SetCenterGroundItemId(0);
    }
    m_previewPanel->SetBorderItems(m_borderItems);
    m_previewPanel->Refresh();
    m_gridPanel->Refresh();
}

bool BorderEditorDialog::ValidateBorder() {
    // Check for empty name
    if (m_nameCtrl->GetValue().IsEmpty()) {
        wxMessageBox("Please enter a name for the border.", "Validation Error", wxICON_ERROR);
        return false;
    }
    
    if (m_borderItems.empty()) {
        wxMessageBox("The border must have at least one item.", "Validation Error", wxICON_ERROR);
        return false;
    }
    
    // Check that there are no duplicate positions
    std::set<BorderEdgePosition> positions;
    for (const BorderItem& item : m_borderItems) {
        if (positions.find(item.position) != positions.end()) {
            wxMessageBox("The border contains duplicate positions.", "Validation Error", wxICON_ERROR);
            return false;
        }
        positions.insert(item.position);
    }
    
    // Check for ID validity
    int id = m_idCtrl->GetValue();
    if (id <= 0) {
        wxMessageBox("Border ID must be greater than 0.", "Validation Error", wxICON_ERROR);
        return false;
    }
    
    return true;
}

void BorderEditorDialog::SaveBorder() {
    if (!ValidateBorder()) {
        return;
    }
    
    // Get the border properties
    int id = m_idCtrl->GetValue();
    wxString name = m_nameCtrl->GetValue();
    
    // Double check that we have a name (it's also checked in ValidateBorder)
    if (name.IsEmpty()) {
        wxMessageBox("You must provide a name for the border.", "Error", wxICON_ERROR);
        return;
    }
    
    bool isOptional = m_isOptionalCheck->GetValue();
    bool isGround = m_isGroundCheck->GetValue();
    int group = m_groupCtrl->GetValue();
    
    wxString bordersFile = GetVersionFilePath("borders.xml");
    
    if (bordersFile.IsEmpty() || !wxFileExists(bordersFile)) {
        wxMessageBox("Cannot find borders.xml file in the data directory.", "Error", wxICON_ERROR);
        return;
    }
    
    // Load the XML file
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(nstr(bordersFile).c_str());
    
    if (!result) {
        wxMessageBox("Failed to load borders.xml: " + wxString(result.description()), "Error", wxICON_ERROR);
        return;
    }
    
    pugi::xml_node materials = doc.child("materials");
    if (!materials) {
        wxMessageBox("Invalid borders.xml file: missing 'materials' node", "Error", wxICON_ERROR);
        return;
    }
    
    // Check if a border with this ID already exists
    bool borderExists = false;
    pugi::xml_node existingBorder;
    
    for (pugi::xml_node borderNode = materials.child("border"); borderNode; borderNode = borderNode.next_sibling("border")) {
        pugi::xml_attribute idAttr = borderNode.attribute("id");
        if (idAttr && idAttr.as_int() == id) {
            borderExists = true;
            existingBorder = borderNode;
            break;
        }
    }
    
    if (borderExists) {
        // Check if there's a comment node before the existing border
        pugi::xml_node commentNode = existingBorder.previous_sibling();
        bool hadComment = (commentNode && commentNode.type() == pugi::node_comment);
        
        // Ask for confirmation to overwrite
        if (wxMessageBox("A border with ID " + wxString::Format("%d", id) + " already exists. Do you want to overwrite it?", 
                        "Confirm Overwrite", wxYES_NO | wxICON_QUESTION) != wxYES) {
            return;
        }
        
        // If there was a comment node, remove it too
        if (hadComment) {
            materials.remove_child(commentNode);
        }
        
        // Remove the existing border
        materials.remove_child(existingBorder);
    }
    
 
    
    // Create the new border node
    pugi::xml_node borderNode = materials.append_child("border");
    borderNode.append_attribute("id").set_value(id);
    
    if (isOptional) {
        borderNode.append_attribute("type").set_value("optional");
    }
    
    if (isGround) {
        borderNode.append_attribute("ground").set_value("true");
    }
    
    if (group > 0) {
        borderNode.append_attribute("group").set_value(group);
    }
    
    // Add all border items
    for (const BorderItem& item : m_borderItems) {
        pugi::xml_node itemNode = borderNode.append_child("borderitem");
        itemNode.append_attribute("edge").set_value(edgePositionToString(item.position).c_str());
        itemNode.append_attribute("item").set_value(item.itemId);
    }
    
    // Save the file
    if (!doc.save_file(nstr(bordersFile).c_str())) {
        wxMessageBox("Failed to save changes to borders.xml", "Error", wxICON_ERROR);
        return;
    }
    
    wxMessageBox("Border saved successfully.", "Success", wxICON_INFORMATION);
    
    ReloadBorderInMemory(id);
    LoadExistingBorders();
}

void BorderEditorDialog::OnSave(wxCommandEvent& event) {
    if (m_activeTab == 0) {
        // Border tab
    SaveBorder();
    } else {
        // Ground tab
        SaveGroundBrush();
    }
}

void BorderEditorDialog::OnClose(wxCommandEvent& event) {
    Close();
}

void BorderEditorDialog::OnGridCellClicked(wxMouseEvent& event) {
    // This event is handled by the BorderGridPanel directly
    event.Skip();
}

// ============================================================================
// BorderItemButton

BorderItemButton::BorderItemButton(wxWindow* parent, BorderEdgePosition position, wxWindowID id) :
    wxButton(parent, id, "", wxDefaultPosition, wxSize(32, 32)),
    m_itemId(0),
    m_position(position) {
    // Set up the button to show sprite graphics
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

BorderItemButton::~BorderItemButton() {
    // No need to destroy anything manually
}

void BorderItemButton::SetItemId(uint16_t id) {
    m_itemId = id;
    Refresh();
}

void BorderItemButton::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    
    // Draw the button background
    wxRect rect = GetClientRect();
    dc.SetBrush(wxBrush(GetBackgroundColour()));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle(rect);
    
    // Draw the item sprite if available
    if (m_itemId > 0) {
        const ItemType& type = g_items.getItemType(m_itemId);
        if (type.id != 0) {
            Sprite* sprite = g_gui.gfx.getSprite(type.clientID);
            if (sprite) {
                sprite->DrawTo(&dc, SPRITE_SIZE_32x32, 0, 0, rect.GetWidth(), rect.GetHeight());
            }
        }
    }
    
    // Draw a border around the button if it's focused
    if (HasFocus()) {
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRectangle(rect);
    }
}

// ============================================================================
// BorderItemPicker

BorderItemPicker::BorderItemPicker(wxWindow* parent, wxWindowID id) :
    DCButton(parent, id, wxDefaultPosition, DC_BTN_TOGGLE, RENDER_SIZE_32x32, 0),
    m_itemId(0) {
    SetToolTip("Left-click to use the current palette brush");
}

void BorderItemPicker::SetItemId(uint16_t id) {
    if (m_itemId == id) {
        return;
    }
    m_itemId = id;
    if (m_itemId != 0) {
        const ItemType& it = g_items.getItemType(m_itemId);
        if (it.id != 0) {
            SetSprite(it.clientID);
            return;
        }
    }
    SetSprite(0);
}

// ============================================================================
// BorderGridPanel

BorderGridPanel::BorderGridPanel(wxWindow* parent, wxWindowID id) :
    wxPanel(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN),
    m_selectedPosition(EDGE_NONE),
    m_viewMode(GRID_VIEW_EDGES),
    m_centerGroundItemId(0) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

BorderGridPanel::~BorderGridPanel() {
}

void BorderGridPanel::SetViewMode(BorderGridViewMode mode) {
    m_viewMode = mode;
    Refresh();
}

void BorderGridPanel::SetCenterGroundItemId(uint16_t itemId) {
    m_centerGroundItemId = itemId;
    Refresh();
}

void BorderGridPanel::SetItemId(BorderEdgePosition pos, uint16_t itemId) {
    if (pos >= 0 && pos < EDGE_COUNT) {
        m_items[pos] = itemId;
        Refresh();
    }
}

uint16_t BorderGridPanel::GetItemId(BorderEdgePosition pos) const {
    auto it = m_items.find(pos);
    if (it != m_items.end()) {
        return it->second;
    }
    return 0;
}

void BorderGridPanel::Clear() {
    m_items.clear();
    Refresh();
}

void BorderGridPanel::SetSelectedPosition(BorderEdgePosition pos) {
    m_selectedPosition = pos;
    Refresh();
}

void BorderGridPanel::GetEdgeCellRect(int col, int row, const wxRect& panelRect, wxRect& out) const {
    const int cell = std::min(panelRect.width, panelRect.height) / 3;
    const int offsetX = panelRect.x + (panelRect.width - cell * 3) / 2;
    const int offsetY = panelRect.y + (panelRect.height - cell * 3) / 2;
    out = wxRect(offsetX + col * cell, offsetY + row * cell, cell, cell);
}

void BorderGridPanel::DrawSpriteForItem(wxDC& dc, uint16_t itemId, int x, int y, int w, int h) {
    if (itemId == 0) {
        return;
    }
    const ItemType& type = g_items.getItemType(itemId);
    if (type.id == 0) {
        return;
    }
    Sprite* sprite = g_gui.gfx.getSprite(type.clientID);
    if (sprite) {
        sprite->DrawTo(&dc, SPRITE_SIZE_32x32, x, y, w, h);
    }
}

void BorderGridPanel::DrawEdgeLayout(wxDC& dc, const wxRect& rect) {
    dc.SetPen(wxPen(wxColour(90, 90, 90)));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            wxRect cellRect;
            GetEdgeCellRect(col, row, rect, cellRect);
            dc.DrawRectangle(cellRect);

            const int cellIndex = row * 3 + col;
            const BorderGridCell& cell = s_edgeCells[cellIndex];

            if (cell.primary == EDGE_NONE && cell.secondary == EDGE_NONE) {
                dc.SetBrush(wxBrush(wxColour(150, 200, 130)));
                dc.SetPen(*wxTRANSPARENT_PEN);
                dc.DrawRectangle(cellRect.Deflate(4));
                dc.SetPen(wxPen(wxColour(90, 90, 90)));
                dc.SetBrush(*wxTRANSPARENT_BRUSH);
                DrawSpriteForItem(dc, m_centerGroundItemId, cellRect.x + 8, cellRect.y + 8, cellRect.width - 16, cellRect.height - 16);
                dc.DrawText("ground", cellRect.x + 6, cellRect.y + 4);
                continue;
            }

            auto drawEdge = [&](BorderEdgePosition pos, int insetX, int insetY, int drawW, int drawH) {
                if (pos == EDGE_NONE) {
                    return;
                }
                if (pos == m_selectedPosition) {
                    dc.SetPen(*wxRED_PEN);
                    dc.SetBrush(wxBrush(wxColour(255, 220, 220)));
                    dc.DrawRectangle(cellRect);
                    dc.SetPen(wxPen(wxColour(90, 90, 90)));
                    dc.SetBrush(*wxTRANSPARENT_BRUSH);
                }
                wxString label = wxstr(edgePositionToString(pos));
                dc.DrawText(label, cellRect.x + 4, cellRect.y + 4);
                DrawSpriteForItem(dc, GetItemId(pos), cellRect.x + insetX, cellRect.y + insetY, drawW, drawH);
            };

            if (cell.secondary != EDGE_NONE) {
                const int halfW = cellRect.width / 2 - 4;
                const int halfH = cellRect.height / 2 - 4;
                drawEdge(cell.primary, 4, cellRect.height / 2, halfW, halfH);
                if (cell.secondary == m_selectedPosition) {
                    dc.SetPen(*wxRED_PEN);
                    dc.SetBrush(wxBrush(wxColour(255, 220, 220)));
                    dc.DrawRectangle(cellRect);
                    dc.SetPen(wxPen(wxColour(90, 90, 90)));
                    dc.SetBrush(*wxTRANSPARENT_BRUSH);
                }
                wxString label2 = wxstr(edgePositionToString(cell.secondary));
                dc.DrawText(label2, cellRect.x + cellRect.width / 2 + 2, cellRect.y + 4);
                DrawSpriteForItem(dc, GetItemId(cell.secondary), cellRect.x + cellRect.width / 2, cellRect.y + 4, halfW, halfH);
            } else {
                drawEdge(cell.primary, 8, 16, cellRect.width - 16, cellRect.height - 24);
            }
        }
    }
}

void BorderGridPanel::DrawMapPreview(wxDC& dc, const wxRect& rect, bool innerStyle) {
    const int gridSize = innerStyle ? 3 : 5;
    const int cell = std::min(rect.width, rect.height) / gridSize;
    const int offsetX = rect.x + (rect.width - cell * gridSize) / 2;
    const int offsetY = rect.y + (rect.height - cell * gridSize) / 2;

    dc.SetPen(wxPen(wxColour(180, 180, 180)));
    for (int i = 0; i <= gridSize; ++i) {
        dc.DrawLine(offsetX + i * cell, offsetY, offsetX + i * cell, offsetY + gridSize * cell);
        dc.DrawLine(offsetX, offsetY + i * cell, offsetX + gridSize * cell, offsetY + i * cell);
    }

    const int center = gridSize / 2;
    const int centerX = offsetX + center * cell;
    const int centerY = offsetY + center * cell;

    if (m_centerGroundItemId > 0) {
        DrawSpriteForItem(dc, m_centerGroundItemId, centerX, centerY, cell, cell);
    } else {
        dc.SetBrush(wxBrush(innerStyle ? wxColour(100, 140, 180) : wxColour(120, 180, 100)));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(centerX, centerY, cell, cell);
    }

    auto drawBorderAt = [&](BorderEdgePosition pos, int dx, int dy) {
        uint16_t itemId = GetItemId(pos);
        if (itemId == 0) {
            return;
        }
        DrawSpriteForItem(dc, itemId, centerX + dx * cell, centerY + dy * cell, cell, cell);
    };

    drawBorderAt(EDGE_N, 0, -1);
    drawBorderAt(EDGE_S, 0, 1);
    drawBorderAt(EDGE_E, 1, 0);
    drawBorderAt(EDGE_W, -1, 0);
    drawBorderAt(EDGE_CNW, -1, -1);
    drawBorderAt(EDGE_CNE, 1, -1);
    drawBorderAt(EDGE_CSW, -1, 1);
    drawBorderAt(EDGE_CSE, 1, 1);

    dc.SetTextForeground(innerStyle ? wxColour(40, 80, 140) : wxColour(40, 100, 40));
    dc.DrawText(innerStyle ? "Inner preview" : "Outer preview", rect.x + 8, rect.y + 8);
}

void BorderGridPanel::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    wxRect rect = GetClientRect();
    dc.SetBackground(wxBrush(wxColour(210, 210, 210)));
    dc.Clear();

    switch (m_viewMode) {
        case GRID_VIEW_MAP:
            DrawMapPreview(dc, rect, false);
            break;
        case GRID_VIEW_OUTER:
            DrawMapPreview(dc, rect, false);
            break;
        case GRID_VIEW_INNER:
            DrawMapPreview(dc, rect, true);
            break;
        case GRID_VIEW_EDGES:
        default:
            DrawEdgeLayout(dc, rect);
            break;
    }
}

BorderEdgePosition BorderGridPanel::HitTestEdgeCell(int x, int y, const wxRect& rect) const {
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            wxRect cellRect;
            GetEdgeCellRect(col, row, rect, cellRect);
            if (!cellRect.Contains(x, y)) {
                continue;
            }

            const BorderGridCell& cell = s_edgeCells[row * 3 + col];
            if (cell.secondary != EDGE_NONE) {
                wxRect rightHalf(cellRect.x + cellRect.width / 2, cellRect.y, cellRect.width / 2, cellRect.height);
                return rightHalf.Contains(x, y) ? cell.secondary : cell.primary;
            }
            if (cell.primary != EDGE_NONE) {
                return cell.primary;
            }
            return EDGE_NONE;
        }
    }
    return EDGE_NONE;
}

BorderEdgePosition BorderGridPanel::GetPositionFromCoordinates(int x, int y) const {
    if (m_viewMode != GRID_VIEW_EDGES) {
        return EDGE_NONE;
    }
    return HitTestEdgeCell(x, y, GetClientRect());
}

void BorderGridPanel::OnMouseClick(wxMouseEvent& event) {
    int x = event.GetX();
    int y = event.GetY();
    
    BorderEdgePosition pos = GetPositionFromCoordinates(x, y);
    if (pos != EDGE_NONE) {
        // Set the position as selected in the grid
        SetSelectedPosition(pos);
        
        // Notify the parent dialog that a position was selected
        wxCommandEvent selEvent(wxEVT_COMMAND_BUTTON_CLICKED, ID_BORDER_GRID_SELECT);
        selEvent.SetInt(static_cast<int>(pos));
        
        // Find the parent BorderEditorDialog
        wxWindow* parent = GetParent();
        while (parent && !dynamic_cast<BorderEditorDialog*>(parent)) {
            parent = parent->GetParent();
        }
        
        // Send the event to the parent dialog
        BorderEditorDialog* dialog = dynamic_cast<BorderEditorDialog*>(parent);
        if (dialog) {
            // Call the event handler directly
            OutputDebugStringA(wxString::Format("BorderGridPanel::OnMouseClick: Calling OnPositionSelected directly for position %s\n", 
                            wxstr(edgePositionToString(pos)).c_str()).mb_str());
            dialog->OnPositionSelected(selEvent);
        } else {
            // If we couldn't find the parent dialog, post the event to the parent
            OutputDebugStringA("BorderGridPanel::OnMouseClick: Could not find BorderEditorDialog parent, posting event\n");
            wxPostEvent(GetParent(), selEvent);
        }
    }
}

void BorderGridPanel::OnMouseDown(wxMouseEvent& event) {
    // Get the position from the coordinates
    BorderEdgePosition pos = GetPositionFromCoordinates(event.GetX(), event.GetY());
    
    // Set the selected position
    SetSelectedPosition(pos);
    
    event.Skip();
}

// ============================================================================
// BorderPreviewPanel

BorderPreviewPanel::BorderPreviewPanel(wxWindow* parent, wxWindowID id) :
    wxPanel(parent, id, wxDefaultPosition, wxSize(BORDER_PREVIEW_SIZE, BORDER_PREVIEW_SIZE)) {
    // Set up the panel to handle painting
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

BorderPreviewPanel::~BorderPreviewPanel() {
    // Nothing to destroy manually
}

void BorderPreviewPanel::SetBorderItems(const std::vector<BorderItem>& items) {
    m_borderItems = items;
    Refresh();
}

void BorderPreviewPanel::Clear() {
    m_borderItems.clear();
    Refresh();
}

void BorderPreviewPanel::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    
    // Draw the panel background
    wxRect rect = GetClientRect();
    dc.SetBackground(wxBrush(wxColour(240, 240, 240)));
    dc.Clear();
    
    // Draw a grid to simulate a map
    const int GRID_SIZE = 5;
    const int preview_cell_size = BORDER_PREVIEW_SIZE / GRID_SIZE;
    
    // Draw grid lines
    dc.SetPen(wxPen(wxColour(200, 200, 200)));
    for (int i = 0; i <= GRID_SIZE; i++) {
        // Vertical lines
        dc.DrawLine(i * preview_cell_size, 0, i * preview_cell_size, BORDER_PREVIEW_SIZE);
        // Horizontal lines
        dc.DrawLine(0, i * preview_cell_size, BORDER_PREVIEW_SIZE, i * preview_cell_size);
    }
    
    // Draw a sample ground tile in the center
    dc.SetBrush(wxBrush(wxColour(120, 180, 100)));
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.DrawRectangle((GRID_SIZE / 2) * preview_cell_size, (GRID_SIZE / 2) * preview_cell_size, preview_cell_size, preview_cell_size);
    
    // Draw border items around the center
    for (const BorderItem& item : m_borderItems) {
        wxPoint offset(0, 0);
        
        // Calculate position based on the edge type
        switch (item.position) {
            case EDGE_N:   offset = wxPoint(0, -1); break;
            case EDGE_E:   offset = wxPoint(1, 0); break;
            case EDGE_S:   offset = wxPoint(0, 1); break;
            case EDGE_W:   offset = wxPoint(-1, 0); break;
            case EDGE_CNW: offset = wxPoint(-1, -1); break;
            case EDGE_CNE: offset = wxPoint(1, -1); break;
            case EDGE_CSE: offset = wxPoint(1, 1); break;
            case EDGE_CSW: offset = wxPoint(-1, 1); break;
            case EDGE_DNW: offset = wxPoint(-1, -1); break; // Diagonal positions use same offsets as corners
            case EDGE_DNE: offset = wxPoint(1, -1); break;
            case EDGE_DSE: offset = wxPoint(1, 1); break;
            case EDGE_DSW: offset = wxPoint(-1, 1); break;
            default: continue;
        }
        
        // Calculate the position on the grid
        int x = (GRID_SIZE / 2 + offset.x) * preview_cell_size;
        int y = (GRID_SIZE / 2 + offset.y) * preview_cell_size;
        
        // Draw the item sprite
        const ItemType& type = g_items.getItemType(item.itemId);
        if (type.id != 0) {
            Sprite* sprite = g_gui.gfx.getSprite(type.clientID);
            if (sprite) {
                sprite->DrawTo(&dc, SPRITE_SIZE_32x32, x, y, preview_cell_size, preview_cell_size);
            }
        }
    }
}

void BorderEditorDialog::LoadExistingGroundBrushes() {
    // Clear the combo box
    m_existingGroundBrushesCombo->Clear();
    
    // Add "Create New" as the first option
    m_existingGroundBrushesCombo->Append("<Create New>");
    m_existingGroundBrushesCombo->SetSelection(0);
    
    // Find the grounds.xml file based on the current version
    wxString dataDir = g_gui.GetDataDirectory();
    
    // Get version string and convert to proper directory format
    wxString versionString = g_gui.GetCurrentVersion().getName();
    std::string versionStr = std::string(versionString.mb_str());
    
    // Convert version number to data directory format
    // Remove dots first
    versionStr.erase(std::remove(versionStr.begin(), versionStr.end(), '.'), versionStr.end());
    
    // Handle special cases for 2-digit versions (add 0)
    if(versionStr.length() == 2) {
        versionStr += "0";
    }
    // Handle special case for 10.10 -> 10100
    else if(versionStr == "1010") {
        versionStr = "10100";
    }
    
    wxString groundsFile = dataDir + wxFileName::GetPathSeparator() + 
                          wxString(versionStr.c_str()) + 
                          wxFileName::GetPathSeparator() + "grounds.xml";
    
    if (!wxFileExists(groundsFile)) {
        wxMessageBox("Cannot find grounds.xml file in the data directory.", "Error", wxICON_ERROR);
        return;
    }
    
    // Load the XML file
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(nstr(groundsFile).c_str());
    
    if (!result) {
        wxMessageBox("Failed to load grounds.xml: " + wxString(result.description()), "Error", wxICON_ERROR);
        return;
    }
    
    // Look for all brush nodes
    pugi::xml_node materials = doc.child("materials");
    if (!materials) {
        wxMessageBox("Invalid grounds.xml file: missing 'materials' node", "Error", wxICON_ERROR);
        return;
    }
    
    for (pugi::xml_node brushNode = materials.child("brush"); brushNode; brushNode = brushNode.next_sibling("brush")) {
        pugi::xml_attribute nameAttr = brushNode.attribute("name");
        pugi::xml_attribute serverLookIdAttr = brushNode.attribute("server_lookid");
        pugi::xml_attribute typeAttr = brushNode.attribute("type");
        
        // Only include ground brushes
        if (!typeAttr || std::string(typeAttr.as_string()) != "ground") {
            continue;
        }
        
        if (nameAttr && serverLookIdAttr) {
            wxString brushName = wxString(nameAttr.as_string());
            int serverId = serverLookIdAttr.as_int();
            
            // Add the brush name to the combo box with the serverId as client data
            wxStringClientData* data = new wxStringClientData(wxString::Format("%d", serverId));
            m_existingGroundBrushesCombo->Append(brushName, data);
        }
    }
}

void BorderEditorDialog::ClearGroundItems() {
    m_groundItems.clear();
    m_nameCtrl->SetValue("");
    m_idCtrl->SetValue(m_nextBorderId);
    m_serverLookIdCtrl->SetValue(0);
    if (m_serverLookPicker) {
        m_serverLookPicker->SetItemId(0);
    }
    m_zOrderCtrl->SetValue(0);
    if (m_zOrderChoice && m_zOrderChoice->GetCount() > 0) {
        m_zOrderChoice->SetSelection(0);
        OnZOrderChoice(wxCommandEvent(wxEVT_CHOICE, ID_ZORDER_CHOICE));
    }
    m_groundItemIdCtrl->SetValue(0);
    if (m_groundItemPicker) {
        m_groundItemPicker->SetItemId(0);
    }
    m_groundItemChanceCtrl->SetValue(10);
    
    // Reset border alignment options
    m_borderAlignmentChoice->SetSelection(0); // Default to "outer"
    m_includeToNoneCheck->SetValue(true);     // Default to checked
    m_includeInnerCheck->SetValue(false);     // Default to unchecked
    
    UpdateGroundItemsList();
}

void BorderEditorDialog::UpdateGroundItemsList() {
    m_groundItemsList->Clear();
    
    for (const GroundItem& item : m_groundItems) {
        wxString itemText = wxString::Format("Item ID: %d, Chance: %d", item.itemId, item.chance);
        m_groundItemsList->Append(itemText);
    }
}

void BorderEditorDialog::OnPageChanged(wxBookCtrlEvent& event) {
    m_activeTab = event.GetSelection();
    
    // When switching to the ground tab, use the same border items for the ground brush
    if (m_activeTab == 1) {
        // Update the ground items preview (not implemented yet)
    }
}

void BorderEditorDialog::OnAddGroundItem(wxCommandEvent& event) {
    uint16_t itemId = m_groundItemIdCtrl->GetValue();
    int chance = m_groundItemChanceCtrl->GetValue();
    
    if (itemId > 0) {
        // Check if this item already exists, if so update its chance
        bool updated = false;
        for (size_t i = 0; i < m_groundItems.size(); i++) {
            if (m_groundItems[i].itemId == itemId) {
                m_groundItems[i].chance = chance;
                updated = true;
                break;
            }
        }
        
        if (!updated) {
            // Add new item
            m_groundItems.push_back(GroundItem(itemId, chance));
        }
        
        // Update the list
        UpdateGroundItemsList();
        UpdatePreview();
    }
}

void BorderEditorDialog::OnRemoveGroundItem(wxCommandEvent& event) {
    int selection = m_groundItemsList->GetSelection();
    if (selection != wxNOT_FOUND && selection < static_cast<int>(m_groundItems.size())) {
        m_groundItems.erase(m_groundItems.begin() + selection);
        UpdateGroundItemsList();
    }
}

void BorderEditorDialog::OnGroundBrowse(wxCommandEvent& event) {
    FindItemDialog dialog(this, "Select Ground Item");
    if (dialog.ShowModal() == wxID_OK) {
        uint16_t itemId = dialog.getResultID();
        if (itemId > 0) {
            ApplyItemIdFromBrush(itemId, m_groundItemPicker, m_groundItemIdCtrl);
        }
    }
}

void BorderEditorDialog::OnLoadGroundBrush(wxCommandEvent& event) {
    int selection = m_existingGroundBrushesCombo->GetSelection();
    if (selection <= 0) {
        // Selected "Create New" or nothing
        ClearGroundItems();
        return;
    }
    
    wxStringClientData* data = static_cast<wxStringClientData*>(m_existingGroundBrushesCombo->GetClientObject(selection));
    if (!data) return;
    
    int serverLookId = wxAtoi(data->GetData());
    
    // Find the grounds.xml file using the same version path conversion
    wxString dataDir = g_gui.GetDataDirectory();
    
    // Get version string and convert to proper directory format
    wxString versionString = g_gui.GetCurrentVersion().getName();
    std::string versionStr = std::string(versionString.mb_str());
    
    // Convert version number to data directory format
    // Remove dots first
    versionStr.erase(std::remove(versionStr.begin(), versionStr.end(), '.'), versionStr.end());
    
    // Handle special cases for 2-digit versions (add 0)
    if(versionStr.length() == 2) {
        versionStr += "0";
    }
    // Handle special case for 10.10 -> 10100
    else if(versionStr == "1010") {
        versionStr = "10100";
    }
    
    wxString groundsFile = dataDir + wxFileName::GetPathSeparator() + 
                          wxString(versionStr.c_str()) + 
                          wxFileName::GetPathSeparator() + "grounds.xml";
    
    if (!wxFileExists(groundsFile)) {
        wxMessageBox("Cannot find grounds.xml file in the data directory.", "Error", wxICON_ERROR);
        return;
    }
    
    // Load the XML file
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(nstr(groundsFile).c_str());
    
    if (!result) {
        wxMessageBox("Failed to load grounds.xml: " + wxString(result.description()), "Error", wxICON_ERROR);
        return;
    }
    
    // Clear existing items
    ClearGroundItems();
    
    // Find the brush with the specified server_lookid
    pugi::xml_node materials = doc.child("materials");
    if (!materials) {
        wxMessageBox("Invalid grounds.xml file: missing 'materials' node", "Error", wxICON_ERROR);
        return;
    }
    
    for (pugi::xml_node brushNode = materials.child("brush"); brushNode; brushNode = brushNode.next_sibling("brush")) {
        pugi::xml_attribute serverLookIdAttr = brushNode.attribute("server_lookid");
        
        if (serverLookIdAttr && serverLookIdAttr.as_int() == serverLookId) {
            // Found the brush, load its properties
            pugi::xml_attribute nameAttr = brushNode.attribute("name");
            pugi::xml_attribute zOrderAttr = brushNode.attribute("z-order");
            
            if (nameAttr) {
                m_nameCtrl->SetValue(wxString(nameAttr.as_string()));
            }
            
            m_serverLookIdCtrl->SetValue(serverLookId);
            if (m_serverLookPicker) {
                m_serverLookPicker->SetItemId(static_cast<uint16_t>(serverLookId));
            }
            
            if (zOrderAttr) {
                int zOrder = zOrderAttr.as_int();
                m_zOrderCtrl->SetValue(zOrder);
                int choiceIdx = m_zOrderChoice->FindString(wxString::Format("%d", zOrder));
                if (choiceIdx != wxNOT_FOUND) {
                    m_zOrderChoice->SetSelection(choiceIdx);
                    m_zOrderCtrl->Enable(false);
                } else {
                    int customIdx = m_zOrderChoice->FindString("Custom...");
                    if (customIdx != wxNOT_FOUND) {
                        m_zOrderChoice->SetSelection(customIdx);
                    }
                    m_zOrderCtrl->Enable(true);
                }
            }
            
            // Load all item nodes
            for (pugi::xml_node itemNode = brushNode.child("item"); itemNode; itemNode = itemNode.next_sibling("item")) {
                pugi::xml_attribute idAttr = itemNode.attribute("id");
                pugi::xml_attribute chanceAttr = itemNode.attribute("chance");
                
                if (idAttr) {
                    uint16_t itemId = idAttr.as_uint();
                    int chance = chanceAttr ? chanceAttr.as_int() : 10;
                    
                    m_groundItems.push_back(GroundItem(itemId, chance));
                }
            }
            
            // Load all border nodes and add to the border grid
            m_borderItems.clear();
            m_gridPanel->Clear();
            
            // Reset alignment options
            m_borderAlignmentChoice->SetSelection(0); // Default to "outer"
            m_includeToNoneCheck->SetValue(true);     // Default to checked
            m_includeInnerCheck->SetValue(false);     // Default to unchecked
            
            bool hasNormalBorder = false;
            bool hasToNoneBorder = false;
            bool hasInnerBorder = false;
            bool hasInnerToNoneBorder = false;
            wxString alignment = "outer"; // Default
            
            for (pugi::xml_node borderNode = brushNode.child("border"); borderNode; borderNode = borderNode.next_sibling("border")) {
                pugi::xml_attribute alignAttr = borderNode.attribute("align");
                pugi::xml_attribute toAttr = borderNode.attribute("to");
                pugi::xml_attribute idAttr = borderNode.attribute("id");
                
                if (idAttr) {
                    int borderId = idAttr.as_int();
                    m_idCtrl->SetValue(borderId);
                    
                    // Check border type and attributes
                    if (alignAttr) {
                        wxString alignVal = wxString(alignAttr.as_string());
                        
                        if (alignVal == "outer") {
                            if (toAttr && wxString(toAttr.as_string()) == "none") {
                                hasToNoneBorder = true;
                            } else {
                                hasNormalBorder = true;
                                alignment = "outer";
                            }
                        } else if (alignVal == "inner") {
                            if (toAttr && wxString(toAttr.as_string()) == "none") {
                                hasInnerToNoneBorder = true;
                            } else {
                                hasInnerBorder = true;
                            }
                        }
                    }
                    
                    // Load the border details from borders.xml
                    wxString bordersFile = dataDir + wxFileName::GetPathSeparator() + 
                                         wxString(versionStr.c_str()) + 
                                         wxFileName::GetPathSeparator() + "borders.xml";
                    
                    if (!wxFileExists(bordersFile)) {
                        // Just skip if we can't find borders.xml
                        continue;
                    }
                    
                    pugi::xml_document bordersDoc;
                    pugi::xml_parse_result bordersResult = bordersDoc.load_file(nstr(bordersFile).c_str());
                    
                    if (!bordersResult) {
                        // Skip if we can't load borders.xml
                        continue;
                    }
                    
                    pugi::xml_node bordersMaterials = bordersDoc.child("materials");
                    if (!bordersMaterials) {
                        continue;
                    }
                    
                    for (pugi::xml_node targetBorder = bordersMaterials.child("border"); targetBorder; targetBorder = targetBorder.next_sibling("border")) {
                        pugi::xml_attribute targetIdAttr = targetBorder.attribute("id");
                        
                        if (targetIdAttr && targetIdAttr.as_int() == borderId) {
                            // Found the border, load its items
                            for (pugi::xml_node borderItemNode = targetBorder.child("borderitem"); borderItemNode; borderItemNode = borderItemNode.next_sibling("borderitem")) {
                                pugi::xml_attribute edgeAttr = borderItemNode.attribute("edge");
                                pugi::xml_attribute itemAttr = borderItemNode.attribute("item");
                                
                                if (!edgeAttr || !itemAttr) continue;
                                
                                BorderEdgePosition pos = edgeStringToPosition(edgeAttr.as_string());
                                uint16_t borderItemId = itemAttr.as_uint();
                                
                                if (pos != EDGE_NONE && borderItemId > 0) {
                                    m_borderItems.push_back(BorderItem(pos, borderItemId));
                                    m_gridPanel->SetItemId(pos, borderItemId);
                                }
                            }
                            
                            break;
                        }
                    }
                }
            }
            
            // Update the ground items list and border preview
            UpdateGroundItemsList();
            UpdatePreview();
            
            // Apply the loaded border alignment settings
            if (hasInnerBorder) {
                m_includeInnerCheck->SetValue(true);
            }
            
            if (!hasToNoneBorder) {
                m_includeToNoneCheck->SetValue(false);
            }
            
            int alignmentIndex = 0; // Default to outer
            if (alignment == "inner") {
                alignmentIndex = 1;
            }
            m_borderAlignmentChoice->SetSelection(alignmentIndex);
            
            break;
        }
    }
    
    // Keep selection
    m_existingGroundBrushesCombo->SetSelection(selection);
}

bool BorderEditorDialog::ValidateGroundBrush() {
    // Check for empty name
    if (m_nameCtrl->GetValue().IsEmpty()) {
        wxMessageBox("Please enter a name for the ground brush.", "Validation Error", wxICON_ERROR);
        return false;
    }
    
    if (m_groundItems.empty()) {
        wxMessageBox("The ground brush must have at least one item.", "Validation Error", wxICON_ERROR);
        return false;
    }
    
    if (m_serverLookIdCtrl->GetValue() <= 0) {
        wxMessageBox("You must specify a valid server look ID.", "Validation Error", wxICON_ERROR);
        return false;
    }
    
    // Check tileset selection
    if (m_tilesetChoice->GetSelection() == wxNOT_FOUND) {
        wxMessageBox("Please select a tileset for the ground brush.", "Validation Error", wxICON_ERROR);
        return false;
    }
    
    return true;
}

void BorderEditorDialog::SaveGroundBrush() {
    if (!ValidateGroundBrush()) {
        return;
    }
    
    // Get the ground brush properties
    wxString name = m_nameCtrl->GetValue();
    
    // Double check that we have a name (it's also checked in ValidateGroundBrush)
    if (name.IsEmpty()) {
        wxMessageBox("You must provide a name for the ground brush.", "Error", wxICON_ERROR);
        return;
    }
    
    int serverId = m_serverLookIdCtrl->GetValue();
    int zOrder = m_zOrderCtrl->GetValue();
    int borderId = m_idCtrl->GetValue();  // This should be taken from common properties
    
    // Get the selected tileset
    int tilesetSelection = m_tilesetChoice->GetSelection();
    if (tilesetSelection == wxNOT_FOUND) {
        wxMessageBox("Please select a tileset.", "Validation Error", wxICON_ERROR);
        return;
    }
    wxString tilesetName = m_tilesetChoice->GetString(tilesetSelection);
    
    // Find the grounds.xml file using the same version path conversion
    wxString dataDir = g_gui.GetDataDirectory();
    
    // Get version string and convert to proper directory format
    wxString versionString = g_gui.GetCurrentVersion().getName();
    std::string versionStr = std::string(versionString.mb_str());
    
    // Convert version number to data directory format
    // Remove dots first
    versionStr.erase(std::remove(versionStr.begin(), versionStr.end(), '.'), versionStr.end());
    
    // Handle special cases for 2-digit versions (add 0)
    if(versionStr.length() == 2) {
        versionStr += "0";
    }
    // Handle special case for 10.10 -> 10100
    else if(versionStr == "1010") {
        versionStr = "10100";
    }
    
    wxString groundsFile = dataDir + wxFileName::GetPathSeparator() + 
                          wxString(versionStr.c_str()) + 
                          wxFileName::GetPathSeparator() + "grounds.xml";
    
    if (!wxFileExists(groundsFile)) {
        wxMessageBox("Cannot find grounds.xml file in the data directory.", "Error", wxICON_ERROR);
        return;
    }
    
    // Make sure the border is saved first if we have border items
    if (!m_borderItems.empty()) {
        SaveBorder();
    }
    
    // Load the XML file
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(nstr(groundsFile).c_str());
    
    if (!result) {
        wxMessageBox("Failed to load grounds.xml: " + wxString(result.description()), "Error", wxICON_ERROR);
        return;
    }
    
    pugi::xml_node materials = doc.child("materials");
    if (!materials) {
        wxMessageBox("Invalid grounds.xml file: missing 'materials' node", "Error", wxICON_ERROR);
        return;
    }
    
    // Check if a brush with this name already exists
    bool brushExists = false;
    pugi::xml_node existingBrush;
    
    for (pugi::xml_node brushNode = materials.child("brush"); brushNode; brushNode = brushNode.next_sibling("brush")) {
        pugi::xml_attribute nameAttr = brushNode.attribute("name");
        
        if (nameAttr && wxString(nameAttr.as_string()) == name) {
            brushExists = true;
            existingBrush = brushNode;
            break;
        }
    }
    
    if (brushExists) {
        // Ask for confirmation to overwrite
        if (wxMessageBox("A ground brush with name '" + name + "' already exists. Do you want to overwrite it?", 
                        "Confirm Overwrite", wxYES_NO | wxICON_QUESTION) != wxYES) {
            return;
        }
        
        // Remove the existing brush
        materials.remove_child(existingBrush);
    }
    
    // Create the new brush node
    pugi::xml_node brushNode = materials.append_child("brush");
    brushNode.append_attribute("name").set_value(nstr(name).c_str());
    brushNode.append_attribute("type").set_value("ground");
    brushNode.append_attribute("server_lookid").set_value(serverId);
    brushNode.append_attribute("z-order").set_value(zOrder);
    
    // Add all ground items
    for (const GroundItem& item : m_groundItems) {
        pugi::xml_node itemNode = brushNode.append_child("item");
        itemNode.append_attribute("id").set_value(item.itemId);
        itemNode.append_attribute("chance").set_value(item.chance);
    }
    
    // Add border reference if we have border items, or if border ID is specified (even without items)
    if (!m_borderItems.empty() || borderId > 0) {
        // Get alignment type
        wxString alignmentType = m_borderAlignmentChoice->GetStringSelection();
        
        // Main border
        pugi::xml_node borderNode = brushNode.append_child("border");
        borderNode.append_attribute("align").set_value(nstr(alignmentType).c_str());
        borderNode.append_attribute("id").set_value(borderId);
        
        // "to none" border if checked
        if (m_includeToNoneCheck->IsChecked()) {
            pugi::xml_node borderToNoneNode = brushNode.append_child("border");
            borderToNoneNode.append_attribute("align").set_value(nstr(alignmentType).c_str());
            borderToNoneNode.append_attribute("to").set_value("none");
            borderToNoneNode.append_attribute("id").set_value(borderId);
        }
        
        // Inner border if checked
        if (m_includeInnerCheck->IsChecked()) {
            pugi::xml_node innerBorderNode = brushNode.append_child("border");
            innerBorderNode.append_attribute("align").set_value("inner");
            innerBorderNode.append_attribute("id").set_value(borderId);
            
            // Inner "to none" border if checked
            if (m_includeToNoneCheck->IsChecked()) {
                pugi::xml_node innerToNoneNode = brushNode.append_child("border");
                innerToNoneNode.append_attribute("align").set_value("inner");
                innerToNoneNode.append_attribute("to").set_value("none");
                innerToNoneNode.append_attribute("id").set_value(borderId);
            }
        }
    }
    
    // Save the file
    if (!doc.save_file(nstr(groundsFile).c_str())) {
        wxMessageBox("Failed to save changes to grounds.xml", "Error", wxICON_ERROR);
        return;
    }
    
    // Now also add this brush to the selected tileset
    wxString tilesetsFile = dataDir + wxFileName::GetPathSeparator() + 
                           wxString(versionStr.c_str()) + 
                           wxFileName::GetPathSeparator() + "tilesets.xml";
    
    if (!wxFileExists(tilesetsFile)) {
        wxMessageBox("Cannot find tilesets.xml file in the data directory.", "Error", wxICON_ERROR);
        return;
    }
    
    // Load the tilesets XML file
    pugi::xml_document tilesetsDoc;
    pugi::xml_parse_result tilesetsResult = tilesetsDoc.load_file(nstr(tilesetsFile).c_str());
    
    if (!tilesetsResult) {
        wxMessageBox("Failed to load tilesets.xml: " + wxString(tilesetsResult.description()), "Error", wxICON_ERROR);
        return;
    }
    
    pugi::xml_node tilesetsMaterials = tilesetsDoc.child("materials");
    if (!tilesetsMaterials) {
        wxMessageBox("Invalid tilesets.xml file: missing 'materials' node", "Error", wxICON_ERROR);
        return;
    }
    
    // Find the selected tileset
    bool tilesetFound = false;
    for (pugi::xml_node tilesetNode = tilesetsMaterials.child("tileset"); tilesetNode; tilesetNode = tilesetNode.next_sibling("tileset")) {
        pugi::xml_attribute nameAttr = tilesetNode.attribute("name");
        
        if (nameAttr && wxString(nameAttr.as_string()) == tilesetName) {
            // Find the terrain node
            pugi::xml_node terrainNode = tilesetNode.child("terrain");
            if (!terrainNode) {
                // Create terrain node if it doesn't exist
                terrainNode = tilesetNode.append_child("terrain");
            }
            
            // Check if the brush is already in this tileset
            bool brushFound = false;
            for (pugi::xml_node brushNode = terrainNode.child("brush"); brushNode; brushNode = brushNode.next_sibling("brush")) {
                pugi::xml_attribute brushNameAttr = brushNode.attribute("name");
                
                if (brushNameAttr && wxString(brushNameAttr.as_string()) == name) {
                    brushFound = true;
                    break;
                }
            }
            
            // If brush not found, add it
            if (!brushFound) {
                // Add a brush node directly under terrain - no empty attributes
                pugi::xml_node newBrushNode = terrainNode.append_child("brush");
                newBrushNode.append_attribute("name").set_value(nstr(name).c_str());
            }
            
            tilesetFound = true;
            break;
        }
    }
    
    if (!tilesetFound) {
        wxMessageBox("Selected tileset not found in tilesets.xml", "Error", wxICON_ERROR);
        return;
    }
    
    // Save the tilesets.xml file
    if (!tilesetsDoc.save_file(nstr(tilesetsFile).c_str())) {
        wxMessageBox("Failed to save changes to tilesets.xml", "Error", wxICON_ERROR);
        return;
    }
    
    wxMessageBox("Ground brush saved successfully and added to the " + tilesetName + " tileset.", "Success", wxICON_INFORMATION);
    
    // Reload the existing ground brushes list
    LoadExistingGroundBrushes();
}

void BorderEditorDialog::LoadTilesets() {
    m_tilesetChoice->Clear();
    m_tilesets.clear();

    wxString tilesetsFile = GetVersionFilePath("tilesets.xml");
    if (tilesetsFile.IsEmpty() || !wxFileExists(tilesetsFile)) {
        wxMessageBox("Cannot find tilesets.xml file in the data directory.", "Error", wxICON_ERROR);
        return;
    }

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(nstr(tilesetsFile).c_str());
    if (!result) {
        wxMessageBox("Failed to load tilesets.xml: " + wxString(result.description()), "Error", wxICON_ERROR);
        return;
    }
    
    pugi::xml_node materials = doc.child("materials");
    if (!materials) {
        wxMessageBox("Invalid tilesets.xml file: missing 'materials' node", "Error", wxICON_ERROR);
        return;
    }
    
    // Parse all tilesets
    wxArrayString tilesetNames; // Store in sorted order
    for (pugi::xml_node tilesetNode = materials.child("tileset"); tilesetNode; tilesetNode = tilesetNode.next_sibling("tileset")) {
        pugi::xml_attribute nameAttr = tilesetNode.attribute("name");
        
        if (nameAttr) {
            wxString tilesetName = wxString(nameAttr.as_string());
            
            // Add to our array of names
            tilesetNames.Add(tilesetName);
            
            // Add to the map for later use
            m_tilesets[tilesetName] = tilesetName;
        }
    }
    
    // Sort tileset names alphabetically
    tilesetNames.Sort();
    
    // Add sorted names to the choice control
    for (size_t i = 0; i < tilesetNames.GetCount(); ++i) {
        m_tilesetChoice->Append(tilesetNames[i]);
    }
    
    // Select the first tileset by default if any exist
    if (m_tilesetChoice->GetCount() > 0) {
        m_tilesetChoice->SetSelection(0);
    }
} 
