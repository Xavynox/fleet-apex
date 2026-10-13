#ifndef FLEET_APEX_LEVEL_ANALYZER_H
#define FLEET_APEX_LEVEL_ANALYZER_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>

namespace fleetapex {

class LevelAnalyzer {
public:
    enum class ObjectType {
        PLAYER = 0,
        CUBE = 1,
        SPHERE = 2,
        UFO = 3,
        WALKER = 4,
        SHIP = 5,
        BALL = 8,
        DIAMOND = 13,
        ORB = 17,
        TRIGGER = 99,
        OTHER = 100
    };

    enum class EffectCategory {
        GLOW = 0,
        PARTICLES = 1,
        BACKGROUND = 2,
        FOREGROUND = 3,
        UI_ELEMENT = 4,
        SPAWNER = 5,
        ANIMATOR = 6,
        CONTROLLER = 7,
        NONE = 8
    };

    struct ObjectInfo {
        int32_t objectId = 0;
        ObjectType type = ObjectType::OTHER;
        EffectCategory effectCat = EffectCategory::NONE;
        bool isStatic = false;
        bool isGameplayCritical = true;
        double estimatedCost = 0.0;
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
        EffectCategory dominantEffect = EffectCategory::NONE;
        bool particleHeavy = false;
        bool glowHeavy = false;
        bool shaderHeavy = false;
    };

private:
    struct LevelProfileImpl {
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
        bool realtimeGlow = false;
        std::vector<RegionInfo> highCostRegions;
        std::vector<RegionInfo> particleHeavyRegions;
        std::vector<RegionInfo> glowHeavyRegions;
        std::vector<int32_t> staticObjectIds;
        std::vector<std::string> optimizationCandidates;
        std::string primaryBottleneck;
    };

    static ObjectType classifyObjectType(int32_t objectTypeValue);
    static EffectCategory analyzeEffects(const ObjectInfo& obj);
    static bool isObjectStatic(const ObjectInfo& obj);
    static double estimateObjectCost(const ObjectInfo& obj);

private:
    std::unique_ptr<LevelProfileImpl> m_profile;
    std::vector<ObjectInfo> m_objects;
    bool m_analyzed = false;
    std::string m_levelInternalName;

public:
    LevelAnalyzer();
    ~LevelAnalyzer();

    void beginLevelScan(const std::string& levelName, int32_t levelId);
    void addObject(int32_t objectId, int32_t x, int32_t y, int32_t objectType,
                   int32_t rotation = 0, int32_t scale = 100, const std::string& metaData = "");
    void addParticleSystem(int32_t objectId, int32_t x, int32_t y);
    void markAsStatic(int32_t objectId);
    void markAsGameplayCritical(int32_t objectId, bool critical = true);
    void endLevelScan();

    void updateObjectPosition(int32_t objectId, int32_t newX, int32_t newY);
    void invalidateStaticCache(int32_t objectId);

    const std::vector<ObjectInfo>& getObjects() const;
    std::vector<ObjectInfo> findStaticObjects() const;
    std::vector<ObjectInfo> findPotentiallyCullable() const;
    std::vector<ObjectInfo> findByEffectCategory(EffectCategory cat) const;
    std::vector<RegionInfo> getHighCostRegions(double threshold) const;

    void clear();
    void reset();
    bool isAnalyzed() const { return m_analyzed; }
};

} // namespace fleetapex

#endif // FLEET_APEX_LEVEL_ANALYZER_H
