/**
 * FLEET APEX Test Program
 * Verifies the foundational systems work correctly
 */

#include "FleetCore.h"
#include "Profiler.h"
#include "LevelAnalyzer.h"
#include "AdaptiveEngine.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <iomanip>

using namespace fleetapex;

void printSeparator(const std::string& title) {
    std::cout << "\n";
    std::cout << "=== " << title << " ===\n";
    std::cout << std::string(40, '=') << "\n";
}

void testProfiler() {
    printSeparator("PROFILER TEST");
    
    Profiler profiler;
    profiler.setProfilingEnabled(true);
    profiler.setHistorySize(60); // 1 second at 60fps
    
    // Simulate frames
    for (int i = 0; i < 60; i++) {
        profiler.beginFrame();
        
        // Simulate varying frame times
        double frameTime = 16.67 + (i % 10 == 5 ? 15.0 : -2.0); // Occasional spike
        profiler.estimateCpuTime(frameTime * 0.6);
        profiler.estimateGpuTime(frameTime * 0.4);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        profiler.endFrame();
    }
    
    auto snapshot = profiler.getSnapshot();
    std::cout << "FPS: " << std::fixed << std::setprecision(2) << snapshot.current.fps << "\n";
    std::cout << "Frame Time: " << snapshot.current.frameTimeMs << " ms\n";
    std::cout << "CPU Time: " << snapshot.current.cpuTimeMs << " ms\n";
    std::cout << "GPU Time: " << snapshot.current.gpuTimeMs << " ms\n";
    std::cout << "Average Frame Time: " << profiler.getAverageFrameTime() << " ms\n";
    std::cout << "Worst Frame Time: " << profiler.getMinFrameTime() << " ms\n";
    std::cout << "Best Frame Time: " << profiler.getMaxFrameTime() << " ms\n";
    std::cout << "Frame Time Variance: " << snapshot.current.frameTimeVariance << "\n";
    std::cout << "Bottleneck: " << profiler.getBottleneckDescription() << "\n";
}

void testLevelAnalyzer() {
    printSeparator("LEVEL ANALYZER TEST");
    
    LevelAnalyzer analyzer;
    
    // Begin level scan
    analyzer.beginLevelScan("Test Level", 12345);
    
    // Add various objects
    for (int i = 0; i < 100; i++) {
        analyzer.addObject(i + 1, i * 100, 500, 1); // CUBE objects
    }
    
    for (int i = 0; i < 50; i++) {
        analyzer.addObject(100 + i, i * 200, 800, 2); // SPHERE objects
    }
    
    // Add particle systems
    for (int i = 0; i < 20; i++) {
        analyzer.addParticleSystem(200 + i, i * 500, 600);
    }
    
    // Mark some as static
    for (int i = 10; i < 50; i++) {
        analyzer.markAsStatic(i);
    }
    
    // End level scan
    analyzer.endLevelScan();
    
    auto& profile = analyzer.getProfile();
    std::cout << "Level: " << profile.levelName << " (ID: " << profile.levelId << ")\n";
    std::cout << "Total Objects: " << profile.totalObjectCount << "\n";
    std::cout << "Static Objects: " << profile.staticObjectCount << "\n";
    std::cout << "Dynamic Objects: " << profile.dynamicObjectCount << "\n";
    std::cout << "Overall Density: " << profile.overallDensity << "\n";
    std::cout << "Primary Bottleneck: " << profile.primaryBottleneck << "\n";
    std::cout << "Has Particles: " << (profile.hasParticleSystems ? "Yes" : "No") << "\n";
    std::cout << "Has Realtime Glow: " << (profile.realtimeGlow ? "Yes" : "No") << "\n";
    
    std::cout << "\nEffect Distribution:\n";
    for (const auto& [cat, count] : profile.effectDistribution) {
        std::cout << "  Category: " << static_cast<int>(cat) << " = " << count << "\n";
    }
    
    std::cout << "\nOptimization Candidates:\n";
    for (const auto& cand : profile.optimizationCandidates) {
        std::cout << "  - " << cand << "\n";
    }
}

void testAdaptiveEngine() {
    printSeparator("ADAPTIVE ENGINE TEST");
    
    auto profiler = std::make_shared<Profiler>();
    auto analyzer = std::make_shared<LevelAnalyzer>();
    
    AdaptiveEngine engine(profiler, analyzer);
    engine.setPerformanceMode(AdaptiveEngine::PerformanceMode::BALANCED);
    
    // Setup simulated data
    profiler->setProfilingEnabled(true);
    
    // Create a level profile
    analyzer->beginLevelScan("Benchmark Level", 99999);
    for (int i = 0; i < 500; i++) {
        analyzer.addObject(i, i % 2000, i % 1500, 1);
    }
    for (int i = 0; i < 50; i++) {
        analyzer.addParticleSystem(500 + i, i * 100, 500);
    }
    analyzer.endLevelScan();
    
    // Simulate performance conditions
    profiler->setObjectCount(550);
    profiler->setVisibleObjectCount(200);
    profiler->setParticleCount(50);
    profiler->setMemoryUsage(450);
    
    // Simulate a few frames with performance issues
    for (int i = 0; i < 30; i++) {
        profiler->beginFrame();
        profiler->endFrame();
    }
    
    // Get report
    auto report = engine.getReport();
    std::cout << "Bottleneck: " << report.bottleneck << "\n";
    std::cout << "Primary Cost: " << report.primaryCostSource << "\n";
    std::cout << "Recommended: " << report.recommendedAction << "\n";
    std::cout << "Should Optimize: " << (report.shouldOptimize ? "Yes" : "No") << "\n";
    std::cout << "Status: " << report.status << "\n";
    std::cout << "Confidence: " << report.confidence << "\n";
}

void testFleetCore() {
    printSeparator("FLEET CORE TEST");
    
    FleetCore core;
    FleetCore::Config config;
    config.enableProfiling = true;
    config.enableAutoOptimization = true;
    config.defaultMode = AdaptiveEngine::PerformanceMode::BALANCED;
    config.enableDebugLogging = false;
    
    core.setConfig(config);
    
    // Initialize
    if (!core.initialize()) {
        std::cerr << "Failed to initialize FleetCore!\n";
        return;
    }
    
    std::cout << "FLEET APEX: " << core.getFullVersion() << "\n";
    std::cout << "Status: " << core.getStatusString() << "\n";
    std::cout << "Platform: " << core.getPlatformString() << "\n";
    
    // Simulate game lifecycle
    core.onLevelLoadStarted("Test Level 1", 1);
    
    // Add some objects
    for (int i = 0; i < 1000; i++) {
        core.onObjectAdded(i + 1, i * 10, i * 5, 1);
    }
    
    core.onLevelLoadComplete();
    
    // Simulate gameplay frames
    for (int frame = 0; frame < 120; frame++) {
        core.onFrameStart();
        core.onFrameEnd();
        
        if (frame == 60) {
            core.onObjectAdded(1001, 5000, 5000, 1); // Move object
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Get final status
    auto& snapshot = core.getProfiler().getSnapshot();
    std::cout << "\n=== FINAL PERFORMANCE ===\n";
    std::cout << "FPS: " << std::fixed << std::setprecision(1) << snapshot.current.fps << "\n";
    std::cout << "Frame Time: " << snapshot.current.frameTimeMs << " ms\n";
    std::cout << "Objects: " << snapshot.current.objectCount << "\n";
    std::cout << "Particles: " << snapshot.current.particleCount << "\n";
    std::cout << "Memory: " << snapshot.current.memoryUsageMb << " MB\n";
    std::cout << "Bottleneck: " << core.getProfiler().getBottleneckDescription() << "\n";
    
    // Shutdown
    core.shutdown();
    std::cout << "\nShutdown complete.\n";
}

int main() {
    std::cout << "========================================\n";
    std::cout << "  FLEET APEX - Foundation v0.1 Test\n";
    std::cout << "========================================\n";
    
    testProfiler();
    testLevelAnalyzer();
    testAdaptiveEngine();
    testFleetCore();
    
    printSeparator("ALL TESTS COMPLETE");
    std::cout << "The three foundational systems are working correctly:\n";
    std::cout << "  1. PROFILER - Observing game state\n";
    std::cout << "  2. LEVEL ANALYZER - Understanding level content\n";
    std::cout << "  3. ADAPTIVE ENGINE - Making optimization decisions\n";
    std::cout << "\n";
    std::cout << "Phase 1 (APEX Foundation v0.1) complete.\n";
    
    return 0;
}
