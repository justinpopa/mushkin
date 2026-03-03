/**
 * world_mxp.cpp — MXP implementation moved to mxp_engine.cpp
 *
 * All MXP protocol parsing and execution has been extracted into the
 * MXPEngine companion object (mxp_engine.h / mxp_engine.cpp).
 * WorldDocument forwards MXP calls to m_mxpEngine via inline wrappers
 * in world_document.h.
 *
 * This file is kept in the build to avoid CMakeLists.txt churn.
 * It contains no definitions — the world library target will simply
 * compile an empty translation unit here.
 */
