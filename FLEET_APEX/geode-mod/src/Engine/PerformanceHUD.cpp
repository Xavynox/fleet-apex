#include "PerformanceHUD.h"
#include "Profiler.h"
#include "BottleneckClassifier.h"

#include <cstdio>
#include <string>

using namespace geode;

namespace fleetapex {

PerformanceHUD::PerformanceHUD() = default;

PerformanceHUD::~PerformanceHUD() {
    destroy();
}

bool PerformanceHUD::initialize() {
    if (m_initialized) return true;
    
    if (!m_core) {
        log::warn("FLEET APEX HUD: no core bound, cannot initialize");
        return false;
    }
    
    buildUI();
    m_initialized = true;
    log::info("FLEET APEX HUD initialized");
    return true;
}

void PerformanceHUD::destroy() {
    if (!m_initialized) return;
    
    // Remove all nodes from parent safely
    auto nodes = {
        m_background.get(),
        m_titleLabel.get(),
        m_fpsLabel.get(),
        m_frameTimeLabel.get(),
        m_bottleneckLabel.get(),
        m_dataStatusLabel.get(),
        m_logoSprite.get()
    };
    
    for (auto* node : nodes) {
        if (node && node->getParent()) {
            node->removeFromParentAndCleanup(true);
        }
    }
    
    m_background.reset();
    m_titleLabel.reset();
    m_fpsLabel.reset();
    m_frameTimeLabel.reset();
    m_bottleneckLabel.reset();
    m_dataStatusLabel.reset();
    m_logoSprite.reset();
    
    m_initialized = false;
}

void PerformanceHUD::bindCore(FleetCore* core) {
    m_core = core;
}

void PerformanceHUD::setEnabled(bool enabled) {
    m_enabled = enabled;
    if (!enabled) {
        hide();
    }
}

bool PerformanceHUD::isVisible() const {
    return m_background && m_background->isVisible();
}

void PerformanceHUD::show() {
    if (!m_initialized || !m_enabled) return;
    if (m_background) m_background->setVisible(true);
    if (m_titleLabel) m_titleLabel->setVisible(true);
    if (m_fpsLabel) m_fpsLabel->setVisible(true);
    if (m_frameTimeLabel) m_frameTimeLabel->setVisible(true);
    if (m_bottleneckLabel) m_bottleneckLabel->setVisible(true);
    if (m_dataStatusLabel) m_dataStatusLabel->setVisible(true);
    if (m_logoSprite) m_logoSprite->setVisible(true);
}

void PerformanceHUD::hide() {
    if (m_background) m_background->setVisible(false);
    if (m_titleLabel) m_titleLabel->setVisible(false);
    if (m_fpsLabel) m_fpsLabel->setVisible(false);
    if (m_frameTimeLabel) m_frameTimeLabel->setVisible(false);
    if (m_bottleneckLabel) m_bottleneckLabel->setVisible(false);
    if (m_dataStatusLabel) m_dataStatusLabel->setVisible(false);
    if (m_logoSprite) m_logoSprite->setVisible(false);
}

void PerformanceHUD::buildUI() {
    // Get the running scene safely
    auto scene = CCDirector::sharedDirector()->getRunningScene();
    if (!scene) {
        log::warn("FLEET APEX HUD: no running scene, HUD deferred");
        return;
    }
    
    const float panelWidth = 200.0f;
    const float panelHeight = 112.0f;
    const float x = 10.0f;
    const float y = 10.0f;
    
    // Background panel - dark translucent
    m_background = CCSprite::createWithRect(CCRectMake(0, 0, panelWidth, panelHeight), 0, 0, 
                                              1.0f, 1.0f, panelWidth, panelHeight);
    if (m_background) {
        auto color = CCLayerColor::create(ccc4(10, 14, 20, 200), panelWidth, panelHeight);
        if (color) {
            m_background->addChild(color, -1);
        }
        scene->addChild(m_background, 1000);
        m_background->setPosition({x, y});
        m_background->setZOrder(1000);
    }
    
    // Title
    m_titleLabel = CCLabelTTF::create("FLEET APEX v0.3", "bigFont.fnt", 13);
    if (m_titleLabel) {
        m_titleLabel->setPosition({x + 6.0f, panelHeight - 12.0f});
        m_titleLabel->setColor({140, 220, 255});
        m_background->addChild(m_titleLabel);
    }
    
    // FPS + frame time
    m_fpsLabel = CCLabelTTF::create("FPS: N/A", "bigFont.fnt", 11);
    if (m_fpsLabel) {
        m_fpsLabel->setPosition({x + 6.0f, panelHeight - 28.0f});
        m_fpsLabel->setColor({235, 245, 255});
        m_background->addChild(m_fpsLabel);
    }
    
    // Bottleneck classification
    m_bottleneckLabel = CCLabelTTF::create("State: N/A", "bigFont.fnt", 11);
    if (m_bottleneckLabel) {
        m_bottleneckLabel->setPosition({x + 6.0f, panelHeight - 44.0f});
        m_bottleneckLabel->setColor({160, 230, 255});
        m_background->addChild(m_bottleneckLabel);
    }
    
    // Data status line (honest about what's unavailable)
    m_dataStatusLabel = CCLabelTTF::create("GPU: N/A", "bigFont.fnt", 10);
    if (m_dataStatusLabel) {
        m_dataStatusLabel->setPosition({x + 6.0f, panelHeight - 60.0f});
        m_dataStatusLabel->setColor({150, 160, 175});
        m_background->addChild(m_dataStatusLabel);
    }
    
    // Frame time label
    m_frameTimeLabel = CCLabelTTF::create("Frame: N/A", "bigFont.fnt", 10);
    if (m_frameTimeLabel) {
        m_frameTimeLabel->setPosition({x + 6.0f, panelHeight - 76.0f});
        m_frameTimeLabel->setColor({150, 160, 175});
        m_background->addChild(m_frameTimeLabel);
    }
    
    // Load logo as visual identity element
    auto* logo = CCSprite::createWithSpriteFrameName("logo.png"_spr);
    if (logo) {
        m_logoSprite = std::make_unique<CCSprite>(logo);
        // Scale logo to a small corner badge
        auto texSize = m_logoSprite->getContentSize();
        if (texSize.width > 0 && texSize.height > 0) {
            float scale = 26.0f / texSize.width;
            m_logoSprite->setScale(scale);
        }
        m_logoSprite->setPosition({panelWidth - 20.0f, panelHeight - 20.0f});
        m_background->addChild(m_logoSprite.get());
    } else {
        log::warn("FLEET APEX HUD: logo sprite unavailable, continuing without it");
    }
}

void PerformanceHUD::update() {
    if (!m_initialized || !m_enabled || !m_core) return;
    
    // Throttle UI refresh to avoid per-frame overhead
    m_timeSinceLastUpdate += CCDirector::sharedDirector()->getDeltaTime();
    if (m_timeSinceLastUpdate < m_updateInterval) return;
    m_timeSinceLastUpdate = 0.0f;
    
    refreshMetrics();
}

void PerformanceHUD::refreshMetrics() {
    if (!m_core || !m_core->isReady()) {
        clearMetrics();
        return;
    }
    
    auto& snapshot = m_core->getProfiler().getSnapshot();
    auto& engine = m_core->getAdaptiveEngine();
    
    char buffer[256];
    
    // FPS - only show when we have real measurements
    if (snapshot.frameSamples > 0 && snapshot.current.frameTimeMs > 0) {
        snprintf(buffer, sizeof(buffer), "FPS: %.1f", snapshot.current.fps);
        if (m_fpsLabel) m_fpsLabel->setString(buffer);
    } else {
        if (m_fpsLabel) m_fpsLabel->setString("FPS: warming up");
    }
    
    // Frame time
    if (snapshot.frameSamples > 0) {
        snprintf(buffer, sizeof(buffer), "Frame: %.2f ms (avg %.2f)", 
                 snapshot.current.frameTimeMs, snapshot.averageFrameTimeMs);
        if (m_frameTimeLabel) m_frameTimeLabel->setString(buffer);
    } else {
        if (m_frameTimeLabel) m_frameTimeLabel->setString("Frame: N/A");
    }
    
    // Bottleneck classification with honest confidence
    auto report = engine.getReport();
    if (m_bottleneckLabel) {
        snprintf(buffer, sizeof(buffer), "State: %s", report.bottleneck.c_str());
        m_bottleneckLabel->setString(buffer);
    }
    
    // Data availability line - explicitly mark unsupported metrics
    if (m_dataStatusLabel) {
        snprintf(buffer, sizeof(buffer), "GPU: N/A  CPU: %s  Samples: %zu",
                 snapshot.cpuTimeValid ? "meas." : "N/A",
                 snapshot.frameSamples);
        m_dataStatusLabel->setString(buffer);
    }
}

void PerformanceHUD::clearMetrics() {
    if (m_fpsLabel) m_fpsLabel->setString("FPS: N/A");
    if (m_frameTimeLabel) m_frameTimeLabel->setString("Frame: N/A");
    if (m_bottleneckLabel) m_bottleneckLabel->setString("State: N/A");
    if (m_dataStatusLabel) m_dataStatusLabel->setString("GPU: N/A  CPU: N/A");
}

} // namespace fleetapex
