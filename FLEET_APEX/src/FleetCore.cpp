#include "FleetCore.h"
#include <sstream>

namespace fleetapex {

FleetCore::FleetCore() {
    m_profiler = std::make_unique<Profiler>();
    m_analyzer = std::make_unique<LevelAnalyzer>();
    m_adaptiveEngine = std::make_unique<AdaptiveEngine>(
        std::shared_ptr<Profiler>(m_profiler.get(), [](Profiler*){}),
        std::shared_ptr<LevelAnalyzer>(m_analyzer.get(), [](LevelAnalyzer*){})
    );
}

FleetCore::~FleetCore() {
    shutdown();
}

bool FleetCore::initialize() {
    if (m_status != InitStatus::NOT_INITIALIZED) {
        return m_status == InitStatus::READY;
    }
    
    m_status = InitStatus::INITIALIZING;
    
    // Update platform info
    updatePlatform();
    
    // Initialize subsystems
    if (!initializeProfiler()) {
        m_status = InitStatus::ERROR;
        if (m_onError) m_onError("Failed to initialize profiler");
        return false;
    }
    
    if (!initializeAnalyzer()) {
        m_status = InitStatus::ERROR;
        if (m_onError) m_onError("Failed to initialize analyzer");
        return false;
    }
    
    if (!initializeAdaptiveEngine()) {
        m_status = InitStatus::ERROR;
        if (m_onError) m_onError("Failed to initialize adaptive engine");
        return false;
    }
    
    // Apply configuration
    m_profiler->setProfilingEnabled(m_config.enableProfiling);
    m_profiler->setHistorySize(m_config.frameHistorySize);
    
    // Set performance mode
    m_adaptiveEngine->setPerformanceMode(m_config.defaultMode);
    m_adaptiveEngine->setTargetFrameRate(60.0);
    
    m_status = InitStatus::READY;
    
    if (m_onInitComplete) {
        m_onInitComplete();
    }
    
    return true;
}

void FleetCore::shutdown() {
    if (m_status == InitStatus::NOT_INITIALIZED) return;
    
    // Reset state
    m_status = InitStatus::NOT_INITIALIZED;
    
    // Clear data
    m_profiler->reset();
    m_analyzer->reset();
    m_adaptiveEngine->resetOptimizationState();
}

bool FleetCore::initializeProfiler() {
    if (!m_profiler) return false;
    m_profiler->setProfilingEnabled(m_config.enableProfiling);
    return true;
}

bool FleetCore::initializeAnalyzer() {
    if (!m_analyzer) return false;
    m_analyzer->clear();
    return true;
}

bool FleetCore::initializeAdaptiveEngine() {
    if (!m_adaptiveEngine) return false;
    m_adaptiveEngine->initialize();
    return true;
}

void FleetCore::updatePlatform() {
#ifdef __ANDROID__
    m_platform = Platform::ANDROID;
#elif defined(_WIN32)
    m_platform = Platform::DESKTOP_WINDOWS;
#elif defined(__linux__)
    m_platform = Platform::DESKTOP_LINUX;
#elif defined(__APPLE__)
    m_platform = Platform::DESKTOP_MACOS;
#else
    m_platform = Platform::UNKNOWN;
#endif
}

std::string FleetCore::getPlatformString() const {
    switch (m_platform) {
        case Platform::ANDROID: return "Android";
        case Platform::DESKTOP_WINDOWS: return "Windows";
        case Platform::DESKTOP_LINUX: return "Linux";
        case Platform::DESKTOP_MACOS: return "macOS";
        default: return "Unknown";
    }
}

void FleetCore::setConfig(const Config& config) {
    m_config = config;
    
    // Apply new settings if already initialized
    if (m_status == InitStatus::READY) {
        m_profiler->setProfilingEnabled(config.enableProfiling);
        m_profiler->setHistorySize(config.frameHistorySize);
        m_adaptiveEngine->setPerformanceMode(config.defaultMode);
    }
}

// Game lifecycle hooks

void FleetCore::onLevelLoadStarted(const std::string& levelName, int32_t levelId) {
    if (m_status != InitStatus::READY) return;
    
    m_analyzer->beginLevelScan(levelName, levelId);
    m_profiler->reset();
}

void FleetCore::onObjectAdded(int32_t objectId, int32_t x, int32_t y, int32_t type) {
    if (m_status != InitStatus::READY) return;
    
    m_analyzer->addObject(objectId, x, y, type);
}

void FleetCore::onLevelLoadComplete() {
    if (m_status != InitStatus::READY) return;
    
    m_analyzer->endLevelScan();
}

void FleetCore::onFrameStart() {
    if (m_status != InitStatus::READY || !m_config.enableProfiling) return;
    
    if (m_config.enableAutoOptimization) {
        m_adaptiveEngine->update();
    }
    
    m_profiler->beginFrame();
}

void FleetCore::onFrameEnd() {
    if (m_status != InitStatus::READY || !m_config.enableProfiling) return;
    
    m_profiler->endFrame();
}

void FleetCore::update() {
    // Main update loop - called externally or from game loop
    if (m_config.enableAutoOptimization) {
        m_adaptiveEngine->update();
    }
}

std::string FleetCore::getFullVersion() const {
    std::ostringstream ss;
    ss << "FLEET APEX v" << m_version << " [" << getPlatformString() << "]";
    return ss.str();
}

std::string FleetCore::getStatusString() const {
    switch (m_status) {
        case InitStatus::NOT_INITIALIZED: return "Not Initialized";
        case InitStatus::INITIALIZING: return "Initializing";
        case InitStatus::READY: return "Ready";
        case InitStatus::ERROR: return "Error";
        default: return "Unknown";
    }
}

} // namespace fleetapex
