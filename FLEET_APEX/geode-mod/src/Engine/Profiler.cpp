#include "Profiler.h"
#include <cmath>
#include <numeric>
#include <algorithm>
#include <sstream>

namespace fleetapex {

Profiler::Profiler() {
    m_snapshot.frameHistory.reserve(m_historySize);
    m_frameStart = std::chrono::high_resolution_clock::now();
    m_initialized = true;
}

void Profiler::beginFrame() {
    if (!m_profilingEnabled) return;
    m_frameStart = std::chrono::high_resolution_clock::now();
}

void Profiler::endFrame() {
    if (!m_profilingEnabled) return;
    
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double, std::milli>(now - m_frameStart).count();
    
    auto& frame = m_snapshot.current;
    
    frame.frameTimeMs = duration;
    frame.fps = (duration > 0) ? (1000.0 / duration) : 0.0;
    
    m_frameHistory.push_back(duration);
    if (m_frameHistory.size() > m_historySize) {
        m_frameHistory.erase(m_frameHistory.begin());
    }
    
    if (!m_frameHistory.empty()) {
        std::vector<double> sortedHistory = m_frameHistory;
        std::sort(sortedHistory.begin(), sortedHistory.end());
        
        frame.medianFrameTimeMs = sortedHistory[sortedHistory.size() / 2];
        frame.worstFrameTimeMs = sortedHistory.back();
        frame.frameTimeVariance = getFrameTimeVariance();
        
        m_snapshot.frameHistory = m_frameHistory;
        
        if (duration > 30.0) {
            m_snapshot.recentSpikeIndex = m_frameHistory.size() - 1;
        }
    }
    
    frame.objectCount = m_actualObjectCount;
    frame.visibleObjectCount = m_actualVisibleCount;
    frame.particleCount = m_actualParticleCount;
}

void Profiler::estimateCpuTime(double ms) {
    m_snapshot.current.cpuTimeMs = ms;
}

void Profiler::estimateGpuTime(double ms) {
    m_snapshot.current.gpuTimeMs = ms;
}

Profiler::BottleneckType Profiler::classifyBottleneck() {
    auto& current = m_snapshot.current;
    
    if (current.fps < 30) {
        double cpuRatio = 0.0;
        double gpuRatio = 0.0;
        if (current.frameTimeMs > 0) {
            cpuRatio = current.cpuTimeMs / current.frameTimeMs;
            gpuRatio = current.gpuTimeMs / current.frameTimeMs;
        }
        
        if (cpuRatio > 0.7) {
            m_snapshot.bottleneck = BottleneckType::CPU_BOUND;
        } else if (gpuRatio > 0.7) {
            m_snapshot.bottleneck = BottleneckType::GPU_BOUND;
        } else if (current.memoryUsageMb > 1000) {
            m_snapshot.bottleneck = BottleneckType::MEMORY_BOUND;
        } else {
            m_snapshot.bottleneck = BottleneckType::UNKNOWN;
        }
    } else if (current.fps < 50) {
        m_snapshot.bottleneck = BottleneckType::UNKNOWN;
    } else {
        m_snapshot.bottleneck = BottleneckType::STABLE;
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
        case BottleneckType::UNKNOWN: return "Unknown/normal";
        default: return "Unknown";
    }
}

double Profiler::getAverageFrameTime() const {
    if (m_frameHistory.empty()) return 0.0;
    double sum = std::accumulate(m_frameHistory.begin(), m_frameHistory.end(), 0.0);
    return sum / m_frameHistory.size();
}

double Profiler::getMinFrameTime() const {
    if (m_frameHistory.empty()) return 0.0;
    return *std::min_element(m_frameHistory.begin(), m_frameHistory.end());
}

double Profiler::getMaxFrameTime() const {
    if (m_frameHistory.empty()) return 0.0;
    return *std::max_element(m_frameHistory.begin(), m_frameHistory.end());
}

double Profiler::getFrameTimeVariance() const {
    if (m_frameHistory.size() < 2) return 0.0;
    
    double mean = getAverageFrameTime();
    double variance = 0.0;
    
    for (double frameTime : m_frameHistory) {
        double diff = frameTime - mean;
        variance += diff * diff;
    }
    
    return variance / m_frameHistory.size();
}

int Profiler::countSpikes(double thresholdMs, double durationWindow) const {
    int count = 0;
    for (double frameTime : m_frameHistory) {
        if (frameTime > thresholdMs) {
            count++;
        }
    }
    return count;
}

void Profiler::reset() {
    m_frameHistory.clear();
    m_counterValues.clear();
    m_actualObjectCount = 0;
    m_actualVisibleCount = 0;
    m_actualParticleCount = 0;
    m_snapshot = PerformanceSnapshot();
}

std::map<std::string, double> Profiler::getMetricsMap() const {
    std::map<std::string, double> metrics;
    metrics["fps"] = m_snapshot.current.fps;
    metrics["frame_time_ms"] = m_snapshot.current.frameTimeMs;
    metrics["cpu_time_ms"] = m_snapshot.current.cpuTimeMs;
    metrics["gpu_time_ms"] = m_snapshot.current.gpuTimeMs;
    metrics["objects"] = static_cast<double>(m_snapshot.current.objectCount);
    metrics["visible_objects"] = static_cast<double>(m_snapshot.current.visibleObjectCount);
    metrics["particles"] = static_cast<double>(m_snapshot.current.particleCount);
    metrics["draw_calls"] = static_cast<double>(m_snapshot.current.drawCalls);
    metrics["memory_mb"] = static_cast<double>(m_snapshot.current.memoryUsageMb);
    metrics["variance"] = m_snapshot.current.frameTimeVariance;
    metrics["worst_frame_time"] = m_snapshot.current.worstFrameTimeMs;
    metrics["median_frame_time"] = m_snapshot.current.medianFrameTimeMs;
    return metrics;
}

} // namespace fleetapex
