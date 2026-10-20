#include "BottleneckClassifier.h"
#include "Profiler.h"

namespace fleetapex {

BottleneckClassifier::BottleneckClassifier(Profiler& profiler)
    : m_profiler(profiler) {
}

BottleneckClassifier::~BottleneckClassifier() {}

void BottleneckClassifier::setTargetFrameTime(double fps) {
    if (fps > 0) {
        m_targetFrameTime = 1000.0 / fps;
    }
}

BottleneckClassifier::BottleneckReport BottleneckClassifier::classify() {
    auto snapshot = m_profiler.getSnapshot();
    auto current = snapshot.current;
    
    BottleneckReport report;
    // Zero-initialize all members so no branch can leave them uninitialized
    report.state = BottleneckState::UNKNOWN;
    report.confidence = ConfidenceLevel::LOW;
    report.cpuWorkRatio = 0.0;
    report.gpuWorkRatio = 0.0;
    report.stabilityScore = 0.0;
    report.frameTime = 0.0;
    report.targetFrameTime = 0.0;
    
    // Calculate stability score
    if (current.frameTimeMs > 0) {
        report.stabilityScore = std::max(0.0, 1.0 - (current.frameTimeVariance / 
            (current.frameTimeMs * current.frameTimeMs + 0.001)));
    }
    
    // Classify bottleneck
    if (current.memoryUsageMb > 1500) {
        report.state = BottleneckState::MEMORY_BOUND;
        report.confidence = ConfidenceLevel::MEDIUM;
    } else if (current.frameTimeMs > m_targetFrameTime * 2.0) {
        if (report.cpuWorkRatio > 0.7) {
            report.state = BottleneckState::CPU_BOUND;
            report.confidence = ConfidenceLevel::HIGH;
        } else if (report.gpuWorkRatio > 0.7) {
            report.state = BottleneckState::GPU_BOUND;
            report.confidence = ConfidenceLevel::HIGH;
        } else {
            report.state = BottleneckState::HYBRID;
            report.confidence = ConfidenceLevel::MEDIUM;
        }
    } else if (current.frameTimeMs > m_targetFrameTime * 1.3) {
        if (report.cpuWorkRatio > 0.65 && report.gpuWorkRatio > 0.65) {
            report.state = BottleneckState::HYBRID;
            report.confidence = ConfidenceLevel::MEDIUM;
        } else if (report.cpuWorkRatio > 0.6) {
            report.state = BottleneckState::CPU_BOUND;
            report.confidence = ConfidenceLevel::HIGH;
        } else if (report.gpuWorkRatio > 0.6) {
            report.state = BottleneckState::GPU_BOUND;
            report.confidence = ConfidenceLevel::HIGH;
        } else {
            report.state = BottleneckState::UNKNOWN;
            report.confidence = ConfidenceLevel::LOW;
        }
    } else if (current.frameTimeMs > m_targetFrameTime * 1.05) {
        if (report.stabilityScore < 0.8) {
            report.state = BottleneckState::HYBRID;
            report.confidence = ConfidenceLevel::LOW;
        }
    } else if (current.fps >= 55 && report.stabilityScore > 0.9) {
        report.state = BottleneckState::STABLE;
        report.confidence = ConfidenceLevel::HIGH;
    } else {
        report.state = BottleneckState::UNKNOWN;
        report.confidence = ConfidenceLevel::LOW;
    }
    
    // Spike correlation
    if (snapshot.recentSpikeIndex >= 0 && !snapshot.frameHistory.empty()) {
        SpikeCorrelation spike;
        spike.spikeFrameTime = snapshot.frameHistory[snapshot.recentSpikeIndex];
        spike.correlationSource = "unknown";
        spike.confidence = ConfidenceLevel::UNKNOWN;
        spike.details = "No clear correlation found";
        report.spikeCorrelations.push_back(spike);
    }
    
    // Primary cost sources (estimated)
    report.primaryCpuCostSource = "object_processing";
    report.primaryGpuCostSource = "rendering";
    
    // Generate description
    switch (report.state) {
        case BottleneckState::CPU_BOUND:
            report.description = "CPU-bound: CPU ratio " + std::to_string(static_cast<int>(report.cpuWorkRatio * 100)) + "%, frame time above target";
            break;
        case BottleneckState::GPU_BOUND:
            report.description = "GPU-bound: GPU ratio " + std::to_string(static_cast<int>(report.gpuWorkRatio * 100)) + "%, frame time above target";
            break;
        case BottleneckState::HYBRID:
            report.description = "Hybrid bottleneck: CPU " + std::to_string(static_cast<int>(report.cpuWorkRatio * 100)) + "%, GPU " + std::to_string(static_cast<int>(report.gpuWorkRatio * 100)) + "%";
            break;
        case BottleneckState::MEMORY_BOUND:
            report.description = "Memory pressure detected";
            break;
        case BottleneckState::STABLE:
            report.description = "Stable: " + std::to_string(static_cast<int>(report.stabilityScore * 100)) + "% frame time stability";
            break;
        default:
            report.description = "Performance state unclear";
            break;
    }
    
    m_history.push_back(report);
    if (m_history.size() > 100) m_history.erase(m_history.begin());
    
    return report;
}

const std::vector<BottleneckClassifier::BottleneckReport>& BottleneckClassifier::getHistory() const {
    return m_history;
}

const BottleneckClassifier::BottleneckReport* BottleneckClassifier::getLastReport() const {
    return m_history.empty() ? nullptr : &m_history.back();
}

} // namespace fleetapex
