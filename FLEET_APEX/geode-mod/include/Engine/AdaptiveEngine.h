/**
 * FLEET APEX v0.3 - Adaptive Engine Header
 * 
 * Decision and reporting layer.
 * 
 * In v0.3 the engine is OBSERVATIONAL ONLY: it collects and reports
 * diagnostics but never applies gameplay-affecting optimizations.
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
        bool cpuRatioValid = false;
        bool gpuRatioValid = false;
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

    /**
     * Diagnostic report.
     *
     * Every field reflects what was actually measured. Fields that could
     * not be determined are reported as such instead of being filled with
     * plausible-looking defaults.
     */
    struct Report {
        // Observed bottleneck classification ("Stable", "CPU-bound", ...).
        std::string bottleneck;
        
        // Confidence in the classification, 0.0 - 1.0, derived from the
        // number and consistency of collected samples. Zero means the
        // classification could not be made.
        double confidence = 0.0;
        
        // Whether the classification rests on enough samples to be useful.
        bool hasSufficientData = false;
        
        // Number of valid frame samples behind the classification.
        size_t sampleCount = 0;
        
        // Measured values. `has*` flags say whether the value is real.
        double measuredFps = 0.0;
        bool hasFps = false;
        double averageFrameTimeMs = 0.0;
        bool hasAverageFrameTime = false;
        double cpuCallbackMs = 0.0;
        bool hasCpuCallbackTime = false;
        
        // Explicitly unavailable metrics are reported as such.
        bool gpuTimeAvailable = false;
        bool memoryAvailable = false;
        bool objectCountAvailable = false;
        
        // Human-readable summary of what is and is not known.
        std::string status;
        std::string dataAvailability;
        
        // v0.3: recommendations are observational only. No optimization
        // is ever applied by this engine.
        std::string recommendedAction;
        bool shouldOptimize = false;
    };
    
    Report getReport() const;
};

} // namespace fleetapex

#endif // FLEET_APEX_ADAPTIVE_ENGINE_H
