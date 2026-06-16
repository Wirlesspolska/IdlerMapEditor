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

#ifndef RME_CREATURE_SPRITE_MANAGER_H_
#define RME_CREATURE_SPRITE_MANAGER_H_

#include "main.h"
#include "creature_brush.h"
#include "graphics.h"
#include <map>

class CreatureSpriteManager {
public:
    CreatureSpriteManager();
    ~CreatureSpriteManager();

    // Get or create a bitmap for a specific creature looktype
    wxBitmap* getSpriteBitmap(int looktype, int width = 32, int height = 32);
    
    // Get or create a bitmap for a specific creature with outfit colors
    wxBitmap* getSpriteBitmap(int looktype, int head, int body, int legs, int feet, int width = 32, int height = 32);
    
    // Pre-generate creature sprites for the palette view
    void generateCreatureSprites(const BrushVector& creatures, int width = 32, int height = 32);
    
    // Clear all cached sprites
    void clear();

private:
    // Cache of generated creature sprites
    std::map<std::string, wxBitmap*> sprite_cache;
    
    // Helper to create a bitmap for a specific looktype
    wxBitmap* createSpriteBitmap(int looktype, int width, int height);
    
    // Helper to create a bitmap with outfit colors
    wxBitmap* createSpriteBitmap(int looktype, int head, int body, int legs, int feet, int width, int height);
};

extern CreatureSpriteManager g_creature_sprites;

#endif // RME_CREATURE_SPRITE_MANAGER_H_ 