#include "modules/Hotkey.h"

#include <ranges>
#include <SDL_keycode.h>

#include "Core.h"
#include "ColorText.h"
#include "MiscUtils.h"
#include "PluginManager.h"

#include "modules/DFSDL.h"
#include "modules/Gui.h"

#include "df/global_objects.h"
#include "df/viewscreen.h"
#include "df/interfacest.h"


using namespace DFHack;
using Hotkey::KeySpec;
using Hotkey::KeyBinding;

enum HotkeySignal {
    None = 0,
    CmdReady,
    Shutdown,
};

bool operator==(const KeySpec& a, const KeySpec& b) {
    return a.modifiers == b.modifiers && a.sym == b.sym &&
        a.focus.size() == b.focus.size() &&
        std::equal(a.focus.begin(), a.focus.end(), b.focus.begin());
}

// Equality operator for key bindings
bool operator==(const KeyBinding& a, const KeyBinding& b) {
    return a.spec == b.spec &&
        a.command == b.command &&
        a.cmdline == b.cmdline;
}

std::string KeySpec::toString(bool include_focus) const {
    std::string out;
    if (modifiers & DFH_MOD_CTRL) out += "Ctrl-";
    if (modifiers & DFH_MOD_ALT) out += "Alt-";
    if (modifiers & DFH_MOD_SUPER) out += "Super-";
    if (modifiers & DFH_MOD_SHIFT) out += "Shift-";

    std::string key_name;
    if (this->sym < 0) {
        key_name = "MOUSE" + std::to_string(-this->sym);
    } else {
        key_name = DFSDL::DFSDL_GetKeyName(this->sym);
    }
    out += key_name;

    if (include_focus && !this->focus.empty()) {
        out += "@";
        bool first = true;
        for (const auto& fc : this->focus) {
            if (first) {
                first = false;
                out += fc;
            } else {
                out += "|" + fc;
            }
        }
    }

    return out;
}

std::optional<KeySpec> KeySpec::parse(std::string spec, std::string* err) {
    KeySpec out;

    // Determine focus string, if present
    size_t focus_idx = spec.find('@');
    if (focus_idx != std::string::npos) {
        split_string(&out.focus, spec.substr(focus_idx + 1), "|");
        spec.erase(focus_idx);
    }

    // Treat remaining keyspec as lowercase for case-insensitivity.
    std::transform(spec.begin(), spec.end(), spec.begin(), tolower);

    // Determine modifier flags
    auto match_modifier = [&out, &spec](std::string_view prefix, int mod) {
        bool found = spec.starts_with(prefix);
        if (found) {
            out.modifiers |= mod;
            spec.erase(0, prefix.size());
        }
        return found;
    };
    while (match_modifier("shift-", DFH_MOD_SHIFT)
            || match_modifier("ctrl-", DFH_MOD_CTRL)
            || match_modifier("alt-", DFH_MOD_ALT)
            || match_modifier("super-", DFH_MOD_SUPER)) {}

    out.sym = DFSDL::DFSDL_GetKeyFromName(spec.c_str());
    if (out.sym != SDLK_UNKNOWN)
        return out;

    // Attempt to parse as a mouse binding
    if (spec.starts_with("mouse")) {
        spec.erase(0, 5);
        // Read button number, ensuring between 4 and 15 inclusive
        try {
            int mbutton = std::stoi(spec);
            if (mbutton >= 4 && mbutton <= 15) {
                out.sym = -mbutton;
                return out;
            }
        } catch (...) {
            // If integer parsing fails, it isn't valid
        }
        if (err)
            *err = "Invalid mouse button '" + spec + "', only 4-15 are valid";
        return std::nullopt;
    }

    if (err)
        *err = "Unknown key '" + spec + "'";

    // Invalid key binding
    return std::nullopt;
}

bool KeySpec::isDisruptive() const {
    // SDLK enum uses the actual characters for a key as its value.
    // Escaped values included are Return, Escape, Backspace, and Tab
    const std::string essential_key_set = "\r\x1B\b\t -=[]\\;',./";

    // Letters A-Z, 0-9, and other special keys such as return/escape, and other general typing keys
    bool is_essential_key = (this->sym >= SDLK_a && this->sym <= SDLK_z) // A-Z
        || (this->sym >= SDLK_0 && this->sym <= SDLK_9) // 0-9
        || essential_key_set.find(this->sym) != std::string::npos
        || (this->sym >= SDLK_LEFT && this->sym <= SDLK_UP); // Arrow keys

    // Essential keys are safe, so long as they have a modifier that isn't Shift
    if (is_essential_key && !(this->modifiers & ~DFH_MOD_SHIFT))
        return true;

    return false;
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
    std::optional<KeySpec> spec_opt = KeySpec::parse(keyspec);
    if (!spec_opt.has_value())
        return false;
    return this->addKeybind(spec_opt.value(), cmd);
}

bool HotkeyManager::removeKeybind(const KeySpec& spec, bool match_focus, std::string_view cmdline) {
    std::lock_guard<std::mutex> l(lock);
    if (!bindings.contains(spec.sym))
        return false;
    auto& binds = bindings[spec.sym];

    auto new_end = std::remove_if(binds.begin(), binds.end(), [match_focus, spec, &cmdline](const auto& v) {
            return v.spec.sym == spec.sym
                && v.spec.modifiers == spec.modifiers
                && (!match_focus || v.spec.focus == spec.focus)
                && (cmdline.empty() || v.cmdline == cmdline);
    });
    if (new_end == binds.end())
        return false; // No bindings removed

    binds.erase(new_end, binds.end());
    return true;
}

bool HotkeyManager::removeKeybind(std::string keyspec, bool match_focus, std::string_view cmdline) {
    std::optional<KeySpec> spec_opt = KeySpec::parse(keyspec);
    if (!spec_opt.has_value())
        return false;
    return this->removeKeybind(spec_opt.value(), match_focus, cmdline);
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
    std::lock_guard<std::mutex> l(lock);
    std::optional<KeySpec> spec_opt = KeySpec::parse(keyspec);
    if (!spec_opt.has_value())
        return {};
    return this->listKeybinds(spec_opt.value());
}

std::vector<KeyBinding> HotkeyManager::listActiveKeybinds() {
    std::lock_guard<std::mutex> l(lock);
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

std::vector<KeyBinding> HotkeyManager::listAllKeybinds() {
    std::lock_guard<std::mutex> l(lock);
    std::vector<KeyBinding> out;

    for (const auto& [_, bind_set] : bindings) {
        for (const auto& bind : bind_set) {
            out.emplace_back(bind);
        }
    }
    return out;
}

bool HotkeyManager::handleKeybind(int sym, int modifiers) {
    // Ensure gamestate is ready
    if (!df::global::gview || !df::global::plotinfo)
        return false;

    // Get topmost active screen
    df::viewscreen *screen = &df::global::gview->view;
    while (screen->child)
        screen = screen->child;

    // Map keypad return to return
    if (sym == SDLK_KP_ENTER)
        sym = SDLK_RETURN;

    std::unique_lock<std::mutex> l(lock);

    // If reading input for a keybinding screen, save the input and exit early
    if (keybind_save_requested) {
        KeySpec spec;
        spec.sym = sym;
        spec.modifiers = modifiers;
        requested_keybind = spec.toString(false);
        keybind_save_requested = false;
        return true;
    }

    if (!bindings.contains(sym))
        return false;
    auto& binds = bindings[sym];

    auto& core = Core::getInstance();
    bool mortal_mode = core.getMortalMode();

    // Iterate in reverse, prioritizing the last added keybinds
    for (const auto& bind : binds | std::views::reverse) {
        if (bind.spec.modifiers != modifiers)
            continue;

        if (!bind.spec.focus.empty()) {
            bool matched = false;
            for (const auto& focus : bind.spec.focus) {
                if (Gui::matchFocusString(focus)) {
                    matched = true;
                    break;
                }
            }
            if (!matched)
                continue;
        }

        if (!core.getPluginManager()->CanInvokeHotkey(bind.command, screen))
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

void HotkeyManager::requestKeybindingInput(bool cancel) {
    std::lock_guard<std::mutex> l(lock);
    keybind_save_requested = !cancel;
    requested_keybind.clear();
}

std::string HotkeyManager::getKeybindingInput() {
    std::lock_guard<std::mutex> l(lock);
    return requested_keybind;
}

void HotkeyManager::handleKeybindingCommand(color_ostream &con, const std::vector<std::string>& parts) {
    std::string parse_error;
    if (parts.size() >= 3 && (parts[0] == "set" || parts[0] == "add")) {
        std::string keystr = parts[1];
        if (parts[0] == "set")
            removeKeybind(keystr);
        for (const auto& part : parts | std::views::drop(2) | std::views::reverse) {
            auto spec = KeySpec::parse(keystr, &parse_error);
            if (!spec.has_value()) {
                con.printerr("%s\n", parse_error.c_str());
                break;
            }
            if (!addKeybind(spec.value(), part)) {
                con.printerr("Invalid command: '%s'\n", part.c_str());
                break;
            }
        }
    }
    else if (parts.size() >= 2 && parts[0] == "clear") {
        for (const auto& part : parts | std::views::drop(1)) {
            auto spec = KeySpec::parse(part, &parse_error);
            if (!spec.has_value()) {
                con.printerr("%s\n", parse_error.c_str());
            }
            if (!removeKeybind(spec.value())) {
                con.printerr("No matching keybinds to remove\n");
                break;
            }
        }
    }
    else if (parts.size() == 2 && parts[0] == "list") {
        auto spec = KeySpec::parse(parts[1], &parse_error);
        if (!spec.has_value()) {
            con.printerr("%s\n", parse_error.c_str());
            return;
        }
        std::vector<std::string> list = listKeybinds(spec.value());
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
            << "Key format: [Ctrl-][Alt-][Super-][Shift-](A-Z, 0-9, F1-F12, `, etc.)." << std::endl
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
