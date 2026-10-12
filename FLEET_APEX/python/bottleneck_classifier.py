"""
FLEET APEX Bottleneck Classifier

Classifies the current performance bottleneck state using multiple observations.
"""

from dataclasses import dataclass
from enum import Enum
from typing import Optional, List
from profiler import Profiler, PerformanceSnapshot


class BottleneckState(Enum):
    """Possible bottleneck classifications"""
    CPU_BOUND = "cpu_bound"
    GPU_BOUND = "gpu_bound"
    MEMORY_BOUND = "memory_bound"
    LOADING_BOUND = "loading_bound"
    THERMALLY_CONSTRAINED = "thermally_constrained"
    HYBRID = "hybrid"
    STABLE = "stable"
    UNKNOWN = "unknown"


class ConfidenceLevel(Enum):
    HIGH = "high"
    MEDIUM = "medium"
    LOW = "low"
    UNKNOWN = "unknown"


@dataclass
class SpikeCorrelation:
    """Correlation of a frame spike with a workload type"""
    spikeFrameTime: float
    correlationSource: str
    confidence: ConfidenceLevel
    details: str = ""


@dataclass
class BottleneckReport:
    """Complete bottleneck analysis"""
    state: BottleneckState
    confidence: ConfidenceLevel
    cpuWorkRatio: float
    gpuWorkRatio: float
    frameTime: float
    targetFrameTime: float
    stabilityScore: float
    spikeCorrelations: List[SpikeCorrelation]
    primaryCpuCostSource: str
    primaryGpuCostSource: str
    description: str


class BottleneckClassifier:
    """
    Classifies the current performance bottleneck using multiple observations.
    Does not rely on single utilization numbers.
    """
    
    def __init__(self, profiler: Profiler):
        self._profiler = profiler
        self._history: List[BottleneckReport] = []
        self._targetFrameTime = 16.67  # 60 FPS default
        
    def setTargetFrameTime(self, fps: float) -> None:
        if fps > 0:
            self._targetFrameTime = 1000.0 / fps
            
    def classify(self) -> BottleneckReport:
        """Classify the current bottleneck state."""
        snapshot = self._profiler.snapshot
        current = snapshot.current
        
        # Calculate ratios
        cpuWorkRatio = 0.0
        gpuWorkRatio = 0.0
        if current.frameTimeMs > 0:
            cpuWorkRatio = current.cpuTimeMs / current.frameTimeMs
            gpuWorkRatio = current.gpuTimeMs / current.frameTimeMs
            
        # Stability score (inverse of variance, normalized)
        stabilityScore = 1.0
        if current.frameTimeMs > 0:
            stabilityScore = max(0.0, 1.0 - (current.frameTimeVariance / (current.frameTimeMs * current.frameTimeMs + 0.001)))
            
        # Classify based on multiple signals
        state, confidence = self._classifyBottleneck(
            cpuWorkRatio, gpuWorkRatio, current.fps, current.frameTimeMs,
            current.memoryUsageMb, stabilityScore
        )
        
        # Correlate spikes
        spikeCorrelations = self._correlateSpikes(snapshot)
        
        # Identify primary cost sources
        primaryCpu = self._identifyPrimaryCpuCostSource(current, snapshot)
        primaryGpu = self._identifyPrimaryGpuCostSource(current, snapshot)
        
        report = BottleneckReport(
            state=state,
            confidence=confidence,
            cpuWorkRatio=cpuWorkRatio,
            gpuWorkRatio=gpuWorkRatio,
            frameTime=current.frameTimeMs,
            targetFrameTime=self._targetFrameTime,
            stabilityScore=stabilityScore,
            spikeCorrelations=spikeCorrelations,
            primaryCpuCostSource=primaryCpu,
            primaryGpuCostSource=primaryGpu,
            description=self._generateDescription(state, cpuWorkRatio, gpuWorkRatio, stabilityScore)
        )
        
        self._history.append(report)
        if len(self._history) > 100:
            self._history.pop(0)
            
        return report
        
    def _classifyBottleneck(self, cpuRatio: float, gpuRatio: float, fps: float, 
                           frameTime: float, memoryMb: int, stability: float) -> tuple:
        """Classify bottleneck using multiple signals."""
        
        # Severe memory pressure
        if memoryMb > 1500:
            return BottleneckState.MEMORY_BOUND, ConfidenceLevel.MEDIUM
            
        # Frame time severely high
        if frameTime > self._targetFrameTime * 2.0:
            if cpuRatio > 0.7:
                return BottleneckState.CPU_BOUND, ConfidenceLevel.HIGH
            elif gpuRatio > 0.7:
                return BottleneckState.GPU_BOUND, ConfidenceLevel.HIGH
            else:
                return BottleneckState.HYBRID, ConfidenceLevel.MEDIUM
                
        # Frame time moderately high
        if frameTime > self._targetFrameTime * 1.3:
            if cpuRatio > 0.65 and gpuRatio > 0.65:
                return BottleneckState.HYBRID, ConfidenceLevel.MEDIUM
            elif cpuRatio > 0.6:
                return BottleneckState.CPU_BOUND, ConfidenceLevel.HIGH
            elif gpuRatio > 0.6:
                return BottleneckState.GPU_BOUND, ConfidenceLevel.HIGH
            else:
                return BottleneckState.UNKNOWN, ConfidenceLevel.LOW
                
        # Frame time near target but unstable
        if frameTime > self._targetFrameTime * 1.05:
            if stability < 0.8:
                return BottleneckState.HYBRID, ConfidenceLevel.LOW
                
        # Stable
        if fps >= 55 and stability > 0.9:
            return BottleneckState.STABLE, ConfidenceLevel.HIGH
            
        return BottleneckState.UNKNOWN, ConfidenceLevel.LOW
        
    def _correlateSpikes(self, snapshot: PerformanceSnapshot) -> List[SpikeCorrelation]:
        """Correlate frame spikes with active workloads."""
        correlations = []
        
        if snapshot.recentSpikeIndex >= 0 and snapshot.frameHistory:
            spikeIdx = snapshot.recentSpikeIndex
            spikeTime = snapshot.frameHistory[spikeIdx]
            
            # Simple correlation based on available data
            if snapshot.current.particleCount > 100:
                correlations.append(SpikeCorrelation(
                    spikeFrameTime=spikeTime,
                    correlationSource="particle_burst",
                    confidence=ConfidenceLevel.MEDIUM,
                    details=f"Particles: {snapshot.current.particleCount}"
                ))
            elif snapshot.current.objectCount > 5000:
                correlations.append(SpikeCorrelation(
                    spikeFrameTime=spikeTime,
                    correlationSource="object_processing",
                    confidence=ConfidenceLevel.MEDIUM,
                    details=f"Objects: {snapshot.current.objectCount}"
                ))
            elif snapshot.current.gpuTimeMs > snapshot.current.cpuTimeMs:
                correlations.append(SpikeCorrelation(
                    spikeFrameTime=spikeTime,
                    correlationSource="render_workload",
                    confidence=ConfidenceLevel.LOW,
                    details="GPU time dominant"
                ))
            else:
                correlations.append(SpikeCorrelation(
                    spikeFrameTime=spikeTime,
                    correlationSource="unknown",
                    confidence=ConfidenceLevel.UNKNOWN,
                    details="No clear correlation found"
                ))
                
        return correlations
        
    def _identifyPrimaryCpuCostSource(self, current, snapshot) -> str:
        """Identify primary CPU cost source."""
        if current.objectCount > 10000:
            return "high_object_processing"
        elif current.particleCount > 500:
            return "particle_update"
        elif current.frameTimeMs > 30 and current.cpuTimeMs > current.gpuTimeMs:
            return "object_update_logic"
        return "moderate_cpu_work"
        
    def _identifyPrimaryGpuCostSource(self, current, snapshot) -> str:
        """Identify primary GPU cost source."""
        if current.gpuTimeMs > 15:
            return "heavy_rendering"
        elif current.drawCalls > 1000:
            return "high_draw_calls"
        elif current.visibleObjectCount > 10000:
            return "overdraw"
        return "moderate_gpu_work"
        
    def _generateDescription(self, state: BottleneckState, cpuRatio: float, 
                           gpuRatio: float, stability: float) -> str:
        """Generate human-readable description."""
        if state == BottleneckState.CPU_BOUND:
            return f"CPU-bound: CPU ratio {cpuRatio:.1%}, frame time above target"
        elif state == BottleneckState.GPU_BOUND:
            return f"GPU-bound: GPU ratio {gpuRatio:.1%}, frame time above target"
        elif state == BottleneckState.HYBRID:
            return f"Hybrid bottleneck: CPU {cpuRatio:.1%}, GPU {gpuRatio:.1%}"
        elif state == BottleneckState.MEMORY_BOUND:
            return "Memory pressure detected"
        elif state == BottleneckState.STABLE:
            return f"Stable: {stability:.1%} frame time stability"
        return "Performance state unclear"
        
    def getHistory(self) -> List[BottleneckReport]:
        return self._history.copy()
        
    def getLastReport(self) -> Optional[BottleneckReport]:
        return self._history[-1] if self._history else None
