/**
 * FLEET APEX v0.2 - FleetCore Header
 * 
 * Core orchestration module for the FLEET APEX Geode mod.
 */

#ifndef FLEET_APEX_FLEET_CORE_H
#define FLEET_APEX_FLEET_CORE_H

#include "Profiler.h"
#include "LevelAnalyzer.h"
#include "AdaptiveEngine.h"

#include <cstdint>
#include <memory>
#include <string>
#include <functional>

namespace fleetapex {

class FleetCore {
public:
    enum class InitStatus {
        NOT_INITIALIZED,
        INITIALIZING,
        READY,
        ERROR
    };

    enum class Platform {
        Android,
        DESKTOP_WINDOWS,
        DESKTOP_LINUX,
        DESKTOP_MACOS,
        UNKNOWN
    };

    struct Config {
        // Profiling
        bool enableProfiling = true;
        size_t frameHistorySize = 120;
        
        // Analysis
        bool enableLevelAnalysis = true;
        
        // Adaptive optimization
        bool enableAutoOptimization = true;
        
        // Performance mode (diagnostic-only for first release)
        AdaptiveEngine::PerformanceMode defaultMode = AdaptiveEngine::PerformanceMode::SAFE;
        
        // Debug/Developer settings
        bool enableDebugLogging = false;
        bool enablePerformanceHUD = false;
        bool enablePerformanceMap = false;
        bool enableLevelReport = false;
        
        // Engine control (v0.2)
        bool enableCPULiberation = false;  // Off for diagnostic release
        bool enableGPUAssist = false;      // Off for diagnostic release
    };

private:
    InitStatus m_status = InitStatus::NOT_INITIALIZED;
    Platform m_platform = Platform::UNKNOWN;
    
    std::unique_ptr<Profiler> m_profiler;
    std::unique_ptr<LevelAnalyzer> m_analyzer;
    std::unique_ptr<AdaptiveEngine> m_adaptiveEngine;
    
    Config m_config;
    std::string m_version = "0.2.0";
    
    // Lifecycle callbacks
    std::function<void()> m_onInitComplete;
    std::function<void(const std::string&)> m_onError;
    
    // Internal helpers
    bool initializeProfiler();
    bool initializeAnalyzer();
    bool initializeAdaptiveEngine();
    void updatePlatform();
    
public:
    FleetCore();
    ~FleetCore();

    // Core lifecycle
    bool initialize();
    void shutdown();
    InitStatus getStatus() const { return m_status; }
    
    // Configuration
    void setConfig(const Config& config);
    const Config& getConfig() const { return m_config; }
    Config& getConfig() { return m_config; }
    
    // Accessors
    Profiler& getProfiler() const { return *m_profiler; }
    LevelAnalyzer& getAnalyzer() const { return *m_analyzer; }
    AdaptiveEngine& getAdaptiveEngine() const { return *m_adaptiveEngine; }
    
    // Platform info
    Platform getPlatform() const { return m_platform; }
    std::string getPlatformString() const;
    const std::string& getVersion() const { return m_version; }
    
    // Game lifecycle hooks
    void onLevelLoadStarted(const std::string& levelName, int32_t levelId);
    void onObjectAdded(int32_t objectId, int32_t x, int32_t y, int32_t type);
    void onLevelLoadComplete();
    void onFrameStart();
    void onFrameEnd();
    void onObjectMoved(int32_t objectId, int32_t newX, int32_t newY);
    
    // Update loop
    void update();
    
    // Callbacks
    void setOnInitComplete(std::function<void()> callback) { m_onInitComplete = callback; }
    void setOnError(std::function<void(const std::string&)> callback) { m_onError = callback; }
    
    // Utility
    std::string getFullVersion() const;
    bool isReady() const { return m_status == InitStatus::READY; }
    std::string getStatusString() const;
};

} // namespace fleetapex

#endif // FLEET_APEX_FLEET_CORE_H
