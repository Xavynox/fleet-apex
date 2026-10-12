/**
 * FLEET APEX v0.2 - Adaptive Engine Header
 */

#ifndef FLEET_APEX_ADAPTIVE_ENGINE_H
#define FLEET_APEX_ADAPTIVE_ENGINE_H

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

namespace fleetapex {

class Profiler;
class LevelAnalyzer;
class BottleneckClassifier;
class CPULiberationEngine;
class GPUAssistEngine;

class AdaptiveEngine {
public:
    enum class PerformanceMode {
        SAFE,
        BALANCED,
        PERFORMANCE,
        TURBO,
        CUSTOM
    };

    struct OptimizationDecision {
        PerformanceMode mode = PerformanceMode::SAFE;
        bool shouldOptimize = false;
        double expectedBenefitMs = 0.0;
        double riskLevel = 0.0;
    };

    struct RuntimeState {
        double frameTimeStability = 0.0;
        int recentSpikeCount = 0;
        double cpuUsageRatio = 0.0;
        double gpuUsageRatio = 0.0;
        uint64_t memoryPressure = 0;
    };

private:
    Profiler& m_profiler;
    LevelAnalyzer& m_analyzer;
    std::unique_ptr<BottleneckClassifier> m_bottleneckClassifier;
    std::unique_ptr<CPULiberationEngine> m_cpuLiberation;
    std::unique_ptr<GPUAssistEngine> m_gpuAssist;
    
    PerformanceMode m_mode = PerformanceMode::SAFE;
    bool m_isOptimized = false;
    bool m_hasEnoughData = false;
    RuntimeState m_runtimeState;
    std::vector<OptimizationDecision> m_pendingDecisions;
    std::vector<OptimizationDecision> m_decisionHistory;

public:
    explicit AdaptiveEngine(Profiler& profiler, LevelAnalyzer& analyzer);
    ~AdaptiveEngine();

    void initialize();
    void update();
    
    void setPerformanceMode(PerformanceMode mode);
    void setTargetFrameRate(double fps);

    const RuntimeState& getRuntimeState() const { return m_runtimeState; }
    bool isOptimized() const { return m_isOptimized; }
    bool hasEnoughData() const { return m_hasEnoughData; }

    const std::vector<OptimizationDecision>& getPendingDecisions() const { return m_pendingDecisions; }
    void clearPendingDecisions() { m_pendingDecisions.clear(); }

    struct Report {
        std::string bottleneck;
        std::string primaryCostSource;
        std::string recommendedAction;
        double confidence;
        bool shouldOptimize;
        std::string status;
    };
    Report getReport() const;
};

} // namespace fleetapex

#endif // FLEET_APEX_ADAPTIVE_ENGINE_H
