#include "GPUAssistEngine.h"

namespace fleetapex {

GPUAssistEngine::GPUAssistEngine(Profiler& profiler, LevelAnalyzer& analyzer)
    : m_profiler(profiler), m_analyzer(analyzer) {
}

void GPUAssistEngine::enable() {
    m_isActive = true;
    // NOTE: GPU assist optimizations are disabled by default in this
    // diagnostic-only release. They will be implemented in Phase 2.
}

void GPUAssistEngine::disable() {
    m_isActive = false;
}

bool GPUAssistEngine::isActive() const {
    return m_isActive;
}

std::vector<GPUAssistEngine::GPUOffloadCandidate> GPUAssistEngine::analyze() {
    // Empty analysis for diagnostic release.
    // Will be implemented in Phase 2.
    return {};
}

bool GPUAssistEngine::executeCandidate(GPUOffloadCandidate) {
    return false;
}

std::vector<std::string> GPUAssistEngine::getHistory() const {
    return {};
}

std::string GPUAssistEngine::getStatus() const {
    return m_isActive ? "ENABLED" : "DISABLED";
}

void GPUAssistEngine::clearHistory() {
}

} // namespace fleetapex
