/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2022 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <algorithm>
#include <condition_variable>
#include <forward_list>
#include <mutex>

#include <rex/assert.h>
#include <rex/thread.h>
#include <rex/thread/timer_queue.h>

namespace rex::thread {

using WaitItem = TimerQueueWaitItem;

class TimerQueue {
 public:
  using clock = WaitItem::clock;
  static_assert(clock::is_steady);

 public:
  TimerQueue() {
    dispatch_thread_ =
        std::jthread([this](std::stop_token stop_token) { TimerThreadMain(stop_token); });
  }

  ~TimerQueue() {
    dispatch_thread_.request_stop();
    cv_.notify_one();
  }

  void TimerThreadMain(std::stop_token stop_token) {
    const auto comp = [](const std::shared_ptr<WaitItem>& left,
                         const std::shared_ptr<WaitItem>& right) {
      return left->due_ < right->due_;
    };

    set_current_thread_name("rex::thread::TimerQueue");
    std::stop_callback stop_callback(stop_token, [this] { cv_.notify_one(); });

    while (!stop_token.stop_requested()) {
      std::unique_lock lock(mutex_);
      if (wait_queue_.empty()) {
        cv_.wait(lock, [&] { return stop_token.stop_requested() || !wait_queue_.empty(); });
      } else {
        cv_.wait_until(lock, wait_queue_.front()->due_,
                       [&] { return stop_token.stop_requested(); });
      }
      if (stop_token.stop_requested()) {
        break;
      }

      std::forward_list<std::shared_ptr<WaitItem>> ready_items;
      while (!wait_queue_.empty() && wait_queue_.front()->due_ <= clock::now()) {
        ready_items.push_front(std::move(wait_queue_.front()));
        wait_queue_.pop_front();
      }
      lock.unlock();

      std::forward_list<std::shared_ptr<WaitItem>> reschedule_items;
      for (auto& wait_item : ready_items) {
        auto state = WaitItem::State::kIdle;
        if (wait_item->state_.compare_exchange_strong(state, WaitItem::State::kInCallback,
                                                      std::memory_order_acq_rel)) {
          assert_not_null(wait_item->callback_);
          wait_item->callback_(wait_item->userdata_);

          if (wait_item->interval_ != clock::duration::zero() &&
              wait_item->state_.load(std::memory_order_acquire) !=
                  WaitItem::State::kInCallbackSelfDisarmed) {
            wait_item->due_ += wait_item->interval_;
            wait_item->state_.store(WaitItem::State::kIdle, std::memory_order_release);
            wait_item->state_.notify_all();
            reschedule_items.push_front(std::move(wait_item));
          } else {
            wait_item->state_.store(WaitItem::State::kDisarmed, std::memory_order_release);
            wait_item->state_.notify_all();
          }
        } else {
          assert_true(WaitItem::State::kDisarmed == state);
        }
      }

      reschedule_items.sort(comp);
      if (!reschedule_items.empty()) {
        lock.lock();
        wait_queue_.merge(reschedule_items, comp);
        lock.unlock();
        cv_.notify_one();
      }
    }
  }

  std::weak_ptr<WaitItem> QueueTimer(std::shared_ptr<WaitItem> wait_item) {
    auto wait_item_weak = std::weak_ptr<WaitItem>(wait_item);

    // Mitigate callback flooding
    wait_item->due_ = std::max(clock::now() - wait_item->interval_, wait_item->due_);

    {
      std::scoped_lock lock(mutex_);
      wait_queue_.push_front(std::move(wait_item));
      wait_queue_.sort([](const std::shared_ptr<WaitItem>& left,
                          const std::shared_ptr<WaitItem>& right) {
        return left->due_ < right->due_;
      });
    }
    cv_.notify_one();

    return wait_item_weak;
  }

  std::jthread::id dispatch_thread_id() const { return dispatch_thread_.get_id(); }

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::forward_list<std::shared_ptr<WaitItem>> wait_queue_;
  std::jthread dispatch_thread_;
};

rex::thread::TimerQueue timer_queue_;

void TimerQueueWaitItem::Disarm() {
  State state;

  // Special case for calling from a callback itself
  if (std::this_thread::get_id() == parent_queue_->dispatch_thread_id()) {
    state = State::kInCallback;
    if (state_.compare_exchange_strong(state, State::kInCallbackSelfDisarmed,
                                       std::memory_order_acq_rel)) {
      // If we are self disarming from the callback set this special state and
      // exit
      return;
    }
    // Normal case can handle the rest
  }

  state = State::kIdle;
  // Classes which hold WaitItems will often call Disarm() to cancel them during
  // destruction. This may lead to race conditions when the dispatch thread
  // executes a callback which accesses memory that is freed simultaneously due
  // to this. Therefore, we need to guarantee that no callbacks will be running
  // once Disarm() has returned.
  while (!state_.compare_exchange_weak(state, State::kDisarmed, std::memory_order_acq_rel)) {
    if (state == State::kDisarmed) {
      break;
    }
    if (state == State::kInCallback || state == State::kInCallbackSelfDisarmed) {
      // Wait for callback to complete - dispatch thread will notify
      state_.wait(state, std::memory_order_acquire);
    }
    state = State::kIdle;
  }
}

std::weak_ptr<WaitItem> QueueTimerOnce(std::move_only_function<void(void*)> callback,
                                       void* userdata, WaitItem::clock::time_point due) {
  return timer_queue_.QueueTimer(std::make_shared<WaitItem>(
      std::move(callback), userdata, &timer_queue_, due, WaitItem::clock::duration::zero()));
}

std::weak_ptr<WaitItem> QueueTimerRecurring(std::move_only_function<void(void*)> callback,
                                            void* userdata, WaitItem::clock::time_point due,
                                            WaitItem::clock::duration interval) {
  return timer_queue_.QueueTimer(
      std::make_shared<WaitItem>(std::move(callback), userdata, &timer_queue_, due, interval));
}

}  // namespace rex::thread
