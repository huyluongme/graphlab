#pragma once

namespace GraphLab
{
    namespace UI
    {
        /**
         * @brief Main navigation menu bar component for GraphLab.
         */
        class MainMenuBar
        {
        public:
            MainMenuBar() = default;
            ~MainMenuBar() = default;

            void OnRenderUI();

        private:
            void ShowAboutPopup();

        private:
            bool m_ShowAboutPopup = false;
        };
    }
}
