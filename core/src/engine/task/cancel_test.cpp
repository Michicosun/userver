#include <gtest/gtest.h>

#include <chrono>
#include <stdexcept>

#include <engine/async.hpp>
#include <engine/single_consumer_event.hpp>
#include <engine/sleep.hpp>
#include <engine/task/cancel.hpp>
#include <engine/task/task_context.hpp>
#include <engine/task/task_with_result.hpp>

#include <utest/utest.hpp>

// Functors defined in dtors should unwind though
TEST(Cancel, UnwindWorksInDtorSubtask) {
  class DetachingRaii final {
   public:
    DetachingRaii(engine::SingleConsumerEvent& detach_event,
                  engine::TaskWithResult<void>& detached_task)
        : detach_event_(detach_event), detached_task_(detached_task) {}

    ~DetachingRaii() {
      detached_task_ = engine::impl::Async([] {
        while (!engine::current_task::IsCancelRequested()) {
          engine::InterruptibleSleepFor(std::chrono::milliseconds(100));
        }
        engine::current_task::CancellationPoint();
        ADD_FAILURE() << "Cancelled task ran past cancellation point";
      });
      detach_event_.Send();
    }

   private:
    engine::SingleConsumerEvent& detach_event_;
    engine::TaskWithResult<void>& detached_task_;
  };

  RunInCoro([] {
    engine::TaskWithResult<void> detached_task;
    engine::SingleConsumerEvent task_detached_event;
    auto task = engine::impl::Async(
        [&] { DetachingRaii raii(task_detached_event, detached_task); });
    ASSERT_TRUE(task_detached_event.WaitForEvent());
    task.Wait();

    detached_task.WaitFor(std::chrono::milliseconds(10));
    ASSERT_FALSE(detached_task.IsFinished());
    detached_task.SyncCancel();
  });
}
