#ifndef AUTOMATION_CONSTANTS_H
#define AUTOMATION_CONSTANTS_H

#include <QtGlobal>

// Maximum wildcards for trigger/alias matching
// Defined once here to eliminate ODR hazard from duplicated #defines in trigger.h and alias.h
inline constexpr int MAX_WILDCARDS = 10;

#endif // AUTOMATION_CONSTANTS_H
