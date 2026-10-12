#ifndef FLEET_APEX_LEVEL_ANALYZER_H
#define FLEET_APEX_LEVEL_ANALYZER_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <memory>

namespace fleetapex {

/**
 * APEX LEVEL ANALYZER
 * Understanding layer for level content analysis
 * 
 * Answers: "What exists inside this level, and which parts are likely to be expensive?"
 */
class LevelAnalyzer {
public:
    enum class ObjectType {
        PLAYER,
        CUBE,
        SPHERE,
        UFO,
        WALKER,
        SHIP,
        BALL,
        DIAMOND,
        ORB,
        TRIGGER,
        OTHER
    };

    enum class EffectCategory {
        GLOW,
        PARTICLES,
        BACKGROUND,
        FOREGROUND,
        UI_ELEMENT,
        SPAWNER,
        ANIMATOR,
        CONTROLLER,
        NONE
    };

    struct ObjectInfo {
        int32_t objectId = 0;
        ObjectType type;
        EffectCategory effectCat;
        bool isStatic = false;
        bool isGameplayCritical = true;
        double estimatedCost = 0.0;
        double estimatedRenderCost = 0.0;
        double estimatedUpdateCost = 0.0;
        bool safeForCulling = false;
        bool safeForCaching = false;
        int32_t x = 0;
        int32_t y = 0;
        int32_t rotation = 0;
        int32_t scale = 100;
        std::string metaData;
    };

    struct RegionInfo {
        int32_t x = 0;
        int32_t y = 0;
        int32_t width = 0;
        int32_t height = 0;
        uint64_t objectCount = 0;
        double avgCost = 0.0;
        EffectCategory dominantEffect;
        bool particleHeavy = false;
        bool glowHeavy = false;
        bool shaderHeavy = false;
    };

    struct LevelProfile {
        std::string levelName;
        int32_t levelId = 0;
        uint64_t totalObjectCount = 0;
        uint64_t staticObjectCount = 0;
        uint64_t dynamicObjectCount = 0;
        std::map<ObjectType, uint64_t> objectDistribution;
        std::map<EffectCategory, uint64_t> effectDistribution;
        std::vector<RegionInfo> regions;
        double overallDensity = 0.0;
        double estimatedTotalCost = 0.0;
        bool hasComplexTriggers = false;
        bool hasSpawners = false;
        bool hasParticleSystems = false;
        bool hasRealtimeGlow = false;
        std::string primaryBottleneck;
        std::vector<std::string> optimizationCandidates;
        
        // Cachable objects
        std::vector<int32_t> staticObjectIds;
        std::vector<int32_t> cullableObjectIds;
        
        // Region-based analysis
        std::vector<RegionInfo> highCostRegions;
        std::vector<RegionInfo> particleHeavyRegions;
        std::vector<RegionInfo> glowHeavyRegions;
    };

private:
    LevelProfile m_profile;
    std::vector<ObjectInfo> m_objects;
    bool m_analyzed = false;
    std::string m_levelInternalName;
    
    // Internal helpers
    ObjectType classifyObjectType(int32_t objectTypeValue);
    EffectCategory analyzeEffects(const ObjectInfo& obj);
    bool isObjectStatic(const ObjectInfo& obj);
    double estimateObjectCost(const ObjectInfo& obj);
    void calculateRegionCosts();
    void identifyOptimizationCandidates();

public:
    LevelAnalyzer();
    ~LevelAnalyzer() = default;

    // Main analysis pipeline
    void beginLevelScan(const std::string& levelName, int32_t levelId);
    void addObject(int32_t objectId, int32_t x, int32_t y, int32_t objectType, 
                   int32_t rotation = 0, int32_t scale = 100, 
                   const std::string& metaData = "");
    void addParticleSystem(int32_t objectId, int32_t x, int32_t y);
    void markAsStatic(int32_t objectId);
    void markAsGameplayCritical(int32_t objectId, bool critical = true);
    void endLevelScan();

    // Incremental updates
    void updateObjectPosition(int32_t objectId, int32_t newX, int32_t newY);
    void invalidateStaticCache(int32_t objectId);

    // Access profile
    const LevelProfile& getProfile() const { return m_profile; }
    LevelProfile& getProfile() { return m_profile; }
    bool isAnalyzed() const { return m_analyzed; }

    // Query methods
    std::vector<ObjectInfo> findStaticObjects() const;
    std::vector<ObjectInfo> findPotentiallyCullable() const;
    std::vector<ObjectInfo> findByEffectCategory(EffectCategory cat) const;
    std::vector<RegionInfo> getHighCostRegions(double threshold) const;
    
    // Cost estimation
    double estimateRegionCost(int32_t x, int32_t y, int32_t radius) const;
    std::vector<ObjectInfo> findObjectsInRegion(int32_t x, int32_t y, int32_t radius) const;

    // Utility
    void clear();
    void reset();
};

} // namespace fleetapex

#endif // FLEET_APEX_LEVEL_ANALYZER_H
