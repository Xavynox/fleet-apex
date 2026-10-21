FLEET APEX — Advanced Performance Diagnostics for Geometry Dash

**Measure. Analyze. Adapt.**

FLEET APEX is an experimental performance-diagnostics project for Geometry Dash. Its goal is to provide a modular foundation for understanding gameplay timing, identifying performance instability, analyzing level complexity, and guiding future performance improvements without compromising gameplay integrity.

## Performance Analysis

FLEET APEX is being developed around a profiling system, a level-analysis engine, and an adaptive diagnostic architecture. These components are intended to help identify performance concerns through collected observations rather than arbitrary scores or unsupported assumptions.

## Runtime Instrumentation

Accurate runtime instrumentation is a core development goal. Future diagnostic releases aim to measure supported gameplay update timings, investigate frame-time spikes, and distinguish available measurements from estimates or unsupported metrics.

CPU-side update duration must not be confused with total frame time or verified GPU execution time. FLEET APEX should clearly communicate these differences to users.

## Level Analysis

The project includes a level-analysis foundation intended to examine supported object information, object distributions, potential processing costs, and areas worth investigating. The reliability of each analysis depends on the runtime data actually available.

## Adaptive Diagnostic Architecture

FLEET APEX is designed to connect performance observations with bottleneck classification and evidence-based recommendations. Diagnostic collection must remain independent of experimental optimization features.

## Safety First

FLEET APEX prioritizes measurement before modification. CPU Liberation, GPU Assist, and automatic optimization remain disabled by default while their implementations undergo development and verification.

The mod must never claim an optimization was applied when it merely identified a possible opportunity. Unsupported GPU measurements and unavailable runtime features must be reported honestly.

## Development Status

FLEET APEX v0.2 is an early architectural foundation. Actual gameplay hooks, fully verified runtime measurements, and the complete in-game diagnostic HUD remain development targets until they have been implemented and tested.

The presence of profiling classes or successfully compiled code does not, by itself, prove that a metric is being collected correctly during gameplay.

## Future Direction

The project aims to progress toward reliable runtime instrumentation, a lightweight in-game performance HUD, more informative level reports, improved bottleneck analysis, and eventually carefully validated optimizations.

Every future optimization should be evaluated using genuine measurements, explicit safety checks, and repeatable before-and-after tests.

---

**FLEET APEX — Understand the bottleneck before attempting to remove it.**

Target platform: Android ARM64. Target game version: Geometry Dash 2.2081. Geode SDK target: 5.11.1.
