# Script Loading Functionality Export

Complete export of OTS data loading system for RevScripts, Monsters, and NPCs.

## Table of Contents
1. [System Overview](#system-overview)
2. [Manager Classes](#manager-classes)
3. [Data Structures](#data-structures)
4. [Core Loading Functions](#core-loading-functions)
5. [GUI Integration](#gui-integration)
6. [Preferences Panel](#preferences-panel)
7. [Visual Indicators](#visual-indicators)
8. [Right-Click Context Menu](#right-click-context-menu)
9. [Configuration Settings](#configuration-settings)
10. [Complete Code Reference](#complete-code-reference)

---

## System Overview

The script loading system consists of three manager classes that scan and parse OTS data files:

- **RevScriptManager**: Handles RevScript .lua files and XML-based actions/movements
- **MonsterManager**: Handles monster definitions from monsters.xml and individual monster files
- **NPCManager**: Handles NPC definitions from the npc directory

### Key Features
- Automatic detection of OTS data directory when loading maps
- Manual configuration via preferences panel
- Visual indicators (tooltips, icons) showing which items have scripts
- Right-click menu integration for opening scripts directly
- Support for multiple directory search strategies

### Directory Search Strategy

When loading a map, the system searches for OTS data in this order:

1. **Custom OTS Data Directory** (if configured in preferences)
2. **Same directory as map** (`/map_directory/data/`)
3. **Parent directory** (`/map_directory/../` - handles `/data/world/map.otbm` → `/data/`)
4. **Sibling directory** (`/map_directory/../data/`)

---

## Manager Classes

### RevScriptManager (`source/revscript_manager.h`, `source/revscript_manager.cpp`)

**Purpose**: Scans and indexes RevScript files and XML-based action/movement scripts.

**Key Methods**:
```cpp
bool scanRevScriptsDirectory(const std::string& data_dir);
std::vector<RevScriptEntry> findByActionID(int action_id) const;
std::vector<RevScriptEntry> findByUniqueID(int unique_id) const;
std::vector<RevScriptEntry> findByItemID(int item_id) const;
bool hasScriptForActionID(int action_id) const;
bool hasScriptForUniqueID(int unique_id) const;
bool hasScriptForItemID(int item_id) const;
bool openRevScript(const RevScriptEntry& entry) const;
std::string getScanStatistics() const;
```

**Scans**:
- `/data/scripts/` directory recursively for `.lua` files
- `/data/actions/actions.xml` for action definitions
- `/data/movements/movements.xml` for movement definitions

**Parsing Logic**:
- Uses regex to find patterns like `item:aid(1234)`, `item:uid(5678)`, `item:id(9012)`
- Parses XML files for `<action>` and `<movevent>` nodes with itemid/actionid/uniqueid attributes
- Supports ID ranges (fromid/toid, fromaid/toaid, fromuid/touid)

### MonsterManager (`source/monster_manager.h`, `source/monster_manager.cpp`)

**Purpose**: Scans and indexes monster definitions.

**Key Methods**:
```cpp
bool scanMonstersDirectory(const std::string& data_dir);
MonsterEntry* findByName(const std::string& name) const;
bool loadMonsterDetails(MonsterEntry& entry) const;
bool createMonsterXML(const MonsterEntry& entry, const std::string& output_path) const;
bool openMonsterXML(const MonsterEntry& entry) const;
std::string getScanStatistics() const;
```

**Scans**:
- `/data/monsters.xml` or `/data/monster/monsters.xml` for monster index
- Individual monster XML files referenced in monsters.xml

### NPCManager (`source/npc_manager.h`, `source/npc_manager.cpp`)

**Purpose**: Scans and indexes NPC definitions.

**Key Methods**:
```cpp
bool scanNPCDirectory(const std::string& data_dir);
NPCEntry* findByName(const std::string& name) const;
bool loadNPCDetails(NPCEntry& entry) const;
bool createNPCXML(const NPCEntry& entry, const std::string& output_path) const;
bool openNPCXML(const NPCEntry& entry) const;
bool openNPCScript(const NPCEntry& entry) const;
std::string getScanStatistics() const;
```

**Scans**:
- `/data/npc/` directory recursively for `.xml` files
- Parses NPC XML files for name, script path, and properties

---

## Data Structures

### RevScriptEntry
```cpp
struct RevScriptEntry {
    std::string filename;        // Full path to script file
    std::string function_name;   // Function name (if applicable)
    int line_number;             // Line number in file
    int id_value;                // The ID value (action/unique/item)
    std::string id_type;         // "aid", "uid", or "id"
    std::string script_type;     // "revscript", "action", "movement"
};

struct XmlScriptEntry {
    std::string script_path;     // Relative path to script file
    std::string full_path;       // Full absolute path to script file
    std::string script_type;     // "action" or "movement"
    int from_id;                 // Start of ID range
    int to_id;                   // End of ID range
    std::string id_attribute;    // "itemid", "actionid", "uniqueid"
    
    bool isInRange(int id) const {
        return id >= from_id && id <= to_id;
    }
};
```

### MonsterEntry
```cpp
struct MonsterEntry {
    std::string name;
    std::string filename;
    std::string full_path;
    bool loaded;
    
    // Basic properties
    std::string race;
    int experience;
    int speed;
    int manacost;
    int health_max;
    
    // Look properties
    int look_type;
    int look_head, look_body, look_legs, look_feet;
    int look_addons, look_mount;
    
    // Complex systems
    std::vector<AttackEntry> attacks;
    std::vector<DefenseEntry> defenses;
    std::vector<ElementEntry> elements;
    std::vector<ImmunityEntry> immunities;
    std::vector<SummonEntry> summons;
    std::vector<VoiceEntry> voices;
    std::vector<LootEntry> loot;
};
```

### NPCEntry
```cpp
struct NPCEntry {
    std::string name;
    std::string filename;
    std::string full_path;
    std::string script_path;
    std::string script_full_path;
    bool loaded;
    bool hasScript;
    
    // Basic properties
    int walkinterval;
    int floorchange;
    int health_now, health_max;
    
    // Look properties
    int look_type;
    int look_head, look_body, look_legs, look_feet;
    int look_addons;
    
    // Parameters
    std::vector<ParameterEntry> parameters;
};
```

---

## Core Loading Functions

### Map Load Integration (`source/gui.cpp`)

When a map is loaded, the system automatically attempts to load OTS data:

```cpp
bool GUI::LoadMap(const FileName& fileName) {
    // ... map loading code ...
    
    // Load OTS scripts directory for the map
    wxFileName mapPath(fileName.GetFullPath());
    wxString mapDir = mapPath.GetPath();
    
    bool scriptsLoaded = false;
    bool monstersLoaded = false;
    bool npcsLoaded = false;
    
    // Strategy 0: Use custom OTS Data directory if set
    std::string customDataPath = g_settings.getString(Config::OTS_DATA_DIRECTORY);
    if (!customDataPath.empty()) {
        wxString customPath = wxstr(customDataPath);
        if (wxDir::Exists(customPath)) {
            if (revscript_manager.scanRevScriptsDirectory(customDataPath)) {
                scriptsLoaded = true;
            }
            if (monster_manager.scanMonstersDirectory(customDataPath)) {
                monstersLoaded = true;
            }
            if (npc_manager.scanNPCDirectory(customDataPath)) {
                npcsLoaded = true;
            }
            if (scriptsLoaded || monstersLoaded || npcsLoaded) {
                SetStatusText("OTS data directory loaded (custom): " + customPath);
            }
        }
    }
    
    // Strategy 1: Look for data directory in same directory as map
    if (!scriptsLoaded && !monstersLoaded && !npcsLoaded) {
        wxString dataPath = mapDir + wxFileName::GetPathSeparator() + "data";
        if (wxDir::Exists(dataPath)) {
            if (revscript_manager.scanRevScriptsDirectory(nstr(dataPath))) {
                scriptsLoaded = true;
            }
            if (monster_manager.scanMonstersDirectory(nstr(dataPath))) {
                monstersLoaded = true;
            }
            if (npc_manager.scanNPCDirectory(nstr(dataPath))) {
                npcsLoaded = true;
            }
            if (scriptsLoaded || monstersLoaded || npcsLoaded) {
                SetStatusText("OTS data directory loaded: " + dataPath);
            }
        }
    }
    
    // Strategy 2: Go up one directory level and look for data
    // This handles the common case where map is in /data/world/ and we need to find /data/
    if (!scriptsLoaded && !monstersLoaded && !npcsLoaded) {
        wxFileName parentPath(mapDir);
        if (parentPath.GetDirCount() > 0) {
            parentPath.RemoveLastDir();  // Go up one level (from world to data)
            wxString parentDataPath = parentPath.GetPath();
            
            // Check if this looks like the data directory
            wxString scriptsCheck = parentDataPath + wxFileName::GetPathSeparator() + "scripts";
            wxString actionsCheck = parentDataPath + wxFileName::GetPathSeparator() + "actions";
            wxString monstersCheck = parentDataPath + wxFileName::GetPathSeparator() + "monsters";
            wxString npcsCheck = parentDataPath + wxFileName::GetPathSeparator() + "npc";
            if (wxDir::Exists(scriptsCheck) || wxDir::Exists(actionsCheck) || 
                wxDir::Exists(monstersCheck) || wxDir::Exists(npcsCheck)) {
                if (revscript_manager.scanRevScriptsDirectory(nstr(parentDataPath))) {
                    scriptsLoaded = true;
                }
                if (monster_manager.scanMonstersDirectory(nstr(parentDataPath))) {
                    monstersLoaded = true;
                }
                if (npc_manager.scanNPCDirectory(nstr(parentDataPath))) {
                    npcsLoaded = true;
                }
                if (scriptsLoaded || monstersLoaded || npcsLoaded) {
                    SetStatusText("OTS data directory loaded: " + parentDataPath);
                }
            }
        }
    }
    
    // Strategy 3: Look for data directory as sibling to map directory
    if (!scriptsLoaded && !monstersLoaded && !npcsLoaded) {
        wxFileName mapParent(mapDir);
        if (mapParent.GetDirCount() > 0) {
            mapParent.RemoveLastDir();  // Go up to parent of map directory
            wxString siblingDataPath = mapParent.GetPath() + wxFileName::GetPathSeparator() + "data";
            if (wxDir::Exists(siblingDataPath)) {
                if (revscript_manager.scanRevScriptsDirectory(nstr(siblingDataPath))) {
                    scriptsLoaded = true;
                }
                if (monster_manager.scanMonstersDirectory(nstr(siblingDataPath))) {
                    monstersLoaded = true;
                }
                if (npc_manager.scanNPCDirectory(nstr(siblingDataPath))) {
                    npcsLoaded = true;
                }
                if (scriptsLoaded || monstersLoaded || npcsLoaded) {
                    SetStatusText("OTS data directory loaded: " + siblingDataPath);
                }
            }
        }
    }
    
    if (!scriptsLoaded && !monstersLoaded && !npcsLoaded) {
        SetStatusText("No OTS data directory found");
    }
    
    return true;
}
```

### Manual Reload Functions

```cpp
void GUI::ReloadRevScripts() {
    if (!IsEditorOpen()) {
        SetStatusText("No map open - cannot reload scripts");
        return;
    }
    
    // Get the current map file path
    Editor* editor = GetCurrentEditor();
    if (!editor || !editor->map.hasFile()) {
        SetStatusText("Map has no file - cannot determine OTS data directory");
        return;
    }
    
    // Use same search strategy as LoadMap
    // ... (same code as above) ...
}

void GUI::ReloadNPCs() {
    // Similar implementation for NPCs only
}
```

---

## GUI Integration

### GUI Class Members (`source/gui.h`)

```cpp
class GUI {
public:
    // Manager instances
    RevScriptManager revscript_manager;
    MonsterManager monster_manager;
    NPCManager npc_manager;
    
    // Reload functions
    void ReloadRevScripts();
    void ReloadNPCs();
};

extern GUI g_gui;
```

---

## Preferences Panel

### Preferences Window (`source/preferences.h`, `source/preferences.cpp`)

**UI Controls**:
```cpp
class PreferencesWindow : public wxDialog {
private:
    wxDirPickerCtrl* revscript_directory_picker;
    wxButton* force_reload_revscripts_btn;
};
```

**UI Layout** (in `CreateGeneralPage()`):
```cpp
grid_sizer->Add(tmptext = newd wxStaticText(general_page, wxID_ANY, "OTS Data directory: "), 0);
revscript_directory_picker = newd wxDirPickerCtrl(general_page, wxID_ANY);
grid_sizer->Add(revscript_directory_picker, 1, wxEXPAND);
wxString revscript_dir = wxstr(g_settings.getString(Config::OTS_DATA_DIRECTORY));
revscript_directory_picker->SetPath(revscript_dir);
SetWindowToolTip(tmptext, revscript_directory_picker, 
    "Directory containing OTS data files (/data/ folder with scripts, actions, movements, etc.).");

grid_sizer->Add(newd wxStaticText(general_page, wxID_ANY, ""), 0); // Empty cell for alignment
force_reload_revscripts_btn = newd wxButton(general_page, wxID_ANY, "Force Reload Scripts");
grid_sizer->Add(force_reload_revscripts_btn, 0);
SetWindowToolTip(force_reload_revscripts_btn, 
    "Force reload all script files from the selected OTS data directory.");
```

**Event Handler**:
```cpp
void PreferencesWindow::OnForceReloadRevScripts(wxCommandEvent& event) {
    // Save the current RevScript directory setting first
    g_settings.setString(Config::OTS_DATA_DIRECTORY, nstr(revscript_directory_picker->GetPath()));
    
    // Call the GUI's reload method
    g_gui.ReloadRevScripts();
    
    // Show statistics for debugging
    std::string revscript_stats = g_gui.revscript_manager.getScanStatistics();
    std::string monster_stats = g_gui.monster_manager.getScanStatistics();
    std::string npc_stats = g_gui.npc_manager.getScanStatistics();
    
    wxString message = "Scripts reloaded!\n\n" + wxstr(revscript_stats) + 
                       "\n\n" + wxstr(monster_stats) + "\n\n" + wxstr(npc_stats);
    wxMessageBox(message, "Reload Complete", wxOK | wxICON_INFORMATION, this);
}
```

---

## Visual Indicators

### Tooltip Integration (`source/map_drawer.cpp`)

Items with scripts show a special indicator in tooltips:

```cpp
// In DrawTileTooltip or similar function
int action = item->getActionID();
int unique = item->getUniqueID();
int id = item->getID();

// Check if scripts exist for this item
bool hasScript = false;
if (g_gui.revscript_manager.isLoaded()) {
    if (action > 0 && g_gui.revscript_manager.hasScriptForActionID(action)) {
        hasScript = true;
    } else if (unique > 0 && g_gui.revscript_manager.hasScriptForUniqueID(unique)) {
        hasScript = true;
    } else if (g_gui.revscript_manager.hasScriptForItemID(id)) {
        hasScript = true;
    }
}

if (hasScript) {
    // Add visual indicator (e.g., icon, color, text)
    tooltip += " [SCRIPT]";
}
```

---

## Right-Click Context Menu

### Context Menu Integration (`source/map_display.cpp`)

Right-clicking items with scripts shows "Open Script" option:

```cpp
void MapCanvas::OnRightClick(wxMouseEvent& event) {
    // ... existing context menu code ...
    
    // Get selected item
    Item* topSelectedItem = /* ... */;
    if (topSelectedItem) {
        int actionId = topSelectedItem->getActionID();
        int uniqueId = topSelectedItem->getUniqueID();
        int itemId = topSelectedItem->getID();
        
        bool hasActionScript = actionId != 0 && 
            g_gui.revscript_manager.hasScriptForActionID(actionId);
        bool hasUniqueScript = uniqueId != 0 && 
            g_gui.revscript_manager.hasScriptForUniqueID(uniqueId);
        bool hasItemScript = g_gui.revscript_manager.hasScriptForItemID(itemId);
        
        if (actionId != 0 || uniqueId != 0 || hasItemScript) {
            wxMenuItem* scriptItem = popupMenu.Append(wxID_ANY, "Open Script");
            popupMenu.Bind(wxEVT_COMMAND_MENU_SELECTED, [=](wxCommandEvent&) {
                OpenScriptForItem(itemId, actionId, uniqueId);
            }, scriptItem->GetId());
        }
    }
}

void MapCanvas::OpenScriptForItem(int item_id, int action_id, int unique_id) {
    // Check if the RevScript manager is loaded
    if (!g_gui.revscript_manager.isLoaded()) {
        g_gui.PopupDialog("Script Error", 
            "No OTS data directory has been loaded. Please load a map first or configure the OTS data directory in preferences.", 
            wxOK);
        return;
    }
    
    std::vector<RevScriptEntry> entries;
    
    // Search for scripts by action ID
    if (action_id > 0) {
        auto aid_entries = g_gui.revscript_manager.findByActionID(action_id);
        entries.insert(entries.end(), aid_entries.begin(), aid_entries.end());
    }
    
    // Search for scripts by unique ID
    if (unique_id > 0) {
        auto uid_entries = g_gui.revscript_manager.findByUniqueID(unique_id);
        entries.insert(entries.end(), uid_entries.begin(), uid_entries.end());
    }
    
    // Search for scripts by item ID
    auto id_entries = g_gui.revscript_manager.findByItemID(item_id);
    entries.insert(entries.end(), id_entries.begin(), id_entries.end());
    
    if (entries.empty()) {
        g_gui.PopupDialog("No Scripts Found", 
            "No scripts found for this item.", wxOK);
        return;
    }
    
    // If only one script, open it directly
    if (entries.size() == 1) {
        const RevScriptEntry& entry = entries[0];
        if (!g_gui.revscript_manager.openRevScript(entry)) {
            g_gui.PopupDialog("Error", 
                "Failed to open script file.", wxOK);
        }
    } else {
        // Multiple scripts - show selection dialog
        wxArrayString choices;
        for (const auto& entry : entries) {
            wxString choice = wxString::Format("%s (%s:%d)", 
                wxstr(entry.filename), 
                wxstr(entry.id_type), 
                entry.id_value);
            choices.Add(choice);
        }
        
        wxSingleChoiceDialog dialog(this, 
            "Multiple scripts found. Select one to open:", 
            "Select Script", choices);
        
        if (dialog.ShowModal() == wxID_OK) {
            int selection = dialog.GetSelection();
            const RevScriptEntry& entry = entries[selection];
            if (!g_gui.revscript_manager.openRevScript(entry)) {
                g_gui.PopupDialog("Error", 
                    "Failed to open script file.", wxOK);
            }
        }
    }
}
```

---

## Configuration Settings

### Settings Definition (`source/settings.h`, `source/settings.cpp`)

```cpp
// In settings.h
enum SettingsID {
    // ... other settings ...
    OTS_DATA_DIRECTORY,
    // ... other settings ...
};

// In settings.cpp initialization
String(OTS_DATA_DIRECTORY, "");
```

---

## Complete Code Reference

### File List

**Manager Classes**:
- `source/revscript_manager.h` - RevScript manager header
- `source/revscript_manager.cpp` - RevScript manager implementation
- `source/monster_manager.h` - Monster manager header
- `source/monster_manager.cpp` - Monster manager implementation
- `source/npc_manager.h` - NPC manager header
- `source/npc_manager.cpp` - NPC manager implementation

**GUI Integration**:
- `source/gui.h` - GUI class declaration with manager instances
- `source/gui.cpp` - LoadMap(), ReloadRevScripts(), ReloadNPCs() implementations

**UI Components**:
- `source/preferences.h` - Preferences window declaration
- `source/preferences.cpp` - Preferences UI and event handlers

**Visual Indicators**:
- `source/map_drawer.cpp` - Tooltip integration
- `source/map_display.cpp` - Right-click context menu integration

**Configuration**:
- `source/settings.h` - Settings enum
- `source/settings.cpp` - Settings initialization

### Dependencies

**External Libraries**:
- wxWidgets (for UI, file system, dialogs)
- pugixml (for XML parsing)
- Standard C++ regex (for Lua file parsing)

**Internal Dependencies**:
- `main.h` - Common includes and definitions
- `gui.h` - GUI class
- `settings.h` - Configuration system

---

## Implementation Notes

### Regex Patterns for RevScript Parsing

```cpp
std::regex aid_pattern(R"((\w+):aid\s*\(\s*(\d+)\s*\))");
std::regex uid_pattern(R"((\w+):uid\s*\(\s*(\d+)\s*\))");
std::regex id_pattern(R"((\w+):id\s*\(\s*(\d+)\s*\))");
std::regex function_pattern(R"(function\s+(\w+[\w\.]*)\s*\()");
```

### XML Parsing Notes

- Uses pugixml for robust XML parsing
- Handles both single IDs and ID ranges
- Supports nested directory structures
- Skips hidden directories (starting with `.`)
- Skips `scripts` subdirectory in NPC scanning to avoid confusion

### Opening Files

All managers use `wxLaunchDefaultApplication()` to open files with the system's default application:

```cpp
bool RevScriptManager::openRevScript(const RevScriptEntry& entry) const {
    wxString filename = wxstr(entry.filename);
    return wxLaunchDefaultApplication(filename);
}
```

### Error Handling

- All scan functions return `bool` indicating success/failure
- Debug output using `OutputDebugStringA()` for Windows debugging
- User-friendly error messages via `wxMessageBox()`
- Graceful degradation when OTS data is not found

---

## Usage Example

```cpp
// In your application initialization
GUI g_gui;

// When loading a map
g_gui.LoadMap(fileName);  // Automatically scans for OTS data

// Manual reload from preferences
g_gui.ReloadRevScripts();

// Check if item has script
if (g_gui.revscript_manager.hasScriptForActionID(1234)) {
    // Show indicator
}

// Find and open script
auto entries = g_gui.revscript_manager.findByActionID(1234);
if (!entries.empty()) {
    g_gui.revscript_manager.openRevScript(entries[0]);
}

// Get statistics
std::string stats = g_gui.revscript_manager.getScanStatistics();
```

---

## End of Export

This document contains all the code context and implementation details needed to port the script loading functionality to another repository. All source files referenced are included in the complete codebase.
