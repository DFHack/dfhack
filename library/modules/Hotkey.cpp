#include "modules/Hotkey.h"

#include "Core.h"
#include "ColorText.h"
#include "MiscUtils.h"
#include "PluginManager.h"

#include "modules/DFSDL.h"
#include "modules/Gui.h"

#include "df/global_objects.h"
#include "df/viewscreen.h"
#include "df/interfacest.h"

#include <ranges>
#include <SDL_keycode.h>

using namespace DFHack;

enum HotkeySignal {
    None = 0,
    CmdReady,
    Shutdown,
};

bool operator==(const HotkeyManager::KeySpec& a, const HotkeyManager::KeySpec& b) {
    return a.modifiers == b.modifiers && a.sym == b.sym &&
        a.focus.size() == b.focus.size() &&
        std::equal(a.focus.begin(), a.focus.end(), b.focus.begin());
}

// Equality operator for key bindings
bool operator==(const HotkeyManager::KeyBinding& a, const HotkeyManager::KeyBinding& b) {
    return a.spec == b.spec &&
        a.command == b.command &&
        a.cmdline == b.cmdline;
}

// Hotkeys actions are executed from an external thread to avoid deadlocks
// that may occur if running commands from the render or simulation threads.
void HotkeyManager::hotkey_thread_fn() {
    auto& core = DFHack::Core::getInstance();

    std::unique_lock<std::mutex> l(lock);
    while (true) {
        cond.wait(l, [this]() { return this->hotkey_sig != HotkeySignal::None; });
        if (hotkey_sig == HotkeySignal::Shutdown)
            return;
        if (hotkey_sig != HotkeySignal::CmdReady)
            continue;

        // Copy and reset important data, then release the lock
        this->hotkey_sig = HotkeySignal::None;
        std::string cmd = this->queued_command;
        this->queued_command.clear();
        l.unlock();

        // Attempt execution of command
        DFHack::color_ostream_proxy out(core.getConsole());
        auto res = core.runCommand(out, cmd);
        if (res == DFHack::CR_NOT_IMPLEMENTED)
            out.printerr("Invalid hotkey command: '%s'\n", cmd.c_str());
        l.lock();
    }
}

std::optional<HotkeyManager::KeySpec> HotkeyManager::parseKeySpec(std::string spec) {
    KeySpec out;

    // Determine focus string, if present
    size_t focus_idx = spec.find('@');
    if (focus_idx != std::string::npos) {
        split_string(&out.focus, spec.substr(focus_idx + 1), "|");
        spec.erase(focus_idx);
    }

    // Determine modifier flags
    auto match_modifier = [&out, &spec](std::string_view prefix, int mod) {
        bool found = spec.starts_with(prefix);
        if (found) {
            out.modifiers |= mod;
            spec.erase(0, prefix.size());
        }
        return found;
    };
    while (match_modifier("Shift-", DFH_MOD_SHIFT) || match_modifier("Ctrl-", DFH_MOD_CTRL) || match_modifier("Alt-", DFH_MOD_ALT)) {}

    out.sym = DFSDL::DFSDL_GetKeyFromName(spec.c_str());
    if (out.sym != SDLK_UNKNOWN)
        return out;

    // Attempt to parse as a mouse binding
    if (spec.starts_with("MOUSE")) {
        spec.erase(0, 5);
        // Read button number, ensuring between 1 and 15 inclusive
        try {
            int mbutton = std::stoi(spec);
            if (mbutton >= 1 && mbutton <= 15) {
                out.sym = -mbutton;
                return out;
            }
        } catch (...) {
            // If integer parsing fails, it isn't valid
        }
    }

    // Invalid key binding
    return std::nullopt;
}

bool HotkeyManager::addKeybind(KeySpec spec, std::string cmd) {
    // No point in a hotkey with no action
    if (cmd.empty())
        return false;

    KeyBinding binding;
    binding.spec = spec;
    binding.cmdline = cmd;
    size_t space_idx = cmd.find(' ');
    binding.command = space_idx == std::string::npos ? cmd : cmd.substr(0, space_idx);

    std::lock_guard<std::mutex> l(lock);
    auto& bindings = this->bindings[binding.spec.sym];
    for (auto& bind : bindings) {
        // Don't set a keybind twice, but return true as there isn't an issue
        if (bind == binding)
            return true;
    }

    bindings.emplace_back(binding);
    return true;
}

bool HotkeyManager::addKeybind(std::string keyspec, std::string cmd) {
    std::optional<KeySpec> spec_opt = parseKeySpec(keyspec);
    if (!spec_opt.has_value())
        return false;
    return this->addKeybind(spec_opt.value(), cmd);
}

bool HotkeyManager::clearKeybind(const KeySpec& spec, bool any_focus) {
    std::lock_guard<std::mutex> l(lock);
    if (!bindings.contains(spec.sym))
        return false;
    auto& binds = bindings[spec.sym];

    auto new_end = std::remove_if(binds.begin(), binds.end(), [any_focus, spec](const auto& v) {
            return any_focus
                ? v.spec.sym == spec.sym && v.spec.modifiers == spec.modifiers
                : v.spec == spec;
    });
    if (new_end == binds.end())
        return false; // No bindings removed

    binds.erase(new_end, binds.end());
    return true;
}

bool HotkeyManager::clearKeybind(std::string keyspec, bool any_focus) {
    std::optional<KeySpec> spec_opt = parseKeySpec(keyspec);
    if (!spec_opt.has_value())
        return false;
    return this->clearKeybind(spec_opt.value(), any_focus);
}

std::vector<std::string> HotkeyManager::listKeybinds(const KeySpec& spec) {
    std::lock_guard<std::mutex> l(lock);
    if (!bindings.contains(spec.sym))
        return {};

    std::vector<std::string> out;

    auto& binds = bindings[spec.sym];
    for (const auto& bind : binds) {
        if (bind.spec.modifiers != spec.modifiers)
            continue;

        // If no focus is required, it is always active
        if (spec.focus.empty() || bind.spec.focus.empty()) {
            out.push_back(bind.cmdline);
            continue;
        }

        // If a focus is required, determine if search spec if the same or more specific
        for (const auto& requested : spec.focus) {
            for (const auto& to_match : bind.spec.focus) {
                if (prefix_matches(to_match, requested))
                    out.push_back("@" + to_match + ":" + bind.cmdline);
            }
        }
    }

    return out;
}

std::vector<std::string> HotkeyManager::listKeybinds(std::string keyspec) {
    std::optional<KeySpec> spec_opt = parseKeySpec(keyspec);
    if (!spec_opt.has_value())
        return {};
    return this->listKeybinds(spec_opt.value());
}

std::vector<HotkeyManager::KeyBinding> HotkeyManager::listActiveKeybinds() {
    std::vector<KeyBinding> out;

    for(const auto& [_, bind_set] : bindings) {
        for (const auto& binding : bind_set) {
            if (binding.spec.focus.empty()) {
                // Binding always active
                out.emplace_back(binding);
                continue;
            }
            for (const auto& focus : binding.spec.focus) {
                // Determine if focus string allows this binding
                if (Gui::matchFocusString(focus)) {
                    out.emplace_back(binding);
                    break;
                }
            }
        }
    }

    return out;
}

bool HotkeyManager::handleKeybind(int sym, int modifiers) {
    // Ensure gamestate is ready
    if (!df::global::gview || !df::global::plotinfo)
        return false;

    // Get bottommost active screen
    df::viewscreen *screen = &df::global::gview->view;
    while (screen->child)
        screen = screen->child;

    if (sym == SDLK_KP_ENTER)
        sym = SDLK_RETURN;

    std::unique_lock<std::mutex> l(lock);
    if (!bindings.contains(sym))
        return false;
    auto& binds = bindings[sym];

    auto& core = Core::getInstance();
    bool mortal_mode = core.getMortalMode();

    for (const auto& bind : binds | std::views::reverse) {
        if (bind.spec.modifiers != modifiers)
            continue;

        if (!bind.spec.focus.empty()) {
            bool matched = false;
            for (const auto& focus : bind.spec.focus) {
                printf("Focus check for: %s", focus.c_str());
                if (Gui::matchFocusString(focus)) {
                    printf("Matched\n");
                    matched = true;
                    break;
                }
            }
            if (!matched)
                continue;
        }

        if (!Core::getInstance().getPluginManager()->CanInvokeHotkey(bind.command, screen))
            continue;

        if (mortal_mode && core.isArmokTool(bind.command))
            continue;

        queued_command = bind.cmdline;
        hotkey_sig = HotkeySignal::CmdReady;
        l.unlock();
        cond.notify_all();
        return true;
    }

    return false;
}

void HotkeyManager::setHotkeyCommand(std::string cmd) {
    std::unique_lock<std::mutex> l(lock);
    queued_command = cmd;
    hotkey_sig = HotkeySignal::CmdReady;
    l.unlock();
    cond.notify_all();
}


void HotkeyManager::handleKeybindingCommand(color_ostream &con, const std::vector<std::string>& parts) {
    if (parts.size() >= 3 && (parts[0] == "set" || parts[0] == "add")) {
        std::string keystr = parts[1];
        if (parts[0] == "set")
            clearKeybind(keystr);
        for (const auto& part : parts | std::views::drop(2) | std::views::reverse) {
            if (!addKeybind(keystr, part)) {
                con.printerr("Invalid key spec: %s\n", keystr.c_str());
                break;
            }
        }
    }
    else if (parts.size() >= 2 && parts[0] == "clear") {
        for (const auto& part : parts | std::views::drop(1)) {
            if (!clearKeybind(part)) {
                con.printerr("Invalid key spec: %s\n", part.c_str());
                break;
            }
        }
    }
    else if (parts.size() == 2 && parts[0] == "list") {
        std::vector<std::string> list = listKeybinds(parts[1]);
        if (list.empty())
            con << "No bindings." << std::endl;
        for (const auto& kb : list)
            con << "  " << kb << std::endl;
    }
    else {
        con << "Usage:" << std::endl
            << "  keybinding list <key>" << std::endl
            << "  keybinding clear <key>[@context]..." << std::endl
            << "  keybinding set <key>[@context] \"cmdline\" \"cmdline\"..." << std::endl
            << "  keybinding add <key>[@context] \"cmdline\" \"cmdline\"..." << std::endl
            << "Later adds, and earlier items within one command have priority." << std::endl
            << "Supported keys: [Ctrl-][Alt-][Shift-](A-Z, 0-9, F1-F12, `, or Enter)." << std::endl
            << "Context may be used to limit the scope of the binding, by" << std::endl
            << "requiring the current context to have a certain prefix." << std::endl
            << "Current UI context is: " << std::endl
            << join_strings("\n", Gui::getCurFocus(true)) << std::endl;
    }
}

HotkeyManager::HotkeyManager() {
    this->hotkey_thread = std::thread(&HotkeyManager::hotkey_thread_fn, this);
}

HotkeyManager::~HotkeyManager() {
    // Set shutdown signal and notify thread
    {
        std::lock_guard<std::mutex> l(lock);
        this->hotkey_sig = HotkeySignal::Shutdown;
    }
    cond.notify_all();

    if (this->hotkey_thread.joinable())
        this->hotkey_thread.join();
}
