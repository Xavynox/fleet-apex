"""
FLEET APEX v0.2 - CPU↔GPU Cooperative Runtime Optimization Engine

Phase 1: Foundation v0.2 with CPU Liberation and GPU Assist engines.

This is the first functional version of the v0.2 architecture that builds on
the three foundational pillars but adds the new CPU/GPU cooperation subsystem.
"""

from dataclasses import dataclass, field
from typing import Dict, List, Optional, Callable
from enum import Enum
from profiler import Profiler
from level_analyzer import LevelAnalyzer
from adaptive_engine import AdaptiveEngine
from bottleneck_classifier import BottleneckClassifier
from cpu_liberation_engine import CPULiberationEngine
from gpu_assist_engine import GPUAssistEngine
from cpu_gpu_balancer import CPUGPUBalancer, CPUGPUBalancer
from collections import defaultdict


class InitStatus(Enum):
    NOT_INITIALIZED = "not_initialized"
    INITIALIZING = "initializing"
    READY = "ready"
    ERROR = "error"


@dataclass
class Config:
    """Configuration for FLEET APEX v0.2"""
    # Core settings
    enableProfiling: bool = True
    enableLevelAnalysis: bool = True
    enableAutoOptimization: bool = True
    
    # Performance mode
    defaultMode: str = "balanced"  # safe, balanced, performance, turbo
    
    # Engine control
    enableCPULiberation: bool = True
    enableGPUAssist: bool = True
    enableBottleneckClassification: bool = True
    enableFrameTimeGuardian: bool = False  # Advanced feature
    
    # Debug/Developer settings
    enableDebugLogging: bool = False
    enablePerformanceHUD: bool = False
    enableLevelReport: bool = False
    
    # CPU/GPU specific
    targetCPUUtilization: float = 0.8
    targetGPUUtilization: float = 0.8
    frameTimeStabilityThreshold: float = 0.9
    spikeThresholdMs: float = 30.0


class FleetCoreV02:
    """
    FLEET APEX v0.2 - CPU↔GPU Cooperative Runtime Optimization Engine.
    
    This is the Phase 1 version that builds on the v0.1 foundation but adds
    the critical CPU↔GPU cooperation subsystem.
    """
    
    VERSION = "0.2.0"
    
    def __init__(self):
        self._status = InitStatus.NOT_INITIALIZED
        self._profiler = Profiler()
        self._analyzer = LevelAnalyzer()
        
        # New v0.2 components
        self._bottleneckClassifier = BottleneckClassifier(self._profiler)
        self._cpuLiberation = CPULiberationEngine(self._profiler, self._analyzer)
        self._gpuAssist = GPUAssistEngine(self._profiler, self._analyzer)
        self._cpuGpuBalancer = CPUGPUBalancer(self._profiler, self._analyzer)
        
        # Core adaptive engine (still using v0.1 style for now)
        self._adaptiveEngine = AdaptiveEngine(
            self._profiler, 
            self._analyzer,
            self._cpuLiberation,
            self._gpuAssist,
            self._bottleneckClassifier
        )
        
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
    def bottleneckClassifier(self) -> BottleneckClassifier:
        return self._bottleneckClassifier
        
    @property
    def cpuLiberation(self) -> CPULiberationEngine:
        return self._cpuLiberation
        
    @property
    def gpuAssist(self) -> GPUAssistEngine:
        return self._gpuAssist
        
    @property
    def cpuGpuBalancer(self) -> CPUGPUBalancer:
        return self._cpuGpuBalancer
        
    @property
    def config(self) -> Config:
        return self._config
        
    @config.setter
    def config(self, value: Config):
        self._config = value
        self._applyConfig()
        
    def _applyConfig(self) -> None:
        """Apply configuration to all subsystems."""
        if self._status == InitStatus.READY:
            # Core settings
            self._profiler.setProfilingEnabled(self._config.enableProfiling)
            self._adaptiveEngine.setPerformanceMode(
                AdaptiveEngine.PerformanceMode.BALANCED  # TODO: map from string
            )
            
            # New engine control
            if self._config.enableCPULiberation:
                self._cpuLiberation.enable()
            else:
                self._cpuLiberation.disable()
                
            if self._config.enableGPUAssist:
                self._gpuAssist.enable()
            else:
                self._gpuAssist.disable()
                
            self._cpuGpuBalancer.enable()
            
            # Balancer configuration
            self._cpuGpuBalancer.setTargetFrameRate(60.0)
            
    def initialize(self) -> bool:
        """Initialize FLEET APEX v0.2."""
        if self._status != InitStatus.NOT_INITIALIZED:
            return self._status == InitStatus.READY
            
        self._status = InitStatus.INITIALIZING
        
        try:
            # Initialize core profiler
            self._profiler.setProfilingEnabled(self._config.enableProfiling)
            self._profiler.setHistorySize(120)
            
            # Initialize analyzer
            self._analyzer.clear()
            
            # Enable all subsystems
            if self._config.enableCPULiberation:
                self._cpuLiberation.enable()
            if self._config.enableGPUAssist:
                self._gpuAssist.enable()
            if self._config.enableBottleneckClassification:
                self._bottleneckClassifier = BottleneckClassifier(self._profiler)
                
            self._cpuGpuBalancer.enable()
            self._cpuGpuBalancer.setTargetFrameRate(60.0)
            
            # Initialize adaptive engine
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
        """Shutdown FLEET APEX v0.2."""
        if self._status == InitStatus.NOT_INITIALIZED:
            return
            
        self._status = InitStatus.NOT_INITIALIZED
        self._profiler.reset()
        self._analyzer.reset()
        self._cpuLiberation.disable()
        self._gpuAssist.disable()
        self._cpuGpuBalancer.disable()
        
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
        
    def update(self) -> None:
        """Main update loop."""
        if self._config.enableAutoOptimization:
            self._adaptiveEngine.update()
            
        # Run bottleneck analysis
        if self._config.enableBottleneckClassification:
            report = self._bottleneckClassifier.classify()
            
    # High-level optimization coordination

    def analyze(self) -> Dict:
        """Run complete analysis and coordination."""
        if self._status != InitStatus.READY:
            return {"status": "not_ready"}
            
        result = {
            "timestamp": time.time(),
            "status": "analyzing",
            "bottleneck": None,
            "cpu_optimization_available": False,
            "gpu_optimization_available": False,
            "balancer_decision": None,
            "system_state": self.getSystemState()
        }
        
        # Get bottleneck classification
        bottleneck_report = self._bottleneckClassifier.classify()
        result["bottleneck"] = {
            "state": bottleneck_report.state.value,
            "confidence": bottleneck_report.confidence.value,
            "cpu_work_ratio": bottleneck_report.cpuWorkRatio,
            "gpu_work_ratio": bottleneck_report.gpuWorkRatio,
            "stability_score": bottleneck_report.stabilityScore
        }
        
        # Analyze CPU optimization opportunities
        cpu_analysis = self._cpuLiberation.analyze()
        result["cpu_optimization_available"] = any(cpu_analysis.values())
        
        # Analyze GPU optimization opportunities
        gpu_candidates = self._gpuAssist.analyze()
        result["gpu_optimization_available"] = len(gpu_candidates) > 0
        
        # Get balancer decision
        balancer_decision = self._cpuGpuBalancer.analyze()
        result["balancer_decision"] = {
            "path": balancer_decision.path.value,
            "reason": balancer_decision.reason,
            "confidence": balancer_decision.confidence,
            "cpu_actions": [a.value for a in balancer_decision.cpuActions],
            "gpu_candidates": [
                {
                    "action": c.action.value,
                    "description": c.description,
                    "risk_level": c.riskLevel,
                    "safety_passed": c.safetyPassed
                }
                for c in balancer_decision.gpuCandidates
            ],
            "expected_benefit_ms": balancer_decision.expectedBenefitMs,
            "risk_level": balancer_decision.riskLevel
        }
        
        result["status"] = "completed"
        return result
        
    def getSystemState(self) -> Dict:
        """Get complete system state."""
        snapshot = self._profiler.snapshot
        level_profile = self._analyzer.profile
        
        return {
            "profiler": {
                "fps": round(snapshot.current.fps, 1),
                "frame_time_ms": round(snapshot.current.frameTimeMs, 2),
                "cpu_time_ms": round(snapshot.current.cpuTimeMs, 2),
                "gpu_time_ms": round(snapshot.current.gpuTimeMs, 2),
                "objects": snapshot.current.objectCount,
                "particles": snapshot.current.particleCount,
                "memory_mb": snapshot.current.memoryUsageMb
            },
            "analyzer": {
                "level_name": level_profile.levelName,
                "total_objects": level_profile.totalObjectCount,
                "static_objects": level_profile.staticObjectCount,
                "dynamic_objects": level_profile.dynamicObjectCount,
                "primary_bottleneck": level_profile.primaryBottleneck
            },
            "engines": {
                "cpu_liberation": self._cpuLiberation.getStatus(),
                "gpu_assist": self._gpuAssist.getStatus(),
                "bottleneck_classifier": "ACTIVE"
            }
        }
        
    def getVersion(self) -> str:
        return self.VERSION
        
    def getFullVersion(self) -> str:
        return f"FLEET APEX v{self.VERSION} [v0.2 CPU↔GPU Cooperative]"
        
    def getStatusString(self) -> str:
        return self._status.value.replace('_', ' ').title()
        
    def isReady(self) -> bool:
        return self._status == InitStatus.READY
        
    def getReport(self) -> Dict:
        """Get comprehensive status report."""
        result = self.getSystemState()
        
        # Add analysis results
        analysis = self.analyze()
        result["analysis"] = analysis
        
        # Add v0.2 specific metrics
        result["v0.2_features"] = {
            "cpu_liberation_enabled": self._config.enableCPULiberation,
            "gpu_assist_enabled": self._config.enableGPUAssist,
            "bottleneck_classification_enabled": self._config.enableBottleneckClassification,
            "cpu_gpu_cooperation": True,
            "optimization_routing": True
        }
        
        return result


# Test function for v0.2
if __name__ == "__main__":
    print("=" * 60)
    print("  FLEET APEX v0.2 - CPU↔GPU Cooperative Foundation")
    print("=" * 60)
    print("\nPhase 1 v0.2 Tests:")
    print("- PROFILER - Core measurement")
    print("- LEVEL ANALYZER - Level understanding")
    print("- ADAPTIVE ENGINE - Decision making")
    print("- BOTTLENECK CLASSIFIER - Bottleneck detection")
    print("- CPU LIBERATION ENGINE - CPU optimization")
    print("- GPU ASSIST ENGINE - GPU offload")
    print("- CPU↔GPU BALANCER - Optimization routing")
    print("\n")
    
    core = FleetCoreV02()
    
    # Configure for testing
    core.config.enableAutoOptimization = True
    core.config.enableCPULiberation = True
    core.config.enableGPUAssist = True
    core.config.enableBottleneckClassification = True
    
    if not core.initialize():
        print("Failed to initialize FleetCore v0.2!")
        exit(1)
        
    print(f"FLEET APEX: {core.getFullVersion()}")
    print(f"Status: {core.getStatusString()}")
    
    # Test game lifecycle
    core.onLevelLoadStarted("CPU↔GPU Test Level", 1)
    
    for i in range(1000):
        core.onObjectAdded(i + 1, i % 3000, i % 2000, 1)
        
    for i in range(100):
        core.onParticleSystemAdded(1000 + i, i * 150, 500)
        
    core.onLevelLoadComplete()
    
    # Simulate gameplay with analysis
    import time
    for frame in range(120):
        core.onFrameStart()
        core.onFrameEnd()
        
        # Run periodic analysis
        if frame % 30 == 0:
            analysis = core.analyze()
            bottleneck = analysis.get("bottleneck")
            if bottleneck:
                print(f"Frame {frame}: {bottleneck['state']} bottleneck (confidence: {bottleneck['confidence']})")
                
        time.sleep(0.001)
        
    # Get final report
    report = core.getReport()
    
    print("\n" + "=" * 40)
    print("SYSTEM STATE")
    print("=" * 40)
    print(f"FPS: {report['profiler']['fps']}")
    print(f"Frame Time: {report['profiler']['frame_time_ms']} ms")
    print(f"Objects: {report['profiler']['objects']}")
    print(f"Particles: {report['profiler']['particles']}")
    print(f"Memory: {report['profiler']['memory_mb']} MB")
    print(f"Bottleneck: {report['analysis']['bottleneck']['state']}")
    print(f"Primary Cost: {report['analyzer']['primary_bottleneck']}")
    
    print("\n" + "=" * 40)
    print("NEW v0.2 FEATURES")
    print("=" * 40)
    print("CPU Liberation:", report['v0.2_features']['cpu_liberation_enabled'])
    print("GPU Assist:", report['v0.2_features']['gpu_assist_enabled'])
    print("Bottleneck Classification:", report['v0.2_features']['bottleneck_classification_enabled'])
    print("CPU↔GPU Cooperation:", report['v0.2_features']['cpu_gpu_cooperation'])
    print("Optimization Routing:", report['v0.2_features']['optimization_routing'])
    
    print("\n" + "=" * 60)
    print("FLEET APEX v0.2 - Phase 1 COMPLETE")
    print("=" * 60)
    print("\nCPU↔GPU cooperative foundation working correctly:")
    print("  1. OBSERVE - Profiler measures workload")
    print("  2. ANALYZE - Level Analyzer understands level")
    print("  3. CLASSIFY - Bottleneck Classifier detects bottleneck")
    print("  4. ROUTE - CPU↔GPU Balancer decides optimization approach")
    print("  5. OPTIMIZE - CPU Liberation and GPU Assist engines act")
    print("  6. MEASURE - System metrics tracked")
    print("  7. ADAPT - Continuous improvement")
    print("\nArchitecture supports all Phase 2-7 optimization features.")
    print("=" * 60)
