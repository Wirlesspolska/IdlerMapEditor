// border_editor_window.h - Provides a visual editor for auto borders

#ifndef RME_BORDER_EDITOR_WINDOW_H_
#define RME_BORDER_EDITOR_WINDOW_H_

#include <wx/dialog.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/listbox.h>
#include <wx/spinctrl.h>
#include <wx/grid.h>
#include <wx/panel.h>
#include <wx/checkbox.h>
#include <wx/combobox.h>
#include <wx/notebook.h>
#include <wx/choice.h>
#include <vector>
#include <map>

class BorderItemButton;
class BorderGridPanel;

// Represents a border edge position
enum BorderEdgePosition {
    EDGE_NONE = -1,
    EDGE_N,   // North
    EDGE_E,   // East
    EDGE_S,   // South
    EDGE_W,   // West
    EDGE_CNW, // Corner Northwest
    EDGE_CNE, // Corner Northeast
    EDGE_CSE, // Corner Southeast
    EDGE_CSW, // Corner Southwest
    EDGE_DNW, // Diagonal Northwest
    EDGE_DNE, // Diagonal Northeast
    EDGE_DSE, // Diagonal Southeast
    EDGE_DSW, // Diagonal Southwest
    EDGE_COUNT
};

// Alignment options for borders
enum BorderAlignment {
    ALIGN_OUTER,
    ALIGN_INNER
};

// Utility function to convert border edge string to position
BorderEdgePosition edgeStringToPosition(const std::string& edgeStr);

// Utility function to convert border position to string
std::string edgePositionToString(BorderEdgePosition pos);

// Represents a border item
struct BorderItem {
    BorderEdgePosition position;
    uint16_t itemId;
    
    BorderItem() : position(EDGE_NONE), itemId(0) {}
    BorderItem(BorderEdgePosition pos, uint16_t id) : position(pos), itemId(id) {}
    
    bool operator==(const BorderItem& other) const {
        return position == other.position && itemId == other.itemId;
    }
    
    bool operator!=(const BorderItem& other) const {
        return !(*this == other);
    }
};

// Represents a ground item with chance
struct GroundItem {
    uint16_t itemId;
    int chance;
    
    GroundItem() : itemId(0), chance(10) {}
    GroundItem(uint16_t id, int c) : itemId(id), chance(c) {}
    
    bool operator==(const GroundItem& other) const {
        return itemId == other.itemId && chance == other.chance;
    }
    
    bool operator!=(const GroundItem& other) const {
        return !(*this == other);
    }
};

class BorderEditorDialog : public wxDialog {
public:
    BorderEditorDialog(wxWindow* parent, const wxString& title);
    virtual ~BorderEditorDialog();

    // Event handlers - made public so they can be accessed by other components
    void OnItemIdChanged(wxCommandEvent& event);
    void OnPositionSelected(wxCommandEvent& event);
    void OnAddItem(wxCommandEvent& event);
    void OnClear(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    void OnBrowse(wxCommandEvent& event);
    void OnLoadBorder(wxCommandEvent& event);
    void OnGridCellClicked(wxMouseEvent& event);
    void OnPageChanged(wxBookCtrlEvent& event);
    void OnAddGroundItem(wxCommandEvent& event);
    void OnRemoveGroundItem(wxCommandEvent& event);
    void OnLoadGroundBrush(wxCommandEvent& event);
    void OnGroundBrowse(wxCommandEvent& event);

protected:
    void CreateGUIControls();
    void LoadExistingBorders();
    void LoadExistingGroundBrushes();
    void LoadTilesets();
    void SaveBorder();
    void SaveGroundBrush();
    bool ValidateBorder();
    bool ValidateGroundBrush();
    void UpdatePreview();
    void ClearItems();
    void ClearGroundItems();
    void UpdateGroundItemsList();

public:
    // UI Elements - made public so they can be accessed by other components
    // Common
    wxTextCtrl* m_nameCtrl;
    wxSpinCtrl* m_idCtrl;
    wxNotebook* m_notebook;
    
    // Border Tab
    wxPanel* m_borderPanel;
    wxComboBox* m_existingBordersCombo;
    wxCheckBox* m_isOptionalCheck;
    wxCheckBox* m_isGroundCheck;
    wxSpinCtrl* m_groupCtrl;
    wxSpinCtrl* m_itemIdCtrl;
    
    // Ground Tab
    wxPanel* m_groundPanel;
    wxComboBox* m_existingGroundBrushesCombo;
    wxSpinCtrl* m_serverLookIdCtrl;
    wxSpinCtrl* m_zOrderCtrl;
    wxSpinCtrl* m_groundItemIdCtrl;
    wxSpinCtrl* m_groundItemChanceCtrl;
    wxListBox* m_groundItemsList;
    
    // Border alignment for ground brushes
    wxChoice* m_borderAlignmentChoice;
    wxCheckBox* m_includeToNoneCheck;
    wxCheckBox* m_includeInnerCheck;
    
    // Tileset selector for ground brushes
    wxChoice* m_tilesetChoice;
    
    // Map of tileset names to internal identifiers
    std::map<wxString, wxString> m_tilesets;
    
    // Border items
    std::vector<BorderItem> m_borderItems;
    
    // Ground items
    std::vector<GroundItem> m_groundItems;
    
    // Border grid
    BorderGridPanel* m_gridPanel;
    
    // Border item buttons for each position
    std::map<BorderEdgePosition, BorderItemButton*> m_borderButtons;
    
    // Border preview panel
    class BorderPreviewPanel* m_previewPanel;
    
private:
    // Next available border ID
    int m_nextBorderId;
    
    // Current active tab (0 = border, 1 = ground)
    int m_activeTab;
    
    DECLARE_EVENT_TABLE()
};

// Custom button to represent a border item
class BorderItemButton : public wxButton {
public:
    BorderItemButton(wxWindow* parent, BorderEdgePosition position, wxWindowID id = wxID_ANY);
    virtual ~BorderItemButton();
    
    void SetItemId(uint16_t id);
    uint16_t GetItemId() const { return m_itemId; }
    BorderEdgePosition GetPosition() const { return m_position; }
    
    void OnPaint(wxPaintEvent& event);
    
private:
    uint16_t m_itemId;
    BorderEdgePosition m_position;
    
    DECLARE_EVENT_TABLE()
};

// Grid panel to visually show border item positions
class BorderGridPanel : public wxPanel {
public:
    BorderGridPanel(wxWindow* parent, wxWindowID id = wxID_ANY);
    virtual ~BorderGridPanel();
    
    void SetItemId(BorderEdgePosition pos, uint16_t itemId);
    uint16_t GetItemId(BorderEdgePosition pos) const;
    void Clear();
    
    void SetSelectedPosition(BorderEdgePosition pos);
    BorderEdgePosition GetSelectedPosition() const { return m_selectedPosition; }
    
    void OnPaint(wxPaintEvent& event);
    void OnMouseClick(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    
    // Made public so it can be accessed from other components
    wxPoint GetPositionCoordinates(BorderEdgePosition pos) const;
    BorderEdgePosition GetPositionFromCoordinates(int x, int y) const;
    
private:
    std::map<BorderEdgePosition, uint16_t> m_items;
    BorderEdgePosition m_selectedPosition;
    
    DECLARE_EVENT_TABLE()
};

// Panel to preview how the border would look
class BorderPreviewPanel : public wxPanel {
public:
    BorderPreviewPanel(wxWindow* parent, wxWindowID id = wxID_ANY);
    virtual ~BorderPreviewPanel();
    
    void SetBorderItems(const std::vector<BorderItem>& items);
    void Clear();
    
    void OnPaint(wxPaintEvent& event);
    
private:
    std::vector<BorderItem> m_borderItems;
    
    DECLARE_EVENT_TABLE()
};

#endif // RME_BORDER_EDITOR_WINDOW_H_ 