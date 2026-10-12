#ifndef FLEET_APEX_MOD_H
#define FLEET_APEX_MOD_H

#include "FleetCore.h"
#include "Profiler.h"
#include "LevelAnalyzer.h"
#include "AdaptiveEngine.h"
#include "BottleneckClassifier.h"
#include "CPULiberationEngine.h"
#include "GPUAssistEngine.h"
#include "CPUGPUBalancer.h"

namespace fleetapex {

/**
 * FLEET APEX v0.2 - Geode Mod Entry Point
 * 
 * Main entry point for the FLEET APEX Geode mod.
 * Integrates with Geometry Dash through Geode's mod system.
 */
class FleetAPEXMod {
private:
    std::unique_ptr<FleetCore> m_core;
    bool m_isInitialized = false;
    
public:
    FleetAPEXMod();
    ~FleetAPEXMod();
    
    bool initialize();
    void shutdown();
    
    void onFrameStart();
    void onFrameEnd();
    
    bool isReady() const;
};

} // namespace fleetapex

#endif // FLEET_APEX_MOD_H
