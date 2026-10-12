"""
FLEET APEX Level Analyzer Module

Understanding layer for level content analysis

Answers: "What exists inside this level, and which parts are likely to be expensive?"
"""

from dataclasses import dataclass, field
from typing import Dict, List, Optional
from enum import Enum
from collections import defaultdict


class ObjectType(Enum):
    PLAYER = 0
    CUBE = 1
    SPHERE = 2
    UFO = 3
    WALKER = 4
    SHIP = 5
    BALL = 8
    DIAMOND = 13
    ORB = 17
    TRIGGER = 99
    OTHER = 100


class EffectCategory(Enum):
    GLOW = 0
    PARTICLES = 1
    BACKGROUND = 2
    FOREGROUND = 3
    UI_ELEMENT = 4
    SPAWNER = 5
    ANIMATOR = 6
    CONTROLLER = 7
    NONE = 8


@dataclass
class ObjectInfo:
    objectId: int = 0
    type: ObjectType = ObjectType.OTHER
    effectCat: EffectCategory = EffectCategory.NONE
    isStatic: bool = False
    isGameplayCritical: bool = True
    estimatedCost: float = 0.0
    estimatedRenderCost: float = 0.0
    estimatedUpdateCost: float = 0.0
    safeForCulling: bool = False
    safeForCaching: bool = False
    x: int = 0
    y: int = 0
    rotation: int = 0
    scale: int = 100
    metaData: str = ""


@dataclass
class RegionInfo:
    x: int = 0
    y: int = 0
    width: int = 0
    height: int = 0
    objectCount: int = 0
    avgCost: float = 0.0
    dominantEffect: EffectCategory = EffectCategory.NONE
    particleHeavy: bool = False
    glowHeavy: bool = False
    shaderHeavy: bool = False


@dataclass
class LevelProfile:
    levelName: str = ""
    levelId: int = 0
    totalObjectCount: int = 0
    staticObjectCount: int = 0
    dynamicObjectCount: int = 0
    objectDistribution: Dict[ObjectType, int] = field(default_factory=lambda: defaultdict(int))
    effectDistribution: Dict[EffectCategory, int] = field(default_factory=lambda: defaultdict(int))
    regions: List[RegionInfo] = field(default_factory=list)
    overallDensity: float = 0.0
    estimatedTotalCost: float = 0.0
    hasComplexTriggers: bool = False
    hasSpawners: bool = False
    hasParticleSystems: bool = False
    realtimeGlow: bool = False
    primaryBottleneck: str = "rendering_operations"
    optimizationCandidates: List[str] = field(default_factory=list)
    staticObjectIds: List[int] = field(default_factory=list)
    cullableObjectIds: List[int] = field(default_factory=list)
    highCostRegions: List[RegionInfo] = field(default_factory=list)
    particleHeavyRegions: List[RegionInfo] = field(default_factory=list)
    glowHeavyRegions: List[RegionInfo] = field(default_factory=list)


class LevelAnalyzer:
    """
    Level analyzer for understanding level content.
    Analyzes object counts, categories, effects, and identifies optimization candidates.
    """
    
    def __init__(self):
        self._profile = LevelProfile()
        self._objects: List[ObjectInfo] = []
        self._analyzed = False
        self._levelInternalName = ""
        
    def beginLevelScan(self, levelName: str, levelId: int) -> None:
        """Begin scanning a new level."""
        self._profile = LevelProfile()
        self._profile.levelName = levelName
        self._profile.levelId = levelId
        self._objects.clear()
        self._analyzed = False
        self._levelInternalName = levelName
        
    def addObject(self, objectId: int, x: int, y: int, objectType: int,
                  rotation: int = 0, scale: int = 100, metaData: str = "") -> None:
        """Add an object to the level analysis."""
        obj = ObjectInfo()
        obj.objectId = objectId
        obj.x = x
        obj.y = y
        obj.rotation = rotation
        obj.scale = scale
        obj.type = self._classifyObjectType(objectType)
        obj.effectCat = self._analyzeEffects(obj)
        obj.metaData = metaData
        
        self._objects.append(obj)
        self._profile.totalObjectCount += 1
        self._profile.objectDistribution[obj.type] += 1
        self._profile.effectDistribution[obj.effectCat] += 1
        
    def addParticleSystem(self, objectId: int, x: int, y: int) -> None:
        """Add a particle system to the level."""
        obj = ObjectInfo()
        obj.objectId = objectId
        obj.x = x
        obj.y = y
        obj.type = ObjectType.OTHER
        obj.effectCat = EffectCategory.PARTICLES
        obj.isStatic = False
        obj.safeForCulling = False
        obj.safeForCaching = False
        
        self._objects.append(obj)
        self._profile.totalObjectCount += 1
        self._profile.effectDistribution[EffectCategory.PARTICLES] += 1
        self._profile.hasParticleSystems = True
        
    def markAsStatic(self, objectId: int) -> None:
        """Mark an object as static (cacheable)."""
        for obj in self._objects:
            if obj.objectId == objectId:
                obj.isStatic = True
                obj.safeForCaching = True
                self._profile.staticObjectCount += 1
                self._profile.dynamicObjectCount -= 1
                return
        self._profile.staticObjectIds.append(objectId)
        
    def markAsGameplayCritical(self, objectId: int, critical: bool = True) -> None:
        """Mark an object as gameplay-critical (cannot be culled)."""
        for obj in self._objects:
            if obj.objectId == objectId:
                obj.isGameplayCritical = critical
                if critical:
                    obj.safeForCulling = False
                return
                
    def endLevelScan(self) -> None:
        """Finish scanning the level."""
        self._profile.dynamicObjectCount = self._profile.totalObjectCount - self._profile.staticObjectCount
        self._calculateRegionCosts()
        self._identifyOptimizationCandidates()
        self._analyzed = True
        
    def updateObjectPosition(self, objectId: int, newX: int, newY: int) -> None:
        """Update object position (makes it non-static)."""
        for obj in self._objects:
            if obj.objectId == objectId:
                obj.x = newX
                obj.y = newY
                obj.isStatic = False
                return
                
    def invalidateStaticCache(self, objectId: int) -> None:
        """Invalidate static cache for an object."""
        for obj in self._objects:
            if obj.objectId == objectId:
                obj.isStatic = False
                obj.safeForCaching = False
                self._profile.staticObjectCount -= 1
                self._profile.dynamicObjectCount += 1
                return
                
    @property
    def profile(self) -> LevelProfile:
        return self._profile
        
    @property
    def isAnalyzed(self) -> bool:
        return self._analyzed
        
    def _classifyObjectType(self, objectTypeValue: int) -> ObjectType:
        """Classify object type from game value."""
        typeMap = {
            0: ObjectType.PLAYER,
            1: ObjectType.CUBE,
            2: ObjectType.SPHERE,
            3: ObjectType.UFO,
            4: ObjectType.WALKER,
            5: ObjectType.SHIP,
            8: ObjectType.BALL,
            13: ObjectType.DIAMOND,
            17: ObjectType.ORB,
        }
        if objectTypeValue in typeMap:
            return typeMap[objectTypeValue]
        return ObjectType.OTHER
        
    def _analyzeEffects(self, obj: ObjectInfo) -> EffectCategory:
        """Analyze effects for an object."""
        if obj.type in [ObjectType.CUBE, ObjectType.SPHERE, ObjectType.UFO, 
                        ObjectType.SHIP, ObjectType.BALL, ObjectType.DIAMOND, ObjectType.ORB]:
            return EffectCategory.UI_ELEMENT
        if obj.type == ObjectType.OTHER:
            if "glow" in obj.metaData.lower():
                return EffectCategory.GLOW
            if "particle" in obj.metaData.lower():
                return EffectCategory.PARTICLES
            return EffectCategory.NONE
        if obj.type == ObjectType.TRIGGER:
            return EffectCategory.CONTROLLER
        return EffectCategory.NONE
        
    def _isObjectStatic(self, obj: ObjectInfo) -> bool:
        """Check if object is static."""
        if obj.isGameplayCritical:
            return False
        if obj.effectCat == EffectCategory.PARTICLES:
            return False
        if obj.effectCat == EffectCategory.SPAWNER:
            return False
        if obj.effectCat == EffectCategory.ANIMATOR:
            return False
        return True
        
    def _estimateObjectCost(self, obj: ObjectInfo) -> float:
        """Estimate cost of an object."""
        baseCost = 1.0
        
        if obj.effectCat == EffectCategory.GLOW:
            baseCost += 2.0
        if obj.effectCat == EffectCategory.PARTICLES:
            baseCost += 3.0
        if obj.effectCat == EffectCategory.BACKGROUND:
            baseCost += 0.5
        if obj.effectCat == EffectCategory.FOREGROUND:
            baseCost += 1.5
            
        scaleFactor = obj.scale / 100.0
        baseCost *= max(0.5, scaleFactor)
        
        return baseCost
        
    def _calculateRegionCosts(self) -> None:
        """Calculate costs for grid regions."""
        REGION_SIZE = 500
        regionMap: Dict[tuple, RegionInfo] = {}
        
        for obj in self._objects:
            regionX = (obj.x // REGION_SIZE) * REGION_SIZE
            regionY = (obj.y // REGION_SIZE) * REGION_SIZE
            key = (regionX, regionY)
            
            if key not in regionMap:
                region = RegionInfo()
                region.x = regionX
                region.y = regionY
                region.width = REGION_SIZE
                region.height = REGION_SIZE
                regionMap[key] = region
            else:
                region = regionMap[key]
                
            region.objectCount += 1
            region.dominantEffect = obj.effectCat
            if obj.effectCat == EffectCategory.PARTICLES:
                region.particleHeavy = True
            if obj.effectCat == EffectCategory.GLOW:
                region.glowHeavy = True
                
            region.avgCost += self._estimateObjectCost(obj)
            
        for region in regionMap.values():
            if region.objectCount > 0:
                region.avgCost /= region.objectCount
            self._profile.regions.append(region)
            
    def _identifyOptimizationCandidates(self) -> None:
        """Identify optimization candidates."""
        self._profile.optimizationCandidates.clear()
        self._profile.highCostRegions.clear()
        self._profile.particleHeavyRegions.clear()
        self._profile.glowHeavyRegions.clear()
        
        for region in self._profile.regions:
            if region.objectCount > 10 or region.avgCost > 15:
                self._profile.highCostRegions.append(region)
            if region.particleHeavy:
                self._profile.particleHeavyRegions.append(region)
            if region.glowHeavy:
                self._profile.glowHeavyRegions.append(region)
                
        if self._profile.effectDistribution[EffectCategory.GLOW] > 10:
            self._profile.realtimeGlow = True
            self._profile.optimizationCandidates.append("glow")
        if self._profile.effectDistribution[EffectCategory.PARTICLES] > 50:
            self._profile.hasParticleSystems = True
            self._profile.optimizationCandidates.append("particles")
            
        if self._profile.regions:
            totalDensity = sum(r.objectCount for r in self._profile.regions)
            self._profile.overallDensity = totalDensity / len(self._profile.regions)
            
        self._profile.estimatedTotalCost = self._profile.overallDensity * 0.5
        
        if len(self._profile.particleHeavyRegions) > 3:
            self._profile.primaryBottleneck = "particle_rendering"
        elif len(self._profile.glowHeavyRegions) > 3:
            self._profile.primaryBottleneck = "glow"
        elif self._profile.overallDensity > 50:
            self._profile.primaryBottleneck = "shader_workload"
        else:
            self._profile.primaryBottleneck = "rendering_operations"
            
    def findStaticObjects(self) -> List[ObjectInfo]:
        """Find all static objects."""
        return [obj for obj in self._objects if obj.isStatic]
        
    def findPotentiallyCullable(self) -> List[ObjectInfo]:
        """Find objects that might be culled."""
        return [obj for obj in self._objects 
                if obj.safeForCulling and not obj.isGameplayCritical]
                
    def findByEffectCategory(self, category: EffectCategory) -> List[ObjectInfo]:
        """Find objects by effect category."""
        return [obj for obj in self._objects if obj.effectCat == category]
        
    def getHighCostRegions(self, threshold: float = 15.0) -> List[RegionInfo]:
        """Get regions with high average cost."""
        return [r for r in self._profile.regions if r.avgCost > threshold]
        
    def estimateRegionCost(self, x: int, y: int, radius: int) -> float:
        """Estimate total cost in a region."""
        totalCost = 0.0
        for obj in self._objects:
            dx = obj.x - x
            dy = obj.y - y
            if dx * dx + dy * dy <= radius * radius:
                totalCost += self._estimateObjectCost(obj)
        return totalCost
        
    def findObjectsInRegion(self, x: int, y: int, radius: int) -> List[ObjectInfo]:
        """Find objects within a radius of a point."""
        result = []
        for obj in self._objects:
            dx = obj.x - x
            dy = obj.y - y
            if dx * dx + dy * dy <= radius * radius:
                result.append(obj)
        return result
        
    def clear(self) -> None:
        """Clear the analysis."""
        self._profile = LevelProfile()
        self._objects.clear()
        self._analyzed = False
        
    def reset(self) -> None:
        """Reset the analyzer (alias for clear)."""
        self.clear()


if __name__ == "__main__":
    print("=== LEVEL ANALYZER TEST ===")
    
    analyzer = LevelAnalyzer()
    
    analyzer.beginLevelScan("Test Level", 12345)
    
    # Add various objects
    for i in range(100):
        analyzer.addObject(i + 1, i * 100, 500, 1)  # CUBE
        
    for i in range(50):
        analyzer.addObject(100 + i, i * 200, 800, 2)  # SPHERE
        
    # Add particle systems
    for i in range(20):
        analyzer.addParticleSystem(200 + i, i * 500, 600)
        
    # Mark some as static
    for i in range(10, 50):
        analyzer.markAsStatic(i)
        
    analyzer.endLevelScan()
    
    profile = analyzer.profile
    print(f"Level: {profile.levelName} (ID: {profile.levelId})")
    print(f"Total Objects: {profile.totalObjectCount}")
    print(f"Static Objects: {profile.staticObjectCount}")
    print(f"Dynamic Objects: {profile.dynamicObjectCount}")
    print(f"Overall Density: {profile.overallDensity:.2f}")
    print(f"Primary Bottleneck: {profile.primaryBottleneck}")
    print(f"Has Particles: {'Yes' if profile.hasParticleSystems else 'No'}")
    print(f"Has Realtime Glow: {'Yes' if profile.realtimeGlow else 'No'}")
    
    print("\nEffect Distribution:")
    for cat, count in profile.effectDistribution.items():
        print(f"  {cat.name}: {count}")
        
    print("\nOptimization Candidates:")
    for cand in profile.optimizationCandidates:
        print(f"  - {cand}")
        
    print("\nLevel Analyzer test PASSED")
