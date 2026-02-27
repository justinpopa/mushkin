// timer.cpp
// Timer Data Structure
//
// Implementation of Timer class

#include "timer.h"

// DISPID constant for script callbacks
#define DISPID_UNKNOWN (-1)

// Initialize static sequence counter
quint32 Timer::next_create_sequence = 0;

// Constructor - matches CTimer constructor from OtherTypes.h
Timer::Timer()
    : invocation_count(0)
      // DISPID constant for script callbacks
      ,
      at_hour(0), at_minute(0), at_second(0.0), every_hour(0), every_minute(0), every_second(0.0),
      offset_hour(0), offset_minute(0), offset_second(0.0), type(eInterval), one_shot(false),
      dispid(DISPID_UNKNOWN) // Initialize to "not checked yet"
      ,
      fire_time(QDateTime::currentDateTime()), temporary(false), active_when_closed(false),
      included(false), selected(false), send_to(0) // eSendToWorld = 0
      ,
      user_option(0), omit_from_output(false), omit_from_log(false), executing_script(false),
      scriptLanguage(ScriptLanguage::Lua), enabled(true) // Timers enabled by default
      ,
      update_number(0), matched(0) // Initialize match count
      ,
      when_fired(QDateTime::currentDateTime()), create_sequence(GetNextTimerSequence())
{
}
