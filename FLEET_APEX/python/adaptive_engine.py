"""
FLEET APEX Adaptive Runtime Engine

Decision and execution layer

Answers: "Performance is dropping. What can safely be changed right now?"
"""

from dataclasses import dataclass, field
from typing import Dict, List, Optional, Callable, TYPE_CHECKING
from enum import Enum
import time
from profiler import Profiler, BottleneckType
from level_analyzer import LevelAnalyzer, EffectCategory
from cpu_liberation_engine import CPULiberationEngine, CPULiberationAction
from gpu_assist_engine import GPUAssistEngine, GPUAssistAction
from bottleneck_classifier import BottleneckClassifier, BottleneckReport, BottleneckState, ConfidenceLevel
from abc import ABC, abstractmethod


class OptimizationAction(Enum):
    NONE = "none"
    REDUCE_PARTICLES = "reduce_particles"
    ADAPT_GLOW = "adapt_glow"
    INCREASE_CULLING = "increase_culling"
    STATIC_CACHING = "static_caching"
    REDUCE_BACKGROUND = "reduce_background"
    MEMORY_CLEANUP = "memory_cleanup"
    LOW_QUALITY_MODE = "low_quality_mode"
    CUSTOM_OPTIMIZATION = "custom_optimization"


@dataclass
class OptimizationDecision:
    """Decision about what optimization to apply."""
    action: str = "none"
    intensity: float = 0.0
    reason: str = ""
    expectedBenefitMs: float = 0.0
    riskLevel: float = 0.0
    affectedObjectIds: List[int] = field(default_factory=list)


@dataclass
class OptimizationResult:
    """Result of applying an optimization."""
    applied: bool = False
    benefitMs: float = 0.0
    regressionMs: float = 0.0
    rollbackNeeded: bool = False
    message: str = ""


@dataclass
class RuntimeState:
    """Current runtime performance state."""
    currentBottleneck: BottleneckType = BottleneckType.UNKNOWN
    frameTimeStability: float = 0.0
    recentSpikeCount: int = 0
    cpuUsageRatio: float = 0.0
    gpuUsageRatio: float = 0.0
    memoryPressure: int = 0
    thermalLimited: bool = False
    stateDescription: str = ""


class PerformanceMode(Enum):
    SAFE = "safe"        # Highly conservative
    BALANCED = "balanced" # Normal adaptive
    PERFORMANCE = "performance" # Aggressive visual
    TURBO = "turbo"      # Extreme weak hardware
    CUSTOM = "custom"    # User-defined


class AdaptiveEngine:
    """
    Adaptive runtime engine for making optimization decisions.
    Implements the closed-loop: Observe -> Analyze -> Decide -> Optimize -> Measure -> Adapt
    """
    
    def __init__(self, profiler: Profiler, analyzer: LevelAnalyzer,
                 cpu_liberation: Optional[CPULiberationEngine] = None,
                 gpu_assist: Optional[GPUAssistEngine] = None,
                 bottleneck_classifier: Optional[BottleneckClassifier] = None):
        self._profiler = profiler
        self._analyzer = analyzer
        self._runtimeState = RuntimeState()
        self._pendingDecisions: List[OptimizationDecision] = []
        self._history: List[OptimizationResult] = []
        self._mode = PerformanceMode.BALANCED
        self._targetFrameTimeMs = 16.67
        self._spikeThresholdMs = 30.0
        self._spikeWindowFrames = 300
        self._isOptimized = False
        self._lastDecision: Optional[OptimizationDecision] = None
        self._lastResult: Optional[OptimizationResult] = None
        
        # Initialize engines - create if not provided
        self._cpuLiberation = cpu_liberation or CPULiberationEngine(profiler, analyzer)
        self._gpuAssist = gpu_assist or GPUAssistEngine(profiler, analyzer)
        self._bottleneckClassifier = bottleneck_classifier or BottleneckClassifier(profiler)
        self._is_active = True
        
    def initialize(self) -> None:
        """Initialize the adaptive engine."""
        self._updateRuntimeState()
        self._classifyBottleneck()
        
    def update(self) -> None:
        """Main update loop - called per frame."""
        if not self._profiler or not self._analyzer:
            return
            
        # Update profiler measurements
        self._profiler.classifyBottleneck()
        
        # Update bottleneck classification with new system
        bottleneck_report = self._bottleneckClassifier.classify()
        
        # Update runtime state
        self._updateRuntimeState()
        
        # Analyze CPU liberation opportunities
        cpu_analysis = self._cpuLiberation.analyze()
        
        # Analyze GPU assist opportunities
        gpu_candidates = self._gpuAssist.analyze()
        
        # Generate optimization decisions using CPU↔GPU balancer logic
        self._pendingDecisions = self._generateDecisions(bottleneck_report, cpu_analysis, gpu_candidates)
        
        # Execute the best decision
        if self._pendingDecisions:
            # Sort by risk level (low risk first) and expected benefit (high benefit first)
            self._pendingDecisions.sort(
                key=lambda d: (d.riskLevel, -d.expectedBenefitMs)
            )
            
            # Execute highest priority decision
            result = self._executeDecision(self._pendingDecisions[0])
            self._lastResult = result
            self._history.append(result)
            self._isOptimized = True
            
            # Keep only last 100 results
            if len(self._history) > 100:
                self._history.pop(0)
    
    def _updateRuntimeState(self) -> None:
        """Update the runtime state from profiler data."""
        snapshot = self._profiler.snapshot
        current = snapshot.current
        
        self._runtimeState.currentBottleneck = snapshot.bottleneck
        
        # Calculate frame time stability
        if current.frameTimeMs > 0:
            self._runtimeState.frameTimeStability = 1.0 - (
                current.frameTimeVariance / (current.frameTimeMs * current.frameTimeMs + 0.001)
            )
        
        # Count recent spikes
        self._runtimeState.recentSpikeCount = self._profiler.countSpikes(self._spikeThresholdMs)
        
        # CPU/GPU ratios
        if current.frameTimeMs > 0:
            self._runtimeState.cpuUsageRatio = current.cpuTimeMs / current.frameTimeMs
            self._runtimeState.gpuUsageRatio = current.gpuTimeMs / current.frameTimeMs
            
        # Memory pressure
        self._runtimeState.memoryPressure = current.memoryUsageMb
        
        # Update description
        self._runtimeState.stateDescription = (
            f"Bottleneck: {self._profiler.getBottleneckDescription()}"
        )
        
    def _classifyBottleneck(self) -> None:
        """Classify the current performance bottleneck."""
        # Already done in profiler.classifyBottleneck()
        # This method can be extended for more detailed analysis
        pass
        
    def _generateDecisions(self, bottleneck_report: Optional[BottleneckReport] = None,
                           cpu_analysis: Optional[Dict[str, bool]] = None,
                           gpu_candidates: Optional[List] = None) -> List[OptimizationDecision]:
        """Generate optimization decisions based on current state and new engines."""
        decisions: List[OptimizationDecision] = []
        
        # Use bottleneck classification if available, otherwise fall back to old logic
        if bottleneck_report:
            # Generate decisions based on CPU liberation and GPU assist engines
            decisions = self._generateDecisionsFromEngines(bottleneck_report, cpu_analysis or {}, gpu_candidates or [])
        else:
            # Legacy path for backward compatibility
            if self._mode == PerformanceMode.SAFE:
                # Only very conservative optimizations
                for dec in self._generateParticleOptimizations():
                    decisions.append(dec)
                    
            elif self._mode == PerformanceMode.BALANCED:
                # Normal adaptive set
                for dec in self._generateParticleOptimizations():
                    decisions.append(dec)
                for dec in self._generateGlowOptimizations():
                    decisions.append(dec)
                for dec in self._generateCullingOptimizations():
                    decisions.append(dec)
                    
            elif self._mode == PerformanceMode.PERFORMANCE:
                # More aggressive
                for dec in self._generateParticleOptimizations():
                    decisions.append(dec)
                for dec in self._generateGlowOptimizations():
                    decisions.append(dec)
                for dec in self._generateCullingOptimizations():
                    decisions.append(dec)
                for dec in self._generateCachingOptimizations():
                    decisions.append(dec)
                    
            elif self._mode == PerformanceMode.TURBO:
                # Maximum optimization
                for dec in self._generateParticleOptimizations():
                    decisions.append(dec)
                for dec in self._generateGlowOptimizations():
                    decisions.append(dec)
                for dec in self._generateCullingOptimizations():
                    decisions.append(dec)
                for dec in self._generateCachingOptimizations():
                    decisions.append(dec)
                for dec in self._generateMemoryOptimizations():
                    decisions.append(dec)
                    
        return decisions
        
    def _generateDecisionsFromEngines(self, bottleneck_report: BottleneckReport,
                                       cpu_analysis: Dict[str, bool],
                                       gpu_candidates: List) -> List[OptimizationDecision]:
        """Generate decisions from new CPU/GPU engines."""
        decisions: List[OptimizationDecision] = []
        
        # Get available CPU actions
        cpu_actions = []
        if cpu_analysis.get("can_pool_objects"):
            cpu_actions.append(CPULiberationAction.INCREASE_OBJECT_POOLING)
        if cpu_analysis.get("can_cache_static"):
            cpu_actions.append(CPULiberationAction.STATIC_CACHING)
        if cpu_analysis.get("needs_processing_reduction"):
            cpu_actions.append(CPULiberationAction.REDUCE_REDUNDANT_PROCESSING)
        if cpu_analysis.get("can_preprocess"):
            cpu_actions.append(CPULiberationAction.SAFE_PREPROCESSING)
        if cpu_analysis.get("can_optimize_visibility"):
            cpu_actions.append(CPULiberationAction.OPTIMIZE_VISIBILITY_CHECKS)
            cpu_actions.append(CPULiberationAction.OPTIMIZE_VISIBILITY_SCANS)
        if cpu_analysis.get("can_optimize_triggers"):
            cpu_actions.append(CPULiberationAction.OPTIMIZE_TRIGGER_PROCESSING)
            
        # Filter safe GPU candidates
        safe_gpu_candidates = [c for c in gpu_candidates if getattr(c, 'safetyPassed', True)]
        
        # Generate decisions based on bottleneck state
        if bottleneck_report.state == BottleneckState.CPU_BOUND:
            # CPU-bound: prioritize CPU liberation
            for action in cpu_actions[:3]:  # Limit to top 3
                dec = self._createDecisionFromCpuAction(action, bottleneck_report)
                decisions.append(dec)
                
            # Also include GPU actions that also help CPU (reduce submission)
            gpu_help_cpu = [c for c in safe_gpu_candidates 
                           if c.action in [GPUAssistAction.REDUCE_CPU_SUBMISSION, 
                                     GPUAssistAction.STATIC_BATCHING]]
            for candidate in gpu_help_cpu[:2]:
                dec = self._createDecisionFromGpuCandidate(candidate, bottleneck_report)
                decisions.append(dec)
                
        elif bottleneck_report.state == BottleneckState.GPU_BOUND:
            # GPU-bound: prioritize GPU assist
            for candidate in safe_gpu_candidates[:3]:
                dec = self._createDecisionFromGpuCandidate(candidate, bottleneck_report)
                decisions.append(dec)
                
        elif bottleneck_report.state == BottleneckState.HYBRID:
            # Hybrid: balanced approach
            for action in cpu_actions[:2]:
                dec = self._createDecisionFromCpuAction(action, bottleneck_report)
                decisions.append(dec)
            for candidate in safe_gpu_candidates[:2]:
                dec = self._createDecisionFromGpuCandidate(candidate, bottleneck_report)
                decisions.append(dec)
                
        elif bottleneck_report.state == BottleneckState.MEMORY_BOUND:
            # Memory pressure: reduce allocations
            if cpu_analysis.get("can_pool_objects"):
                dec = self._createDecisionFromCpuAction(CPULiberationAction.INCREASE_OBJECT_POOLING, bottleneck_report)
                decisions.append(dec)
            if cpu_analysis.get("can_cache_static"):
                dec = self._createDecisionFromCpuAction(CPULiberationAction.STATIC_CACHING, bottleneck_report)
                decisions.append(dec)
                
        elif bottleneck_report.state == BottleneckState.STABLE:
            # Stable: only conservative optimizations
            if cpu_analysis.get("can_pool_objects") and cpu_analysis.get("needs_processing_reduction"):
                dec = self._createDecisionFromCpuAction(CPULiberationAction.INCREASE_OBJECT_POOLING, bottleneck_report)
                decisions.append(dec)
                
        # Unknown: don't apply aggressive optimizations
        
        # Sort by risk (low first) and expected benefit (high first)
        decisions.sort(key=lambda d: (d.riskLevel, -d.expectedBenefitMs))
        
        return decisions
        
    def _createDecisionFromCpuAction(self, action: CPULiberationAction, 
                                      bottleneck_report: BottleneckReport) -> OptimizationDecision:
        """Create optimization decision from CPU liberation action."""
        dec = OptimizationDecision()
        dec.reason = f"CPU Liberation: {action.value}"
        
        # Map action to existing optimization actions
        action_mapping = {
            CPULiberationAction.INCREASE_OBJECT_POOLING: OptimizationAction.STATIC_CACHING,
            CPULiberationAction.STATIC_CACHING: OptimizationAction.STATIC_CACHING,
            CPULiberationAction.REDUCE_REDUNDANT_PROCESSING: OptimizationAction.REDUCE_BACKGROUND,
            CPULiberationAction.SAFE_PREPROCESSING: OptimizationAction.STATIC_CACHING,
            CPULiberationAction.OPTIMIZE_VISIBILITY_CHECKS: OptimizationAction.INCREASE_CULLING,
            CPULiberationAction.OPTIMIZE_VISIBILITY_SCANS: OptimizationAction.INCREASE_CULLING,
            CPULiberationAction.OPTIMIZE_TRIGGER_PROCESSING: OptimizationAction.REDUCE_BACKGROUND,
            CPULiberationAction.BATCH_SIMILAR_OPERATIONS: OptimizationAction.STATIC_CACHING,
            CPULiberationAction.DELAY_NON_CRITICAL_UPDATES: OptimizationAction.REDUCE_BACKGROUND,
        }
        
        dec.action = action_mapping.get(action, OptimizationAction.CUSTOM_OPTIMIZATION)
        dec.intensity = 0.5
        dec.expectedBenefitMs = 1.5
        dec.riskLevel = 0.1
        
        return dec
        
    def _createDecisionFromGpuCandidate(self, candidate, 
                                         bottleneck_report: BottleneckReport) -> OptimizationDecision:
        """Create optimization decision from GPU assist candidate."""
        dec = OptimizationDecision()
        dec.reason = f"GPU Assist: {candidate.description}"
        
        action_mapping = {
            GPUAssistAction.STATIC_BATCHING: OptimizationAction.STATIC_CACHING,
            GPUAssistAction.INSTANCING: OptimizationAction.STATIC_CACHING,
            GPUAssistAction.GPU_PARTICLES: OptimizationAction.REDUCE_PARTICLES,
            GPUAssistAction.SHADER_EFFECTS: OptimizationAction.ADAPT_GLOW,
            GPUAssistAction.BACKGROUND_ANIMATION: OptimizationAction.REDUCE_BACKGROUND,
            GPUAssistAction.VISUAL_TRANSFORMATIONS: OptimizationAction.STATIC_CACHING,
            GPUAssistAction.REDUCE_CPU_SUBMISSION: OptimizationAction.STATIC_CACHING,
            GPUAssistAction.STATIC_SCENE_REPRESENTATION: OptimizationAction.STATIC_CACHING,
        }
        
        dec.action = action_mapping.get(candidate.action, OptimizationAction.CUSTOM_OPTIMIZATION)
        dec.intensity = 0.4
        dec.expectedBenefitMs = candidate.estimatedCpuReductionMs
        dec.riskLevel = candidate.riskLevel
        
        return dec
        
    def _generateParticleOptimizations(self) -> List[OptimizationDecision]:
        """Generate particle optimization decisions."""
        decisions: List[OptimizationDecision] = []
        
        dec = OptimizationDecision()
        dec.action = OptimizationAction.REDUCE_PARTICLES
        dec.intensity = 0.5
        dec.reason = "Particle system optimization"
        dec.expectedBenefitMs = 2.0
        dec.riskLevel = 0.1
        
        decisions.append(dec)
        return decisions
        
    def _generateGlowOptimizations(self) -> List[OptimizationDecision]:
        """Generate glow-related optimizations."""
        from level_analyzer import EffectCategory
        decisions: List[OptimizationDecision] = []
        
        # Check if glow is expensive
        glowObjects = self._analyzer.findByEffectCategory(
            EffectCategory.GLOW
        )
        if glowObjects:
            dec = OptimizationDecision()
            dec.action = OptimizationAction.ADAPT_GLOW
            dec.intensity = 0.3
            dec.reason = "Adaptive glow optimization detected"
            dec.expectedBenefitMs = 1.5
            dec.riskLevel = 0.2
            
            decisions.append(dec)
            
        return decisions
        
    def _generateCullingOptimizations(self) -> List[OptimizationDecision]:
        """Generate culling optimizations."""
        decisions: List[OptimizationDecision] = []
        
        culled = self._analyzer.findPotentiallyCullable()
        if len(culled) > 100:
            dec = OptimizationDecision()
            dec.action = OptimizationAction.INCREASE_CULLING
            dec.intensity = 0.4
            dec.reason = "Visibility culling for distant objects"
            dec.expectedBenefitMs = 1.0
            dec.riskLevel = 0.3
            
            decisions.append(dec)
            
        return decisions
        
    def _generateCachingOptimizations(self) -> List[OptimizationDecision]:
        """Generate static caching optimizations."""
        decisions: List[OptimizationDecision] = []
        
        statics = self._analyzer.findStaticObjects()
        if len(statics) > 50:
            dec = OptimizationDecision()
            dec.action = OptimizationAction.STATIC_CACHING
            dec.intensity = 0.6
            dec.reason = "Static object caching"
            dec.expectedBenefitMs = 1.2
            dec.riskLevel = 0.2
            
            decisions.append(dec)
            
        return decisions
        
    def _generateMemoryOptimizations(self) -> List[OptimizationDecision]:
        """Generate memory-related optimizations."""
        decisions: List[OptimizationDecision] = []
        
        if self._runtimeState.memoryPressure > 800:
            dec = OptimizationDecision()
            dec.action = OptimizationAction.MEMORY_CLEANUP
            dec.intensity = 0.7
            dec.reason = "Memory pressure detected"
            dec.expectedBenefitMs = 0.5
            dec.riskLevel = 0.4
            
            decisions.append(dec)
            
        return decisions
        
    def setPerformanceMode(self, mode: PerformanceMode) -> None:
        """Set the performance mode."""
        self._mode = mode
        
    def setTargetFrameRate(self, fps: float) -> None:
        """Set target frame rate."""
        if fps > 0:
            self._targetFrameTimeMs = 1000.0 / fps
            
    def setSpikeThreshold(self, ms: float) -> None:
        """Set spike detection threshold."""
        self._spikeThresholdMs = ms
        
    @property
    def mode(self) -> PerformanceMode:
        return self._mode
        
    @property
    def runtimeState(self) -> RuntimeState:
        return self._runtimeState
        
    @property
    def lastDecision(self) -> Optional[OptimizationDecision]:
        return self._lastDecision
        
    @property
    def lastResult(self) -> Optional[OptimizationResult]:
        return self._lastResult
        
    @property
    def pendingDecisions(self) -> List[OptimizationDecision]:
        return self._pendingDecisions.copy()
        
    def clearPendingDecisions(self) -> None:
        """Clear pending decisions."""
        self._pendingDecisions.clear()
        
    def triggerManualOptimization(self) -> None:
        """Manually trigger optimization."""
        self._pendingDecisions = self._generateDecisions()
        if self._pendingDecisions:
            result = self._executeDecision(self._pendingDecisions[0])
            self._history.append(result)
            self._isOptimized = True
            
    def rollbackLastOptimization(self) -> None:
        """Rollback the last optimization."""
        self._pendingDecisions.clear()
        
        if self._history:
            last = self._history[-1]
            last.rollbackNeeded = False
            last.applied = False
            last.message = "Rolled back optimization"
            
        self._isOptimized = False
        
    def resetOptimizationState(self) -> None:
        """Reset optimization state."""
        self._pendingDecisions.clear()
        self._history.clear()
        self._isOptimized = False
        self._runtimeState = RuntimeState()
        
    def getReport(self) -> Dict:
        """Get a report of current state."""
        return {
            "bottleneck": self._profiler.getBottleneckDescription(),
            "primary_cost_source": self._analyzer.profile.primaryBottleneck,
            "recommended_action": self._getRecommendedAction(),
            "confidence": self._calculateConfidence(),
            "should_optimize": len(self._pendingDecisions) > 0,
            "status": "Optimized" if self._isOptimized else "Normal"
        }
        
    def _getRecommendedAction(self) -> str:
        """Get recommended optimization action."""
        bottleneck = self._runtimeState.currentBottleneck
        
        if bottleneck == BottleneckType.GPU_BOUND:
            return "Reduce particles, adaptive glow"
        elif bottleneck == BottleneckType.CPU_BOUND:
            return "Increase object pooling, caching"
        elif bottleneck == BottleneckType.MEMORY_BOUND:
            return "Memory cleanup, resource management"
        else:
            return "Monitor and maintain"
            
    def _calculateConfidence(self) -> float:
        """Calculate confidence level."""
        if not self._lastDecision:
            return 0.0
            
        confidence = 1.0 - self._lastDecision.riskLevel
        
        # Adjust for performance mode
        if self._mode in [PerformanceMode.PERFORMANCE, PerformanceMode.TURBO]:
            confidence = min(1.0, confidence * 1.2)
        elif self._mode == PerformanceMode.SAFE:
            confidence = confidence * 0.8
            
        return max(0.0, min(1.0, confidence))
        
    @property
    def isOptimized(self) -> bool:
        return self._isOptimized


if __name__ == "__main__":
    print("=== ADAPTIVE ENGINE TEST ===")
    
    from profiler import Profiler
    from level_analyzer import LevelAnalyzer
    
    profiler = Profiler(historySize=120)
    analyzer = LevelAnalyzer()
    
    engine = AdaptiveEngine(profiler, analyzer)
    engine.setPerformanceMode(PerformanceMode.BALANCED)
    
    # Setup simulated data
    profiler.setProfilingEnabled(True)
    
    # Create a level profile
    analyzer.beginLevelScan("Benchmark Level", 99999)
    for i in range(500):
        analyzer.addObject(i, i % 2000, i % 1500, 1)
    for i in range(50):
        analyzer.addParticleSystem(500 + i, i * 100, 500)
    analyzer.endLevelScan()
    
    # Simulate performance conditions
    profiler.setObjectCount(550)
    profiler.setVisibleObjectCount(200)
    profiler.setParticleCount(50)
    profiler.setMemoryUsage(450)
    
    # Simulate a few frames
    for i in range(30):
        profiler.beginFrame()
        profiler.endFrame()
    
    # Get report
    report = engine.getReport()
    print(f"Bottleneck: {report['bottleneck']}")
    print(f"Primary Cost: {report['primary_cost_source']}")
    print(f"Recommended: {report['recommended_action']}")
    print(f"Should Optimize: {report['should_optimize']}")
    print(f"Status: {report['status']}")
    print(f"Confidence: {report['confidence']:.2f}")
    
    print("\nAdaptive Engine test PASSED")
