"""
FLEET APEX CPU↔GPU Balancer

Coordinates CPU Liberation and GPU Assist engines based on bottleneck classification.

This is the heart of the v0.2 architecture - deciding which processor is the 
limiting resource and routing optimization work accordingly.
"""

from dataclasses import dataclass
from typing import List, Dict, Optional
from enum import Enum
from bottleneck_classifier import BottleneckClassifier, BottleneckState, BottleneckReport
from cpu_liberation_engine import CPULiberationEngine, CPULiberationAction
from gpu_assist_engine import GPUAssistEngine, GPUOffloadCandidate
from profiler import Profiler
from level_analyzer import LevelAnalyzer
from adaptive_engine import AdaptiveEngine, PerformanceMode


class OptimizationPath(Enum):
    """Which optimization path to take."""
    CPU_LIBERATION = "cpu_liberation"
    GPU_ASSIST = "gpu_assist"
    HYBRID = "hybrid"
    STABLE = "stable"
    NONE = "none"


@dataclass
class BalancerDecision:
    """Decision from the CPU↔GPU balancer."""
    path: OptimizationPath
    reason: str
    confidence: float
    cpuActions: List[CPULiberationAction]
    gpuCandidates: List[GPUOffloadCandidate]
    expectedBenefitMs: float
    riskLevel: float


class CPUGPUBalancer:
    """
    CPU↔GPU Balancer - routes optimization work based on bottleneck analysis.
    
    This is the central coordinator that determines whether to optimize
    CPU, GPU, or both based on actual measured performance.
    """
    
    def __init__(self, profiler: Profiler, analyzer: LevelAnalyzer):
        self._profiler = profiler
        self._analyzer = analyzer
        self._bottleneckClassifier = BottleneckClassifier(profiler)
        self._cpuLiberation = CPULiberationEngine(profiler, analyzer)
        self._gpuAssist = GPUAssistEngine(profiler, analyzer)
        self._decision_history: List[BalancerDecision] = []
        self._is_active = False
        
    def enable(self) -> None:
        """Enable the balancer and sub-engines."""
        self._cpuLiberation.enable()
        self._gpuAssist.enable()
        self._is_active = True
        
    def disable(self) -> None:
        """Disable the balancer and sub-engines."""
        self._cpuLiberation.disable()
        self._gpuAssist.disable()
        self._is_active = False
        
    def isActive(self) -> bool:
        return self._is_active
        
    def setTargetFrameRate(self, fps: float) -> None:
        """Set target frame rate for bottleneck classification."""
        self._bottleneckClassifier.setTargetFrameTime(fps)
        
    def analyze(self) -> BalancerDecision:
        """Analyze current state and make optimization decision."""
        if not self._is_active:
            return BalancerDecision(
                path=OptimizationPath.NONE,
                reason="Balancer disabled",
                confidence=0.0,
                cpuActions=[],
                gpuCandidates=[],
                expectedBenefitMs=0.0,
                riskLevel=0.0
            )
            
        # Get bottleneck classification
        report = self._bottleneckClassifier.classify()
        
        # Get optimization opportunities
        cpu_analysis = self._cpuLiberation.analyze()
        gpu_candidates = self._gpuAssist.analyze()
        
        # Decide optimization path based on bottleneck
        decision = self._makeDecision(report, cpu_analysis, gpu_candidates)
        
        self._decision_history.append(decision)
        if len(self._decision_history) > 100:
            self._decision_history.pop(0)
            
        return decision
        
    def _makeDecision(self, report: BottleneckReport, cpu_analysis: Dict[str, bool],
                     gpu_candidates: List[GPUOffloadCandidate]) -> BalancerDecision:
        """Make optimization decision based on bottleneck state."""
        
        # CPU-BOUND: Focus on CPU liberation
        if report.state == BottleneckState.CPU_BOUND:
            return self._decideCpuPath(report, cpu_analysis, gpu_candidates)
            
        # GPU-BOUND: Focus on GPU assist
        elif report.state == BottleneckState.GPU_BOUND:
            return self._decideGpuPath(report, cpu_analysis, gpu_candidates)
            
        # HYBRID: Do both
        elif report.state == BottleneckState.HYBRID:
            return self._decideHybridPath(report, cpu_analysis, gpu_candidates)
            
        # MEMORY_BOUND: CPU liberation for allocations
        elif report.state == BottleneckState.MEMORY_BOUND:
            return self._decideMemoryPath(report, cpu_analysis)
            
        # STABLE: Monitor but don't aggressively optimize
        elif report.state == BottleneckState.STABLE:
            return self._decideStablePath(report, cpu_analysis, gpu_candidates)
            
        # UNKNOWN: Conservative approach
        else:
            return self._decideUnknownPath(report, cpu_analysis, gpu_candidates)
            
    def _decideCpuPath(self, report: BottleneckReport, cpu_analysis: Dict[str, bool],
                      gpu_candidates: List[GPUOffloadCandidate]) -> BalancerDecision:
        """Decision for CPU-bound state."""
        cpu_actions = []
        
        # Generate CPU actions based on analysis
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
            
        # Filter GPU candidates that also help CPU (reduce submission)
        gpu_help_cpu = [c for c in gpu_candidates 
                       if c.action in [GPUAssistAction.REDUCE_CPU_SUBMISSION, 
                                     GPUAssistAction.STATIC_BATCHING]]
        
        expected_benefit = len(cpu_actions) * 1.5 + len(gpu_help_cpu) * 2.0
        risk = len(cpu_actions) * 0.1
        
        return BalancerDecision(
            path=OptimizationPath.CPU_LIBERATION,
            reason=f"CPU bottleneck detected (CPU ratio: {report.cpuWorkRatio:.1%}). "
                  f"Focus: CPU liberation with GPU submission reduction.",
            confidence=0.85 if report.confidence.value == "high" else 0.6,
            cpuActions=cpu_actions,
            gpuCandidates=gpu_help_cpu,
            expectedBenefitMs=expected_benefit,
            riskLevel=risk
        )
        
    def _decideGpuPath(self, report: BottleneckReport, cpu_analysis: Dict[str, bool],
                      gpu_candidates: List[GPUOffloadCandidate]) -> BalancerDecision:
        """Decision for GPU-bound state."""
        # Filter safe GPU candidates
        safe_gpu_candidates = [c for c in gpu_candidates if c.safetyPassed]
        
        # Sort by benefit/risk ratio
        safe_gpu_candidates.sort(key=lambda c: c.estimatedCpuReductionMs / max(c.riskLevel, 0.01))
        
        # Limit to top 3 to avoid over-optimization
        selected_candidates = safe_gpu_candidates[:3]
        
        expected_benefit = sum(c.estimatedCpuReductionMs for c in selected_candidates)
        risk = max((c.riskLevel for c in selected_candidates), default=0.0)
        
        return BalancerDecision(
            path=OptimizationPath.GPU_ASSIST,
            reason=f"GPU bottleneck detected (GPU ratio: {report.gpuWorkRatio:.1%}). "
                  f"Focus: GPU assist with visual optimization.",
            confidence=0.85 if report.confidence.value == "high" else 0.6,
            cpuActions=[],
            gpuCandidates=selected_candidates,
            expectedBenefitMs=expected_benefit,
            riskLevel=risk
        )
        
    def _decideHybridPath(self, report: BottleneckReport, cpu_analysis: Dict[str, bool],
                         gpu_candidates: List[GPUOffloadCandidate]) -> BalancerDecision:
        """Decision for hybrid bottleneck."""
        # Do both but with lower intensity
        cpu_actions = []
        if cpu_analysis.get("can_pool_objects"):
            cpu_actions.append(CPULiberationAction.INCREASE_OBJECT_POOLING)
        if cpu_analysis.get("can_cache_static"):
            cpu_actions.append(CPULiberationAction.STATIC_CACHING)
            
        safe_gpu_candidates = [c for c in gpu_candidates if c.safetyPassed][:2]
        
        expected_benefit = len(cpu_actions) * 1.0 + sum(c.estimatedCpuReductionMs for c in safe_gpu_candidates)
        risk = len(cpu_actions) * 0.05 + max((c.riskLevel for c in safe_gpu_candidates), default=0.0)
        
        return BalancerDecision(
            path=OptimizationPath.HYBRID,
            reason=f"Hybrid bottleneck (CPU: {report.cpuWorkRatio:.1%}, GPU: {report.gpuWorkRatio:.1%}). "
                  f"Balanced CPU liberation and GPU assist.",
            confidence=0.7,
            cpuActions=cpu_actions,
            gpuCandidates=safe_gpu_candidates,
            expectedBenefitMs=expected_benefit,
            riskLevel=risk
        )
        
    def _decideMemoryPath(self, report: BottleneckReport, cpu_analysis: Dict[str, bool]) -> BalancerDecision:
        """Decision for memory-bound state."""
        cpu_actions = [CPULiberationAction.INCREASE_OBJECT_POOLING]
        if cpu_analysis.get("can_pool_objects"):
            cpu_actions.append(CPULiberationAction.STATIC_CACHING)
            
        return BalancerDecision(
            path=OptimizationPath.CPU_LIBERATION,
            reason=f"Memory pressure ({report.primaryCpuCostSource}). "
                  f"Focus: Reduce allocations and pool objects.",
            confidence=0.7,
            cpuActions=cpu_actions,
            gpuCandidates=[],
            expectedBenefitMs=len(cpu_actions) * 1.0,
            riskLevel=0.1
        )
        
    def _decideStablePath(self, report: BottleneckReport, cpu_analysis: Dict[str, bool],
                         gpu_candidates: List[GPUOffloadCandidate]) -> BalancerDecision:
        """Decision for stable state - maintain but monitor."""
        # Only apply very conservative optimizations
        cpu_actions = []
        if cpu_analysis.get("can_pool_objects") and cpu_analysis.get("needs_processing_reduction"):
            cpu_actions.append(CPULiberationAction.INCREASE_OBJECT_POOLING)
            
        return BalancerDecision(
            path=OptimizationPath.STABLE,
            reason=f"Stable performance ({report.frameTime:.1f}ms). Maintaining with monitoring.",
            confidence=0.9,
            cpuActions=cpu_actions,
            gpuCandidates=[],
            expectedBenefitMs=len(cpu_actions) * 0.5,
            riskLevel=0.05
        )
        
    def _decideUnknownPath(self, report: BottleneckReport, cpu_analysis: Dict[str, bool],
                          gpu_candidates: List[GPUOffloadCandidate]) -> BalancerDecision:
        """Decision for unknown state - conservative monitoring."""
        return BalancerDecision(
            path=OptimizationPath.NONE,
            reason=f"Performance state unknown. Monitoring only.",
            confidence=0.3,
            cpuActions=[],
            gpuCandidates=[],
            expectedBenefitMs=0.0,
            riskLevel=0.0
        )
        
    def executeDecision(self, decision: BalancerDecision) -> Dict[str, bool]:
        """Execute a balancer decision."""
        if not self._is_active:
            return {"success": False, "reason": "Balancer disabled"}
            
        results = {"cpu_actions": [], "gpu_candidates": []}
        
        # Execute CPU actions
        for action in decision.cpuActions:
            result = self._cpuLiberation.executeAction(action)
            results["cpu_actions"].append({"action": action.value, "success": result})
            
        # Execute GPU candidates
        for candidate in decision.gpuCandidates:
            result = self._gpuAssist.executeCandidate(candidate)
            results["gpu_candidates"].append({"action": candidate.action.value, "success": result})
            
        return results
        
    def getHistory(self) -> List[BalancerDecision]:
        return self._decision_history.copy()
        
    def getLastDecision(self) -> Optional[BalancerDecision]:
        return self._decision_history[-1] if self._decision_history else None
        
    def getStatus(self) -> str:
        return "ENABLED" if self._is_active else "DISABLED"
        
    def getSubEngineStatus(self) -> Dict[str, str]:
        return {
            "cpu_liberation": self._cpuLiberation.getStatus(),
            "gpu_assist": self._gpuAssist.getStatus(),
            "bottleneck_classifier": "ACTIVE"
        }
        
    def getBottleneckHistory(self) -> List[BottleneckReport]:
        return self._bottleneckClassifier.getHistory()
