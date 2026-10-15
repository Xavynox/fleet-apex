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

#include <Geode/DefaultInclude.hpp>

#include "FleetCore.h"

using namespace geode;
using namespace fleetapex;

// Global state
static std::unique_ptr<FleetCore> g_core = nullptr;

// Mod lifecycle handlers using Geode's documented pattern
// Geode v5.11.1 defines: $on_mod(Loaded) - called when mod is loaded
// Geode v5.11.1 has NO $on_mod(Unload) - use RAII / RAII pattern for cleanup
$on_mod(Loaded) {
    log::info("FLEET APEX v0.2 loading...");
    
    try {
        // Initialize core with diagnostic defaults
        auto config = FleetCore::Config();
        config.enableProfiling = true;
        config.enableLevelAnalysis = true;
        config.enableAutoOptimization = false;
        
        // v0.2 diagnostic mode: SAFETY FIRST
        config.defaultMode = AdaptiveEngine::PerformanceMode::SAFE;
        config.enableCPULiberation = false;  // OFF for diagnostic
        config.enableGPUAssist = false;      // OFF for diagnostic
        config.enableDebugLogging = false;
        
        g_core = std::make_unique<FleetCore>();
        g_core->setConfig(config);
        
        if (!g_core->initialize()) {
            log::error("FLEET APEX v0.2 failed to initialize");
            return;
        }
        
        log::info("FLEET APEX v0.2 initialized successfully (diagnostic mode)");
        
    #if defined(ANDROID)
        log::info("Running on Android platform");
    #elif defined(_WIN32)
        log::info("Running on Windows platform");
    #elif defined(__linux__)
        log::info("Running on Linux platform");
    #elif defined(__APPLE__)
        log::info("Running on macOS platform");
    #endif
        
    } catch (const std::exception& e) {
        log::error("FLEET APEX v0.2 exception during load: {}", e.what());
    }
}

// Geode v5.11.1 has NO $on_mod(Unload) - cleanup via RAII pattern
// g_core unique_ptr destructor handles cleanup automatically
// when the mod is unloaded by Geode

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
