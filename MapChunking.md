# Map Chunking System Documentation

## 1. Overview

The Map Chunking System enables efficient large map handling in Remere's Map Editor (RME) while providing advanced collaborative editing features through chunk-based management.

### 1.1 Key Features
- Chunk-based map management (128x128 tiles)
- Dynamic chunk loading/unloading
- Empty chunk optimization
- Minimap-based chunk visualization and control
- Advanced access control and collaboration
- Backward compatibility with OTBM format

### 1.2 System Architecture
```
Client Layer:      [Minimap UI] <-> [Chunk Manager] <-> [Local Cache]
                         ↕               ↕                ↕
Network Layer:     [Request Handler] <-> [Security Layer] <-> [Sync Manager]
                         ↕               ↕                ↕
Server Layer:      [Access Control] <-> [Chunk Storage] <-> [Version Control]
```

## 2. Technical Specifications

### 2.1 Map Limitations
- Maximum dimensions: 65535 x 65535 x 15
- Chunk size: 128 x 128 tiles
- Chunks per floor: 512 x 512
- Total possible chunks: 3,932,160 (512 x 512 x 15)

### 2.2 Core Structures

#### Chunk Data Structure
```cpp
struct MapChunk {
    static const int CHUNK_SIZE = 128;
    
    // Location
    uint16_t baseX;
    uint16_t baseY;
    uint8_t floor;
    
    // State
    bool isLoaded;
    bool isDirty;
    bool isEmpty;
    bool isLocked;
    
    // Access Control
    uint32_t lockedBy;
    uint32_t lastAccessed;
    uint32_t lastModified;
    std::string password;
    
    // Data
    std::vector<Tile*> tiles;
    uint32_t checksum;
};
```

#### Access Control
```cpp
enum class ChunkAccess {
    NONE,       // Invisible/inaccessible
    READ_ONLY,  // View only
    WRITE,      // Can modify
    OWNER,      // Full control
    HOST        // Server control
};

struct ChunkAccessInfo {
    ChunkAccess level;
    std::string owner;
    std::string password;
    std::vector<std::string> authorized_users;
    bool auto_download;
    wxColor outline_color;
};
```

## 3. File System

### 3.1 Directory Structure
```
/mapchunks/
  ├── mapname/
  │   ├── manifest.json    # Map metadata
  │   ├── locks.json      # Access control
  │   ├── chunks/         # Chunk data
  │   │   ├── x_y_z.chunk # Individual chunks
  │   │   └── ...
  │   └── backup/         # Automatic backups
  └── ...
```

### 3.2 File Formats

#### Manifest File (manifest.json)
```json
{
    "name": "example_map",
    "version": 1,
    "chunk_size": 128,
    "dimensions": {
        "width": 65535,
        "height": 65535,
        "floors": 15
    },
    "chunks": [
        {
            "x": 0,
            "y": 0,
            "z": 7,
            "file": "0_0_7.chunk",
            "checksum": "abc123",
            "isEmpty": false,
            "lastModified": 1234567890
        }
    ]
}
```

#### Access Control File (locks.json)
```json
{
    "global_lock": true,
    "lock_owner": "admin",
    "unlocked_chunks": [
        {
            "x": [0, 10],
            "y": [0, 10],
            "z": [7, 7],
            "access": ["user1", "user2"],
            "password": "optional_hash"
        }
    ],
    "locked_chunks": [
        {
            "x": 15,
            "y": 15,
            "z": 7,
            "locked_by": "user1",
            "timestamp": 1234567890
        }
    ]
}
```

## 4. Minimap Integration

### 4.1 Minimap Window
```cpp
class MinimapWindow : public wxPanel {
public:
    // Visualization
    void DrawChunkGrid(bool enabled);
    void SetChunkOutlineColor(const Position& pos, const wxColor& color);
    void HighlightChunk(const Position& pos, const wxColor& color, int duration);
    void AnimateChunkLoad(const Position& pos);
    
    // Management
    void RequestChunkDownload(const Position& pos);
    void MarkChunkOutdated(const Position& pos);
    void UpdateChunkAccess(const Position& pos, const ChunkAccess& access);
    
    // Events
    void OnRightClick(wxMouseEvent& event);
    void OnChunkHover(wxMouseEvent& event);
    void OnChunkContextMenu(wxContextMenuEvent& event);
};
```

### 4.2 Visual Features
- Checkerboard pattern for chunk boundaries
- Custom chunk outline colors (merge visually when same)
- Status indicators:
  - Red outline: Outdated/needs update
  - Green animation: Downloading
  - Lock icon: Access restricted
  - User indicator: Current editor
- Hover tooltips showing:
  - Chunk status (empty/locked/etc)
  - Owner/editor information
  - Access rights
  - Last modified time

### 4.3 Context Menu
- Download Chunk
- Request Lock
- Set Access Rights (Host)
- Set Chunk Color
- View Properties
- Accept/Reject Changes
- Toggle Auto-Download

## 5. Collaboration System

### 5.1 Host Features
- Invisible to clients
- Global map locking
- Chunk access management
- Change approval system
- Client monitoring
- Password protection

### 5.2 Client Features
- Chunk discovery via minimap
- Visual status indicators
- Change submission
- Custom chunk coloring
- Auto-update preferences
- Access requests

### 5.3 Chunk States
```cpp
enum class ChunkState {
    EMPTY,            // No data
    LOCKED,           // Not accessible
    READABLE,         // Can download
    WRITABLE,         // Can modify
    MODIFIED,         // Has changes
    DOWNLOADING,      // In progress
    UPLOADING,        // Sending
    PENDING_APPROVAL  // Awaiting review
};
```

## 6. Implementation Plan

### 6.1 Phase 1: Core Features
1. Chunk data structure
2. File format implementation
3. Basic loading/saving
4. OTBM compatibility

### 6.2 Phase 2: Minimap Integration
1. Chunk grid visualization
2. Selection handling
3. Context menu system
4. Status indicators
5. Animation system
6. Hover tooltips

### 6.3 Phase 3: Access Control
1. Access levels
2. Password protection
3. Host interface
4. Client requests
5. Change tracking
6. Approval system

### 6.4 Phase 4: Optimization
1. Chunk caching
2. Background loading
3. Delta updates
4. Compression
5. Memory management
6. Performance monitoring

## 7. Security

### 7.1 Access Control
- Server-side validation
- Chunk-level permissions
- Password protection
- Change verification
- Activity logging

### 7.2 Data Protection
- Encrypted transmission
- Checksum verification
- Token management
- Change history
- Automatic backups

## 8. Future Extensions

### 8.1 Planned Features
- Region-based access
- Advanced animations
- Collaborative tools
- Conflict resolution
- Analytics system
- Extended security

### 8.2 Performance Enhancements
- Predictive loading
- Smart caching
- Network optimization
- Memory management
- Background processing

