"""
FLEET APEX CPU Liberation Engine

Reduces unnecessary CPU workload while preserving gameplay.

This is a first-class subsystem that focuses on reducing unnecessary CPU overhead
without changing gameplay behavior.
"""

from dataclasses import dataclass, field
from typing import List, Dict, Set, Optional
from enum import Enum
from profiler import Profiler
from level_analyzer import LevelAnalyzer


class CPULiberationAction(Enum):
    """Types of CPU optimization actions."""
    NONE = "none"
    INCREASE_OBJECT_POOLING = "increase_object_pooling"
    STATIC_CACHING = "static_caching"
    REDUCE_REDUNDANT_PROCESSING = "reduce_redundant_processing"
    SAFE_PREPROCESSING = "safe_preprocessing"
    OPTIMIZE_VISIBILITY_CHECKS = "optimize_visibility_checks"
    BATCH_SIMILAR_OPERATIONS = "batch_similar_operations"
    DELAY_NON_CRITICAL_UPDATES = "delay_non_critical_updates"
    OPTIMIZE_VISIBILITY_SCANS = "optimize_visibility_scans"
    OPTIMIZE_TRIGGER_PROCESSING = "optimize_trigger_processing"


class CPULiberationStrategy:
    """
    CPU Liberation Engine - reduces unnecessary CPU workload.
    
    This engine focuses on reducing unnecessary CPU processing while preserving
    gameplay correctness.
    """
    
    def __init__(self, profiler: Profiler, analyzer: LevelAnalyzer):
        self._profiler = profiler
        self._analyzer = analyzer
        self._is_active = False
        self._optimization_history: List[str] = []
        
    def enable(self) -> None:
        """Enable the CPU liberation engine."""
        self._is_active = True
        
    def disable(self) -> None:
        """Disable the CPU liberation engine."""
        self._is_active = False
        
    def isActive(self) -> bool:
        return self._is_active
        
    def analyze(self) -> Dict[str, bool]:
        """Analyze for CPU optimization opportunities."""
        if not self._is_active:
            return {}
            
        report = {}
        
        # Check for redundant processing
        static_objects = self._analyzer.findStaticObjects()
        dynamic_objects = self._analyzer.findPotentiallyCullable()
        
        # Get gameplay critical objects from analyzer
        gameplay_critical = [obj for obj in self._analyzer._objects 
                            if obj.isGameplayCritical]
        
        # Check for redundant visibility checks
        # Use a direct approach to avoid attribute access issues
        from level_analyzer import ObjectInfo
        visible_objects = [obj for obj in self._analyzer._objects 
                          if hasattr(obj, 'effectCat') and obj.effectCat.value == 0]  # UI_ELEMENT
        
        # Check for high CPU cost regions
        high_cost_regions = self._analyzer.getHighCostRegions(15.0)
        
        # Check for memory pressure that might indicate allocation issues
        memory_pressure = self._profiler.snapshot.current.memoryUsageMb > 800
        
        report = {
            "redundant_processing": len(static_objects) > 0 and len(dynamic_objects) > 0,
            "visibility_checks": len(visible_objects) > 1000,
            "static_objects": len(static_objects) > 0,
            "dynamic_objects": len(dynamic_objects) > 0,
            "gameplay_critical": len(gameplay_critical) > 0,
            "memory_pressure": memory_pressure,
            "high_cost_regions": len(high_cost_regions) > 0,
            "can_pool_objects": len(static_objects) > 50,
            "can_cache_static": len(static_objects) > 0,
            "needs_processing_reduction": len(dynamic_objects) > 10000,
            "can_preprocess": len(high_cost_regions) > 0,
            "can_optimize_visibility": len(visible_objects) > 500,
            "can_optimize_triggers": len([obj for obj in self._analyzer._objects 
                                       if obj.type.value in [99]]) > 0  # TRIGGER
        }
        
        return report
        
    def generateOptimizationActions(self) -> List[str]:
        """Generate CPU optimization action recommendations."""
        if not self._is_active:
            return []
            
        actions = []
        
        # Analyze current state
        analysis = self.analyze()
        
        # Generate actions based on analysis
        if analysis["can_pool_objects"]:
            actions.append(CPULiberationAction.INCREASE_OBJECT_POOLING.value)
            
        if analysis["can_cache_static"] and analysis["static_objects"] > 0:
            actions.append(CPULiberationAction.STATIC_CACHING.value)
            
        if analysis["needs_processing_reduction"]:
            actions.append(CPULiberationAction.REDUCE_REDUNDANT_PROCESSING.value)
            
        if analysis["can_preprocess"]:
            actions.append(CPULiberationAction.SAFE_PREPROCESSING.value)
            
        if analysis["can_optimize_visibility"]:
            actions.append(CPULiberationAction.OPTIMIZE_VISIBILITY_CHECKS.value)
            
        if analysis["can_optimize_visibility"]:
            actions.append(CPULiberationAction.OPTIMIZE_VISIBILITY_SCANS.value)
            
        if analysis["can_optimize_trigger_processing"]:
            actions.append(CPULiberationAction.OPTIMIZE_TRIGGER_PROCESSING.value)
            
        if analysis["can_batch_similar_operations"]:
            actions.append(CPULiberationAction.BATCH_SIMILAR_OPERATIONS.value)
            
        if analysis["can_delay_non_critical_updates"]:
            actions.append(CPULiberationAction.DELAY_NON_CRITICAL_UPDATES.value)
            
        return actions
        
    def executeAction(self, action: CPULiberationAction) -> bool:
        """Execute a CPU optimization action."""
        if not self._is_active:
            return False
            
        # Placeholder for actual implementation
        # In real implementation, this would interface with game engine
        action_name = action.value
        self._optimization_history.append(action_name)
        
        # Log the action
        print(f"CPU Liberation: {action_name}")
        
        return True
        
    def getOptimizationHistory(self) -> List[str]:
        """Get history of CPU optimization actions."""
        return self._optimization_history.copy()
        
    def getStatus(self) -> str:
        return "ENABLED" if self._is_active else "DISABLED"


class CPULiberationEngine:
    """
    Main CPU Liberation Engine that coordinates CPU optimization strategies.
    """
    
    def __init__(self, profiler: Profiler, analyzer: LevelAnalyzer):
        self._profiler = profiler
        self._analyzer = analyzer
        self._cpuLiberation = CPULiberationStrategy(profiler, analyzer)
        self._gpuAssist = None  # Will be set later
        
    def enable(self) -> None:
        """Enable CPU liberation."""
        self._cpuLiberation.enable()
        
    def disable(self) -> None:
        """Disable CPU liberation."""
        self._cpuLiberation.disable()
        if self._gpuAssist:
            self._gpuAssist.disable()
            
    def isEnabled(self) -> bool:
        return self._is_active
        
    def analyze(self) -> Dict[str, bool]:
        """Analyze for CPU optimization opportunities."""
        return self._cpuLiberation.analyze()
        
    def executeAction(self, action: CPULiberationAction) -> bool:
        """Execute a CPU optimization action."""
        return self._cpuLiberation.executeAction(action)
        
    def executeActions(self, actions: List[CPULiberationAction]) -> List[bool]:
        """Execute multiple CPU optimization actions."""
        results = []
        for action in actions:
            result = self.executeAction(action)
            results.append(result)
        return results
        
    def analyze(self) -> Dict[str, bool]:
        """Analyze for CPU optimization opportunities."""
        return self._cpuLiberation.analyze()
        
    def getOptimizationActions(self) -> List[str]:
        """Get current optimization action recommendations."""
        return self._cpuLiberation.generateOptimizationActions()
        
    def getHistory(self) -> List[str]:
        return self._cpuLiberation.getOptimizationHistory()
        
    def getStatus(self) -> str:
        return self._cpuLiberation.getStatus()


class CPULiberationManager:
    """
    Manages the CPU Liberation Engine and its integration with other systems.
    """
    
    def __init__(self, profiler: Profiler, analyzer: LevelAnalyzer, adaptive_engine: AdaptiveEngine):
        self._profiler = profiler
        self._analyzer = analyzer
        self._adaptiveEngine = adaptive_engine
        self._cpuLiberation = CPULiberationEngine(profiler, analyzer)
        self._gpuAssist = None
        self._is_enabled = False
        
    def enable(self) -> None:
        """Enable the CPU Liberation Manager."""
        self._cpuLiberation.enable()
        
    def disable(self) -> None:
        """Disable CPU Liberation Manager."""
        self._cpuLiberation.disable()
        
    def isEnabled(self) -> bool:
        return self._is_enabled
        
    def analyze(self) -> Dict[str, bool]:
        """Analyze for CPU optimization opportunities."""
        analysis = self._cpuLiberation.analyze()
        
        # Feed analysis into adaptive engine for decision making
        if self._is_enabled:
            decisions = self._cpuLiberation.getOptimizationActions()
            if results:
                # Add to pending decisions in adaptive engine
                pass  # Implementation detail
                
        return analysis
        
    def analyze(self) -> Dict[str, bool]:
        """Analyze for CPU optimization opportunities."""
        return self._cpuLiberation.analyze()
        
    def executeAction(self, action: CPULiberationAction) -> bool:
        """Execute a CPU optimization action."""
        return self._cpuLiberation.executeAction(action)
        
    def analyze(self) -> Dict[str, bool]:
        """Analyze for CPU optimization opportunities."""
        return self._cpuLiberation.analyze()
        
    def executeAction(self, action: CPULiberationAction) -> bool:
        """Execute a CPU optimization action."""
        return self._cpuLiberation.executeAction(action)
        
    def executeActions(self, actions: List[CPULiberationAction]) -> List[bool]:
        """Execute multiple CPU optimization actions."""
        return self._cpuLiberation.executeActions(actions)
        
    def getOptimizationActions(self) -> List[str]:
        """Get current optimization action recommendations."""
        return self._cpuLiberation.getOptimizationActions()
        
    def getStatus(self) -> str:
        return "ENABLED" if self._is_enabled else "DISABLED"
        
    def enable(self) -> None:
        """Enable CPU Liberation Manager."""
        self._cpuLiberation.enable()
        self._is_enabled = True
        
    def disable(self) -> None:
        """Disable CPU Liberation Manager."""
        self._cpuLiberation.disable()
        self._is_enabled = False
        
    def isEnabled(self) -> bool:
        return self._is_enabled
