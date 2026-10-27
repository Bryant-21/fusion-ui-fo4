#include "fusionui/Tasks.h"
#include "core/DeferredTaskQueue.h"
#include "core/GameTasks.h"
#include "game/Tasks.h"

#include <F4SE/F4SE.h>

#include <utility>
#include <atomic>

namespace fusionui::game {
    namespace {
        std::atomic<const FUSIONUI_HostApi*> taskHost{};
    }

    void SetGameTaskHost(const FUSIONUI_HostApi* host) {
        taskHost.store(host);
    }

    void QueueLocalGameTask(std::function<void()> task) {
        static core::DeferredTaskQueue queue{[](core::DeferredTaskQueue::Task next) {
            if (auto* tasks = F4SE::GetTaskInterface()) tasks->AddTask(std::move(next));
        }};
        queue.Post(std::move(task));
    }
}

namespace fusionui {
    void QueueGameTask(std::function<void()> task) {
        if (!core::SubmitHostGameTask(game::taskHost.load(), task)) game::QueueLocalGameTask(std::move(task));
    }
}
