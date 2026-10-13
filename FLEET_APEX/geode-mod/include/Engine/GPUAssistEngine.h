/**
 * FLEET APEX v0.2 - GPU Assist Engine Header
 *
 * For diagnostic release, this is a placeholder stub.
 * It will be replaced with actual implementation in Phase 2.
 */

#ifndef FLEET_APEX_GPU_ASSIST_ENGINE_H
#define FLEET_APEX_GPU_ASSIST_ENGINE_H

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

namespace fleetapex {

class Profiler;
class LevelAnalyzer;

class GPUAssistEngine {
public:
    enum class GPUAssistAction {
        NONE, STATIC_BATCHING, INSTANCING, GPU_PARTICLES, SHADER_EFFECTS,
        BACKGROUND_ANIMATION, VISUAL_TRANSFORMATIONS, REDUCE_CPU_SUBMISSION,
        STATIC_SCENE_REPRESENTATION
    };

    struct GPUOffloadCandidate {
        std::vector<int32_t> objectIds;
        GPUAssistAction action = GPUAssistAction::NONE;
        double estimatedCpuReductionMs = 0.0;
        double estimatedGpuIncreaseMs = 0.0;
        double riskLevel = 0.0;
        bool safetyPassed = false;
        std::string description;
    };

private:
    Profiler& m_profiler;
    LevelAnalyzer& m_analyzer;
    bool m_isActive = false;
    std::vector<std::string> m_history;

public:
    GPUAssistEngine(Profiler& profiler, LevelAnalyzer& analyzer);
    ~GPUAssistEngine() = default;

    void enable();
    void disable();
    bool isActive() const;

    std::vector<GPUOffloadCandidate> analyze();
    bool executeCandidate(GPUOffloadCandidate candidate);
    std::vector<std::string> getHistory() const;
    std::string getStatus() const;
    void clearHistory();
};

} // namespace fleetapex

#endif // FLEET_APEX_GPU_ASSIST_ENGINE_H
