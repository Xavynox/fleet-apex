#ifndef FLEET_APEX_FLEET_CORE_H
#define FLEET_APEX_FLEET_CORE_H

#include "Profiler.h"
#include "LevelAnalyzer.h"
#include "AdaptiveEngine.h"
#include <memory>
#include <string>
#include <functional>

namespace fleetapex {

/**
 * FLEET APEX - The Most Advanced Geometry Dash Optimization Mod
 * 
 * Core initialization and orchestration module
 */
class FleetCore {
public:
    enum class InitStatus {
        NOT_INITIALIZED,
        INITIALIZING,
        READY,
        ERROR
    };

    enum class Platform {
        ANDROID,
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
        int maxOptimizationLatencyMs = 1000;
        bool enableAutoOptimization = true;
        
        // Performance mode
        AdaptiveEngine::PerformanceMode defaultMode = AdaptiveEngine::PerformanceMode::BALANCED;
        
        // Debug/Developer settings
        bool enableDebugLogging = false;
        bool enablePerformanceHUD = false;
        bool enablePerformanceMap = false;
        bool enableLevelReport = false;
    };

private:
    InitStatus m_status = InitStatus::NOT_INITIALIZED;
    Platform m_platform = Platform::UNKNOWN;
    
    std::unique_ptr<Profiler> m_profiler;
    std::unique_ptr<LevelAnalyzer> m_analyzer;
    std::unique_ptr<AdaptiveEngine> m_adaptiveEngine;
    
    Config m_config;
    std::string m_version = "0.1.0";
    
    // Lifecycle callbacks
    std::function<void()> m_onInitComplete;
    std::function<void(const std::string&)> m_onError;
    
    // Internal helpers
    bool initializeProfiler();
    bool initializeAnalyzer();
    bool initializeAdaptiveEngine();
    void updatePlatform();
    std::string getPlatformString() const;
    
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
    
    // Accessors
    Profiler& getProfiler() const { return *m_profiler; }
    LevelAnalyzer& getAnalyzer() const { return *m_analyzer; }
    AdaptiveEngine& getAdaptiveEngine() const { return *m_adaptiveEngine; }
    
    // Platform info
    Platform getPlatform() const { return m_platform; }
    std::string getPlatformString() const;
    std::string getVersion() const { return m_version; }
    
    // Game lifecycle hooks
    void onLevelLoadStarted(const std::string& levelName, int32_t levelId);
    void onObjectAdded(int32_t objectId, int32_t x, int32_t y, int32_t type);
    void onLevelLoadComplete();
    void onFrameStart();
    void onFrameEnd();
    
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
