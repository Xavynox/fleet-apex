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
            log::error("Failed to initialize FLEET APEX core");
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

$on_mod(Unload) {
    log::info("FLEET APEX v0.2 unloading");
    if (g_core) {
        g_core->shutdown();
        g_core.reset();
    }
}
