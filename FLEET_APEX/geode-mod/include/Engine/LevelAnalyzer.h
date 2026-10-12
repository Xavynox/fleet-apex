/**
 * FLEET APEX v0.2 - Level Analyzer Header
 */

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
        PLAYER, CUBE, SPHERE, UFO, WALKER, SHIP, BALL, DIAMOND, ORB, TRIGGER, OTHER
    };

    enum class EffectCategory {
        GLOW, PARTICLES, BACKGROUND, FOREGROUND, UI_ELEMENT, SPAWNER, 
        ANIMATOR, CONTROLLER, NONE
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
    };

private:
    struct LevelProfileImpl;
    std::unique_ptr<LevelProfileImpl> m_profile;
    std::vector<ObjectInfo> m_objects;
    bool m_analyzed = false;

public:
    LevelAnalyzer();
    ~LevelAnalyzer();

    void beginLevelScan(const std::string& levelName, int32_t levelId);
    void addObject(int32_t objectId, int32_t x, int32_t y, int32_t objectType,
                     int32_t rotation = 0, int32_t scale = 100,
                     const std::string& metaData = "");
    void addParticleSystem(int32_t objectId, int32_t x, int32_t y);
    void markAsStatic(int32_t objectId);
    void markAsGameplayCritical(int32_t objectId, bool critical = true);
    void endLevelScan();

    void updateObjectPosition(int32_t objectId, int32_t newX, int32_t newY);
    void invalidateStaticCache(int32_t objectId);

    bool isAnalyzed() const { return m_analyzed; }
    const std::vector<ObjectInfo>& getObjects() const { return m_objects; }
    std::vector<ObjectInfo> findStaticObjects() const;
    std::vector<ObjectInfo> findPotentiallyCullable() const;
    std::vector<ObjectInfo> findByEffectCategory(EffectCategory) const;
    std::vector<RegionInfo> getHighCostRegions(double threshold) const;

    void clear();
    void reset();
};

} // namespace fleetapex

#endif // FLEET_APEX_LEVEL_ANALYZER_H
