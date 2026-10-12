/**
 * FLEET APEX v0.2 - Profiler Header
 * 
 * Core measurement and observability layer.
 */

#ifndef FLEET_APEX_PROFILER_H
#define FLEET_APEX_PROFILER_H

#include <chrono>
#include <vector>
#include <string>
#include <map>
#include <cstdint>

namespace fleetapex {

class Profiler {
public:
    struct FrameMetrics {
        double fps = 0.0;
        double frameTimeMs = 0.0;
        double cpuTimeMs = 0.0;
        double gpuTimeMs = 0.0;
        uint64_t objectCount = 0;
        uint64_t visibleObjectCount = 0;
        uint64_t particleCount = 0;
        uint64_t drawCalls = 0;
        uint64_t memoryUsageMb = 0;
        double frameTimeVariance = 0.0;
        double worstFrameTimeMs = 0.0;
        double medianFrameTimeMs = 0.0;
    };

    enum class BottleneckType {
        CPU_BOUND,
        GPU_BOUND,
        MEMORY_BOUND,
        LOADING_BOUND,
        THERMAL_CONSTRAINED,
        UNKNOWN,
        STABLE
    };

    struct PerformanceSnapshot {
        FrameMetrics current;
        FrameMetrics historicalAvg;
        BottleneckType bottleneck;
        std::string primaryCostSource;
        double confidence = 0.0;
        std::vector<double> frameHistory;
        int recentSpikeIndex = -1;
    };

private:
    PerformanceSnapshot m_snapshot;
    std::vector<double> m_frameHistory;
    std::map<std::string, double> m_counterValues;
    std::chrono::high_resolution_clock::time_point m_frameStart;
    size_t m_historySize = 120;
    bool m_profilingEnabled = true;
    bool m_initialized = false;
    
    uint64_t m_actualObjectCount = 0;
    uint64_t m_actualVisibleCount = 0;
    uint64_t m_actualParticleCount = 0;

public:
    Profiler();
    ~Profiler() = default;

    void beginFrame();
    void endFrame();

    void setObjectCount(uint64_t count) { m_actualObjectCount = count; }
    void setVisibleObjectCount(uint64_t count) { m_actualVisibleCount = count; }
    void setParticleCount(uint64_t count) { m_actualParticleCount = count; }
    void setMemoryUsage(uint64_t mb) { m_snapshot.current.memoryUsageMb = mb; }
    void setDrawCallEstimate(uint64_t calls) { m_snapshot.current.drawCalls = calls; }

    void estimateCpuTime(double ms);
    void estimateGpuTime(double ms);

    const PerformanceSnapshot& getSnapshot() const { return m_snapshot; }
    PerformanceSnapshot& getSnapshot() { return m_snapshot; }
    bool isProfilingEnabled() const { return m_profilingEnabled; }
    
    void setProfilingEnabled(bool enabled) { m_profilingEnabled = enabled; }
    void setHistorySize(size_t size) { m_historySize = size; }

    BottleneckType classifyBottleneck();
    std::string getBottleneckDescription() const;

    double getAverageFrameTime() const;
    double getMinFrameTime() const;
    double getMaxFrameTime() const;
    double getFrameTimeVariance() const;
    int countSpikes(double thresholdMs, double durationWindow = 0.1) const;

    void reset();
    std::map<std::string, double> getMetricsMap() const;
};

} // namespace fleetapex

#endif // FLEET_APEX_PROFILER_H
