#pragma once

#include "Export.h"

#include <condition_variable>
#include <map>
#include <thread>
#include <vector>
#include <optional>

namespace DFHack {
    class DFHACK_EXPORT HotkeyManager {
        public:
            HotkeyManager();
            ~HotkeyManager();

            struct KeySpec {
                int modifiers = 0;
                // Negative numbers denote mouse buttons
                int sym = 0;
                std::vector<std::string> focus;
            };

            struct KeyBinding {
                HotkeyManager::KeySpec spec;
                std::string command;
                std::string cmdline;
            };

            bool addKeybind(std::string keyspec, std::string cmd);
            bool addKeybind(KeySpec spec, std::string cmd);
            bool clearKeybind(std::string keyspec, bool any_focus=false);
            bool clearKeybind(const KeySpec& spec, bool any_focus=false);

            std::vector<std::string> listKeybinds(std::string keyspec);
            std::vector<std::string> listKeybinds(const KeySpec& spec);

            std::vector<KeyBinding> listActiveKeybinds();

            bool handleKeybind(int sym, int modifiers);

            void setHotkeyCommand(std::string cmd);

            std::optional<KeySpec> parseKeySpec(std::string spec);
        private:
            std::thread hotkey_thread;
            std::mutex lock {};
            std::condition_variable cond {};

            int hotkey_sig = 0;
            std::string queued_command = "";

            std::map<int, std::vector<KeyBinding>> bindings;

            void hotkey_thread_fn();
    };
}
