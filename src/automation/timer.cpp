// timer.cpp
// Timer Data Structure
//
// Implementation of Timer class

#include "timer.h"
#include "sendto.h"

// DISPID constant for script callbacks
#define DISPID_UNKNOWN (-1)

// Initialize static sequence counter
quint32 Timer::next_create_sequence = 0;

// Equality operator for structural duplicate detection during XML load.
// Matches all persistent fields that the original CTimer::operator== checks
// (xml_load_world.cpp:288-317).
bool Timer::operator==(const Timer& rhs) const
{
    return active_when_closed == rhs.active_when_closed && enabled == rhs.enabled &&
           omit_from_log == rhs.omit_from_log && omit_from_output == rhs.omit_from_output &&
           one_shot == rhs.one_shot && at_second == rhs.at_second &&
           every_second == rhs.every_second && offset_second == rhs.offset_second &&
           at_hour == rhs.at_hour && at_minute == rhs.at_minute && every_hour == rhs.every_hour &&
           every_minute == rhs.every_minute && offset_hour == rhs.offset_hour &&
           offset_minute == rhs.offset_minute && send_to == rhs.send_to && type == rhs.type &&
           user_option == rhs.user_option && contents == rhs.contents && group == rhs.group &&
           label == rhs.label && procedure == rhs.procedure && variable == rhs.variable;
}

// Constructor - matches CTimer constructor from OtherTypes.h
Timer::Timer()
    : invocation_count(0)
      // DISPID constant for script callbacks
      ,
      at_hour(0), at_minute(0), at_second(0.0), every_hour(0), every_minute(0), every_second(0.0),
      offset_hour(0), offset_minute(0), offset_second(0.0), type(TimerType::Interval),
      one_shot(false), dispid(DISPID_UNKNOWN) // Initialize to "not checked yet"
      ,
      fire_time(QDateTime::currentDateTime()), temporary(false), active_when_closed(false),
      included(false), selected(false), send_to(eSendToWorld), user_option(0),
      omit_from_output(false), omit_from_log(false), executing_script(false),
      scriptLanguage(ScriptLanguage::Lua), enabled(true) // Timers enabled by default
      ,
      update_number(0), matched(0) // Initialize match count
      ,
      when_fired(QDateTime::currentDateTime()), create_sequence(GetNextTimerSequence())
{
}
