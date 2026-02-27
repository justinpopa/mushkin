/**
 * world_trigger_matching.cpp - Trigger Pattern Matching Engine
 *
 * The trigger evaluation pipeline (evaluateTriggers, rebuildTriggerArray, and all
 * static matching helpers) has been moved to AutomationRegistry (automation_registry.cpp).
 * WorldDocument forwards evaluateTriggers() and rebuildTriggerArray() via inline wrappers
 * declared in world_document.h.
 *
 * This file is retained as a placeholder to preserve the build target entry.
 */

// No implementations here — all moved to automation_registry.cpp.
