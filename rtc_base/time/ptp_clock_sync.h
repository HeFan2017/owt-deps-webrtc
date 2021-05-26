// Copyright (C) <2021> Intel Corporation
//
// SPDX-License-Identifier: Apache-2.0

#ifndef RTC_BASE_TIME_PTP_CLOCK_SYNC_H_
#define RTC_BASE_TIME_PTP_CLOCK_SYNC_H_

#include <windows.h>
#include <cstdint>

#define MICROSECONDS_FACTOR 1000000.0
#define OFFSET_FACTOR 200000
#define SERVER_FREQUENCY 0.09  // RTP/NTP timestamp runs at 90KHz clock

#ifndef WEBRTC_WIN
// for Android and Linux
#include <time.h>
typedef union _LARGE_INTEGER {
  struct {
    uint32_t LowPart;
    int32_t HighPart;
  } u;
  int64_t QuadPart;
} LARGE_INTEGER;

inline int32_t QueryPerformanceFrequency(LARGE_INTEGER *frequency)
{
    struct timespec  res;
    int32_t          ret;

    if ( (ret = clock_getres(CLOCK_MONOTONIC, &res)) != 0 )
    {
        return -1;
    }

    // resolution (precision) can't be in seconds for current machine and OS
    if (res.tv_sec != 0)
    {
        return -1;
    }
    frequency->QuadPart = (1000000000LL) / res.tv_nsec;

    return 0;
}

inline int32_t QueryPerformanceCounter(LARGE_INTEGER *performanceCount)
{
    struct timespec     res;
    struct timespec     t;
    int32_t             ret;

    if ( (ret = clock_getres (CLOCK_MONOTONIC, &res)) != 0 )
    {
        return -1;
    }
    if (res.tv_sec != 0)
    { // resolution (precision) can't be in seconds for current machine and OS
        return -1;
    }
    if( (ret = clock_gettime(CLOCK_MONOTONIC, &t)) != 0)
    {
        return -1;
    }
    performanceCount->QuadPart = (1000000000LL * t.tv_sec +
        t.tv_nsec) / res.tv_nsec;

    return 0;
}

#endif


// A Windows implementation for PTP using timestamp from RTP and local
// timestamp. We may need to implement something like QueryPerformanceFrequency
// for this to work on Linux platforms.
namespace webrtc {
class PTPClockSync {
 public:
  PTPClockSync()
      : m_server_point(0), m_server_freq(0), m_client_point(0), m_last_ts(0) {
    uint64_t freq; // Performance counter frequency in a second.
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    m_client_freq = (double)freq / MICROSECONDS_FACTOR;
    m_server_freq = SERVER_FREQUENCY;
  }
  ~PTPClockSync() {}

  void Sync(uint32_t ts, uint64_t tc) {
    if (GetDuration(ts, tc) < 0 ||
        (ts - m_last_ts) > OFFSET_FACTOR * m_server_freq) {
      UpdateSync(ts, tc);
    }
    m_last_ts = ts;
  }

  double GetDuration(uint32_t ts, uint64_t tc) {
    int ds = (int)(ts - m_server_point);
    int dc = (int)(tc - m_client_point);
    return (double)dc / m_client_freq - (double)ds / m_server_freq;
  }

 protected:
  uint32_t m_server_point;
  double m_server_freq;  // count per us
  uint64_t m_client_point;
  double m_client_freq;  // count per us
  uint32_t m_last_ts;

  void UpdateSync(uint32_t ts, uint64_t tc) {
    m_client_point = tc;
    m_server_point = ts;
  }
};
}  // namespace webrtc
#endif
