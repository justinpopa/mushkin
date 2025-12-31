// timer.cpp
// Timer Data Structure
//
// Implementation of Timer class

#include "timer.h"

// DISPID constant for script callbacks
#define DISPID_UNKNOWN (-1)

// Initialize static sequence counter
quint32 Timer::nNextCreateSequence = 0;

// Constructor - matches CTimer constructor from OtherTypes.h
Timer::Timer()
    : nInvocationCount(0)
      // DISPID constant for script callbacks
      ,
      iAtHour(0), iAtMinute(0), fAtSecond(0.0), iEveryHour(0), iEveryMinute(0), fEverySecond(0.0),
      iOffsetHour(0), iOffsetMinute(0), fOffsetSecond(0.0), iType(eInterval), bOneShot(false),
      dispid(DISPID_UNKNOWN) // Initialize to "not checked yet"
      ,
      tFireTime(QDateTime::currentDateTime()), bTemporary(false), bActiveWhenClosed(false),
      bIncluded(false), bSelected(false), iSendTo(0) // eSendToWorld = 0
      ,
      iUserOption(0), bOmitFromOutput(false), bOmitFromLog(false), bExecutingScript(false),
      scriptLanguage(ScriptLanguage::Lua), bEnabled(true) // Timers enabled by default
      ,
      nUpdateNumber(0), nMatched(0) // Initialize match count
      ,
      tWhenFired(QDateTime::currentDateTime()), nCreateSequence(GetNextTimerSequence())
{
}
