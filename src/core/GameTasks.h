#pragma once

#include "fusionui/Abi.h"

#include <functional>
#include <memory>
#include <utility>

namespace fusionui::core {
    inline std::function<void()> OwnedGameTask(void* user, FUSIONUI_TaskCallback run, FUSIONUI_TaskCallback destroy) {
        return [data = std::shared_ptr<void>(user, destroy), run] { run(data.get()); };
    }

    inline bool SubmitHostGameTask(const FUSIONUI_HostApi* host, std::function<void()>& task) {
        if (!host || host->size < offsetof(FUSIONUI_HostApi, queueGameTask) + sizeof(host->queueGameTask) ||
            !host->queueGameTask) return false;
        auto* user = new std::function<void()>(std::move(task));
        host->queueGameTask(user,
            +[](void* data) { (*static_cast<std::function<void()>*>(data))(); },
            +[](void* data) { delete static_cast<std::function<void()>*>(data); });
        return true;
    }
}
