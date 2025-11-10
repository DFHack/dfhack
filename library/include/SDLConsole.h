#ifndef SDL_CONSOLE
#define SDL_CONSOLE

#include <string>
#include <memory>
#include <atomic>
#include <optional>
#include <span>

#include <string_view>
#include <variant>
#include <unordered_map>
#include <mutex>
#include <thread>

#include <functional>

#include "Export.h"

struct SDL_Color;
union SDL_Event;

namespace DFHack {
namespace sdl_console {

class SDLConsole_session;

struct Rect {
    int x {};
    int y {};
    int w {};
    int h {};
};

/*
 * Stores configuration for components.
 */
class Property {
public:
    template <typename T>
    struct Key {
        std::string_view name;
    };

    using Value = std::variant<std::string, std::u32string, int,  Rect>;
    template <typename T>
    void set(Key<T> key, const T& value) {
        std::scoped_lock l(m_);
        props_[std::string(key.name)] = value;
    }

    template <typename T>
    std::optional<T> get(Key<T> key) {
        std::scoped_lock l(m_);
        auto it = props_.find(std::string(key.name));
        if (it == props_.end()) { return std::nullopt; }
        if (auto p = std::get_if<T>(&it->second)) { return *p; }
        return std::nullopt;
    }

private:
    std::unordered_map<std::string, Value> props_;
    std::recursive_mutex m_;
};

class DFHACK_EXPORT SDLConsole {
public:
    /*
     * Constructs a console session and its window.
     * Returns true on success, otherwise false on failure.
     * Not thread safe.
     *
     * SDL events and video subsystems must be initialized
     * before this function is called.
     */
    [[nodiscard]] bool init_session();

    /*
     * Requests the console to shutdown.
     * Safe to call any time from any thread.
     */
    void shutdown_session() noexcept;

    /*
     * Performs the actual shutdown.
     * Cleans up any resources associated with the console session.
     * Not thread safe. Must be called from the thread that called init_session().
     */
    bool destroy_session() noexcept;

    /*
     * Prints a colored line of text to the output area.
     * Safe to call any time from any thread.
     */
    void write_line(std::string_view line, SDL_Color color);

    /*
     * Prints a line of text to the output area with the default color (white).
     * Safe to call any time from any thread.
     */
    void write_line(std::string_view line);

    /*
     * Shows the console window.
     * Safe to call any time from any thread.
     */
    void show_window();

    /*
     * Hides the console window.
     * Safe to call any time from any thread.
     */
    void hide_window();

    /*
     * Clears the output history.
     * Safe to call any time from any thread.
     */
    void clear();

    /*
     * Sets the prompt text.
     * Safe to call any time from any thread.
     */
    SDLConsole& set_prompt(std::string_view text);

    /*
     * Gets the prompt text.
     * Safe to call any time from any thread.
     */
    std::string get_prompt();

    /*
     * Sets the prompt input.
     * Safe to call any time from any thread.
     */
    void set_prompt_input(const std::string& text);

    /*
     * Restores previously saved prompt input.
     * Safe to call any time from any thread.
     */
    void restore_prompt();

    /*
     * Saves current prompt input.
     * Safe to call any time from any thread.
     */
    void save_prompt();

    /*
     * Configures the dimensions for the console window.
     * Default values are: width=640, height=480
     * x = 0 (SDL chooses), y = 0 (SDL chooses)
     *
     * Must be called before init_session() to take effect.
     * Safe to call any time from any thread.
     */
    SDLConsole& set_mainwindow_create_rect(int width, int height, int x = 0, int y = 0);

    /*
     * Maximum number of lines the console stores in its output buffer.
     * When the number of lines hits the scrollback limit,
     * old lines will be removed as new ones are added.
     *
     * Safe to call any time from any thread.
     */
    SDLConsole& set_scrollback(int scrollback);

    /*
     * Number of characters that fit horizontally in the output area.
     *
     * Safe to call any time from any thread.
     */
    [[nodiscard]] int get_columns();
    /*
     * Number of characters that fit vertically in the output area.
     *
     * Safe to call any time from any thread.
     */
    [[nodiscard]] int get_rows();

    /*
     * Copies a new line into the provided string.
     * Returns the number of characters read, or -1 if console is shutting down.
     *
     * Safe to call any time from any thread.
     */
    [[nodiscard]] int get_line(std::string& buf);

    /*
     * Sets the prompt's command history
     *
     * Safe to call any time from any thread.
     */
    void set_command_history(std::span<const std::string> entries);

    /*
     * Retrieves an instance of the console.
     * Currently supports only one instance.
     *
     * Safe to call any time from any thread.
     */
    static SDLConsole& get_console() noexcept;

    /*
     * Returns true if a console session is active.
     */
    [[nodiscard]] bool is_active();

    /*
     * True if console was shutdown for any reason.
     * Reasons include:
     * 1. init_session() failed.
     * 2. commanded to shutdown via shutdown_session()
     */
    bool was_shutdown()
    {
        return was_shutdown_.load();
    }


    /*
     * Handle SDL events. Returns true if handled, otherwise false.
     *
     * Not thread safe. Must be called from the thread that called init_session().
     */
    [[nodiscard]] bool sdl_event_hook(const SDL_Event& e);

    /*
     * Render, handle various tasks.
     *
     * Not thread safe. Must be called from the thread that called init_session().
     */
    void update();

    // Not currently used.
    void set_destroy_session_callback(std::function<void()> cb) {
        on_destroy_session = std::move(cb);
    }

    Property props;

    SDLConsole(const SDLConsole&) = delete;
    SDLConsole& operator=(const SDLConsole&) = delete;

    SDLConsole(SDLConsole&&) = delete;
    SDLConsole& operator=(SDLConsole&&) = delete;


protected:
    friend class SDLConsole_session;

private:
    void write_line_(std::string_view line, std::optional<SDL_Color> color);
    template<typename F>
    void push_api_task(F&& func);
    std::shared_ptr<SDLConsole_session> impl;
    std::weak_ptr<SDLConsole_session> impl_weak;
    std::thread::id init_thread_id;
    std::atomic<bool> was_shutdown_ { false };

    /*
     * Callback for when the session is just about to be destroyed.
     */
    std::function<void()> on_destroy_session{ nullptr };

    SDLConsole();
    ~SDLConsole();
};

} // namespace sdl_console
} // namespace DFHack
#endif
