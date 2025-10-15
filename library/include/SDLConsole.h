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
#include <cstdint>
#include <thread>

#include "Export.h"

struct SDL_Rect;
struct SDL_Color;
union SDL_Event;

namespace sdl_console {

class SDLConsole_impl;

/*
 * Stores configuration for components.
 */


struct Rect {
    int x{};
    int y{};
    int w{};
    int h{};
};

class Property {
public:
    template <typename T>
    struct Key {
        std::string_view name;
    };

    using Value = std::variant<std::string, std::u32string, int64_t, int, std::size_t, Rect>;
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
     * Constructs the console window and its widgets.
     * Returns true on success, otherwise false on failure.
     */
    bool init();
    /*
     * Commands the console to shutdown. Safe to call from any thread.
     */
    void shutdown();
    /*
     * Cleans up resources. Must be called from the thread that initialized SDL.
     */
    bool destroy();

    /*
     * Prints a colored line of text to the output area.
     */
    void write_line(std::string line, SDL_Color color);
    /*
     * Prints a line of text to the output area with the default color (white).
     */
    void write_line(std::string line);

    /*
     * Shows the console window.
     */
    void show_window();
    /*
     * Hides the console window.
     */
    void hide_window();

    /*
     * Clears the output area.
     */
    void clear();

    /*
     * Sets the prompt text.
     */
    SDLConsole& set_prompt(const std::string& text);

    std::string get_prompt();

    void set_prompt_input(const std::string& text);
    void restore_prompt();
    void save_prompt();

    /*
     * Configures the dimensions for the console window.
     * Default values are: width=640, height=480
     * x = 0 (SDL chooses), y = 0 (SDL chooses)
     */
    SDLConsole& set_mainwindow_create_rect(int width, int height, int x = 0, int y = 0);

    /*
     * Maximum number of lines the console stores in its output buffer.
     * When the number of lines hits the scrollback limit,
     * old lines will be removed as new ones are added.
     */
    SDLConsole& set_scrollback(int lines);

    /*
     * Number of characters that fit horizontally in the output area.
     */
    [[nodiscard]] int get_columns();
    /*
     * Number of characters that fit vertically in the output area.
     */
    [[nodiscard]] int get_rows();

    /*
     * Copies a new line into the provided string object.
     * Returns the number of characters read, or -1 if the console is closing.
     */
    [[nodiscard]] int get_line(std::string& buf);

    void set_command_history(std::span<const std::string> entries);

    /*
     * Retrieves an instance of the console.
     * Currently supports only one instance.
     */
    static SDLConsole& get_console();

    /*
     * Handle SDL events. Returns true if handled, otherwise false.
     * Must be called from the thread that initialized SDL.
     */
    [[nodiscard]] bool sdl_event_hook(SDL_Event& e);

    /*
     * Render, handle various tasks.
     * Must be called from the thread that initialized SDL.
     */
    void update();

    class RunState {
    public:
        enum States {
            active,     // console is active and operational.
            inactive,   // console is inactive and can be activated.
            shutdown,   // console is shutting down.
        };

        RunState() : current_state(States::inactive) {}

        [[nodiscard]] bool is_active() const { return current_state.load() == States::active; }

        [[nodiscard]] bool is_inactive() const { return current_state.load() == States::inactive; }

        [[nodiscard]] bool is_shutdown() const { return current_state.load() == States::shutdown; }

    protected:
        friend class SDLConsole;
        friend class SDLConsole_impl;
        void set_state(States new_state) {
            current_state.store(new_state);
        }
        void reset() {
            set_state(States::inactive);
        }
        std::atomic<States> current_state;
    };

    RunState run_state;
    Property props;

    SDLConsole(const SDLConsole&) = delete;
    SDLConsole& operator=(const SDLConsole&) = delete;

    SDLConsole(SDLConsole&&) = delete;
    SDLConsole& operator=(SDLConsole&&) = delete;


protected:
    friend class SDLConsole_impl;

private:
    void write_line_(std::string& line, std::optional<SDL_Color> color);
    void reset();
    template<typename F>
    void push_api_task(F&& func);
    std::shared_ptr<SDLConsole_impl> impl;
    std::weak_ptr<SDLConsole_impl> impl_weak;
    std::thread::id init_thread_id;

    SDLConsole();
    ~SDLConsole();
};

} // namespace sdl_console

#endif
