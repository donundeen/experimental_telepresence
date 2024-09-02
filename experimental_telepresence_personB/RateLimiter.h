// MIT License
//
// Copyright (c) 2020 Matthew Lai
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef __RATE_LIMITER_H__
#define __RATE_LIMITER_H__

#include "Arduino.h"

#include <functional>
#include <limits>

using DurationType = unsigned long;
using TokenType = unsigned long;

template <DurationType Period, TokenType CallsPerPeriod>
class RateLimiter {
 public:
  RateLimiter()
    : bucket_(CallsPerPeriod), last_bucket_update_(TimeNow()), dropped_calls_(0) {}

  // Set a function to be called if there are dropped calls. It will be called with
  // the number of dropped calls as the argument right before the next time a call
  // succeeds.
  using DroppedCallCallbackType = std::function<void(unsigned int)>;
  void SetDroppedCallCallback(DroppedCallCallbackType fn) {
    dropped_callback_ = fn;
  }

  template <typename Fn>
  bool CallOrDrop(Fn fn) {
    UpdateBucket();
    if (bucket_ > 0) {
      MaybeCallDroppedCallCallback();
      fn();
      --bucket_;
      return true;
    } else {
      ++dropped_calls_;
      return false;
    }
  }

  template <typename Fn>
  auto Call(Fn fn) -> decltype(fn()) {
    UpdateBucket();
    while (bucket_ == 0) {
      delay(EstimateWaitTime());
      UpdateBucket();
    }
    MaybeCallDroppedCallCallback();
    --bucket_;
    return fn();
  }

 private:
  void UpdateBucket() {
    DurationType elapsed_time = GetElapsedTime();
    unsigned int new_tokens = elapsed_time * CallsPerPeriod / Period;
    bucket_ += new_tokens;
    bucket_ = min(bucket_, CallsPerPeriod);
    // Advance last_bucket_update_ not to now, but to when the last token would have been
    // added. This can overflow, and that's fine (we handled it in GetElapsedTime()).
    last_bucket_update_ += new_tokens * Period / CallsPerPeriod;
  }

  DurationType EstimateWaitTime() {
    DurationType time_per_token = Period / CallsPerPeriod;
    DurationType elapsed_time = GetElapsedTime();
    // We have to be careful of underflow here in case of race condition.
    if (time_per_token > elapsed_time) {
      return time_per_token - elapsed_time;
    } else {
      return 0;
    }
  }

  DurationType GetElapsedTime() {
    DurationType now = TimeNow();
    if (now < last_bucket_update_) {
      // We have a time overflow. Re-calculate elapsed_time using wrap-around arithmetics.
      return std::numeric_limits<DurationType>::max() - last_bucket_update_ + now;
    } else {
      return now - last_bucket_update_;
    }
  }

  void MaybeCallDroppedCallCallback() {
    if (dropped_callback_ && dropped_calls_ > 0) {
      dropped_callback_(dropped_calls_);
      dropped_calls_ = 0;
    }
  }

  DurationType TimeNow() {
    return millis();
  }
 
  TokenType bucket_;
  DurationType last_bucket_update_;
  unsigned int dropped_calls_;
  DroppedCallCallbackType dropped_callback_;
};

#endif // __RATE_LIMITER_H__
