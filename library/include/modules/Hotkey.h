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
        class DFHACK_EXPORT KeySpec {
            public:
                int modifiers = 0;
                // Negative numbers denote mouse buttons
                int sym = 0;
                std::vector<std::string> focus;

                static std::optional<Hotkey::KeySpec> parse(std::string spec, std::string* err = nullptr);
                std::string toString(bool include_focus=true) const;

                // Determines if a keybind could be disruptive to normal gameplay,
                // including typing and navigating the UI.
                bool isDisruptive() const;
        };

        struct KeyBinding {
            KeySpec spec;
            std::string command;
            std::string cmdline;
        };
    }
    class DFHACK_EXPORT HotkeyManager {
        friend class Core;
        public:
            HotkeyManager();
            ~HotkeyManager();


            bool addKeybind(std::string keyspec, std::string_view cmd);
            bool addKeybind(Hotkey::KeySpec spec, std::string_view cmd);
            // Clear a keybind with the given keyspec, optionally for any focus, or with a specific command
            bool removeKeybind(std::string keyspec, bool match_focus=true, std::string_view cmdline="");
            bool removeKeybind(const Hotkey::KeySpec& spec, bool match_focus=true, std::string_view cmdline="");

            std::vector<std::string> listKeybinds(std::string keyspec);
            std::vector<std::string> listKeybinds(const Hotkey::KeySpec& spec);

            std::vector<Hotkey::KeyBinding> listActiveKeybinds();
            std::vector<Hotkey::KeyBinding> listAllKeybinds();

            bool handleKeybind(int sym, int modifiers);
            void setHotkeyCommand(std::string cmd);

            // Used to request the next hotkey-compatible input is saved.
            // This is to allow for graphical keybinding menus.
            void requestKeybindingInput(bool cancel=false);
            // Returns the latest requested keybind input
            std::string getKeybindingInput();

        private:
            std::thread hotkey_thread;
            std::mutex lock;
            std::condition_variable cond;

            bool keybind_save_requested = false;
            std::string requested_keybind;

            uint8_t hotkey_sig = 0;
            std::string queued_command;

            std::map<int, std::vector<Hotkey::KeyBinding>> bindings;

            void hotkey_thread_fn();
            void handleKeybindingCommand(color_ostream& out, const std::vector<std::string>& parts);
    };
}
