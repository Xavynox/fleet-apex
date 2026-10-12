#ifndef FLEET_APEX_CPU_LIBERATION_ENGINE_H
#define FLEET_APEX_CPU_LIBERATION_ENGINE_H

#include "Profiler.h"
#include "LevelAnalyzer.h"
#include <map>
#include <vector>
#include <string>

namespace fleetapex {

/**
 * CPU Liberation Engine
 * Reduces unnecessary CPU workload while preserving gameplay.
 * A first-class subsystem focusing on safe CPU optimization.
 */

class CPULiberationEngine {
public:
    // Object types for classification
    enum class ObjectType {
        PLAYER, CUBE, SPHERE, UFO, WALKER, SHIP, BALL, DIAMOND, ORB, TRIGGER, OTHER
    };

    // CPU optimization actions
    enum class CPULiberationAction {
        NONE, INCREASE_OBJECT_POOLING, STATIC_CACHING, REDUCE_REDUNDANT_PROCESSING,
        SAFE_PREPROCESSING, OPTIMIZE_VISIBILITY_CHECKS, BATCH_SIMILAR_OPERATIONS,
        DELAY_NON_CRITICAL_UPDATES, OPTIMIZE_VISIBILITY_SCANS, OPTIMIZE_TRIGGER_PROCESSING
    };

private:
    Profiler& m_profiler;
    LevelAnalyzer& m_analyzer;
    bool m_isActive = false;
    std::vector<std::string> m_optimizationHistory;

public:
    CPULiberationEngine(Profiler& profiler, LevelAnalyzer& analyzer);
    ~CPULiberationEngine() = default;

    void enable();
    void disable();
    bool isActive() const;

    std::map<std::string, bool> analyze();
    std::vector<CPULiberationAction> generateOptimizationActions();
    bool executeAction(CPULiberationAction action);
    bool executeActions(const std::vector<CPULiberationAction>& actions);

    std::vector<std::string> getOptimizationHistory() const;
    std::string getStatus() const;
};

} // namespace fleetapex

#endif // FLEET_APEX_CPU_LIBERATION_ENGINE_H
