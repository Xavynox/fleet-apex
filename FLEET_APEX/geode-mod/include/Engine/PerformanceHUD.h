/**
 * FLEET APEX v0.3 - In-Game Performance HUD
 * 
 * A lightweight, diagnostic-only HUD using verified Geode/Cocos2d APIs.
 * Displays actual profiler data with clear unavailable-state labeling.
 * No gameplay behavior changes.
 */

#ifndef FLEET_APEX_HUD_H
#define FLEET_APEX_HUD_H

#include <Geode/Geode.hpp>
#include <memory>
#include "FleetCore.h"

namespace fleetapex {

class PerformanceHUD {
public:
    PerformanceHUD();
    ~PerformanceHUD();
    
    bool initialize();
    void destroy();
    void update();
    void show();
    void hide();
    bool isVisible() const;
    
    // Set reference to core for metrics
    void bindCore(FleetCore* core);
    
    // Configuration
    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }
    
private:
    CCSprite* m_background = nullptr;
    CCLabelTTF* m_titleLabel = nullptr;
    CCLabelTTF* m_fpsLabel = nullptr;
    CCLabelTTF* m_frameTimeLabel = nullptr;
    CCLabelTTF* m_bottleneckLabel = nullptr;
    CCLabelTTF* m_dataStatusLabel = nullptr;
    CCSprite* m_logoSprite = nullptr;
    
    FleetCore* m_core = nullptr;
    bool m_enabled = false;
    bool m_initialized = false;
    
    float m_updateInterval = 0.5f;  // Update HUD every 500ms
    float m_timeSinceLastUpdate = 0.0f;
    
    void buildUI();
    void refreshMetrics();
    void clearMetrics();
};

} // namespace fleetapex

#endif // FLEET_APEX_HUD_H
