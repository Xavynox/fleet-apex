#ifndef FLEET_APEX_ADAPTIVE_ENGINE_H
#define FLEET_APEX_ADAPTIVE_ENGINE_H

#include "Profiler.h"
#include "LevelAnalyzer.h"
#include <functional>
#include <memory>
#include <vector>
#include <string>

namespace fleetapex {

/**
 * APEX ADAPTIVE RUNTIME ENGINE
 * Decision and execution layer
 * 
 * Answers: "Performance is dropping. What can safely be changed right now?"
 */
class AdaptiveEngine {
public:
    enum class OptimizationAction {
        NONE,
        REDUCE_PARTICLES,
        ADAPT_GLOW,
        INCREASE_CULLING,
        STATIC_CACHING,
        REDUCE_BACKGROUND,
        MEMORY_CLEANUP,
        LOW_QUALITY_MODE,
        CUSTOM_OPTIMIZATION
    };

    enum class PerformanceMode {
        SAFE,        // Highly conservative
        BALANCED,    // Normal adaptive
        PERFORMANCE, // Aggressive visual
        TURBO,       // Extreme weak hardware
        CUSTOM       // User-defined
    };

    struct OptimizationDecision {
        OptimizationAction action;
        double intensity; // 0.0 - 1.0
        std::string reason;
        double expectedBenefitMs = 0.0;
        double riskLevel = 0.0; // 0.0 = safe, 1.0 = potentially unsafe
        std::vector<int32_t> affectedObjects;
    };

    struct OptimizationResult {
        bool applied = false;
        double benefitMs = 0.0;
        double regressionMs = 0.0;
        bool rollbackNeeded = false;
        std::string message;
    };

    struct RuntimeState {
        Profiler::BottleneckType currentBottleneck = Profiler::BottleneckType::UNKNOWN;
        double frameTimeStability = 0.0;
        int recentSpikeCount = 0;
        double cpuUsageRatio = 0.0;
        double gpuUsageRatio = 0.0;
        uint64_t memoryPressure = 0;
        bool thermalLimited = false;
        std::string stateDescription;
    };

private:
    std::shared_ptr<Profiler> m_profiler;
    std::shared_ptr<LevelAnalyzer> m_analyzer;
    
    RuntimeState m_runtimeState;
    std::vector<OptimizationDecision> m_pendingDecisions;
    std::vector<OptimizationResult> m_history;
    
    PerformanceMode m_mode = PerformanceMode::BALANCED;
    
    double m_targetFrameTimeMs = 16.67; // 60 FPS target
    double m_spikeThresholdMs = 30.0;
    int m_spikesInWindow = 0;
    int m_spikeWindowFrames = 300; // 5 seconds at 60fps
    
    // State tracking
    int m_consecutiveLowFps = 0;
    int m_consecutiveStableFrames = 0;
    bool m_isOptimized = false;
    
    // Internal helpers
    void updateRuntimeState();
    std::vector<OptimizationDecision> generateDecisions();
    void classifyBottleneck();
    double calculateDecisionConfidence(const OptimizationDecision& decision);
    OptimizationResult executeDecision(const OptimizationDecision& decision);
    void measureAndValidate();
    bool shouldRollback(const OptimizationResult& result);
    
    // Decision generators
    std::vector<OptimizationDecision> generateParticleOptimizations();
    std::vector<OptimizationDecision> generateGlowOptimizations();
    std::vector<OptimizationDecision> generateCullingOptimizations();
    std::vector<OptimizationDecision> generateCachingOptimizations();
    std::vector<OptimizationDecision> generateMemoryOptimizations();

public:
    // Callback types for integration with game engine
    using OptimizationCallback = std::function<OptimizationResult(const OptimizationDecision&)>;
    using RollbackCallback = std::function<void()>;
    
    AdaptiveEngine(std::shared_ptr<Profiler> profiler, 
                   std::shared_ptr<LevelAnalyzer> analyzer);
    ~AdaptiveEngine() = default;

    // Main entry point
    void initialize();
    void update();
    
    // Configuration
    void setPerformanceMode(PerformanceMode mode) { m_mode = mode; }
    void setTargetFrameRate(double fps) { m_targetFrameTimeMs = 1000.0 / fps; }
    void setSpikeThreshold(double ms) { m_spikeThresholdMs = ms; }
    
    // Accessors
    PerformanceMode getMode() const { return m_mode; }
    const RuntimeState& getRuntimeState() const { return m_runtimeState; }
    const OptimizationDecision& getLastDecision() const;
    const OptimizationResult& getLastResult() const;
    bool isOptimized() const { return m_isOptimized; }
    
    // Decision management
    const std::vector<OptimizationDecision>& getPendingDecisions() const { return m_pendingDecisions; }
    void clearPendingDecisions() { m_pendingDecisions.clear(); }
    
    // Manual control
    void triggerManualOptimization();
    void rollbackLastOptimization();
    void resetOptimizationState();
    
    // Export state for HUD/display
    struct EngineReport {
        std::string bottleneck;
        std::string primaryCostSource;
        std::string recommendedAction;
        double confidence;
        bool shouldOptimize;
        std::string status;
    };
    EngineReport getReport() const;
};

} // namespace fleetapex

#endif // FLEET_APEX_ADAPTIVE_ENGINE_H
