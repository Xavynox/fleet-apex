#include "CPULiberationEngine.h"
#include <algorithm>

namespace fleetapex {

CPULiberationEngine::CPULiberationEngine(Profiler& profiler, LevelAnalyzer& analyzer)
    : m_profiler(profiler), m_analyzer(analyzer) {
}

void CPULiberationEngine::enable() {
    m_isActive = true;
}

void CPULiberationEngine::disable() {
    m_isActive = false;
}

bool CPULiberationEngine::isActive() const {
    return m_isActive;
}

std::map<std::string, bool> CPULiberationEngine::analyze() {
    std::map<std::string, bool> report;
    
    // Get objects from analyzer
    const auto& objects = m_analyzer.getObjects();
    
    // Check for redundant processing
    auto static_objects = m_analyzer.findStaticObjects();
    auto dynamic_objects = m_analyzer.findPotentiallyCullable();
    
    // Get gameplay critical objects
    size_t gameplay_critical = 0;
    for (const auto& obj : objects) {
        if (obj.isGameplayCritical) gameplay_critical++;
    }
    
    // Check for redundant visibility checks (UI elements)
    size_t visible_objects = 0;
    for (const auto& obj : objects) {
        if (obj.effectCat == LevelAnalyzer::EffectCategory::UI_ELEMENT) {
            visible_objects++;
        }
    }
    
    // Check for high CPU cost regions
    auto high_cost_regions = m_analyzer.getHighCostRegions(15.0);
    
    // Check for memory pressure
    bool memory_pressure = m_profiler.getSnapshot().current.memoryUsageMb > 800;
    
    report = {
        {"redundant_processing", (static_objects.size() > 0 && dynamic_objects.size() > 0)},
        {"visibility_checks", visible_objects > 1000},
        {"static_objects", static_objects.size() > 0},
        {"dynamic_objects", dynamic_objects.size() > 0},
        {"gameplay_critical", gameplay_critical > 0},
        {"memory_pressure", memory_pressure},
        {"high_cost_regions", high_cost_regions.size() > 0},
        {"can_pool_objects", static_objects.size() > 50},
        {"can_cache_static", static_objects.size() > 0},
        {"needs_processing_reduction", dynamic_objects.size() > 10000},
        {"can_preprocess", high_cost_regions.size() > 0},
        {"can_optimize_visibility", visible_objects > 500},
        {"can_optimize_triggers", std::count_if(objects.begin(), objects.end(), 
                         [](const auto& obj) { return obj.type == LevelAnalyzer::ObjectType::TRIGGER; }) > 0}
    };
    
    return report;
}

std::vector<CPULiberationEngine::CPULiberationAction> CPULiberationEngine::generateOptimizationActions() {
    std::vector<CPULiberationAction> actions;
    
    if (!m_isActive) return actions;
    
    auto analysis = analyze();
    
    if (analysis["can_pool_objects"]) {
        actions.push_back(CPULiberationAction::INCREASE_OBJECT_POOLING);
    }
    
    if (analysis["can_cache_static"]) {
        actions.push_back(CPULiberationAction::STATIC_CACHING);
    }
    
    if (analysis["needs_processing_reduction"]) {
        actions.push_back(CPULiberationAction::REDUCE_REDUNDANT_PROCESSING);
    }
    
    if (analysis["can_preprocess"]) {
        actions.push_back(CPULiberationAction::SAFE_PREPROCESSING);
    }
    
    if (analysis["can_optimize_visibility"]) {
        actions.push_back(CPULiberationAction::OPTIMIZE_VISIBILITY_CHECKS);
        actions.push_back(CPULiberationAction::OPTIMIZE_VISIBILITY_SCANS);
    }
    
    if (analysis["can_optimize_triggers"]) {
        actions.push_back(CPULiberationAction::OPTIMIZE_TRIGGER_PROCESSING);
    }
    
    return actions;
}

bool CPULiberationEngine::executeAction(CPULiberationAction action) {
    if (!m_isActive) return false;
    
    // Placeholder - actual implementation would interface with game engine
    m_optimizationHistory.push_back(action == CPULiberationAction::INCREASE_OBJECT_POOLING ? 
                                   "INCREASE_OBJECT_POOLING" :
                                   action == CPULiberationAction::STATIC_CACHING ? 
                                   "STATIC_CACHING" : 
                                   action == CPULiberationAction::REDUCE_REDUNDANT_PROCESSING ?
                                   "REDUCE_REDUNDANT_PROCESSING" :
                                   action == CPULiberationAction::SAFE_PREPROCESSING ?
                                   "SAFE_PREPROCESSING" :
                                   action == CPULiberationAction::OPTIMIZE_VISIBILITY_CHECKS ?
                                   "OPTIMIZE_VISIBILITY_CHECKS" :
                                   action == CPULiberationAction::OPTIMIZE_VISIBILITY_SCANS ?
                                   "OPTIMIZE_VISIBILITY_SCANS" :
                                   action == CPULiberationAction::OPTIMIZE_TRIGGER_PROCESSING ?
                                   "OPTIMIZE_TRIGGER_PROCESSING" :
                                   "UNKNOWN");
                                   
    return true;
}

bool CPULiberationEngine::executeActions(const std::vector<CPULiberationAction>& actions) {
    if (!m_isActive) return false;
    
    bool allSuccess = true;
    for (const auto& action : actions) {
        if (!executeAction(action)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

std::vector<std::string> CPULiberationEngine::getOptimizationHistory() const {
    return m_optimizationHistory;
}

std::string CPULiberationEngine::getStatus() const {
    return m_isActive ? "ENABLED" : "DISABLED";
}

} // namespace fleetapex
