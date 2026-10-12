"""
FLEET APEX - The Most Advanced Geometry Dash Optimization Mod

Core initialization and orchestration module
"""

from dataclasses import dataclass, field
from typing import Dict, List, Optional, Callable
from enum import Enum
from profiler import Profiler, BottleneckType
from level_analyzer import LevelAnalyzer
from adaptive_engine import AdaptiveEngine, PerformanceMode
import platform
import sys


class InitStatus(Enum):
    NOT_INITIALIZED = "not_initialized"
    INITIALIZING = "initializing"
    READY = "ready"
    ERROR = "error"


@dataclass
class Config:
    # Profiling
    enableProfiling: bool = True
    frameHistorySize: int = 120
    
    # Analysis
    enableLevelAnalysis: bool = True
    
    # Adaptive optimization
    maxOptimizationLatencyMs: int = 1000
    enableAutoOptimization: bool = True
    
    # Performance mode
    defaultMode: PerformanceMode = PerformanceMode.BALANCED
    
    # Debug/Developer settings
    enableDebugLogging: bool = False
    enablePerformanceHUD: bool = False
    enablePerformanceMap: bool = False
    enableLevelReport: bool = False


class FleetCore:
    """
    Main FLEET APEX core module.
    Orchestrates the three foundational systems:
    - Profiler
    - Level Analyzer  
    - Adaptive Engine
    """
    
    VERSION = "0.1.0"
    
    def __init__(self):
        self._status = InitStatus.NOT_INITIALIZED
        self._profiler = Profiler()
        self._analyzer = LevelAnalyzer()
        self._adaptiveEngine = AdaptiveEngine(self._profiler, self._analyzer)
        self._config = Config()
        self._onInitComplete: Optional[Callable[[], None]] = None
        self._onError: Optional[Callable[[str], None]] = None
        
    @property
    def status(self) -> InitStatus:
        return self._status
        
    @property
    def profiler(self) -> Profiler:
        return self._profiler
        
    @property
    def analyzer(self) -> LevelAnalyzer:
        return self._analyzer
        
    @property
    def adaptiveEngine(self) -> AdaptiveEngine:
        return self._adaptiveEngine
        
    @property
    def config(self) -> Config:
        return self._config
        
    @config.setter
    def config(self, value: Config):
        self._config = value
        if self._status == InitStatus.READY:
            self._profiler.setProfilingEnabled(value.enableProfiling)
            self._profiler.setHistorySize(value.frameHistorySize)
            self._adaptiveEngine.setPerformanceMode(value.defaultMode)
            
    def initialize(self) -> bool:
        """Initialize FLEET APEX."""
        if self._status != InitStatus.NOT_INITIALIZED:
            return self._status == InitStatus.READY
            
        self._status = InitStatus.INITIALIZING
        
        # Initialize subsystems
        try:
            self._profiler.setProfilingEnabled(self._config.enableProfiling)
            self._profiler.setHistorySize(self._config.frameHistorySize)
            
            self._analyzer.clear()
            
            self._adaptiveEngine.setPerformanceMode(self._config.defaultMode)
            self._adaptiveEngine.setTargetFrameRate(60.0)
            self._adaptiveEngine.initialize()
            
            self._status = InitStatus.READY
            
            if self._onInitComplete:
                self._onInitComplete()
                
            return True
            
        except Exception as e:
            self._status = InitStatus.ERROR
            if self._onError:
                self._onError(str(e))
            return False
            
    def shutdown(self) -> None:
        """Shutdown FLEET APEX."""
        if self._status == InitStatus.NOT_INITIALIZED:
            return
            
        self._status = InitStatus.NOT_INITIALIZED
        self._profiler.reset()
        self._analyzer.reset()
        self._adaptiveEngine.resetOptimizationState()
        
    # Game lifecycle hooks

    def onLevelLoadStarted(self, levelName: str, levelId: int) -> None:
        """Called when a level load starts."""
        if self._status != InitStatus.READY:
            return
        self._analyzer.beginLevelScan(levelName, levelId)
        self._profiler.reset()
        
    def onObjectAdded(self, objectId: int, x: int, y: int, objectType: int) -> None:
        """Called when an object is added to the level."""
        if self._status != InitStatus.READY:
            return
        self._analyzer.addObject(objectId, x, y, objectType)
        
    def onParticleSystemAdded(self, objectId: int, x: int, y: int) -> None:
        """Called when a particle system is added."""
        if self._status != InitStatus.READY:
            return
        self._analyzer.addParticleSystem(objectId, x, y)
        
    def onLevelLoadComplete(self) -> None:
        """Called when level load is complete."""
        if self._status != InitStatus.READY:
            return
        self._analyzer.endLevelScan()
        
    def onFrameStart(self) -> None:
        """Called at the start of each frame."""
        if self._status != InitStatus.READY or not self._config.enableProfiling:
            return
        if self._config.enableAutoOptimization:
            self._adaptiveEngine.update()
        self._profiler.beginFrame()
        
    def onFrameEnd(self) -> None:
        """Called at the end of each frame."""
        if self._status != InitStatus.READY or not self._config.enableProfiling:
            return
        self._profiler.endFrame()
        
    def onObjectMoved(self, objectId: int, newX: int, newY: int) -> None:
        """Called when an object moves."""
        if self._status != InitStatus.READY:
            return
        self._analyzer.updateObjectPosition(objectId, newX, newY)
        
    def onObjectStateChanged(self, objectId: int, state: str) -> None:
        """Called when an object's state changes."""
        # Could invalidate caches, update analysis, etc.
        pass
        
    def update(self) -> None:
        """External update loop."""
        if self._config.enableAutoOptimization:
            self._adaptiveEngine.update()
            
    def triggerManualOptimization(self) -> None:
        """Manually trigger optimization."""
        self._adaptiveEngine.triggerManualOptimization()
        
    # Callbacks
    def setOnInitComplete(self, callback: Callable[[], None]) -> None:
        self._onInitComplete = callback
        
    def setOnError(self, callback: Callable[[str], None]) -> None:
        self._onError = callback
        
    # Utility methods
    def getVersion(self) -> str:
        return self.VERSION
        
    def getPlatform(self) -> str:
        """Detect platform."""
        system = platform.system().lower()
        if 'android' in system:
            return 'Android'
        elif system == 'windows':
            return 'Desktop-Windows'
        elif system == 'linux':
            return 'Desktop-Linux'
        elif system == 'darwin':
            return 'Desktop-macOS'
        return 'Unknown'
        
    def getStatusString(self) -> str:
        return self._status.value.replace('_', ' ').title()
        
    def isReady(self) -> bool:
        return self._status == InitStatus.READY
        
    def getFullVersion(self) -> str:
        return f"FLEET APEX v{self.VERSION} [{self.getPlatform()}]"
        
    def getReport(self) -> Dict:
        """Get complete status report."""
        snapshot = self._profiler.snapshot
        return {
            "version": self.getFullVersion(),
            "status": self.getStatusString(),
            "fps": round(snapshot.current.fps, 1),
            "frame_time_ms": round(snapshot.current.frameTimeMs, 2),
            "cpu_time_ms": round(snapshot.current.cpuTimeMs, 2),
            "gpu_time_ms": round(snapshot.current.gpuTimeMs, 2),
            "objects": snapshot.current.objectCount,
            "visible_objects": snapshot.current.visibleObjectCount,
            "particles": snapshot.current.particleCount,
            "memory_mb": snapshot.current.memoryUsageMb,
            "bottleneck": self._profiler.getBottleneckDescription(),
            "primary_cost_source": self._analyzer.profile.primaryBottleneck,
            "level_name": self._analyzer.profile.levelName,
            "total_objects": self._analyzer.profile.totalObjectCount,
            "static_objects": self._analyzer.profile.staticObjectCount,
            "optimization_status": "Optimized" if self._adaptiveEngine.isOptimized else "Normal",
            "performance_mode": self._config.defaultMode.value,
        }


if __name__ == "__main__":
    print("=" * 50)
    print("  FLEET APEX - Foundation v0.1 Test")
    print("=" * 50)
    
    core = FleetCore()
    config = Config()
    config.enableProfiling = True
    config.enableAutoOptimization = True
    config.defaultMode = PerformanceMode.BALANCED
    core.config = config
    
    if not core.initialize():
        print("Failed to initialize FleetCore!")
        sys.exit(1)
        
    print(f"FLEET APEX: {core.getFullVersion()}")
    print(f"Status: {core.getStatusString()}")
    print(f"Platform: {core.getPlatform()}")
    
    # Simulate game lifecycle
    core.onLevelLoadStarted("Test Level 1", 1)
    
    # Add some objects
    for i in range(1000):
        core.onObjectAdded(i + 1, i * 10, i * 5, 1)
        
    core.onLevelLoadComplete()
    
    # Simulate gameplay frames
    import time
    for frame in range(120):
        core.onFrameStart()
        core.onFrameEnd()
        
        if frame == 60:
            core.onObjectMoved(500, 5000, 5000)
            
        time.sleep(0.001)
        
    # Get final status
    report = core.getReport()
    print("\n" + "=" * 30)
    print("FINAL PERFORMANCE")
    print("=" * 30)
    print(f"FPS: {report['fps']}")
    print(f"Frame Time: {report['frame_time_ms']} ms")
    print(f"Objects: {report['objects']}")
    print(f"Particles: {report['particles']}")
    print(f"Memory: {report['memory_mb']} MB")
    print(f"Bottleneck: {report['bottleneck']}")
    
    core.shutdown()
    print("\nShutdown complete.")
    print("\n" + "=" * 50)
    print("All tests PASSED")
    print("Phase 1 (APEX Foundation v0.1) complete")
    print("=" * 50)
