Map Generation UI Task
This task involves creating a comprehensive UI dialog for generating randomized map areas. The generator will allow users to create realistic map regions with various terrain, doodads, buildings, and roads.
Task Requirements
Create a MapGeneratorWindow dialog that:
Allows selecting a region to fill (x1, y1, x2, y2, z)
Provides layers of generation with proper ordering:
Water (outer border)
Ground/terrain
Roads
Buildings
Trees/plants (doodads)
Includes palette-like browsing for each category
Utilizes existing functionalities like ACTION_BORDERIZE for automatic border generation
Shows generation progress
Technical Implementation Notes
Apply
Implementation Strategy
Layer-based Generation:
Start with water as the base layer
Add ground terrain with proper borders
Place roads as paths through the terrain
Position buildings along roads
Scatter doodads (trees, plants) in remaining open areas
Utilize Existing Tools:
Use BorderizeWindow approach for chunk-based processing
Leverage ACTION_BORDERIZE for automatic border generation
Use GroundBrush::reborderizeTile() for clean borders
Brush Selection:
Create mini-palette widgets for each category
Allow multiple brush selection with probability weights
Random Distribution:
Implement noise-based distribution for natural-looking placement
Ensure buildings have proper spacing and orientation
Create road networks that connect buildings
Technical Considerations
Process in chunks to avoid UI freezing
Use the Action system for undo support
Ensure borders between different terrain types are handled properly
Add options for different map styles (forest, desert, mountain, etc.)
Provide randomization seeds for reproducible generation
UI Layout
The dialog should contain:
A notebook with tabs for different generation layers
Controls for region selection
Mini-palettes for brush selection
Generation settings (density, randomness)
Progress tracking
Generate and Cancel buttons
Example Generation Workflow
Apply to main_menubar...
Integration with Main Application
Add a new menu option in the MainMenuBar::OnGenerateMap section to open this dialog, allowing users to generate a full or partial map based on their specifications.
Apply to main_menubar...
This task will significantly enhance the map creation capabilities of the editor, allowing users to quickly generate realistic map regions while maintaining full control over the generation process.