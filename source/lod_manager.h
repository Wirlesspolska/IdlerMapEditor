
#ifndef RME_LOD_MANAGER_H
#define RME_LOD_MANAGER_H

class LODManager {
public:
    enum LODLevel {
        FULL_DETAIL = 0,    // Zoom 1-3
        MEDIUM_DETAIL = 1,  // Zoom 4-7
        GROUND_ONLY = 2     // Zoom 8+
    };

    LODManager() : current_level(FULL_DETAIL) {}
    
    LODLevel getLevelForZoom(double zoom) {
        if(zoom >= 8.0) return GROUND_ONLY;
        if(zoom >= 4.0) return MEDIUM_DETAIL;
        return FULL_DETAIL;
    }
    
    void updateRenderSettings(double zoom) {
        current_level = getLevelForZoom(zoom);
    }
    
    bool isGroundOnly() const {
        return current_level == GROUND_ONLY;
    }
    
    bool isMediumDetail() const {
        return current_level == MEDIUM_DETAIL;
    }

private:
    LODLevel current_level;
};

#endif // RME_LOD_MANAGER_H