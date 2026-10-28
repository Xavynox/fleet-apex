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
 * Note: CPU Liberation, GPU Assist, and Automatic Optimization are DISABLED
 * by default per v0.4 requirements. These optimizations should only be
 * enabled in controlled testing environments, not in production gameplay.
 * HUD remains disabled by default.
 */

#include <Geode/DefaultInclude.hpp>

#include <Geode/modify/CCDirector.hpp>
#include <chrono>

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
        // Initialize core with v0.4 diagnostic defaults
        // All optimizations remain disabled by default per v0.4 requirements
        auto config = FleetCore::Config();
        config.enableProfiling = true;
        config.enableLevelAnalysis = true;
        config.enableAutoOptimization = false;  // DISABLED by default for v0.4
        
        // v0.4 diagnostic mode: SAFE mode, all optimizations OFF
        config.defaultMode = AdaptiveEngine::PerformanceMode::SAFE;
        config.enableCPULiberation = false;   // DISABLED by default
        config.enableGPUAssist = false;       // DISABLED by default
        config.enableDebugLogging = false;    // Diagnostic output only when needed
        
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

// Frame timing hooks - using Geode's $modify hooking system
// Hook: CCDirector::drawScene() - called every frame by Cocos2d-x
// See: https://docs.geode-sdk.org/classes/cocos2d/CCDirector/
// See: https://docs.geode-sdk.org/tutorials/modify/

// Frame-timing state (static, lives for mod lifetime)
namespace {
    bool s_frameHookRegistered = false;
    std::chrono::steady_clock::time_point s_lastFrameTime;
    bool s_hasFirstFrame = false;
}

// Hook CCDirector::drawScene() to record per-frame callback intervals
$modify(CCDirector) {
    // Hook drawScene() - called every frame by Cocos2d-x
    // This is a per-frame callback; we record timestamps and compute intervals
    // without measuring the hook's own execution duration as frame time.
    void drawScene() {
        // Record monotonic timestamp at entry
        auto now = std::chrono::steady_clock::now();

        // If we have a previous frame, compute interval
        if (s_hasFirstFrame) {
            auto interval = std::chrono::duration<double, std::milli>(now - s_lastFrameTime).count();

            // Record externally measured frame-callback interval via dedicated API
            // This is NOT beginFrame()/endFrame() - it records frame interval, not callback duration
            if (g_core && g_core->isReady() && g_core->getConfig().enableProfiling) {
                g_core->getProfiler().recordFrameInterval(interval);
            }
        }

        // Update timestamp for next frame
        s_lastFrameTime = now;
        s_hasFirstFrame = true;

        // Invoke original method exactly once per intercepted call
        // This preserves original behavior and prevents infinite recursion
        CCDirector::drawScene();
    }
};

// FrameHooks registration helper - kept for compatibility with existing code
class FrameHooks {
public:
    FrameHooks() {
        m_frameCallbackRegistered = false;
    }

    void registerFrameCallback() {
        if (m_frameCallbackRegistered) return;
        s_frameHookRegistered = true;
        m_frameCallbackRegistered = true;
    }

    void unregisterFrameCallback() {
        m_frameCallbackRegistered = false;
        s_frameHookRegistered = false;
    }

private:
    bool m_frameCallbackRegistered = false;
};

// For actual Gameplay Dash integration, the following would be implemented:
// - Hook into object creation functions (requires verified GD object hooks)
// - Hook into frame timing functions (requires verified Geode hook for GD's main loop)
//
// The C++ implementation provides the full diagnostic framework.
// Actual hooks require reverse engineering / SDK discovery and
// will be implemented when the Geode SDK is properly available.
//
// FrameHooks instance instantiated during mod load.
