# FLEET APEX Architecture

## Overview

FLEET APEX is "The Most Advanced Geometry Dash Optimization Mod" - a dynamic performance engine that analyzes levels, identifies bottlenecks, and intelligently optimizes while preserving gameplay.

## Core Principles

1. **Optimize Everything That Does Not Matter to Gameplay Before Sacrificing Anything That Does**
2. Zero intentional gameplay changes
3. Adaptive, not static
4. Measure before optimizing, measure after

## The Three Foundational Systems

### 1. PROFILER - The Observation Layer

**Location:** `FLEET_APEX/python/profiler.py`

Measures what Geometry Dash spends time on:

- FPS and frame times
- Frame-time variance and spikes  
- CPU/GPU workload estimates
- Object counts (total, visible, particles)
- Draw calls (where measurable)
- Memory usage
- Loading times

**Key Features:**
- Lightweight, non-intrusive measurements
- Automatic bottleneck classification
- Historical data with configurable history size
- Spike detection and correlation

### 2. LEVEL ANALYZER - The Understanding Layer

**Location:** `FLEET_APEX/python/level_analyzer.py`

Analyzes level content without changing gameplay:

- Object counts by type (player, cube, sphere, etc.)
- Effect distribution (glow, particles, spawners)
- Static vs dynamic object identification
- Region-based cost analysis
- Optimization candidate identification

**Pipeline:**
```
LEVEL → OBJECT ANALYSIS → VISIBILITY ANALYSIS → EFFECT ANALYSIS → COST ESTIMATION → OPTIMIZATION PLAN
```

### 3. ADAPTIVE ENGINE - The Decision Layer

**Location:** `FLEET_APEX/python/adaptive_engine.py`

Consumes profiler + analyzer data to produce optimization decisions:

- Bottleneck classification (CPU, GPU, memory bound)
- Risk-aware optimization prioritization
- Performance mode support (Safe, Balanced, Performance, Turbo)
- Closed-loop validation (apply → measure → adapt)

**Optimization Pipeline:**
```
PROFILER → LEVEL ANALYZER → BOTTLENECK CLASSIFIER → OPTIMIZATION PRIORITY → SAFE ACTIONS → MEASURE → ADJUST
```

## The Closed-Loop Architecture

```
          ┌───────────────────┐
          │   LEVEL ANALYZER  │
          └─────────┬─────────┘
                    │
                    ▼
   ┌────────────┐ ┌───────────────────┐
   │  PROFILER  │►│ ADAPTIVE ENGINE   │
   └─────┬──────┘ └─────────┬─────────┘
         │                      │
         │                      ▼
         │              SAFE OPTIMIZATION
         │                      │
         └──────────────────────┘
                    MEASURE AGAIN
```

## Performance Modes

| Mode | Description | Risk Level |
|------|-------------|------------|
| SAFE | Highly conservative optimizations | Low |
| BALANCED | Normal adaptive optimization | Medium |
| PERFORMANCE | More aggressive visual optimization | High |
| TURBO | For extremely weak hardware | Very High |

## Integration with Geometry Dash

The C++ headers in `/FLEET_APEX/include/` provide the Geode mod interface:

- `FleetCore.h` - Main orchestration
- `Profiler.h` - Performance measurement
- `LevelAnalyzer.h` - Level understanding  
- `AdaptiveEngine.h` - Optimization decisions
- `APEX.h` - Singleton entry point

## Testing

See `FLEET_APEX/python/test_foundation.py` for the Phase 1 test suite.

Run with:
```bash
cd FLEET_APEX/python
python3 test_foundation.py
```

## Future Phases

- **Phase 2:** Safe visibility culling, object pooling, static caching
- **Phase 3:** Dynamic effect budgets, frame-time guardian
- **Phase 4:** Advanced rendering optimizations (batching, static scene caching)
- **Phase 5:** Mobile-specific optimizations (thermal awareness, sustained performance)
- **Phase 6:** Creator tools (performance HUD, bottleneck visualization)
- **Phase 7:** Experimental engine (deep rendering transformations)
