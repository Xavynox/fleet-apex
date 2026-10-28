#include "Profiler.h"
#include <cmath>
#include <numeric>
#include <algorithm>
#include <sstream>

namespace fleetapex {

Profiler::Profiler() {
    m_snapshot.frameHistory.reserve(m_historySize);
    m_frameStart = std::chrono::steady_clock::now();
    m_initialized = true;
    m_hasFirstFrame = false;
    m_snapshot.hasValidData = false;
    m_snapshot.frameSamples = 0;
    m_snapshot.current.cpuTimeValid = false;
    m_snapshot.current.gpuTimeValid = false;
    m_snapshot.current.memoryValid = false;
    m_snapshot.current.objectCountValid = false;
    m_snapshot.current.particleCountValid = false;
    m_snapshot.current.frameTimeMs = 0.0;
    m_snapshot.current.fps = 0.0;
}

void Profiler::beginFrame() {
    if (!m_profilingEnabled) return;
    m_frameStart = std::chrono::steady_clock::now();
}

void Profiler::endFrame() {
    if (!m_profilingEnabled) return;
    
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(now - m_frameStart).count();
    
    auto& frame = m_snapshot.current;
    
    // Frame interval is time between successive endFrame() / beginFrame() pairs
    // It is NOT total display time or GPU work duration
    frame.frameTimeMs = duration;
    frame.fps = (duration > 0 && duration < 5000.0) ? (1000.0 / duration) : 0.0;
    
    // Only treat as valid when duration is reasonable (not zero, not extreme)
    if (duration > 0 && duration < 5000.0) {
        m_snapshot.hasValidData = true;
        m_snapshot.frameSamples = m_frameHistory.size() + 1;
    }
    
    m_frameHistory.push_back(duration);
    if (m_frameHistory.size() > m_historySize) {
        m_frameHistory.erase(m_frameHistory.begin());
    }
    
    // Update rolling statistics at bounded cadence (every 30 frames) to avoid
    // full-window sort overhead on every frame
    m_framesSinceRecompute++;
    if (m_framesSinceRecompute >= kRecomputeInterval || m_frameHistory.size() >= 2) {
        m_cachedAverage = getAverageFrameTime();
        m_cachedMedian = getMedianFrameTime();
        m_cachedWorst = getMaxFrameTime();
        m_cachedP99 = getP99FrameTime();
        m_cachedVariance = getFrameTimeVariance();
        m_framesSinceRecompute = 0;
    }
    
    if (!m_frameHistory.empty()) {
        frame.medianFrameTimeMs = m_cachedMedian;
        frame.worstFrameTimeMs = m_cachedWorst;
        frame.frameTimeVariance = m_cachedVariance;
        
        m_snapshot.frameHistory = m_frameHistory;
        
        if (duration > m_spikeThresholdMs) {
            m_snapshot.recentSpikeIndex = static_cast<int>(m_frameHistory.size()) - 1;
            m_spikeCount++;
        }
    }
    
    frame.objectCount = m_actualObjectCount;
    frame.visibleObjectCount = m_actualVisibleCount;
    frame.particleCount = m_actualParticleCount;
    
    // GPU time is not measured — explicitly false
    frame.gpuTimeMs = 0.0;
    frame.gpuTimeValid = false;
    // Memory usage requires platform-specific measurement — unavailable
    frame.memoryUsageMb = 0;
    frame.memoryValid = false;
}

void Profiler::recordCallbackDuration(double ms) {
    // This records the CPU-side time spent inside a particular instrumented
    // callback function (e.g., GameScene update). It is NOT total frame time
    // and NOT GPU execution time.
    m_snapshot.current.cpuTimeMs = ms;
    m_snapshot.current.cpuTimeValid = true;
}

Profiler::BottleneckType Profiler::classifyBottleneck() {
    auto& current = m_snapshot.current;
    
    // Only classify when genuine observations exist
    if (!m_snapshot.hasValidData || m_frameHistory.size() < 5) {
        m_snapshot.bottleneck = BottleneckType::UNKNOWN;
        m_snapshot.confidence = 0.0;
        m_snapshot.historicalAvg = current;
        return m_snapshot.bottleneck;
    }
    
    if (current.fps < 30.0) {
        double cpuRatio = 0.0;
        double gpuRatio = 0.0;
        if (current.frameTimeMs > 0.0) {
            cpuRatio = current.cpuTimeMs / current.frameTimeMs;
            gpuRatio = current.gpuTimeMs / current.frameTimeMs;
        }
        
        // Only label CPU-bound when we have valid CPU evidence
        if (cpuRatio > 0.7 && current.cpuTimeValid) {
            m_snapshot.bottleneck = BottleneckType::CPU_BOUND;
            m_snapshot.confidence = 0.7;
        } else if (gpuRatio > 0.7 && current.gpuTimeValid) {
            // GPU-bound requires verified GPU measurement — unavailable here
            m_snapshot.bottleneck = BottleneckType::GPU_BOUND;
            m_snapshot.confidence = 0.3; // Low — GPU not measured
        } else if (current.memoryUsageMb > 1000 && current.memoryValid) {
            m_snapshot.bottleneck = BottleneckType::MEMORY_BOUND;
            m_snapshot.confidence = 0.6;
        } else {
            m_snapshot.bottleneck = BottleneckType::UNKNOWN;
            m_snapshot.confidence = 0.3;
        }
    } else if (current.fps < 50.0) {
        m_snapshot.bottleneck = BottleneckType::UNKNOWN;
        m_snapshot.confidence = 0.4;
    } else if (current.frameTimeMs > 0 && current.frameTimeMs < 16.7) {
        m_snapshot.bottleneck = BottleneckType::STABLE;
        m_snapshot.confidence = 0.85;
    } else {
        m_snapshot.bottleneck = BottleneckType::STABLE;
        m_snapshot.confidence = 0.7;
    }
    
    m_snapshot.historicalAvg = current;
    return m_snapshot.bottleneck;
}

std::string Profiler::getBottleneckDescription() const {
    switch (m_snapshot.bottleneck) {
        case BottleneckType::CPU_BOUND: return "CPU-bound";
        case BottleneckType::GPU_BOUND: return "GPU-bound";
        case BottleneckType::MEMORY_BOUND: return "Memory-bound";
        case BottleneckType::LOADING_BOUND: return "Loading-bound";
        case BottleneckType::THERMAL_CONSTRAINED: return "Thermally constrained";
        case BottleneckType::STABLE: return "Stable";
        case BottleneckType::UNKNOWN: return "Unknown / insufficient data";
        default: return "Unknown";
    }
}

double Profiler::getMinFrameTime() const {
    if (m_frameHistory.empty()) return 0.0;
    return *std::min_element(m_frameHistory.begin(), m_frameHistory.end());
}

double Profiler::getMaxFrameTime() const {
    if (m_frameHistory.empty()) return 0.0;
    return *std::max_element(m_frameHistory.begin(), m_frameHistory.end());
}

int Profiler::countSpikes(double thresholdMs) const {
    int count = 0;
    for (double frameTime : m_frameHistory) {
        if (frameTime > thresholdMs) {
            count++;
        }
    }
    return count;
}

void Profiler::resetFrameStatistics() {
    m_frameHistory.clear();
    m_snapshot.current = FrameMetrics();
    m_snapshot.current.cpuTimeValid = false;
    m_snapshot.current.gpuTimeValid = false;
    m_snapshot.current.memoryValid = false;
    m_snapshot.current.objectCountValid = false;
    m_snapshot.current.particleCountValid = false;
    m_snapshot.frameSamples = 0;
    m_snapshot.hasValidData = false;
    m_snapshot.recentSpikeIndex = -1;
    m_snapshot.bottleneck = BottleneckType::UNKNOWN;
    m_snapshot.confidence = 0.0;
    m_snapshot.historicalAvg = FrameMetrics();
    m_cachedAverage = 0.0;
    m_cachedMedian = 0.0;
    m_cachedWorst = 0.0;
    m_cachedP99 = 0.0;
    m_cachedVariance = 0.0;
    m_spikeCount = 0;
    // Keep m_historySize and profiling settings intact
}

void Profiler::setHistorySize(size_t size) {
    m_historySize = size;
    if (m_frameHistory.size() > size) {
        m_frameHistory.erase(m_frameHistory.begin(), m_frameHistory.begin() + (m_frameHistory.size() - size));
    }
}

void Profiler::reset() {
    resetFrameStatistics();
    m_snapshot.bottleneck = BottleneckType::UNKNOWN;
    m_counterValues.clear();
    m_actualObjectCount = 0;
    m_actualVisibleCount = 0;
    m_actualParticleCount = 0;
}

std::map<std::string, double> Profiler::getMetricsMap() const {
    std::map<std::string, double> metrics;
    auto& c = m_snapshot.current;
    metrics["fps"] = c.fps;
    metrics["frame_time_ms"] = (c.cpuTimeValid ? c.cpuTimeMs : 0.0);  // CPU time is callback duration, not total frame time
    metrics["cpu_time_ms"] = c.cpuTimeValid ? c.cpuTimeMs : 0.0;
    metrics["gpu_time_ms"] = c.gpuTimeValid ? c.gpuTimeMs : 0.0;  // Always unavailable
    metrics["objects"] = c.objectCountValid ? static_cast<double>(c.objectCount) : 0.0;
    metrics["visible_objects"] = c.visibleObjectCountValid ? static_cast<double>(c.visibleObjectCount) : 0.0;
    metrics["particles"] = c.particleCountValid ? static_cast<double>(c.particleCount) : 0.0;
    metrics["draw_calls"] = c.drawCallCountValid ? static_cast<double>(c.drawCalls) : 0.0;
    metrics["memory_mb"] = c.memoryValid ? static_cast<double>(c.memoryUsageMb) : 0.0;
    metrics["frame_time_variance"] = c.frameTimeVariance;
    metrics["worst_frame_time"] = c.worstFrameTimeMs;
    metrics["median_frame_time"] = c.medianFrameTimeMs;
    metrics["frame_samples"] = static_cast<double>(m_snapshot.frameSamples);
    return metrics;
}

void Profiler::recordFrameInterval(double frameTimeMs) {
    if (!m_profilingEnabled) return;

    // Reject invalid intervals
    if (frameTimeMs <= 0.0 || frameTimeMs > 5000.0) return;

    // Store interval in history and compute frame metrics
    m_frameHistory.push_back(frameTimeMs);
    if (m_frameHistory.size() > m_historySize) {
        m_frameHistory.erase(m_frameHistory.begin());
    }

    auto& frame = m_snapshot.current;
    frame.frameTimeMs = frameTimeMs;
    frame.fps = (frameTimeMs > 0.0 && frameTimeMs < 5000.0) ? (1000.0 / frameTimeMs) : 0.0;

    // First call: treat as initialization, do not mark valid data yet
    if (m_frameHistory.size() == 1) {
        m_hasFirstFrame = true;
        return;
    }

    m_snapshot.hasValidData = true;
    m_snapshot.frameSamples = m_frameHistory.size();

    // Update rolling statistics from retained window
    if (m_frameHistory.size() >= 2) {
        auto sortedHistory = m_frameHistory;
        std::sort(sortedHistory.begin(), sortedHistory.end());
        frame.medianFrameTimeMs = sortedHistory[sortedHistory.size() / 2];
        frame.worstFrameTimeMs = sortedHistory.back();

        double sum = std::accumulate(m_frameHistory.begin(), m_frameHistory.end(), 0.0);
        double mean = sum / m_frameHistory.size();
        double variance = 0.0;
        for (double v : m_frameHistory) {
            variance += (v - mean) * (v - mean);
        }
        frame.frameTimeVariance = variance / m_frameHistory.size();
    }
}

} // namespace fleetapex
// v0.3 profiler fix: clarifyBottleneck returns BottleneckType, resolves undefined symbol
// Rebuild trigger
