#include "LevelAnalyzer.h"
#include <algorithm>
#include <cmath>

namespace fleetapex {

LevelAnalyzer::LevelAnalyzer() {
    m_profile = std::make_unique<LevelProfileImpl>();
    m_objects.reserve(50000);
}

LevelAnalyzer::~LevelAnalyzer() = default;

LevelAnalyzer::ObjectType LevelAnalyzer::classifyObjectType(int32_t objectTypeValue) {
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
        case 99: return ObjectType::TRIGGER;
        default: return ObjectType::OTHER;
    }
}

LevelAnalyzer::EffectCategory LevelAnalyzer::analyzeEffects(const ObjectInfo& obj) {
    switch (obj.type) {
        case ObjectType::CUBE:
        case ObjectType::SPHERE:
        case ObjectType::UFO:
        case ObjectType::SHIP:
        case ObjectType::BALL:
        case ObjectType::DIAMOND:
        case ObjectType::ORB:
            return EffectCategory::UI_ELEMENT;
        case ObjectType::TRIGGER:
            return EffectCategory::CONTROLLER;
        case ObjectType::OTHER:
            if (obj.metaData.find("glow") != std::string::npos) {
                return EffectCategory::GLOW;
            }
            if (obj.metaData.find("particle") != std::string::npos) {
                return EffectCategory::PARTICLES;
            }
            return EffectCategory::NONE;
        default:
            return EffectCategory::NONE;
    }
}

bool LevelAnalyzer::isObjectStatic(const ObjectInfo& obj) {
    if (obj.isGameplayCritical) return false;
    if (obj.effectCat == EffectCategory::PARTICLES) return false;
    if (obj.effectCat == EffectCategory::SPAWNER) return false;
    if (obj.effectCat == EffectCategory::ANIMATOR) return false;
    return true;
}

double LevelAnalyzer::estimateObjectCost(const ObjectInfo& obj) {
    double baseCost = 1.0;
    
    if (obj.effectCat == EffectCategory::GLOW) baseCost += 2.0;
    if (obj.effectCat == EffectCategory::PARTICLES) baseCost += 3.0;
    if (obj.effectCat == EffectCategory::BACKGROUND) baseCost += 0.5;
    if (obj.effectCat == EffectCategory::FOREGROUND) baseCost += 1.5;
    
    double scaleFactor = obj.scale / 100.0;
    baseCost *= std::max(0.5, scaleFactor);
    
    return baseCost;
}

LevelAnalyzer::LevelAnalyzer() {
    m_profile = std::make_unique<LevelProfileImpl>();
    m_objects.reserve(50000);
}

LevelAnalyzer::~LevelAnalyzer() = default;

void LevelAnalyzer::beginLevelScan(const std::string& levelName, int32_t levelId) {
    m_profile = std::make_unique<LevelProfileImpl>();
    m_profile->levelName = levelName;
    m_profile->levelId = levelId;
    m_objects.clear();
    m_analyzed = false;
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
    
    m_objects.push_back(obj);
    m_profile->totalObjectCount++;
    m_profile->objectDistribution[obj.type]++;
    m_profile->effectDistribution[obj.effectCat]++;
}

void LevelAnalyzer::addParticleSystem(int32_t objectId, int32_t x, int32_t y) {
    ObjectInfo obj;
    obj.objectId = objectId;
    obj.x = x;
    obj.y = y;
    obj.type = ObjectType::OTHER;
    obj.effectCat = EffectCategory::PARTICLES;
    obj.isStatic = false;
    obj.isGameplayCritical = false;
    
    m_objects.push_back(obj);
    m_profile->totalObjectCount++;
    m_profile->effectDistribution[EffectCategory::PARTICLES]++;
    m_profile->hasParticleSystems = true;
}

void LevelAnalyzer::markAsStatic(int32_t objectId) {
    for (auto& obj : m_objects) {
        if (obj.objectId == objectId) {
            obj.isStatic = true;
            m_profile->staticObjectCount++;
            m_profile->dynamicObjectCount--;
            return;
        }
    }
    m_profile->staticObjectIds.push_back(objectId);
}

void LevelAnalyzer::markAsGameplayCritical(int32_t objectId, bool critical) {
    for (auto& obj : m_objects) {
        if (obj.objectId == objectId) {
            obj.isGameplayCritical = critical;
            return;
        }
    }
}

void LevelAnalyzer::endLevelScan() {
    m_profile->dynamicObjectCount = m_profile->totalObjectCount - m_profile->staticObjectCount;
    
    m_profile->optimizationCandidates.clear();
    m_profile->highCostRegions.clear();
    m_profile->particleHeavyRegions.clear();
    m_profile->glowHeavyRegions.clear();
    
    if (m_profile->effectDistribution[EffectCategory::GLOW] > 10) {
        m_profile->realtimeGlow = true;
        m_profile->optimizationCandidates.push_back("glow");
    }
    if (m_profile->effectDistribution[EffectCategory::PARTICLES] > 50) {
        m_profile->hasParticleSystems = true;
        m_profile->optimizationCandidates.push_back("particles");
    }
    if (m_profile->totalObjectCount > 1000) {
        m_profile->optimizationCandidates.push_back("batching");
    }
    
    if (!m_profile->regions.empty()) {
        double totalDensity = 0;
        for (const auto& region : m_profile->regions) {
            totalDensity += static_cast<double>(region.objectCount);
        }
        m_profile->overallDensity = totalDensity / m_profile->regions.size();
    }
    
    m_profile->estimatedTotalCost = m_profile->overallDensity * 0.5;
    
    if (m_profile->particleHeavyRegions.size() > 3) {
        m_profile->primaryBottleneck = "particle_rendering";
    } else if (m_profile->glowHeavyRegions.size() > 3) {
        m_profile->primaryBottleneck = "glow";
    } else if (m_profile->overallDensity > 50) {
        m_profile->primaryBottleneck = "shader_workload";
    } else {
        m_profile->primaryBottleneck = "rendering_operations";
    }
    
    m_analyzed = true;
}

void LevelAnalyzer::updateObjectPosition(int32_t objectId, int32_t newX, int32_t newY) {
    for (auto& obj : m_objects) {
        if (obj.objectId == objectId) {
            obj.x = newX;
            obj.y = newY;
            obj.isStatic = false;
            return;
        }
    }
}

void LevelAnalyzer::invalidateStaticCache(int32_t objectId) {
    for (auto& obj : m_objects) {
        if (obj.objectId == objectId) {
            obj.isStatic = false;
            return;
        }
    }
}

const std::vector<LevelAnalyzer::ObjectInfo>& LevelAnalyzer::getObjects() const {
    return m_objects;
}

std::vector<LevelAnalyzer::ObjectInfo> LevelAnalyzer::findStaticObjects() const {
    std::vector<ObjectInfo> result;
    for (const auto& obj : m_objects) {
        if (obj.isStatic) result.push_back(obj);
    }
    return result;
}

std::vector<LevelAnalyzer::ObjectInfo> LevelAnalyzer::findPotentiallyCullable() const {
    std::vector<ObjectInfo> result;
    for (const auto& obj : m_objects) {
        if (!obj.isGameplayCritical && obj.effectCat != EffectCategory::PARTICLES) {
            result.push_back(obj);
        }
    }
    return result;
}

std::vector<LevelAnalyzer::ObjectInfo> LevelAnalyzer::findByEffectCategory(EffectCategory cat) const {
    std::vector<ObjectInfo> result;
    for (const auto& obj : m_objects) {
        if (obj.effectCat == cat) result.push_back(obj);
    }
    return result;
}

std::vector<LevelAnalyzer::RegionInfo> LevelAnalyzer::getHighCostRegions(double threshold) const {
    std::vector<RegionInfo> result;
    for (const auto& region : m_profile->regions) {
        if (region.avgCost > threshold) result.push_back(region);
    }
    return result;
}

void LevelAnalyzer::clear() {
    m_profile = std::make_unique<LevelProfileImpl>();
    m_objects.clear();
    m_analyzed = false;
}

void LevelAnalyzer::reset() {
    clear();
}

} // namespace fleetapex
