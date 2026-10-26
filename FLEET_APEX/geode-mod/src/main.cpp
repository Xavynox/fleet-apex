/**
 * FLEET APEX v0.3 - Geode Mod Entry Point
 * 
 * Configuration Testing Release for Geometry Dash
 * 
 * This is the main entry point for the FLEET APEX Geode mod.
 * It initializes the engine and provides the main loop.
 * 
 * Phase 1 v0.3: Configuration Test Mode
 * 
 * Note: CPU Liberation, GPU Assist, and Automatic Optimization are ENABLED
 * for controlled testing purposes. These optimizations should only be
 * enabled in controlled testing environments, not in production gameplay.
 * HUD remains disabled by default.
 */

#include <Geode/DefaultInclude.hpp>

#include "FleetCore.h"
#include "PerformanceHUD.h"

using namespace geode;
using namespace cocos2d;
using namespace fleetapex;

// Global state
static std::unique_ptr<FleetCore> g_core = nullptr;
static std::unique_ptr<PerformanceHUD> g_hud = nullptr;

// Mod lifecycle handlers using Geode's documented pattern
// Geode v5.11.1 defines: $on_mod(Loaded) - called when mod is loaded
// Geode v5.11.1 has NO $on_mod(Unload) - use RAII / RAII pattern for cleanup
$on_mod(Loaded) {
    log::info("FLEET APEX v0.3 loading...");
    
    try {
        // Initialize core with optimization testing defaults
        auto config = FleetCore::Config();
        config.enableProfiling = true;
        config.enableLevelAnalysis = true;
        config.enableAutoOptimization = true;
        
        // v0.3 testing mode: SAFE mode with optimizations ENABLED for testing
        config.defaultMode = AdaptiveEngine::PerformanceMode::SAFE;
        config.enableCPULiberation = true;   // ENABLED for testing
        config.enableGPUAssist = true;       // ENABLED for testing
        config.enableDebugLogging = true;    // ENABLED for diagnostic output
        
        g_core = std::make_unique<FleetCore>();
        g_core->setConfig(config);
        
        if (!g_core->initialize()) {
            log::error("FLEET APEX v0.3 failed to initialize");
            return;
        }
        
        log::info("FLEET APEX v0.3 initialized successfully (diagnostic mode)");
        
    #if defined(ANDROID)
        log::info("Running on Android platform");
    #elif defined(_WIN32)
        log::info("Running on Windows platform");
    #elif defined(__linux__)
        log::info("Running on Linux platform");
    #elif defined(__APPLE__)
        log::info("Running on macOS platform");
    #endif
        
        // Initialize HUD (disabled by default; enable via config if desired)
        g_hud = std::make_unique<PerformanceHUD>();
        g_hud->bindCore(g_core.get());
        if (!g_hud->initialize()) {
            log::warn("FLEET APEX HUD failed to initialize");
            g_hud.reset();
        } else {
            g_hud->setEnabled(false);  // HUD disabled by default
            log::info("FLEET APEX HUD created (disabled by default)");
        }
        
    } catch (const std::exception& e) {
        log::error("FLEET APEX v0.3 exception during load: {}", e.what());
    }
}

// Geode v5.11.1 has NO $on_mod(Unload) - cleanup via RAII / RAII pattern for cleanup
// g_core and g_hud unique_ptr destructors handle cleanup automatically
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
