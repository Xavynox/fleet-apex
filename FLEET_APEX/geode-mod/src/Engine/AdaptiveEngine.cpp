#include "AdaptiveEngine.h"
#include <algorithm>
#include <numeric>

namespace fleetapex {

AdaptiveEngine::AdaptiveEngine(Profiler& profiler, LevelAnalyzer& analyzer)
    : m_profiler(profiler)
    , m_analyzer(analyzer)
    , m_bottleneckClassifier(std::make_unique<BottleneckClassifier>(profiler))
    , m_cpuLiberation(std::make_unique<CPULiberationEngine>(profiler, analyzer))
    , m_gpuAssist(std::make_unique<GPUAssistEngine>(profiler, analyzer))
    , m_mode(PerformanceMode::SAFE)
    , m_isOptimized(false)
    , m_hasEnoughData(false)
{
}

AdaptiveEngine::~AdaptiveEngine() = default;

void AdaptiveEngine::initialize() {
    m_hasEnoughData = false;
    m_isOptimized = false;
    m_runtimeState = RuntimeState{};
    m_pendingDecisions.clear();
    m_decisionHistory.clear();
    m_bottleneckClassifier->setTargetFrameTime(60.0);
    m_cpuLiberation->enable();
    m_gpuAssist->enable();
}

void AdaptiveEngine::update() {
    if (!m_profiler || !m_analyzer) return;

    // Update the profiler with the latest frame
    m_profiler->classifyBottleneck();

    // Update runtime state from profiler
    auto snapshot = m_profiler.getSnapshot();
    auto& current = snapshot.current;
    m_runtimeState.frameTimeStability = 1.0 - (current.frameTimeVariance / (current.frameTimeMs * current.frameTimeMs + 0.001));
    m_runtimeState.recentSpikeCount = 0;
    for (double ft : snapshot.frameHistory) {
        if (ft > 30.0) m_runtimeState.recentSpikeCount++;
    }
    if (current.frameTimeMs > 0) {
        m_runtimeState.cpuUsageRatio = current.cpuTimeMs / current.frameTimeMs;
        m_runtimeState.gpuUsageRatio = current.gpuTimeMs / current.frameTimeMs;
    }
    m_runtimeState.memoryPressure = current.memoryUsageMb;

    // Classify bottleneck
    auto bottleneckReport = m_bottleneckClassifier->classify();

    // Analyze CPU liberation opportunities
    auto cpuAnalysis = m_cpuLiberation->analyze();

    // Analyze GPU assist opportunities
    auto gpuCandidates = m_gpuAssist->analyze();

    // Generate optimization decisions (for now, we just log in diagnostic mode)
    // In a full implementation, we would use the balancer to decide and then act.
    // For v0.2 diagnostic, we only collect data and do not apply optimizations.

    // Determine if we have enough data to make a decision (e.g., at least 30 frames)
    m_hasEnoughData = snapshot.frameHistory.size() >= 30;
}

const AdaptiveEngine::RuntimeState& AdaptiveEngine::getRuntimeState() const {
    return m_runtimeState;
}

void AdaptiveEngine::setPerformanceMode(PerformanceMode mode) {
    m_mode = mode;
}

void AdaptiveEngine::setTargetFrameRate(double fps) {
    // Not used in this diagnostic version, but we set it for the bottleneck classifier
    m_bottleneckClassifier->setTargetFrameTime(fps);
}

const std::vector<AdaptiveEngine::OptimizationDecision>& AdaptiveEngine::getPendingDecisions() const {
    return m_pendingDecisions;
}

void AdaptiveEngine::clearPendingDecisions() {
    m_pendingDecisions.clear();
}

AdaptiveEngine::Report AdaptiveEngine::getReport() const {
    Report report;
    report.bottleneck = m_profiler.getBottleneckDescription();
    report.primaryCostSource = "unknown"; // In a full implementation, we would get this from the level analyzer or bottleneck classifier
    report.recommendedAction = "Monitor and maintain";
    report.confidence = m_hasEnoughData ? 0.8 : 0.3;
    report.shouldOptimize = false; // Diagnostic mode: no optimizations applied
    report.status = m_isOptimized ? "Optimized" : (m_hasEnoughData ? "Ready" : "Warming up");
    return report;
}

} // namespace fleetapex
