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

/*
 * PROPOSED ENHANCEMENT: DYNAMIC MULTI-CELL SPRITE ALLOCATION IN PALETTE
 * 
 * Current implementation limitation:
 * Currently, each creature is assigned to a single grid cell in the palette,
 * regardless of its natural size. This forces larger creatures (64x64, 96x96)
 * to be scaled down to fit in the standard grid, losing detail.
 * 
 * Proposed solution:
 * Allow larger sprites to occupy multiple grid cells based on their natural dimensions.
 * 
 * Implementation steps:
 * 
 * 1. Modify the CreatureSeamlessGridPanel class:
 *    - Add a 'sprite_cell_span' map that stores how many cells each sprite occupies 
 *      (both horizontally and vertically)
 *    - E.g., a 32x32 sprite has span 1x1, a 64x64 has span 2x2, a 96x96 has span 3x3
 * 
 * 2. Update the RecalculateGrid method:
 *    - Consider sprite spans when calculating the grid layout
 *    - Adjust cell positions so large sprites don't overlap
 *    - Create a 2D grid map that tracks which cells are occupied
 * 
 * 3. Update the GetSpriteIndexAt method:
 *    - Map x,y coordinates to the correct sprite, considering spans
 *    - Handle click events anywhere within a sprite's area
 * 
 * 4. Update the DrawItemsToPanel method:
 *    - Calculate each sprite's position based on its span
 *    - Draw larger sprites across multiple grid cells
 *    - Consider a staggered grid for more efficient space usage
 * 
 * 5. Modify sprite placement algorithm:
 *    - Implement a bin-packing algorithm for optimal space usage
 *    - Consider a simple grid-based approach:
 *      a. Sort sprites by size (largest first)
 *      b. For each sprite, find the first available grid position
 *      c. Mark the cells it occupies as taken
 * 
 * 6. Update highlighting and selection:
 *    - Highlight/select the entire area occupied by a sprite
 *    - Use a special border style to indicate multi-cell sprites
 * 
 * 7. Grid navigation improvements:
 *    - Add keyboard navigation that respects sprite boundaries
 *    - Ensure tabbing and arrow keys work logically with varying sprite sizes
 * 
 * Implementation example for determining cell span:
 * 
 * int GetCellSpan(int naturalSize) {
 *     if (naturalSize <= 32) return 1;
 *     if (naturalSize <= 64) return 2;
 *     if (naturalSize <= 96) return 3;
 *     return (naturalSize + 31) / 32; // Round up to nearest cell
 * }
 * 
 * The end result would be a more space-efficient grid where larger creatures
 * get more screen real estate, showing them in their full detailed glory
 * without being constrained to a standard grid cell.
 */

#include "main.h"
#include "creature_sprite_manager.h"
#include "creatures.h"
#include "creature.h"
#include "gui.h"
#include "creature_brush.h"
#include "graphics.h"

CreatureSpriteManager g_creature_sprites;

CreatureSpriteManager::CreatureSpriteManager() {
    // Constructor
}

CreatureSpriteManager::~CreatureSpriteManager() {
    clear();
}

void CreatureSpriteManager::clear() {
    // Delete all cached sprites
    for (auto& pair : sprite_cache) {
        delete pair.second;
    }
    sprite_cache.clear();
}

wxBitmap* CreatureSpriteManager::getSpriteBitmap(int looktype, int width, int height) {
    // Create a combined key that includes dimensions and looktype
    // This ensures we have a unique cache for each size
    std::string key = std::to_string(looktype) + "_" + std::to_string(width) + "x" + std::to_string(height);
    
    // Check if we already have this bitmap
    auto it = sprite_cache.find(key);
    if (it != sprite_cache.end()) {
        return it->second;
    }
    
    // Create a new bitmap
    wxBitmap* bitmap = createSpriteBitmap(looktype, width, height);
    if (bitmap) {
        sprite_cache[key] = bitmap;
    }
    
    return bitmap;
}

wxBitmap* CreatureSpriteManager::getSpriteBitmap(int looktype, int head, int body, int legs, int feet, int width, int height) {
    // Create a combined key that includes dimensions, looktype and outfit colors
    std::string key = std::to_string(looktype) + "_" + 
                      std::to_string(head) + "_" + 
                      std::to_string(body) + "_" + 
                      std::to_string(legs) + "_" + 
                      std::to_string(feet) + "_" + 
                      std::to_string(width) + "x" + std::to_string(height);
    
    // Check if we already have this bitmap
    auto it = sprite_cache.find(key);
    if (it != sprite_cache.end()) {
        return it->second;
    }
    
    // Create a new bitmap
    wxBitmap* bitmap = createSpriteBitmap(looktype, head, body, legs, feet, width, height);
    if (bitmap) {
        sprite_cache[key] = bitmap;
    }
    
    return bitmap;
}

void CreatureSpriteManager::generateCreatureSprites(const BrushVector& creatures, int width, int height) {
    // Pre-generate sprites for a vector of creature brushes
    for (Brush* brush : creatures) {
        if (brush->isCreature()) {
            CreatureBrush* cb = static_cast<CreatureBrush*>(brush);
            if (cb && cb->getType()) {
                const Outfit& outfit = cb->getType()->outfit;
                int looktype = outfit.lookType;
                if (looktype > 0) {
                    // Check if this has color components
                    if (outfit.lookHead || outfit.lookBody || outfit.lookLegs || outfit.lookFeet) {
                        getSpriteBitmap(looktype, outfit.lookHead, outfit.lookBody, 
                                      outfit.lookLegs, outfit.lookFeet, width, height);
                    } else {
                        getSpriteBitmap(looktype, width, height);
                    }
                }
            }
        }
    }
}

wxBitmap* CreatureSpriteManager::createSpriteBitmap(int looktype, int width, int height) {
    // Find a creature with this looktype to get its full outfit details
    Outfit outfit;
    outfit.lookType = looktype;
    
    for (CreatureDatabase::iterator it = g_creatures.begin(); it != g_creatures.end(); ++it) {
        CreatureType* type = it->second;
        if (type && type->outfit.lookType == looktype) {
            outfit = type->outfit;
            break;
        }
    }
    
    // Now create the bitmap using the full outfit
    return createSpriteBitmap(looktype, outfit.lookHead, outfit.lookBody, 
                             outfit.lookLegs, outfit.lookFeet, width, height);
}

wxBitmap* CreatureSpriteManager::createSpriteBitmap(int looktype, int head, int body, int legs, int feet, int width, int height) {
    // Get the sprite from graphics system
    GameSprite* spr = g_gui.gfx.getCreatureSprite(looktype);
    if (!spr) {
        return nullptr;
    }
    
    // Calculate the natural sprite size and scaling needed
    // These are publicly available properties of the GameSprite class
    int natural_width = spr->width > 0 ? spr->width : 32;
    int natural_height = spr->height > 0 ? spr->height : 32;
    
    // Calculate the natural size as the maximum of width and height
    // This ensures square display in the palette
    int natural_size = std::max(natural_width, natural_height);
    
    // Round up to the nearest standard size (32, 64, 96, 128, etc.)
    if (natural_size <= 32) {
        natural_size = 32;
    } else if (natural_size <= 64) {
        natural_size = 64;
    } else if (natural_size <= 96) {
        natural_size = 96;
    } else if (natural_size <= 128) {
        natural_size = 128;
    } else {
        natural_size = ((natural_size + 31) / 32) * 32; // Round up to nearest multiple of 32
    }
    
    // Fallback to heuristics if sprite dimensions are not available
    if (natural_size == 32 && looktype >= 800) {
        natural_size = 64; // Many high looktype monsters are larger
    }
    
    // Fallback for very high looktypes known to be larger
    if (looktype >= 1200 && natural_size < 96) {
        natural_size = 96; // Some newer monsters can be 96x96
    }
    
    bool is_large = (natural_size > 32);
    
    // Create the bitmap with target dimensions
    wxBitmap* bitmap = new wxBitmap(width, height);
    wxMemoryDC dc(*bitmap);
    
    // Set transparent background (magenta)
    dc.SetBackground(wxBrush(wxColour(255, 0, 255)));
    dc.Clear();
    
    // Draw the creature sprite centered in the bitmap
    if (spr) {
        // Calculate offsets to center the sprite
        int offsetX = (width - natural_size) / 2;
        int offsetY = (height - natural_size) / 2;
        
        // Adjust offsets for small target sizes
        if (width < natural_size) {
            offsetX = (width - std::min(width, 32)) / 2; // Minimum size
        }
        if (height < natural_size) {
            offsetY = (height - std::min(height, 32)) / 2; // Minimum size
        }
        
        // Set up the outfit with colors
        Outfit full_outfit;
        full_outfit.lookType = looktype;
        full_outfit.lookHead = head;
        full_outfit.lookBody = body;
        full_outfit.lookLegs = legs;
        full_outfit.lookFeet = feet;
        
        // Determine the sprite size to use based on target dimensions and natural size
        SpriteSize sprite_size = SPRITE_SIZE_32x32;
        if (width < 32 || height < 32) {
            sprite_size = SPRITE_SIZE_16x16;
        } else if (is_large) {
            sprite_size = SPRITE_SIZE_64x64;
        }
        
        // Try to draw with outfit colors or fall back to regular drawing
        if (full_outfit.lookHead > 0 || full_outfit.lookBody > 0 || 
            full_outfit.lookLegs > 0 || full_outfit.lookFeet > 0) {
            // Check if the DrawOutfit method is available in the game's API
            // If not available, fall back to regular DrawTo
            try {
                // Try to use the dedicated outfit drawing method
                spr->DrawOutfit(&dc, sprite_size, offsetX, offsetY, full_outfit);
            } catch (...) {
                // Fallback in case DrawOutfit isn't available in this version
                spr->DrawTo(&dc, sprite_size, offsetX, offsetY);
            }
        } else {
            // Regular sprite without outfit colors
            spr->DrawTo(&dc, sprite_size, offsetX, offsetY);
        }
    }
    
    // Ensure transparency works
    wxImage img = bitmap->ConvertToImage();
    img.SetMaskColour(255, 0, 255);
    
    // Always scale to requested size to ensure proper display in palette
    if (width != img.GetWidth() || height != img.GetHeight()) {
        img = img.Scale(width, height, wxIMAGE_QUALITY_HIGH);
    }
    
    *bitmap = wxBitmap(img);
    
    return bitmap;
} 