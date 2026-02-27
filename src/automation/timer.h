// timer.h
// Timer Data Structure
//
// Port of CTimer from OtherTypes.h
// Timers execute actions at specific times (eAtTime) or intervals (eInterval)

#ifndef TIMER_H
#define TIMER_H

#include "script_language.h"
#include <QDateTime>
#include <QString>
#include <QVariant>

class Timer {
  public:
    // Timer types
    enum TimerType {
        eInterval = 0, // Fire every N hours/minutes/seconds
        eAtTime = 1    // Fire at specific time of day (e.g., 3 PM)
    };

    Timer(); // Constructor with defaults
    ~Timer() = default;

    // ========== Timing Configuration ==========

    int type; // eInterval or eAtTime

    // At-time timing: fire at specific time each day
    qint16 at_hour;   // 0-23
    qint16 at_minute; // 0-59
    double at_second; // 0.0-59.9999

    // Interval timing: fire every N time
    qint16 every_hour;   // 0-23
    qint16 every_minute; // 0-59
    double every_second; // 0.0-59.9999

    // Offset for interval timers (shift timing boundaries)
    // Example: every 5 minutes at  and  past the hour
    // -> every_minute=5, offset_minute=2
    qint16 offset_hour;   // 0-23
    qint16 offset_minute; // 0-59
    double offset_second; // 0.0-59.9999

    // ========== Actions ==========

    QString contents;           // Text to send when timer fires
    quint16 send_to;               // Where to send (eSendToWorld, eSendToScript, etc.)
    QString procedure;             // Script function to call
    ScriptLanguage scriptLanguage; // Script language (Lua or YueScript)
    QString variable;              // Variable name (for eSendToVariable)

    // ========== Behavior Flags ==========

    bool enabled;           // Is timer active?
    bool one_shot;          // Delete after first fire?
    bool temporary;         // Don't save to file?
    bool active_when_closed; // Fire even when disconnected from MUD?
    bool omit_from_output;  // Don't echo to output window?
    bool omit_from_log;      // Don't write to log file?
    bool executing_script;  // Currently executing (prevents deletion)?

    // ========== Metadata ==========

    QString label;      // Timer name/label
    QString group;      // Group name (for batch operations)
    qint32 user_option; // User-defined flags

    // ========== Plugin Support ==========

    bool included; // Loaded from included file?
    bool selected; // Active in plugin?

    // ========== Runtime/Tracking ==========

    QVariant dispid;         // Lua dispatch ID (function cache)
    qint64 update_number;    // For detecting update clashes
    qint32 invocation_count; // How many times script called
    qint32 matched;          // How many times timer fired
    QDateTime fire_time;     // When timer will next fire
    QDateTime when_fired;    // When timer last fired/reset
    quint32 create_sequence; // Creation order

    // ========== Static Sequence Counter ==========

    static quint32 GetNextTimerSequence()
    {
        return next_create_sequence++;
    }

  private:
    static quint32 next_create_sequence;
};

#endif // TIMER_H
