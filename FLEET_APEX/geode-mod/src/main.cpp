/**
 * FLEET APEX v0.2 - Geode Mod Entry Point
 * 
 * Diagnostic-Only First Release for Geometry Dash
 * 
 * This is the main entry point for the FLEET APEX Geode mod.
 * It initializes the engine and provides the main loop.
 * 
 * Phase 1 v0.2: CPU↔GPU Cooperative Foundation (Diagnostic Mode)
 * 
 * Note: In this diagnostic-only release, optimizations are DISABLED.
 * The engine measures performance but does not apply optimizations
 * to ensure zero gameplay changes while validating the architecture.
 */

#include "FleetCore.h"

#include <Geode/Geode.hpp>
#include <Geode/loader/Mod.hpp>

using namespace geode;
using namespace fleetapex;

class FleetAPEXModImpl {
private:
    std::unique_ptr<FleetCore> m_core;
    bool m_isEnabled = true;
    bool m_isInitialized = false;

public:
    FleetAPEXModImpl() : m_core(std::make_unique<FleetCore>()) {}

    ~FleetAPEXModImpl() {
        if (m_isInitialized) {
            shutdown();
        }
    }

    bool initialize() {
        if (m_isInitialized) return true;

        log::info().post("FLEET APEX v0.2 initializing...");
        
        // Load mod settings - diagnostics only, optimizations off
        FleetCore::Config config;
        config.enableProfiling = Mod::get()->getBool("enable_profiling", true);
        config.enableLevelAnalysis = Mod::get()->getBool("enable_level_analysis", true);
        config.enableAutoOptimization = Mod::get()->getBool("enable_auto_optimization", true);
        
        // v0.2 diagnostic mode: SAFETY FIRST
        config.defaultMode = AdaptiveEngine::PerformanceMode::SAFE;
        config.enableCPULiberation = false;  // OFF for diagnostic
        config.enableGPUAssist = false;      // OFF for diagnostic
        
        config.enableDebugLogging = Mod::get()->getBool("enable_debug_logging", Mod::get()->getBool("debug_logs", false));
        
        m_core->setConfig(config);

        if (!m_core->initialize()) {
            log::error().post("Failed to initialize FLEET APEX core");
            return false;
        }

        m_isInitialized = true;
        log::info().post("FLEET APEX v0.2 initialized successfully (diagnostic mode)");
        return true;
    }

    void shutdown() {
        if (!m_isInitialized) return;
        
        log::info().post("Shutting down FLEET APEX...");
        m_core->shutdown();
        m_isInitialized = false;
    }

    void onFrameStart() {
        if (!m_isInitialized || !m_isEnabled) return;
        m_core->onFrameStart();
    }

    void onFrameEnd() {
        if (!m_isInitialized || !m_isEnabled) return;
        m_core->onFrameEnd();
    }

    FleetCore* getCore() { return m_core.get(); }
};

// Global mod instance
static std::unique_ptr<FleetAPEXModImpl> g_mod;

// Frame hook implementation
void onFrameStartHook() {
    if (g_mod) {
        g_mod->onFrameStart();
    }
}

void onFrameEndHook() {
    if (g_mod) {
        g_mod->onFrameEnd();
    }
}

// Mod entry point
$on_load
void init() {
    log::info() << "FLEET APEX v0.2 loading...";
    
    try {
        g_mod = std::make_unique<FleetAPEXModImpl>();
        
        if (!g_mod->initialize()) {
            log::error() << "FLEET APEX v0.2 failed to initialize";
            g_mod.reset();
            return;
        }
        
        log::info() << "FLEET APEX v0.2 loaded successfully";
        
#if defined(ANDROID)
        log::info() << "Running on Android platform";
#elif defined(_WIN32)
        log::info() << "Running on Windows platform";
#elif defined(__linux__)
        log::info() << "Running on Linux platform";
#elif defined(__APPLE__)
        log::info() << "Running on macOS platform";
#endif

    } catch (const std::exception& e) {
        log::error() << "FLEET APEX v0.2 exception: " << e.what();
        g_mod.reset();
    }
}

$on_unload
void shutdown() {
    log::info() << "FLEET APEX v0.2 unloading";
    g_mod.reset();
}

// Frame timing hooks - using Geode's hooking system
class FrameHooks {
public:
    FrameHooks() {
        // These hooks would be implemented based on actual GD version
        // At this diagnostic stage, we're just setting up the structure
    }
};

// For actual Gameplay Dash integration, the following would be implemented:
// - Hook into gd::GameScene::create() for level load
// - Hook into object creation functions
// - Hook into frame timing functions
// 
// The C++ implementation provides the full diagnostic framework.
// Actual hooks require reverse engineering / SDK discovery and
// will be implemented when the Geode SDK is properly available.
