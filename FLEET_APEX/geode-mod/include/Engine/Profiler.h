/**
 * FLEET APEX v0.3 - Profiler Header
 * 
 * Core measurement and observability layer.
 * 
 * Every metric carries an explicit validity flag so that an unavailable
 * measurement can never masquerade as a measured value of zero.
 */

#ifndef FLEET_APEX_PROFILER_H
#define FLEET_APEX_PROFILER_H

#include <chrono>
#include <vector>
#include <string>
#include <map>
#include <cstdint>
#include <cstddef>

namespace fleetapex {

class Profiler {
public:
    struct FrameMetrics {
        // --- Frame interval (measured from consecutive verified callbacks) ---
        double fps = 0.0;
        double frameTimeMs = 0.0;
        
        // --- CPU-side callback execution duration ---
        double cpuTimeMs = 0.0;
        bool cpuTimeValid = false;
        
        // --- GPU execution time (unavailable unless a validated timer exists) ---
        double gpuTimeMs = 0.0;
        bool gpuTimeValid = false;
        
        // --- Counters (only valid when a verified collector updates them) ---
        uint64_t objectCount = 0;
        bool objectCountValid = false;
        uint64_t visibleObjectCount = 0;
        bool visibleObjectCountValid = false;
        uint64_t particleCount = 0;
        bool particleCountValid = false;
        uint64_t drawCalls = 0;
        bool drawCallCountValid = false;
        
        // --- Process memory ---
        uint64_t memoryUsageMb = 0;
        bool memoryValid = false;
        
        // --- Rolling statistics over the retained window ---
        double frameTimeVariance = 0.0;
        double worstFrameTimeMs = 0.0;
        double medianFrameTimeMs = 0.0;
        double averageFrameTimeMs = 0.0;
        double p99FrameTimeMs = 0.0;
        int spikeCount = 0;
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
        BottleneckType bottleneck = BottleneckType::UNKNOWN;
        std::string primaryCostSource;
        double confidence = 0.0;
        std::vector<double> frameHistory;
        int recentSpikeIndex = -1;
        
        // Number of valid frame-interval samples collected so far.
        // Zero means "no measurements yet" - not "frame time was zero".
        size_t frameSamples = 0;
        
        // True once the profiler has received at least one valid sample.
        bool hasValidData = false;
    };

private:
    PerformanceSnapshot m_snapshot;
    std::vector<double> m_frameHistory;
    std::map<std::string, double> m_counterValues;
    std::chrono::steady_clock::time_point m_frameStart;
    std::chrono::steady_clock::time_point m_lastFrameEnd;
    
    size_t m_historySize = 120;
    bool m_profilingEnabled = true;
    bool m_initialized = false;
    bool m_hasFirstFrame = false;
    
    double m_spikeThresholdMs = 33.3;   // Two frames at 60Hz
    double m_lastFrameTimeMs = 0.0;
    int m_spikeCount = 0;
    
    // Cached rolling statistics, recomputed at a bounded cadence rather
    // than sorting the entire window on every single frame.
    double m_cachedAverage = 0.0;
    double m_cachedVariance = 0.0;
    double m_cachedMedian = 0.0;
    double m_cachedP99 = 0.0;
    double m_cachedWorst = 0.0;
    int m_framesSinceRecompute = 0;
    static constexpr int kRecomputeInterval = 30;
    
    uint64_t m_actualObjectCount = 0;
    uint64_t m_actualVisibleCount = 0;
    uint64_t m_actualParticleCount = 0;
    bool m_objectCountValid = false;
    bool m_visibleCountValid = false;
    bool m_particleCountValid = false;

public:
    Profiler();
    ~Profiler() = default;

    // --- Frame interval collection ---
    // beginFrame() marks the start of a frame; endFrame() records the
    // interval since the previous endFrame() and the callback duration
    // between beginFrame() and endFrame().
    void beginFrame();
    void endFrame();
    
    // Records the CPU-side duration of an instrumented callback directly.
    // This is the time spent inside the hooked function - it is NOT total
    // frame time and NOT GPU time.
    void recordCallbackDuration(double ms);
    
    // --- Verified counter setters ---
    // These are only called by an actual collector. Until then the
    // corresponding metric stays marked unavailable.
    void setObjectCount(uint64_t count) { m_actualObjectCount = count; m_objectCountValid = true; }
    void setVisibleObjectCount(uint64_t count) { m_actualVisibleCount = count; m_visibleCountValid = true; }
    void setParticleCount(uint64_t count) { m_actualParticleCount = count; m_particleCountValid = true; }
    void setMemoryUsage(uint64_t mb) { m_snapshot.current.memoryUsageMb = mb; m_snapshot.current.memoryValid = true; }
    void setDrawCallCount(uint64_t calls) { m_snapshot.current.drawCalls = calls; m_snapshot.current.drawCallCountValid = true; }
    
    // Explicitly invalidate a counter so a stale value cannot be read as current.
    void invalidateObjectCount() { m_objectCountValid = false; m_actualObjectCount = 0; }
    void invalidateVisibleObjectCount() { m_visibleCountValid = false; m_actualVisibleCount = 0; }
    void invalidateParticleCount() { m_particleCountValid = false; m_actualParticleCount = 0; }
    void invalidateMemoryUsage() { m_snapshot.current.memoryValid = false; m_snapshot.current.memoryUsageMb = 0; }
    
    // --- GPU time: intentionally not settable without a validated timer ---
    void markGpuTimeUnavailable() { m_snapshot.current.gpuTimeValid = false; m_snapshot.current.gpuTimeMs = 0.0; }

    // --- Externally measured frame-callback interval ---
    /// Record an externally measured frame-callback interval (e.g. from a CCDirector::drawScene() hook).
    /// The elapsed time is recorded directly as frameTimeMs, not as callback execution duration.
    /// The first call is treated as initialization (stored but not flagged as valid data).
    /// Invalid intervals (<=0 or >5000ms) are silently dropped.
    void recordFrameInterval(double frameTimeMs);

    const PerformanceSnapshot& getSnapshot() const { return m_snapshot; }
    PerformanceSnapshot& getSnapshot() { return m_snapshot; }
    bool isProfilingEnabled() const { return m_profilingEnabled; }
    
    void setProfilingEnabled(bool enabled) { m_profilingEnabled = enabled; }
    void setHistorySize(size_t size);
    void setSpikeThreshold(double ms) { m_spikeThresholdMs = ms; }
    double getSpikeThreshold() const { return m_spikeThresholdMs; }
    
    BottleneckType classifyBottleneck();
    std::string getBottleneckDescription() const;

    double getAverageFrameTime() const { return m_cachedAverage; }
    double getMinFrameTime() const;
    double getMaxFrameTime() const;
    double getFrameTimeVariance() const { return m_cachedVariance; }
    double getMedianFrameTime() const { return m_cachedMedian; }
    double getP99FrameTime() const { return m_cachedP99; }
    int countSpikes(double thresholdMs) const;
    
    size_t getSampleCount() const { return m_snapshot.frameSamples; }
    bool hasValidData() const { return m_snapshot.hasValidData; }
    
    // Clears frame-interval statistics but keeps the profiler usable.
    // Used at level boundaries so session-wide monitoring is not destroyed.
    void resetFrameStatistics();
    
    void reset();
    std::map<std::string, double> getMetricsMap() const;
};

} // namespace fleetapex

#endif // FLEET_APEX_PROFILER_H
