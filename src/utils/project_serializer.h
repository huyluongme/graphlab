#pragma once

#include "math/expression.h"
#include "ui/sidebar_panel.h"
#include "imgui.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace GraphLab::Utils {
    /**
     * @brief Handles project workspace serialization (saving and loading expressions, parameters, and viewport state).
     */
    class ProjectSerializer {
    public:
        /**
         * @brief Exports the project state to a .graphlab file.
         */
        static bool SaveProject(
            const std::string& filepath,
            const std::vector<Math::Expression>& expressions,
            const std::unordered_map<std::string, UI::ParamState>& params,
            float zoom,
            ImVec2 panOffset
        );

        /**
         * @brief Imports the project state from a .graphlab file.
         */
        static bool LoadProject(
            const std::string& filepath,
            std::vector<Math::Expression>& expressions,
            std::unordered_map<std::string, UI::ParamState>& params,
            float& outZoom,
            ImVec2& outPanOffset
        );
    };
}
