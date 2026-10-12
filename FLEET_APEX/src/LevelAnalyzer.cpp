#include "LevelAnalyzer.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace fleetapex {

LevelAnalyzer::LevelAnalyzer() {
    m_profile.regions.reserve(100);
    m_objects.reserve(50000);
}

void LevelAnalyzer::beginLevelScan(const std::string& levelName, int32_t levelId) {
    m_profile = LevelProfile();
    m_profile.levelName = levelName;
    m_profile.levelId = levelId;
    m_objects.clear();
    m_analyzed = false;
    m_levelInternalName = levelName;
}

void LevelAnalyzer::addObject(int32_t objectId, int32_t x, int32_t y, int32_t objectType,
                               int32_t rotation, int32_t scale, const std::string& metaData) {
    ObjectInfo obj;
    obj.objectId = objectId;
    obj.x = x;
    obj.y = y;
    obj.rotation = rotation;
    obj.scale = scale;
    obj.type = classifyObjectType(objectType);
    obj.effectCat = analyzeEffects(obj);
    obj.metaData = metaData;
    
    m_objects.push_back(obj);
    m_profile.totalObjectCount++;
    
    // Update distribution counts
    m_profile.objectDistribution[obj.type]++;
    m_profile.effectDistribution[obj.effectCat]++;
}

void LevelAnalyzer::addParticleSystem(int32_t objectId, int32_t x, int32_t y) {
    // Add as an object with particle effect category
    ObjectInfo obj;
    obj.objectId = objectId;
    obj.x = x;
    obj.y = y;
    obj.type = ObjectType::OTHER;
    obj.effectCat = EffectCategory::PARTICLES;
    obj.isStatic = false;
    obj.safeForCulling = false;
    obj.safeForCaching = false;
    
    m_objects.push_back(obj);
    m_profile.totalObjectCount++;
    m_profile.effectDistribution[EffectCategory::PARTICLES]++;
    m_profile.hasParticleSystems = true;
}

void LevelAnalyzer::markAsStatic(int32_t objectId) {
    for (auto& obj : m_objects) {
        if (obj.objectId == objectId) {
            obj.isStatic = true;
            obj.safeForCaching = true;
            m_profile.staticObjectCount++;
            m_profile.dynamicObjectCount--;
            return;
        }
    }
    // Also check if it's a cachable region
    m_profile.staticObjectIds.push_back(objectId);
}

void LevelAnalyzer::markAsGameplayCritical(int32_t objectId, bool critical) {
    for (auto& obj : m_objects) {
        if (obj.objectId == objectId) {
            obj.isGameplayCritical = critical;
            if (critical) {
                obj.safeForCulling = false;
            }
            return;
        }
    }
}

void LevelAnalyzer::endLevelScan() {
    m_profile.dynamicObjectCount = m_profile.totalObjectCount - m_profile.staticObjectCount;
    calculateRegionCosts();
    identifyOptimizationCandidates();
    m_analyzed = true;
}

void LevelAnalyzer::updateObjectPosition(int32_t objectId, int32_t newX, int32_t newY) {
    for (auto& obj : m_objects) {
        if (obj.objectId == objectId) {
            obj.x = newX;
            obj.y = newY;
            obj.isStatic = false; // Moving object is no longer static
            return;
        }
    }
}

void LevelAnalyzer::invalidateStaticCache(int32_t objectId) {
    for (auto& obj : m_objects) {
        if (obj.objectId == objectId) {
            obj.isStatic = false;
            obj.safeForCaching = false;
            m_profile.staticObjectCount--;
            m_profile.dynamicObjectCount++;
            return;
        }
    }
}

LevelAnalyzer::ObjectType LevelAnalyzer::classifyObjectType(int32_t objectTypeValue) {
    // GD object type mappings would be here
    switch (objectTypeValue) {
        case 0: return ObjectType::PLAYER;
        case 1: return ObjectType::CUBE;
        case 2: return ObjectType::SPHERE;
        case 3: return ObjectType::UFO;
        case 4: return ObjectType::WALKER;
        case 5: return ObjectType::SHIP;
        case 8: return ObjectType::BALL;
        case 13: return ObjectType::DIAMOND;
        case 17: return ObjectType::ORB;
        default: return ObjectType::OTHER;
    }
}

LevelAnalyzer::EffectCategory LevelAnalyzer::analyzeEffects(const ObjectInfo& obj) {
    // Analyze based on object type and properties
    switch (obj.type) {
        case ObjectType::CUBE:
        case ObjectType::SPHERE:
        case ObjectType::UFO:
        case ObjectType::SHIP:
        case ObjectType::BALL:
        case ObjectType::DIAMOND:
        case ObjectType::ORB:
            // Player objects
            return EffectCategory::UI_ELEMENT;
        case ObjectType::OTHER:
            // Check metadata for effect info
            if (obj.metaData.find("glow") != std::string::npos) {
                return EffectCategory::GLOW;
            }
            if (obj.metaData.find("particle") != std::string::npos) {
                return EffectCategory::PARTICLES;
            }
            return EffectCategory::NONE;
        case ObjectType::TRIGGER:
            return EffectCategory::CONTROLLER;
        default:
            return EffectCategory::NONE;
    }
}

bool LevelAnalyzer::isObjectStatic(const ObjectInfo& obj) {
    // An object is static if it has no animation/trigger dependencies
    if (obj.isGameplayCritical) return false;
    if (obj.effectCat == EffectCategory::PARTICLES) return false;
    if (obj.effectCat == EffectCategory::SPAWNER) return false;
    if (obj.effectCat == EffectCategory::ANIMATOR) return false;
    return true;
}

double LevelAnalyzer::estimateObjectCost(const ObjectInfo& obj) {
    double baseCost = 1.0;
    
    // Factors affecting cost
    if (obj.effectCat == EffectCategory::GLOW) baseCost += 2.0;
    if (obj.effectCat == EffectCategory::PARTICLES) baseCost += 3.0;
    if (obj.effectCat == EffectCategory::BACKGROUND) baseCost += 0.5;
    if (obj.effectCat == EffectCategory::FOREGROUND) baseCost += 1.5;
    
    // Scale factor
    double scaleFactor = obj.scale / 100.0;
    baseCost *= scaleFactor;
    
    // Region density impact
    for (const auto& region : m_profile.regions) {
        if (obj.x >= region.x && obj.x <= region.x + region.width &&
            obj.y >= region.y && obj.y <= region.y + region.height) {
            baseCost *= (1.0 + region.objectCount / 1000.0);
            break;
        }
    }
    
    return baseCost;
}

void LevelAnalyzer::calculateRegionCosts() {
    // Divide level into grid regions
    const int REGION_SIZE = 500;
    
    // First pass: create regions
    std::map<std::pair<int, int>, RegionInfo> regionMap;
    
    for (const auto& obj : m_objects) {
        int regionX = (obj.x / REGION_SIZE) * REGION_SIZE;
        int regionY = (obj.y / REGION_SIZE) * REGION_SIZE;
        auto key = std::make_pair(regionX, regionY);
        
        auto& region = regionMap[key];
        if (region.objectCount == 0) {
            region.x = regionX;
            region.y = regionY;
            region.width = REGION_SIZE;
            region.height = REGION_SIZE;
        }
        
        region.objectCount++;
        region.dominantEffect = obj.effectCat;
        if (obj.effectCat == EffectCategory::PARTICLES) region.particleHeavy = true;
        if (obj.effectCat == EffectCategory::GLOW) region.glowHeavy = true;
        
        region.avgCost += estimateObjectCost(obj);
    }
    
    // Calculate averages and store
    for (auto& pair : regionMap) {
        auto& region = pair.second;
        if (region.objectCount > 0) {
            region.avgCost /= region.objectCount;
        }
        m_profile.regions.push_back(region);
    }
}

void LevelAnalyzer::identifyOptimizationCandidates() {
    m_profile.optimizationCandidates.clear();
    m_profile.highCostRegions.clear();
    m_profile.particleHeavyRegions.clear();
    m_profile.glowHeavyRegions.clear();
    
    // Find high cost regions (> 10 objects or > 15 cost avg)
    for (const auto& region : m_profile.regions) {
        if (region.objectCount > 10 || region.avgCost > 15) {
            m_profile.highCostRegions.push_back(region);
        }
        if (region.particleHeavy) {
            m_profile.particleHeavyRegions.push_back(region);
        }
        if (region.glowHeavy) {
            m_profile.glowHeavyRegions.push_back(region);
        }
    }
    
    // Detect level complexity
    double totalEffects = 0;
    for (const auto& [cat, count] : m_profile.effectDistribution) {
        totalEffects += count;
    }
    
    if (m_profile.effectDistribution[EffectCategory::GLOW] > 10) {
        m_profile.realtimeGlow = true;
        m_profile.optimizationCandidates.push_back("glow");
    }
    if (m_profile.effectDistribution[EffectCategory::PARTICLES] > 50) {
        m_profile.hasParticleSystems = true;
        m_profile.optimizationCandidates.push_back("particles");
    }
    if (totalEffects > 1000) {
        m_profile.optimizationCandidates.push_back("batching");
    }
    
    // Estimate overall density
    if (m_profile.regions.empty()) return;
    
    double totalDensity = 0;
    for (const auto& region : m_profile.regions) {
        totalDensity += static_cast<double>(region.objectCount);
    }
    m_profile.overallDensity = totalDensity / m_profile.regions.size();
    
    // Estimate total cost
    m_profile.estimatedTotalCost = m_profile.overallDensity * 0.5;
    
    // Determine primary bottleneck
    if (m_profile.particleHeavyRegions.size() > 3) {
        m_profile.primaryBottleneck = "particle_rendering";
    } else if (m_profile.glowHeavyRegions.size() > 3) {
        m_profile.primaryBottleneck = "glow";
    } else if (m_profile.overallDensity > 50) {
        m_profile.primaryBottleneck = "shader_workload";
    } else {
        m_profile.primaryBottleneck = "rendering_operations";
    }
}

void LevelAnalyzer::clear() {
    m_profile = LevelProfile();
    m_objects.clear();
    m_analyzed = false;
}

void LevelAnalyzer::reset() {
    clear();
}

std::vector<LevelAnalyzer::ObjectInfo> LevelAnalyzer::findStaticObjects() const {
    std::vector<ObjectInfo> result;
    for (const auto& obj : m_objects) {
        if (obj.isStatic) {
            result.push_back(obj);
        }
    }
    return result;
}

std::vector<LevelAnalyzer::ObjectInfo> LevelAnalyzer::findPotentiallyCullable() const {
    std::vector<ObjectInfo> result;
    for (const auto& obj : m_objects) {
        if (obj.safeForCulling && !obj.isGameplayCritical) {
            result.push_back(obj);
        }
    }
    return result;
}

std::vector<LevelAnalyzer::ObjectInfo> LevelAnalyzer::findByEffectCategory(EffectCategory cat) const {
    std::vector<ObjectInfo> result;
    for (const auto& obj : m_objects) {
        if (obj.effectCat == cat) {
            result.push_back(obj);
        }
    }
    return result;
}

std::vector<LevelAnalyzer::RegionInfo> LevelAnalyzer::getHighCostRegions(double threshold) const {
    std::vector<RegionInfo> result;
    for (const auto& region : m_profile.regions) {
        if (region.avgCost > threshold) {
            result.push_back(region);
        }
    }
    return result;
}

double LevelAnalyzer::estimateRegionCost(int32_t x, int32_t y, int32_t radius) const {
    double totalCost = 0.0;
    for (const auto& obj : m_objects) {
        int dx = obj.x - x;
        int dy = obj.y - y;
        if (dx * dx + dy * dy <= radius * radius) {
            totalCost += estimateObjectCost(obj);
        }
    }
    return totalCost;
}

std::vector<LevelAnalyzer::ObjectInfo> LevelAnalyzer::findObjectsInRegion(int32_t x, int32_t y, int32_t radius) const {
    std::vector<ObjectInfo> result;
    for (const auto& obj : m_objects) {
        int dx = obj.x - x;
        int dy = obj.y - y;
        if (dx * dx + dy * dy <= radius * radius) {
            result.push_back(obj);
        }
    }
    return result;
}

} // namespace fleetapex
