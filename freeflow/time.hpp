
#ifndef FREEFLOW_TIME_HPP
#define FREEFLOW_TIME_HPP

// The time module provides a comprehensive interface for managing
// time data structures. This is build largely on the C++ std::chrono
// facilities. Note that the default time representation is based
// on the system clock.


#include "socket.hpp"

#include <chrono>


// Bring time literals into scope so we can use them easily within
// our programs.
using namespace std::literals::chrono_literals;


namespace ff
{

// The default clock type is the system clock.
using Clock = std::chrono::system_clock;


// A time is a particular point in time on a clock.
using Time = Clock::time_point;


// A duration is a (possibly negative) span of time between
// time points.
using Duration = Clock::duration;


// Durations
using Nanoseconds = std::chrono::nanoseconds;
using Microseconds = std::chrono::microseconds;
using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;
using Minutes = std::chrono::minutes;
using Hours = std::chrono::hours;


// Floating point durations.
using Fp_seconds = std::chrono::duration<double, Seconds::period>;


// Returns the current time point.
inline Time
now()
{
  return Clock::now();
}


} // namespace ff

#endif
