"""
FLEET APEX Profiler Module
Core measurement and observability layer

Answers: "What is Geometry Dash spending time on right now?"
"""

from dataclasses import dataclass, field
from typing import Dict, List, Optional
from enum import Enum
import time
import statistics
from collections import deque


class BottleneckType(Enum):
    CPU_BOUND = "CPU-bound"
    GPU_BOUND = "GPU-bound"
    MEMORY_BOUND = "Memory-bound"
    LOADING_BOUND = "Loading-bound"
    THERMAL_CONSTRAINED = "Thermally constrained"
    STABLE = "Stable"
    UNKNOWN = "Unknown"


@dataclass
class FrameMetrics:
    fps: float = 0.0
    frameTimeMs: float = 0.0
    cpuTimeMs: float = 0.0
    gpuTimeMs: float = 0.0
    objectCount: int = 0
    visibleObjectCount: int = 0
    particleCount: int = 0
    drawCalls: int = 0
    memoryUsageMb: int = 0
    frameTimeVariance: float = 0.0
    worstFrameTimeMs: float = 0.0
    medianFrameTimeMs: float = 0.0


@dataclass
class PerformanceSnapshot:
    current: FrameMetrics = field(default_factory=FrameMetrics)
    historicalAvg: FrameMetrics = field(default_factory=FrameMetrics)
    bottleneck: BottleneckType = BottleneckType.UNKNOWN
    primaryCostSource: str = "Unknown"
    confidence: float = 0.0
    frameHistory: List[float] = field(default_factory=list)
    recentSpikeIndex: int = -1


class Profiler:
    """
    Core measurement and observability layer.
    Tracks FPS, frame time, CPU/GPU workload, object counts, memory, and more.
    """
    
    def __init__(self, historySize: int = 120):
        self._snapshot = PerformanceSnapshot()
        self._frameHistory = deque(maxlen=historySize)
        self._counterValues: Dict[str, float] = {}
        self._profilingEnabled = True
        self._initialized = False
        
        # Internal counters
        self._actualObjectCount = 0
        self._actualVisibleCount = 0
        self._actualParticleCount = 0
        
        # Spike detection
        self._spikeThresholdMs = 30.0
        
    def beginFrame(self) -> None:
        """Mark the start of a frame."""
        if not self._profilingEnabled:
            return
        self._frameStart = time.perf_counter()
        
    def endFrame(self) -> None:
        """Mark the end of a frame and update metrics."""
        if not self._profilingEnabled:
            return
            
        now = time.perf_counter()
        durationMs = (now - self._frameStart) * 1000.0
        
        frame = self._snapshot.current
        frame.frameTimeMs = durationMs
        frame.fps = 1000.0 / durationMs if durationMs > 0 else 0.0
        
        # Update history
        self._frameHistory.append(durationMs)
        
        # Calculate statistics
        if len(self._frameHistory) >= 3:
            sortedHistory = sorted(self._frameHistory)
            frame.medianFrameTimeMs = sortedHistory[len(sortedHistory) // 2]
            frame.worstFrameTimeMs = sortedHistory[-1]
            frame.frameTimeVariance = self._calculateVariance()
            
            self._snapshot.frameHistory = list(self._frameHistory)
            
            # Detect spikes
            if durationMs > self._spikeThresholdMs:
                self._snapshot.recentSpikeIndex = len(self._frameHistory) - 1
        
        # Update object counts
        frame.objectCount = self._actualObjectCount
        frame.visibleObjectCount = self._actualVisibleCount
        frame.particleCount = self._actualParticleCount
        
    def _calculateVariance(self) -> float:
        """Calculate frame time variance."""
        if len(self._frameHistory) < 2:
            return 0.0
        return statistics.variance(self._frameHistory) if len(self._frameHistory) > 1 else 0.0
    
    def estimateCpuTime(self, ms: float) -> None:
        """Set estimated CPU time (from engine hooks)."""
        self._snapshot.current.cpuTimeMs = ms
        
    def estimateGpuTime(self, ms: float) -> None:
        """Set estimated GPU time (from engine hooks)."""
        self._snapshot.current.gpuTimeMs = ms
        
    def setObjectCount(self, count: int) -> None:
        """Set current object count."""
        self._actualObjectCount = count
        
    def setVisibleObjectCount(self, count: int) -> None:
        """Set visible object count."""
        self._actualVisibleCount = count
        
    def setParticleCount(self, count: int) -> None:
        """Set particle count."""
        self._actualParticleCount = count
        
    def setMemoryUsage(self, mb: int) -> None:
        """Set memory usage in MB."""
        self._snapshot.current.memoryUsageMb = mb
        
    def setDrawCallEstimate(self, calls: int) -> None:
        """Set draw call estimate."""
        self._snapshot.current.drawCalls = calls
        
    @property
    def snapshot(self) -> PerformanceSnapshot:
        return self._snapshot
        
    @property
    def isProfilingEnabled(self) -> bool:
        return self._profilingEnabled
        
    def setProfilingEnabled(self, enabled: bool) -> None:
        self._profilingEnabled = enabled
        
    def setHistorySize(self, size: int) -> None:
        self._frameHistory = deque(list(self._frameHistory), maxlen=size)
        
    def classifyBottleneck(self) -> BottleneckType:
        """Classify the current bottleneck."""
        current = self._snapshot.current
        
        if current.fps < 30:
            cpuRatio = current.cpuTimeMs / current.frameTimeMs if current.frameTimeMs > 0 else 0
            gpuRatio = current.gpuTimeMs / current.frameTimeMs if current.frameTimeMs > 0 else 0
            
            if cpuRatio > 0.7:
                self._snapshot.bottleneck = BottleneckType.CPU_BOUND
            elif gpuRatio > 0.7:
                self._snapshot.bottleneck = BottleneckType.GPU_BOUND
            elif current.memoryUsageMb > 1000:
                self._snapshot.bottleneck = BottleneckType.MEMORY_BOUND
            else:
                self._snapshot.bottleneck = BottleneckType.UNKNOWN
        elif current.fps < 50:
            self._snapshot.bottleneck = BottleneckType.UNKNOWN
        else:
            self._snapshot.bottleneck = BottleneckType.STABLE
            
        self._snapshot.historicalAvg = FrameMetrics(**current.__dict__)
        return self._snapshot.bottleneck
        
    def getBottleneckDescription(self) -> str:
        """Get human-readable bottleneck description."""
        return self._snapshot.bottleneck.value
        
    def getAverageFrameTime(self) -> float:
        """Get average frame time from history."""
        if not self._frameHistory:
            return 0.0
        return sum(self._frameHistory) / len(self._frameHistory)
        
    def getMinFrameTime(self) -> float:
        """Get minimum frame time from history."""
        if not self._frameHistory:
            return 0.0
        return min(self._frameHistory)
        
    def getMaxFrameTime(self) -> float:
        """Get maximum frame time from history."""
        if not self._frameHistory:
            return 0.0
        return max(self._frameHistory)
        
    def getFrameTimeVariance(self) -> float:
        """Get frame time variance."""
        return self._snapshot.current.frameTimeVariance
        
    def countSpikes(self, thresholdMs: float = 30.0) -> int:
        """Count frames above threshold."""
        return sum(1 for ft in self._frameHistory if ft > thresholdMs)
        
    def reset(self) -> None:
        """Reset all metrics."""
        self._frameHistory.clear()
        self._counterValues.clear()
        self._actualObjectCount = 0
        self._actualVisibleCount = 0
        self._actualParticleCount = 0
        self._snapshot = PerformanceSnapshot()
        
    def getMetricsMap(self) -> Dict[str, float]:
        """Export metrics as dictionary."""
        current = self._snapshot.current
        return {
            "fps": current.fps,
            "frame_time_ms": current.frameTimeMs,
            "cpu_time_ms": current.cpuTimeMs,
            "gpu_time_ms": current.gpuTimeMs,
            "objects": float(current.objectCount),
            "visible_objects": float(current.visibleObjectCount),
            "particles": float(current.particleCount),
            "draw_calls": float(current.drawCalls),
            "memory_mb": float(current.memoryUsageMb),
            "variance": current.frameTimeVariance,
            "worst_frame_time": current.worstFrameTimeMs,
            "median_frame_time": current.medianFrameTimeMs,
        }


if __name__ == "__main__":
    # Quick test
    profiler = Profiler(historySize=60)
    
    print("=== PROFILER TEST ===")
    
    # Simulate 60 frames
    for i in range(60):
        profiler.beginFrame()
        
        # Simulate varying frame times
        frameTime = 16.67 + (15.0 if i % 10 == 5 else -2.0)
        profiler.estimateCpuTime(frameTime * 0.6)
        profiler.estimateGpuTime(frameTime * 0.4)
        profiler.setObjectCount(1000)
        profiler.setParticleCount(50)
        
        time.sleep(0.001)
        profiler.endFrame()
    
    snapshot = profiler.snapshot
    print(f"FPS: {snapshot.current.fps:.2f}")
    print(f"Frame Time: {snapshot.current.frameTimeMs:.2f} ms")
    print(f"CPU Time: {snapshot.current.cpuTimeMs:.2f} ms")
    print(f"GPU Time: {snapshot.current.gpuTimeMs:.2f} ms")
    print(f"Average Frame Time: {profiler.getAverageFrameTime():.2f} ms")
    print(f"Worst Frame Time: {profiler.getMaxFrameTime():.2f} ms")
    print(f"Best Frame Time: {profiler.getMinFrameTime():.2f} ms")
    print(f"Bottleneck: {profiler.getBottleneckDescription()}")
    print("Profiler test PASSED")
