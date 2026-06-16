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

#ifndef RME_DARK_MODE_MANAGER_H_
#define RME_DARK_MODE_MANAGER_H_

#include "main.h"

// Forward declarations for custom menu/toolbar classes
class MainMenuBar;
class MainToolBar;

class DarkModeManager {
public:
    DarkModeManager();
    ~DarkModeManager();

    // Initialize and set up dark mode
    void Initialize();

    // Apply theme to a window and all its children
    void ApplyTheme(wxWindow* window);

    // Apply theme to specific controls
    void ApplyThemeToMenuBar(wxMenuBar* menuBar);
    void ApplyThemeToStatusBar(wxStatusBar* statusBar);
    void ApplyThemeToToolBar(wxToolBar* toolBar);
    void ApplyThemeToDialog(wxDialog* dialog);
    
    // Apply theme to custom classes
    void ApplyThemeToMainMenuBar(MainMenuBar* menuBar);
    void ApplyThemeToMainToolBar(MainToolBar* toolBar);

    // Get colors for UI elements
    wxColour GetBackgroundColor() const;
    wxColour GetForegroundColor() const;
    wxColour GetMenuBackgroundColor() const;
    wxColour GetMenuForegroundColor() const;
    wxColour GetSelectionBackgroundColor() const;
    wxColour GetSelectionForegroundColor() const;
    wxColour GetPanelBackgroundColor() const;
    wxColour GetBorderColor() const;

    // Toggle dark mode
    void ToggleDarkMode();
    
    // Check if dark mode is enabled
    bool IsDarkModeEnabled() const;

private:
    bool isDarkModeEnabled;
    
    // Cache colors to avoid constant recalculation
    wxColour backgroundColor;
    wxColour foregroundColor;
    wxColour menuBackgroundColor;
    wxColour menuForegroundColor;
    wxColour selectionBackgroundColor;
    wxColour selectionForegroundColor;
    wxColour panelBackgroundColor;
    wxColour borderColor;
    
    // Update cached colors
    void UpdateColors();
};

extern DarkModeManager g_darkMode;

#endif // RME_DARK_MODE_MANAGER_H_ 