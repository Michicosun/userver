#pragma once

#include <chrono>
#include <exception>
#include <future>
#include <memory>

#include "future_state.hpp"

namespace engine {

// Convenience classes for asynchronous data/event transfer
// from external world (ev loops) to coroutines.
//
// Future can only be used in coroutines.
// Promise should not be used from coroutines (use Async instead).

template <typename T>
class Promise;

template <typename T>
class Future {
 public:
  Future() = default;

  Future(const Future&) = delete;
  Future(Future&&) noexcept = default;
  Future& operator=(const Future&) = delete;
  Future& operator=(Future&&) noexcept = default;

  bool IsValid() const;
  explicit operator bool() const { return IsValid(); }

  T Get();
  void Wait() const;

  template <typename Rep, typename Period>
  std::future_status WaitFor(
      const std::chrono::duration<Rep, Period>& duration) const;

  template <typename Clock, typename Duration>
  std::future_status WaitUntil(
      const std::chrono::time_point<Clock, Duration>& until) const;

 private:
  friend class Promise<T>;

  explicit Future(std::shared_ptr<impl::FutureState<T>> state);

  void CheckValid() const;

  std::shared_ptr<impl::FutureState<T>> state_;
};

template <typename T>
class Promise {
 public:
  Promise();
  ~Promise();

  Promise(const Promise&) = delete;
  Promise(Promise&&) noexcept = default;
  Promise& operator=(const Promise&) = delete;
  Promise& operator=(Promise&&) noexcept = default;

  Future<T> GetFuture();

  void SetValue(const T&);
  void SetValue(T&&);
  void SetException(std::exception_ptr ex);

 private:
  std::shared_ptr<impl::FutureState<T>> state_;
};

template <>
class Promise<void> {
 public:
  Promise();
  ~Promise();

  Promise(const Promise&) = delete;
  Promise(Promise&&) noexcept = default;
  Promise& operator=(const Promise&) = delete;
  Promise& operator=(Promise&&) noexcept = default;

  Future<void> GetFuture();

  void SetValue();
  void SetException(std::exception_ptr ex);

 private:
  std::shared_ptr<impl::FutureState<void>> state_;
};

template <typename T>
bool Future<T>::IsValid() const {
  return !!state_;
}

template <typename T>
T Future<T>::Get() {
  CheckValid();
  auto result = state_->Get();
  state_.reset();
  return result;
}

template <>
inline void Future<void>::Get() {
  CheckValid();
  state_->Get();
  state_.reset();
}

template <typename T>
void Future<T>::Wait() const {
  CheckValid();
  state_->Wait();
}

template <typename T>
template <typename Rep, typename Period>
std::future_status Future<T>::WaitFor(
    const std::chrono::duration<Rep, Period>& duration) const {
  CheckValid();
  return state_->WaitFor(duration);
}

template <typename T>
template <typename Clock, typename Duration>
std::future_status Future<T>::WaitUntil(
    const std::chrono::time_point<Clock, Duration>& until) const {
  CheckValid();
  return state_->WaitUntil(until);
}

template <typename T>
Future<T>::Future(std::shared_ptr<impl::FutureState<T>> state)
    : state_(std::move(state)) {
  CheckValid();
  state_->EnsureNotRetrieved();
}

template <typename T>
void Future<T>::CheckValid() const {
  if (!state_) {
    throw std::future_error(std::future_errc::no_state);
  }
}

template <typename T>
Promise<T>::Promise() : state_(std::make_shared<impl::FutureState<T>>()) {}

template <typename T>
Promise<T>::~Promise() {
  if (state_ && !state_->IsReady()) {
    state_->SetException(std::make_exception_ptr(
        std::future_error(std::future_errc::broken_promise)));
  }
}

template <typename T>
Future<T> Promise<T>::GetFuture() {
  return Future<T>(state_);
}

template <typename T>
void Promise<T>::SetValue(const T& value) {
  state_->SetValue(value);
}

template <typename T>
void Promise<T>::SetValue(T&& value) {
  state_->SetValue(std::move(value));
}

template <typename T>
void Promise<T>::SetException(std::exception_ptr ex) {
  state_->SetException(std::move(ex));
}

inline Promise<void>::Promise()
    : state_(std::make_shared<impl::FutureState<void>>()) {}

inline Promise<void>::~Promise() {
  if (state_ && !state_->IsReady()) {
    state_->SetException(std::make_exception_ptr(
        std::future_error(std::future_errc::broken_promise)));
  }
}

inline Future<void> Promise<void>::GetFuture() { return Future<void>(state_); }

inline void Promise<void>::SetValue() {
  assert(!state_->IsReady());
  state_->SetValue();
}

inline void Promise<void>::SetException(std::exception_ptr ex) {
  state_->SetException(std::move(ex));
}

}  // namespace engine
