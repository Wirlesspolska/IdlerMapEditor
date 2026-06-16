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
#include "dark_mode_manager.h"
#include "settings.h"
#include "main_menubar.h"
#include "main_toolbar.h"
#include "gui_ids.h"

DarkModeManager g_darkMode;

DarkModeManager::DarkModeManager() : isDarkModeEnabled(false) {
    UpdateColors();
}

DarkModeManager::~DarkModeManager() {
    // Nothing to clean up
}

void DarkModeManager::Initialize() {
    isDarkModeEnabled = g_settings.getBoolean(Config::DARK_MODE);
    UpdateColors();
}

void DarkModeManager::ApplyTheme(wxWindow* window) {
    if (!window) return;

    // Apply colors to the window itself
    window->SetBackgroundColour(GetBackgroundColor());
    window->SetForegroundColour(GetForegroundColor());

    // Special handling for specific window types
    if (auto* menuBar = dynamic_cast<wxMenuBar*>(window)) {
        ApplyThemeToMenuBar(menuBar);
    }
    else if (auto* statusBar = dynamic_cast<wxStatusBar*>(window)) {
        ApplyThemeToStatusBar(statusBar);
    }
    else if (auto* toolBar = dynamic_cast<wxToolBar*>(window)) {
        ApplyThemeToToolBar(toolBar);
    }
    else if (auto* dialog = dynamic_cast<wxDialog*>(window)) {
        ApplyThemeToDialog(dialog);
    }

    // Apply theme to all child windows
    for (auto* child : window->GetChildren()) {
        ApplyTheme(child);
    }

    // Force a refresh
    window->Refresh();
}

void DarkModeManager::ApplyThemeToMenuBar(wxMenuBar* menuBar) {
    if (!menuBar) return;
    
    menuBar->SetBackgroundColour(GetMenuBackgroundColor());
    menuBar->SetForegroundColour(GetMenuForegroundColor());
    
    // Don't try to set colors on individual menus, as wxMenu doesn't support
    // SetBackgroundColour and SetForegroundColour methods directly
    // The menu appearance will be controlled by the system theme or
    // parent window settings
}

void DarkModeManager::ApplyThemeToMainMenuBar(MainMenuBar* menuBar) {
    if (!menuBar) return;
    
    // No direct UI elements in MainMenuBar to theme
    // The actual wxMenuBar is part of the MainFrame
    // and will be themed separately
}

void DarkModeManager::ApplyThemeToStatusBar(wxStatusBar* statusBar) {
    if (!statusBar) return;
    
    statusBar->SetBackgroundColour(GetBackgroundColor());
    statusBar->SetForegroundColour(GetForegroundColor());
}

void DarkModeManager::ApplyThemeToToolBar(wxToolBar* toolBar) {
    if (!toolBar) return;
    
    toolBar->SetBackgroundColour(GetBackgroundColor());
    toolBar->SetForegroundColour(GetForegroundColor());
}

void DarkModeManager::ApplyThemeToMainToolBar(MainToolBar* toolBar) {
    if (!toolBar) return;

    // The MainToolBar class contains multiple wxAuiToolBar instances
    // Try to apply theme to them one by one
    try {
        wxAuiPaneInfo standardPane = toolBar->GetPane(TOOLBAR_STANDARD);
        if (wxAuiToolBar* tb = dynamic_cast<wxAuiToolBar*>(standardPane.window)) {
            tb->SetBackgroundColour(GetBackgroundColor());
            tb->SetForegroundColour(GetForegroundColor());
            tb->Refresh();
        }
    } catch (...) {
        // Ignore errors if unable to access this toolbar
    }
    
    try {
        wxAuiPaneInfo brushesPane = toolBar->GetPane(TOOLBAR_BRUSHES);
        if (wxAuiToolBar* tb = dynamic_cast<wxAuiToolBar*>(brushesPane.window)) {
            tb->SetBackgroundColour(GetBackgroundColor());
            tb->SetForegroundColour(GetForegroundColor());
            tb->Refresh();
        }
    } catch (...) {
        // Ignore errors if unable to access this toolbar
    }
    
    try {
        wxAuiPaneInfo positionPane = toolBar->GetPane(TOOLBAR_POSITION);
        if (wxAuiToolBar* tb = dynamic_cast<wxAuiToolBar*>(positionPane.window)) {
            tb->SetBackgroundColour(GetBackgroundColor());
            tb->SetForegroundColour(GetForegroundColor());
            tb->Refresh();
        }
    } catch (...) {
        // Ignore errors if unable to access this toolbar
    }
    
    try {
        wxAuiPaneInfo sizesPane = toolBar->GetPane(TOOLBAR_SIZES);
        if (wxAuiToolBar* tb = dynamic_cast<wxAuiToolBar*>(sizesPane.window)) {
            tb->SetBackgroundColour(GetBackgroundColor());
            tb->SetForegroundColour(GetForegroundColor());
            tb->Refresh();
        }
    } catch (...) {
        // Ignore errors if unable to access this toolbar
    }
}

void DarkModeManager::ApplyThemeToDialog(wxDialog* dialog) {
    if (!dialog) return;
    
    dialog->SetBackgroundColour(GetBackgroundColor());
    dialog->SetForegroundColour(GetForegroundColor());
}

wxColour DarkModeManager::GetBackgroundColor() const {
    return backgroundColor;
}

wxColour DarkModeManager::GetForegroundColor() const {
    return foregroundColor;
}

wxColour DarkModeManager::GetMenuBackgroundColor() const {
    return menuBackgroundColor;
}

wxColour DarkModeManager::GetMenuForegroundColor() const {
    return menuForegroundColor;
}

wxColour DarkModeManager::GetSelectionBackgroundColor() const {
    return selectionBackgroundColor;
}

wxColour DarkModeManager::GetSelectionForegroundColor() const {
    return selectionForegroundColor;
}

wxColour DarkModeManager::GetPanelBackgroundColor() const {
    return panelBackgroundColor;
}

wxColour DarkModeManager::GetBorderColor() const {
    return borderColor;
}

void DarkModeManager::ToggleDarkMode() {
    isDarkModeEnabled = !isDarkModeEnabled;
    g_settings.setInteger(Config::DARK_MODE, isDarkModeEnabled ? 1 : 0);
    UpdateColors();
}

bool DarkModeManager::IsDarkModeEnabled() const {
    return isDarkModeEnabled;
}

void DarkModeManager::UpdateColors() {
    if (isDarkModeEnabled) {
        // Dark mode colors
        if (g_settings.getBoolean(Config::DARK_MODE_CUSTOM_COLOR)) {
            // Use custom dark mode colors from settings
            wxColour customColor(
                g_settings.getInteger(Config::DARK_MODE_RED),
                g_settings.getInteger(Config::DARK_MODE_GREEN),
                g_settings.getInteger(Config::DARK_MODE_BLUE)
            );
            
            backgroundColor = customColor;
            // Adjust other colors based on the custom color
            foregroundColor = wxColour(230, 230, 230);
            menuBackgroundColor = wxColour(
                wxMin(customColor.Red() + 10, 255),
                wxMin(customColor.Green() + 10, 255),
                wxMin(customColor.Blue() + 10, 255)
            );
            menuForegroundColor = wxColour(230, 230, 230);
            selectionBackgroundColor = wxColour(65, 105, 225);
            selectionForegroundColor = wxColour(255, 255, 255);
            panelBackgroundColor = wxColour(
                wxMin(customColor.Red() + 5, 255),
                wxMin(customColor.Green() + 5, 255),
                wxMin(customColor.Blue() + 5, 255)
            );
            borderColor = wxColour(
                wxMin(customColor.Red() + 30, 255),
                wxMin(customColor.Green() + 30, 255),
                wxMin(customColor.Blue() + 30, 255)
            );
        } else {
            // Default dark mode colors
            backgroundColor = wxColour(45, 45, 48);
            foregroundColor = wxColour(230, 230, 230);
            menuBackgroundColor = wxColour(50, 50, 55);
            menuForegroundColor = wxColour(230, 230, 230);
            selectionBackgroundColor = wxColour(65, 105, 225);
            selectionForegroundColor = wxColour(255, 255, 255);
            panelBackgroundColor = wxColour(45, 45, 48);
            borderColor = wxColour(70, 70, 75);
        }
    } else {
        // Light mode colors (default wxWidgets colors)
        backgroundColor = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
        foregroundColor = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
        menuBackgroundColor = wxSystemSettings::GetColour(wxSYS_COLOUR_MENU);
        menuForegroundColor = wxSystemSettings::GetColour(wxSYS_COLOUR_MENUTEXT);
        selectionBackgroundColor = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);
        selectionForegroundColor = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT);
        panelBackgroundColor = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
        borderColor = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNSHADOW);
    }
} 