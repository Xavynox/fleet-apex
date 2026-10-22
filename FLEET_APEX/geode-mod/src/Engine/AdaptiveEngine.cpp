#include "AdaptiveEngine.h"
#include "Profiler.h"
#include "LevelAnalyzer.h"
#include "BottleneckClassifier.h"
#include "CPULiberationEngine.h"
#include "GPUAssistEngine.h"
#include <algorithm>
#include <numeric>
#include <sstream>

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
    // Diagnostic-only: keep optimization engines disabled by default
    // m_cpuLiberation->enable();
    // m_gpuAssist->enable();
}

void AdaptiveEngine::update() {
    // Run bottleneck classification against current profiler snapshot
    auto bottleneck = m_bottleneckClassifier->classify();
    m_profiler.classifyBottleneck();

    // Read the current snapshot
    auto& current = m_profiler.getSnapshot().current;
    
    // Report measured metrics only
    m_hasEnoughData = m_profiler.getSnapshot().hasValidData &&
                      m_profiler.getSnapshot().frameSamples >= 5;

    // Calculate frame-time stability from historical variance
    m_runtimeState.frameTimeStability = 1.0 - (current.frameTimeVariance /
        (current.frameTimeMs * current.frameTimeMs + 0.001));
    m_runtimeState.recentSpikeCount = m_profiler.getSnapshot().frameHistory.empty() ? 0 : 0;
    
    m_runtimeState.cpuUsageRatio = current.cpuTimeMs / std::max(current.frameTimeMs, 0.001);
    m_runtimeState.cpuRatioValid = current.cpuTimeValid;
    m_runtimeState.gpuUsageRatio = current.gpuTimeMs / std::max(current.frameTimeMs, 0.001);
    m_runtimeState.gpuRatioValid = current.gpuTimeValid;
    m_runtimeState.memoryPressure = current.memoryUsageMb;

    // Recent spikes
    m_runtimeState.recentSpikeCount = m_profiler.getSnapshot().recentSpikeIndex >= 0 ? 1 : 0;
}

void AdaptiveEngine::setPerformanceMode(PerformanceMode mode) {
    m_mode = mode;
}

void AdaptiveEngine::setTargetFrameRate(double fps) {
    m_bottleneckClassifier->setTargetFrameTime(fps);
}

AdaptiveEngine::Report AdaptiveEngine::getReport() const {
    Report report;
    
    // Bottleneck classification: honest confidence from profiler
    const auto& snapshot = m_profiler.getSnapshot();
    report.bottleneck = m_profiler.getBottleneckDescription();
    report.confidence = snapshot.confidence;          // honest: 0.0-1.0 based on samples
    report.hasSufficientData = m_hasEnoughData;
    report.sampleCount = snapshot.frameSamples;
    
    // Observed metrics only
    report.measuredFps = snapshot.current.fps;
    report.hasFps = m_hasEnoughData;
    report.averageFrameTimeMs = snapshot.current.averageFrameTimeMs;
    report.hasAverageFrameTime = snapshot.frameSamples > 0;
    report.cpuCallbackMs = snapshot.current.cpuTimeMs;
    report.hasCpuCallbackTime = snapshot.current.cpuTimeValid;
    
    // Explicitly unavailable
    const auto& current = snapshot.current;
    report.gpuTimeAvailable = current.gpuTimeValid;   // always false in v0.3
    report.memoryAvailable = current.memoryValid;     // always false in v0.3
    report.objectCountAvailable = current.objectCountValid;
    
    // Human-readable descriptions
    report.status = m_hasEnoughData ? "Diagnostics active" : "Warming up / insufficient data";
    
    // Compose data-availability summary
    std::ostringstream oss;
    oss << "FPS=" << (m_hasEnoughData ? std::to_string(static_cast<int>(snapshot.current.fps)) : "N/A")
        << ", CPU callback=" << (current.cpuTimeValid ? std::to_string(static_cast<int>(current.cpuTimeMs)) + " ms" : "N/A")
        << ", GPU: N/A  Memory: N/A";
    report.dataAvailability = oss.str();
    
    // Diagnostic-only: never apply optimizations, always recommend monitoring
    report.shouldOptimize = false;
    report.recommendedAction = "Monitor and maintain";
    
    return report;
}

} // namespace fleetapex
