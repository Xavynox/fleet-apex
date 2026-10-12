"""
FLEET APEX GPU Assist Engine

Uses the GPU for visual workloads that are naturally parallel and safe to offload.

This subsystem focuses on identifying visual work currently done on CPU that
can be safely and efficiently moved to GPU.
"""

from dataclasses import dataclass, field
from typing import List, Dict, Set, Optional
from enum import Enum
from profiler import Profiler
from level_analyzer import LevelAnalyzer, EffectCategory


class GPUAssistAction(Enum):
    """Types of GPU assist actions."""
    NONE = "none"
    STATIC_BATCHING = "static_batching"
    INSTANCING = "instancing"
    GPU_PARTICLES = "gpu_particles"
    SHADER_EFFECTS = "shader_effects"
    BACKGROUND_ANIMATION = "background_animation"
    VISUAL_TRANSFORMATIONS = "visual_transformations"
    REDUCE_CPU_SUBMISSION = "reduce_cpu_submission"
    STATIC_SCENE_REPRESENTATION = "static_scene_representation"


class GPUAssistSafetyCheck:
    """Safety checks for GPU offload candidates."""
    
    @staticmethod
    def isPurelyVisual(obj) -> bool:
        """Check if object is purely visual (no gameplay impact)."""
        if obj.isGameplayCritical:
            return False
        if obj.effectCat in [EffectCategory.SPAWNER, EffectCategory.ANIMATOR, EffectCategory.CONTROLLER]:
            return False
        return True
        
    @staticmethod
    def canBeRepresentedIndependently(obj) -> bool:
        """Check if object can be represented independently on GPU."""
        # Static objects with no state changes
        if obj.isStatic:
            return True
        # Objects with predictable transformations
        if obj.effectCat in [EffectCategory.BACKGROUND, EffectCategory.FOREGROUND]:
            return True
        return False
        
    @staticmethod
    def doesOrderingMatter(obj) -> bool:
        """Check if rendering ordering matters."""
        # Background objects typically have fixed ordering
        if obj.effectCat == EffectCategory.BACKGROUND:
            return False
        return True
        
    @staticmethod
    def canSynchronize(obj) -> bool:
        """Check if synchronization can be guaranteed."""
        # Purely visual static objects
        return GPUAssistSafetyCheck.isPurelyVisual(obj) and obj.isStatic


@dataclass
class GPUOffloadCandidate:
    """A candidate for GPU offloading."""
    objectIds: List[int]
    action: GPUAssistAction
    estimatedCpuReductionMs: float
    estimatedGpuIncreaseMs: float
    riskLevel: float  # 0.0 - 1.0
    safetyPassed: bool
    description: str


class GPUAssistEngine:
    """
    GPU Assist Engine - identifies and manages GPU offload opportunities.
    
    This engine finds visual workloads that are naturally parallel and can
    safely be represented through GPU-friendly operations.
    """
    
    def __init__(self, profiler: Profiler, analyzer: LevelAnalyzer):
        self._profiler = profiler
        self._analyzer = analyzer
        self._is_active = False
        self._offload_candidates: List[GPUOffloadCandidate] = []
        self._history: List[str] = []
        
    def enable(self) -> None:
        """Enable GPU assist."""
        self._is_active = True
        
    def disable(self) -> None:
        """Disable GPU assist."""
        self._is_active = False
        
    def isActive(self) -> bool:
        return self._is_active
        
    def analyze(self) -> List[GPUOffloadCandidate]:
        """Analyze for GPU offload opportunities."""
        if not self._is_active:
            return []
            
        candidates = []
        
        # Get objects from analyzer
        objects = self._analyzer._objects
        static_objects = self._analyzer.findStaticObjects()
        particle_objects = self._analyzer.findByEffectCategory(EffectCategory.PARTICLES)
        glow_objects = self._analyzer.findByEffectCategory(EffectCategory.GLOW)
        background_objects = self._analyzer.findByEffectCategory(EffectCategory.BACKGROUND)
        foreground_objects = self._analyzer.findByEffectCategory(EffectCategory.FOREGROUND)
        
        # Static batching candidates
        if len(static_objects) > 50:
            candidate = GPUOffloadCandidate(
                objectIds=[obj.objectId for obj in static_objects[:100]],
                action=GPUAssistAction.STATIC_BATCHING,
                estimatedCpuReductionMs=2.0,
                estimatedGpuIncreaseMs=0.5,
                riskLevel=0.1,
                safetyPassed=True,
                description=f"Static batching for {len(static_objects)} static objects"
            )
            candidates.append(candidate)
            
        # Instancing candidates (visually similar objects)
        similar_groups = self._findSimilarObjectGroups(objects)
        for group in similar_groups:
            if len(group) > 20:
                candidate = GPUOffloadCandidate(
                    objectIds=[obj.objectId for obj in group],
                    action=GPUAssistAction.INSTANCING,
                    estimatedCpuReductionMs=1.5,
                    estimatedGpuIncreaseMs=0.3,
                    riskLevel=0.15,
                    safetyPassed=all(GPUAssistSafetyCheck.isPurelyVisual(obj) for obj in group),
                    description=f"Instancing for {len(group)} similar objects"
                )
                candidates.append(candidate)
                
        # GPU particle candidates
        if len(particle_objects) > 100:
            candidate = GPUOffloadCandidate(
                objectIds=[obj.objectId for obj in particle_objects[:200]],
                action=GPUAssistAction.GPU_PARTICLES,
                estimatedCpuReductionMs=3.0,
                estimatedGpuIncreaseMs=1.0,
                riskLevel=0.2,
                safetyPassed=True,
                description=f"GPU particles for {len(particle_objects)} particle systems"
            )
            candidates.append(candidate)
            
        # Shader effects
        if len(glow_objects) > 20:
            candidate = GPUOffloadCandidate(
                objectIds=[obj.objectId for obj in glow_objects[:50]],
                action=GPUAssistAction.SHADER_EFFECTS,
                estimatedCpuReductionMs=1.0,
                estimatedGpuIncreaseMs=0.8,
                riskLevel=0.2,
                safetyPassed=True,
                description=f"Shader-based glow for {len(glow_objects)} glow objects"
            )
            candidates.append(candidate)
            
        # Background animation
        if len(background_objects) > 30:
            candidate = GPUOffloadCandidate(
                objectIds=[obj.objectId for obj in background_objects],
                action=GPUAssistAction.BACKGROUND_ANIMATION,
                estimatedCpuReductionMs=1.0,
                estimatedGpuIncreaseMs=0.5,
                riskLevel=0.1,
                safetyPassed=True,
                description=f"GPU background animation for {len(background_objects)} objects"
            )
            candidates.append(candidate)
            
        # Reduce CPU submission overhead
        total_objects = len(objects)
        if total_objects > 5000:
            candidate = GPUOffloadCandidate(
                objectIds=[],
                action=GPUAssistAction.REDUCE_CPU_SUBMISSION,
                estimatedCpuReductionMs=2.0,
                estimatedGpuIncreaseMs=0.2,
                riskLevel=0.1,
                safetyPassed=True,
                description=f"Reduce CPU submission overhead for {total_objects} objects"
            )
            candidates.append(candidate)
            
        # Static scene representation
        high_cost_regions = self._analyzer.getHighCostRegions(15.0)
        if len(high_cost_regions) > 0 and len(static_objects) > 100:
            candidate = GPUOffloadCandidate(
                objectIds=[obj.objectId for obj in static_objects],
                action=GPUAssistAction.STATIC_SCENE_REPRESENTATION,
                estimatedCpuReductionMs=4.0,
                estimatedGpuIncreaseMs=1.5,
                riskLevel=0.25,
                safetyPassed=True,
                description=f"Static scene representation for {len(high_cost_regions)} regions"
            )
            candidates.append(candidate)
            
        self._offload_candidates = candidates
        return candidates
        
    def _findSimilarObjectGroups(self, objects: List) -> List[List]:
        """Find groups of visually similar objects."""
        # Simplified grouping by type and effect category
        groups: Dict[str, List] = {}
        for obj in objects:
            key = f"{obj.type}_{obj.effectCat}"
            if key not in groups:
                groups[key] = []
            groups[key].append(obj)
            
        # Filter groups with enough objects
        result = []
        for group in groups.values():
            if len(group) > 10:
                result.append(group)
        return result
        
    def getCandidates(self) -> List[GPUOffloadCandidate]:
        """Get current GPU offload candidates."""
        return self._offload_candidates.copy()
        
    def executeCandidate(self, candidate: GPUOffloadCandidate) -> bool:
        """Execute a GPU offload candidate."""
        if not self._is_active:
            return False
            
        # Placeholder for actual implementation
        # In real implementation, this would interface with game engine
        self._history.append(f"GPU_ASSIST: {candidate.action.value} - {candidate.description}")
        print(f"GPU Assist: {candidate.action.value} - {candidate.description}")
        return True
        
    def getHistory(self) -> List[str]:
        """Get GPU assist action history."""
        return self._history.copy()
        
    def getStatus(self) -> str:
        return "ENABLED" if self._is_active else "DISABLED"
        
    def clearHistory(self) -> None:
        """Clear action history."""
        self._history.clear()


class GPUAssistManager:
    """Manages the GPU Assist Engine and its integration."""
    
    def __init__(self, profiler: Profiler, analyzer: LevelAnalyzer):
        self._profiler = profiler
        self._analyzer = analyzer
        self._gpuAssist = GPUAssistEngine(profiler, analyzer)
        self._is_enabled = False
        
    def enable(self) -> None:
        """Enable GPU Assist Manager."""
        self._gpuAssist.enable()
        self._is_enabled = True
        
    def disable(self) -> None:
        """Disable GPU Assist Manager."""
        self._gpuAssist.disable()
        self._is_enabled = False
        
    def isEnabled(self) -> bool:
        return self._is_enabled
        
    def analyze(self) -> List[GPUOffloadCandidate]:
        """Analyze for GPU offload opportunities."""
        return self._gpuAssist.analyze()
        
    def executeCandidate(self, candidate: GPUOffloadCandidate) -> bool:
        """Execute a GPU offload candidate."""
        return self._gpuAssist.executeCandidate(candidate)
        
    def getCandidates(self) -> List[GPUOffloadCandidate]:
        """Get current GPU offload candidates."""
        return self._gpuAssist.getCandidates()
        
    def getStatus(self) -> str:
        return self._gpuAssist.getStatus()
        
    def clearHistory(self) -> None:
        """Clear action history."""
        self._gpuAssist.clearHistory()
