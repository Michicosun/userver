#pragma once

/// @file engine/task/task.hpp
/// @brief @copybrief engine::Task

#include <chrono>
#include <string>

#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <engine/deadline.hpp>

namespace engine {
namespace ev {
class ThreadControl;
}
namespace impl {
class TaskContext;
class TaskContextHolder;
}  // namespace impl

class TaskProcessor;

/// Asynchronous task
class Task {
 public:
  /// Task importance
  enum class Importance {
    /// Normal task
    kNormal,
    /// Critical task, cannot be cancelled due to task processor overload
    kCritical,
  };

  /// Task state
  enum class State {
    kInvalid,    ///< Unusable
    kNew,        ///< just created, not registered with task processor
    kQueued,     ///< awaits execution
    kRunning,    ///< executing user code
    kSuspended,  ///< suspended, e.g. waiting for blocking call to complete
    kCancelled,  ///< exited user code because of external request
    kCompleted,  ///< exited user code with return or throw
  };

  /// Task cancellation reason
  enum class CancellationReason {
    kNone,         ///< Not cancelled
    kUserRequest,  ///< User request
    kOverload,     ///< Task processor overload
    kAbandoned,    ///< Task destruction before finish
    kShutdown,     ///< Task processor shutdown
  };

  /// @brief Default constructor
  ///
  /// Creates an invalid task.
  Task();

  /// @brief Destructor
  ///
  /// When the task is still valid and is not finished, cancels it and waits
  /// until it finishes
  virtual ~Task();

  Task(Task&&) noexcept;
  Task& operator=(Task&&) noexcept;

  /// Checks whether the task is valid
  bool IsValid() const;
  explicit operator bool() const { return IsValid(); }

  /// Gets the task State
  State GetState() const;

  /// Returns whether the task finished execution
  bool IsFinished() const;

  /// Suspends execution until the task finishes
  void Wait() const;

  /// Suspends execution until the task finishes or after the specified timeout
  template <typename Rep, typename Period>
  void WaitFor(const std::chrono::duration<Rep, Period>&) const;

  /// @brief Suspends execution until the task finishes or until the specified
  /// time point is reached
  template <typename Clock, typename Duration>
  void WaitUntil(const std::chrono::time_point<Clock, Duration>&) const;

  /// @brief Detaches task, allowing it to continue execution out of scope
  /// @note After detach, Task becomes invalid
  void Detach() &&;

  /// Queues task cancellation request
  void RequestCancel();

  /// Gets task CancellationReason
  CancellationReason GetCancellationReason() const;

 protected:
  /// @cond
  /// Constructor for internal use
  Task(impl::TaskContextHolder&&);
  /// @endcond

 private:
  void DoWaitUntil(Deadline) const;
  void Terminate() noexcept;

  boost::intrusive_ptr<impl::TaskContext> context_;
};

/// Returns a string representation of a cancellation reason
std::string ToString(Task::CancellationReason reason);

namespace current_task {

/// Returns reference to the task processor executing the caller
TaskProcessor& GetTaskProcessor();

/// @cond
/// Returns ev thread handle, internal use only
ev::ThreadControl& GetEventThread();

/// Updates spurious wakeup statistics, internal use only
void AccountSpuriousWakeup();
/// @endcond

}  // namespace current_task

template <typename Rep, typename Period>
void Task::WaitFor(const std::chrono::duration<Rep, Period>& duration) const {
  DoWaitUntil(Deadline::FromDuration(duration));
}

template <typename Clock, typename Duration>
void Task::WaitUntil(
    const std::chrono::time_point<Clock, Duration>& until) const {
  DoWaitUntil(Deadline::FromTimePoint(until));
}

}  // namespace engine
