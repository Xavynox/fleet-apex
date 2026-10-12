#ifndef FLEET_APEX_APEX_H
#define FLEET_APEX_APEX_H

#include "FleetCore.h"
#include <memory>

namespace fleetapex {

/**
 * Main entry point for FLEET APEX
 * Provides backward-compatible interface for integration
 */
class APEX {
public:
    // Singleton access
    static APEX& getInstance() {
        static APEX instance;
        return instance;
    }
    
    // Initialize APEX
    bool initialize() {
        return m_core->initialize();
    }
    
    // Shutdown
    void shutdown() {
        m_core->shutdown();
    }
    
    // Game lifecycle
    void onLevelLoadStarted(const std::string& levelName, int32_t levelId) {
        m_core->onLevelLoadStarted(levelName, levelId);
    }
    
    void onObjectAdded(int32_t objectId, int32_t x, int32_t y, int32_t type) {
        m_core->onObjectAdded(objectId, x, y, type);
    }
    
    void onLevelLoadComplete() {
        m_core->onLevelLoadComplete();
    }
    
    void onFrameStart() {
        m_core->onFrameStart();
    }
    
    void onFrameEnd() {
        m_core->onFrameEnd();
    }
    
    // Configuration
    void setPerformanceMode(AdaptiveEngine::PerformanceMode mode) {
        Config config = m_core->getConfig();
        config.defaultMode = mode;
        m_core->setConfig(config);
    }
    
    // Access to core systems
    Profiler& getProfiler() { return m_core->getProfiler(); }
    LevelAnalyzer& getAnalyzer() { return m_core->getAnalyzer(); }
    AdaptiveEngine& getEngine() { return m_core->getAdaptiveEngine(); }
    
    // Check status
    bool isReady() const {
        return m_core->isReady();
    }

private:
    APEX() : m_core(std::make_unique<FleetCore>()) {}
    ~APEX() = default;
    APEX(const APEX&) = delete;
    APEX& operator=(const APEX&) = delete;
    
    std::unique_ptr<FleetCore> m_core;
};

} // namespace fleetapex

#endif // FLEET_APEX_APEX_H
