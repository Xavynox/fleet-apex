#ifndef FLEET_APEX_BOTTLENECK_CLASSIFIER_H
#define FLEET_APEX_BOTTLENECK_CLASSIFIER_H

#include "Profiler.h"
#include <chrono>
#include <vector>
#include <map>
#include <cstdint>

namespace fleetapex {

/**
 * FLEET APEX Bottleneck Classifier
 * Classifies the current performance bottleneck state using multiple observations.
 */

class BottleneckClassifier {
public:
    enum class BottleneckState {
        CPU_BOUND,
        GPU_BOUND,
        MEMORY_BOUND,
        LOADING_BOUND,
        THERMALLY_CONSTRAINED,
        HYBRID,
        STABLE,
        UNKNOWN
    };

    enum class ConfidenceLevel {
        HIGH,
        MEDIUM,
        LOW,
        UNKNOWN
    };

    struct SpikeCorrelation {
        double spikeFrameTime;
        std::string correlationSource;
        ConfidenceLevel confidence;
        std::string details;
    };

    struct BottleneckReport {
        BottleneckState state;
        ConfidenceLevel confidence;
        double cpuWorkRatio;
        double gpuWorkRatio;
        double frameTime;
        double targetFrameTime;
        double stabilityScore;
        std::vector<SpikeCorrelation> spikeCorrelations;
        std::string primaryCpuCostSource;
        std::string primaryGpuCostSource;
        std::string description;
    };

private:
    Profiler& m_profiler;
    std::vector<BottleneckReport> m_history;
    double m_targetFrameTime = 16.67; // 60 FPS default

public:
    explicit BottleneckClassifier(Profiler& profiler);
    ~BottleneckClassifier();

    void setTargetFrameTime(double fps);
    BottleneckReport classify();

    const std::vector<BottleneckReport>& getHistory() const;
    const BottleneckReport* getLastReport() const;
};

} // namespace fleetapex

#endif // FLEET_APEX_BOTTLENECK_CLASSIFIER_H
