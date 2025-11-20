#pragma once

#include "Export.h"
#include "ColorText.h"

#include <condition_variable>
#include <map>
#include <optional>
#include <thread>
#include <vector>

namespace DFHack {
    namespace Hotkey {
        struct KeySpec {
            int modifiers = 0;
            // Negative numbers denote mouse buttons
            int sym = 0;
            std::vector<std::string> focus;
        };

        struct KeyBinding {
            KeySpec spec;
            std::string command;
            std::string cmdline;
        };

        DFHACK_EXPORT std::optional<Hotkey::KeySpec> parseKeySpec(std::string spec, std::string* err = nullptr);
        DFHACK_EXPORT std::string keyspec_to_string(const KeySpec& spec, bool include_focus=false);
    }
    class DFHACK_EXPORT HotkeyManager {
        friend class Core;
        public:
            HotkeyManager();
            ~HotkeyManager();


            bool addKeybind(std::string keyspec, std::string cmd);
            bool addKeybind(Hotkey::KeySpec spec, std::string cmd);
            // Clear a keybind with the given keyspec, optionally for any focus, or with a specific command
            bool clearKeybind(std::string keyspec, bool any_focus=false, std::string cmdline="");
            bool clearKeybind(const Hotkey::KeySpec& spec, bool any_focus=false, std::string cmdline="");

            std::vector<std::string> listKeybinds(std::string keyspec);
            std::vector<std::string> listKeybinds(const Hotkey::KeySpec& spec);

            std::vector<Hotkey::KeyBinding> listActiveKeybinds();
            std::vector<Hotkey::KeyBinding> listAllKeybinds();

            bool handleKeybind(int sym, int modifiers);
            void setHotkeyCommand(std::string cmd);

            // Used to request the next keybind input is saved.
            // This is to allow for graphical keybinding menus.
            void requestKeybindInput();
            // Returns the latest requested keybind input
            std::string readKeybindInput();

        private:
            std::thread hotkey_thread;
            std::mutex lock {};
            std::condition_variable cond {};

            bool keybind_save_requested = false;
            std::string requested_keybind;

            int hotkey_sig = 0;
            std::string queued_command = "";

            std::map<int, std::vector<Hotkey::KeyBinding>> bindings;

            void hotkey_thread_fn();
            void handleKeybindingCommand(color_ostream& out, const std::vector<std::string>& parts);
    };
}
