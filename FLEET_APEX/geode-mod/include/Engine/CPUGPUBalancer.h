/**
 * FLEET APEX v0.2 - CPU↔GPU Balancer Header
 */

#ifndef FLEET_APEX_CPU_GPU_BALANCER_H
#define FLEET_APEX_CPU_GPU_BALANCER_H

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

class CPUGPUBalancer {
public:
    enum class OptimizationPath {
        CPU_LIBERATION,
        GPU_ASSIST,
        HYBRID,
        STABLE,
        NONE
    };

    struct BalancerDecision {
        OptimizationPath path = OptimizationPath::NONE;
        std::string reason;
        double confidence = 0.0;
        bool canRoute = true;  // Whether decision routing is applicable
    };

private:
    Profiler& m_profiler;
    LevelAnalyzer& m_analyzer;
    std::unique_ptr<BottleneckClassifier> m_bottleneckClassifier;
    std::unique_ptr<CPULiberationEngine> m_cpuLiberation;
    std::unique_ptr<GPUAssistEngine> m_gpuAssist;
    bool m_isActive = false;

public:
    CPUGPUBalancer(Profiler& profiler, LevelAnalyzer& analyzer);
    ~CPUGPUBalancer() = default;

    void enable();
    void disable();
    bool isActive() const { return m_isActive; }
    void setTargetFrameRate(double fps) { m_bottleneckTarget = fps; }

    BalancerDecision analyze();
    BalancerDecision getLastDecision() const { return m_lastDecision; }

    std::vector<std::string> getHistory() const;
    std::string getStatus() const;

private:
    Profiler::BottleneckType m_currentBottleneck;
    double m_bottleneckTarget = 16.67;
    BalancerDecision m_lastDecision;
};

} // namespace fleetapex

#endif // FLEET_APEX_CPU_GPU_BALANCER_H
