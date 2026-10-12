#include "AdaptiveEngine.h"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <cmath>

namespace fleetapex {

AdaptiveEngine::AdaptiveEngine(std::shared_ptr<Profiler> profiler, 
                                std::shared_ptr<LevelAnalyzer> analyzer)
    : m_profiler(profiler)
    , m_analyzer(analyzer)
{
}

void AdaptiveEngine::initialize() {
    updateRuntimeState();
    classifyBottleneck();
}

void AdaptiveEngine::update() {
    if (!m_profiler || !m_analyzer) return;
    
    // Update profiler measurements
    m_profiler->classifyBottleneck();
    
    // Update runtime state
    updateRuntimeState();
    
    // Generate optimization decisions
    m_pendingDecisions = generateDecisions();
    
    // Execute the best decision
    if (!m_pendingDecisions.empty()) {
        // Sort by confidence and expected benefit
        std::sort(m_pendingDecisions.begin(), m_pendingDecisions.end(),
            [this](const OptimizationDecision& a, const OptimizationDecision& b) {
                return (a.riskLevel < b.riskLevel && a.expectedBenefitMs > b.expectedBenefitMs);
            });
        
        // Execute highest priority decision
        OptimizationResult result = executeDecision(m_pendingDecisions[0]);
        m_history.push_back(result);
        
        // Keep only last 100 results
        if (m_history.size() > 100) {
            m_history.erase(m_history.begin());
        }
        
        // Auto-rollback if needed
        if (shouldRollback(result)) {
            rollbackLastOptimization();
        }
    }
}

void AdaptiveEngine::updateRuntimeState() {
    auto& snapshot = m_profiler->getSnapshot();
    auto& current = snapshot.current;
    
    m_runtimeState.currentBottleneck = snapshot.bottleneck;
    m_runtimeState.frameTimeStability = 1.0 - (current.frameTimeVariance / (current.frameTimeMs * current.frameTimeMs + 0.001));
    
    // Count recent spikes
    int spikes = 0;
    for (double ft : snapshot.frameHistory) {
        if (ft > m_spikeThresholdMs) spikes++;
    }
    m_runtimeState.recentSpikeCount = spikes;
    
    // CPU/GPU ratios
    if (current.frameTimeMs > 0) {
        m_runtimeState.cpuUsageRatio = current.cpuTimeMs / current.frameTimeMs;
        m_runtimeState.gpuUsageRatio = current.gpuTimeMs / current.frameTimeMs;
    }
    
    // Memory pressure
    m_runtimeState.memoryPressure = current.memoryUsageMb;
    
    // Update description
    std::ostringstream ss;
    ss << "Bottleneck: " << snapshot.getBottleneckDescription();
    m_runtimeState.stateDescription = ss.str();
}

std::vector<AdaptiveEngine::OptimizationDecision> AdaptiveEngine::generateDecisions() {
    std::vector<OptimizationDecision> decisions;
    
    // Mode-specific decision generation
    switch (m_mode) {
        case PerformanceMode::SAFE:
            // Only very conservative optimizations
            decisions.insert(decisions.end(), generateParticleOptimizations().begin(), 
                           generateParticleOptimizations().end());
            break;
            
        case PerformanceMode::BALANCED:
            // Normal adaptive set
            auto particles = generateParticleOptimizations();
            auto glow = generateGlowOptimizations();
            auto culling = generateCullingOptimizations();
            
            decisions.insert(decisions.end(), particles.begin(), particles.end());
            decisions.insert(decisions.end(), glow.begin(), glow.end());
            decisions.insert(decisions.end(), culling.begin(), culling.end());
            break;
            
        case PerformanceMode::PERFORMANCE:
            // More aggressive
            for (auto& dec : generateParticleOptimizations()) decisions.push_back(dec);
            for (auto& dec : generateGlowOptimizations()) decisions.push_back(dec);
            for (auto& dec : generateCullingOptimizations()) decisions.push_back(dec);
            for (auto& dec : generateCachingOptimizations()) decisions.push_back(dec);
            break;
            
        case PerformanceMode::TURBO:
            // Maximum optimization
            for (auto& dec : generateParticleOptimizations()) decisions.push_back(dec);
            for (auto& dec : generateGlowOptimizations()) decisions.push_back(dec);
            for (auto& dec : generateCullingOptimizations()) decisions.push_back(dec);
            for (auto& dec : generateCachingOptimizations()) decisions.push_back(dec);
            for (auto& dec : generateMemoryOptimizations()) decisions.push_back(dec);
            break;
    }
    
    return decisions;
}

void AdaptiveEngine::classifyBottleneck() {
    auto& snapshot = m_profiler->getSnapshot();
    auto& current = snapshot.current;
    
    // Analyze frame time stability and cost ratios
    double avgFrameTime = m_profiler->getAverageFrameTime();
    double variance = current.frameTimeVariance;
    
    // GPU-bound indicators
    if (m_runtimeState.gpuUsageRatio > 0.6) {
        m_runtimeState.currentBottleneck = Profiler::BottleneckType::GPU_BOUND;
    }
    // CPU-bound indicators
    else if (m_runtimeState.cpuUsageRatio > 0.6) {
        m_runtimeState.currentBottleneck = Profiler::BottleneckType::CPU_BOUND;
    }
    // Memory pressure
    else if (m_runtimeState.memoryPressure > 1000) {
        m_runtimeState.currentBottleneck = Profiler::BottleneckType::MEMORY_BOUND;
    }
    // Thermal or unknown
    else if (m_runtimeState.thermalLimited) {
        m_runtimeState.currentBottleneck = Profiler::BottleneckType::THERMAL_CONSTRAINED;
    }
    // Stable if good frame rate
    else if (current.fps > 55) {
        m_runtimeState.currentBottleneck = Profiler::BottleneckType::STABLE;
    }
    else {
        m_runtimeState.currentBottleneck = Profiler::BottleneckType::UNKNOWN;
    }
}

double AdaptiveEngine::calculateDecisionConfidence(const OptimizationDecision& decision) {
    // Base confidence from risk level
    double confidence = 1.0 - decision.riskLevel;
    
    // Adjust based on optimization mode
    switch (m_mode) {
        case PerformanceMode::SAFE:
            confidence *= 0.8; // More conservative
            break;
        case PerformanceMode::PERFORMANCE:
        case PerformanceMode::TURBO:
            confidence *= 1.2; // More confident
            break;
        default:
            break;
    }
    
    // Clip to valid range
    if (confidence < 0.0) confidence = 0.0;
    if (confidence > 1.0) confidence = 1.0;
    
    return confidence;
}

AdaptiveEngine::OptimizationResult AdaptiveEngine::executeDecision(const OptimizationDecision& decision) {
    OptimizationResult result;
    result.applied = true;
    result.benefitMs = decision.expectedBenefitMs * decision.intensity;
    result.message = "Applied optimization: " + decision.reason;
    
    // Log the decision
    m_isOptimized = true;
    
    return result;
}

bool AdaptiveEngine::shouldRollback(const OptimizationResult& result) {
    // Check if any of the last few frames have worsened
    auto& history = m_profiler->getSnapshot().frameHistory;
    if (history.size() < 10) return false;
    
    double recentAvg = 0.0;
    double beforeAvg = 0.0;
    
    // Average of last 5 frames
    for (size_t i = 0; i < 5 && i < history.size(); i++) {
        recentAvg += history[history.size() - 1 - i];
    }
    recentAvg /= 5.0;
    
    // Average of frames before that
    for (size_t i = 5; i < 10 && i < history.size(); i++) {
        beforeAvg += history[history.size() - 1 - i];
    }
    beforeAvg /= 5.0;
    
    // Rollback if recent performance is worse
    return recentAvg > beforeAvg * 1.1;
}

std::vector<AdaptiveEngine::OptimizationDecision> AdaptiveEngine::generateParticleOptimizations() {
    std::vector<OptimizationDecision> decisions;
    
    OptimizationDecision dec;
    dec.action = OptimizationAction::REDUCE_PARTICLES;
    dec.intensity = 0.5;
    dec.reason = "Particle system optimization";
    dec.expectedBenefitMs = 2.0;
    dec.riskLevel = 0.1; // Low risk
    
    decisions.push_back(dec);
    return decisions;
}

std::vector<AdaptiveEngine::OptimizationDecision> AdaptiveEngine::generateGlowOptimizations() {
    std::vector<OptimizationDecision> decisions;
    
    // Check if glow is expensive
    auto particles = m_analyzer->findByEffectCategory(LevelAnalyzer::EffectCategory::GLOW);
    if (!particles.empty()) {
        OptimizationDecision dec;
        dec.action = OptimizationAction::ADAPT_GLOW;
        dec.intensity = 0.3;
        dec.reason = "Adaptive glow optimization detected";
        dec.expectedBenefitMs = 1.5;
        dec.riskLevel = 0.2;
        
        decisions.push_back(dec);
    }
    
    return decisions;
}

std::vector<AdaptiveEngine::OptimizationDecision> AdaptiveEngine::generateCullingOptimizations() {
    std::vector<OptimizationDecision> decisions;
    
    auto culled = m_analyzer->findPotentiallyCullable();
    if (culled.size() > 100) {
        OptimizationDecision dec;
        dec.action = OptimizationAction::INCREASE_CULLING;
        dec.intensity = 0.4;
        dec.reason = "Visibility culling for distant objects";
        dec.expectedBenefitMs = 1.0;
        dec.riskLevel = 0.3;
        
        decisions.push_back(dec);
    }
    
    return decisions;
}

std::vector<AdaptiveEngine::OptimizationDecision> AdaptiveEngine::generateCachingOptimizations() {
    std::vector<OptimizationDecision> decisions;
    
    auto statics = m_analyzer->findStaticObjects();
    if (statics.size() > 50) {
        OptimizationDecision dec;
        dec.action = OptimizationAction::STATIC_CACHING;
        dec.intensity = 0.6;
        dec.reason = "Static object caching";
        dec.expectedBenefitMs = 1.2;
        dec.riskLevel = 0.2;
        
        decisions.push_back(dec);
    }
    
    return decisions;
}

std::vector<AdaptiveEngine::OptimizationDecision> AdaptiveEngine::generateMemoryOptimizations() {
    std::vector<OptimizationDecision> decisions;
    
    if (m_runtimeState.memoryPressure > 800) {
        OptimizationDecision dec;
        dec.action = OptimizationAction::MEMORY_CLEANUP;
        dec.intensity = 0.7;
        dec.reason = "Memory pressure detected";
        dec.expectedBenefitMs = 0.5;
        dec.riskLevel = 0.4;
        
        decisions.push_back(dec);
    }
    
    return decisions;
}

AdaptiveEngine::OptimizationDecision AdaptiveEngine::getLastDecision() const {
    static OptimizationDecision empty;
    if (m_pendingDecisions.empty()) return empty;
    return m_pendingDecisions.back();
}

AdaptiveEngine::OptimizationResult AdaptiveEngine::getLastResult() const {
    static OptimizationResult empty;
    if (m_history.empty()) return empty;
    return m_history.back();
}

void AdaptiveEngine::triggerManualOptimization() {
    // Force generation of optimization decisions
    m_pendingDecisions = generateDecisions();
    if (!m_pendingDecisions.empty()) {
        OptimizationResult result = executeDecision(m_pendingDecisions[0]);
        m_history.push_back(result);
        m_isOptimized = true;
    }
}

void AdaptiveEngine::rollbackLastOptimization() {
    // Clear pending decisions and mark rollback
    m_pendingDecisions.clear();
    
    if (!m_history.empty()) {
        auto& last = m_history.back();
        last.rollbackNeeded = false;
        last.applied = false;
        last.message = "Rolled back optimization";
    }
    
    m_isOptimized = false;
}

void AdaptiveEngine::resetOptimizationState() {
    m_pendingDecisions.clear();
    m_history.clear();
    m_isOptimized = false;
    m_runtimeState = RuntimeState();
}

AdaptiveEngine::EngineReport AdaptiveEngine::getReport() const {
    EngineReport report;
    report.bottleneck = m_profiler->getSnapshot().getBottleneckDescription();
    report.primaryCostSource = m_analyzer->getProfile().primaryBottleneck;
    report.confidence = calculateDecisionConfidence(getLastDecision());
    report.shouldOptimize = !m_pendingDecisions.empty();
    report.status = m_isOptimized ? "Optimized" : "Normal";
    
    // Recommend action based on bottleneck
    switch (m_runtimeState.currentBottleneck) {
        case Profiler::BottleneckType::GPU_BOUND:
            report.recommendedAction = "Reduce particles, adaptive glow";
            break;
        case Profiler::BottleneckType::CPU_BOUND:
            report.recommendedAction = "Increase object pooling, caching";
            break;
        case Profiler::BottleneckType::MEMORY_BOUND:
            report.recommendedAction = "Memory cleanup, resource management";
            break;
        default:
            report.recommendedAction = "Monitor and maintain";
            break;
    }
    
    return report;
}

} // namespace fleetapex
