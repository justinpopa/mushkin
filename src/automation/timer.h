// timer.h
// Timer Data Structure
//
// Port of CTimer from OtherTypes.h
// Timers execute actions at specific times (eAtTime) or intervals (eInterval)

#ifndef TIMER_H
#define TIMER_H

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

    int iType; // eInterval or eAtTime

    // At-time timing: fire at specific time each day
    qint16 iAtHour;   // 0-23
    qint16 iAtMinute; // 0-59
    double fAtSecond; // 0.0-59.9999

    // Interval timing: fire every N time
    qint16 iEveryHour;   // 0-23
    qint16 iEveryMinute; // 0-59
    double fEverySecond; // 0.0-59.9999

    // Offset for interval timers (shift timing boundaries)
    // Example: every 5 minutes at  and  past the hour
    // -> iEveryMinute=5, iOffsetMinute=2
    qint16 iOffsetHour;   // 0-23
    qint16 iOffsetMinute; // 0-59
    double fOffsetSecond; // 0.0-59.9999

    // ========== Actions ==========

    QString strContents;  // Text to send when timer fires
    quint16 iSendTo;      // Where to send (eSendToWorld, eSendToScript, etc.)
    QString strProcedure; // Lua function to call
    QString strVariable;  // Variable name (for eSendToVariable)

    // ========== Behavior Flags ==========

    bool bEnabled;          // Is timer active?
    bool bOneShot;          // Delete after first fire?
    bool bTemporary;        // Don't save to file?
    bool bActiveWhenClosed; // Fire even when disconnected from MUD?
    bool bOmitFromOutput;   // Don't echo to output window?
    bool bOmitFromLog;      // Don't write to log file?
    bool bExecutingScript;  // Currently executing (prevents deletion)?

    // ========== Metadata ==========

    QString strLabel;   // Timer name/label
    QString strGroup;   // Group name (for batch operations)
    qint32 iUserOption; // User-defined flags

    // ========== Plugin Support ==========

    bool bIncluded; // Loaded from included file?
    bool bSelected; // Active in plugin?

    // ========== Runtime/Tracking ==========

    QVariant dispid;         // Lua dispatch ID (function cache)
    qint64 nUpdateNumber;    // For detecting update clashes
    qint32 nInvocationCount; // How many times script called
    qint32 nMatched;         // How many times timer fired
    QDateTime tFireTime;     // When timer will next fire
    QDateTime tWhenFired;    // When timer last fired/reset
    quint32 nCreateSequence; // Creation order

    // ========== Static Sequence Counter ==========

    static quint32 GetNextTimerSequence()
    {
        return nNextCreateSequence++;
    }

  private:
    static quint32 nNextCreateSequence;
};

#endif // TIMER_H
