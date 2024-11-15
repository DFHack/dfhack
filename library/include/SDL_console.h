#ifndef SDL_CONSOLE
#define SDL_CONSOLE

#include <string>
#include <memory>
#include <atomic>
#include <optional>

struct SDL_Color;
union SDL_Event;

namespace sdl_console {

struct SDLConsole_pshare;
struct SDLConsole_impl;

class SDLConsole {
public:
    /*
     * Constructs the console window and its widgets.
     * Returns true on success, otherwise false on failure.
     */
    bool init();
    void reset();
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
    SDLConsole& set_prompt(std::string string);

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

    class State {
    public:
        enum Value {
            active,     // console is active and operational.
            inactive,   // console is inactive and can be activated.
            shutdown    // console is shutting down.
        };

        State() : current_state(Value::inactive) {}

        [[nodiscard]] bool is_active() const { return current_state.load() == Value::active; }

        [[nodiscard]] bool is_inactive() const { return current_state.load() == Value::inactive; }

        [[nodiscard]] bool is_shutdown() const { return current_state.load() == Value::shutdown; }

    protected:
        friend class SDLConsole;
        friend class SDLConsole_impl;
        void set_state(Value new_state) {
            current_state.store(new_state);
        }
        std::atomic<Value> current_state;
    };

    State state;

    SDLConsole(const SDLConsole&) = delete;
    SDLConsole& operator=(const SDLConsole&) = delete;

    SDLConsole(SDLConsole&&) = delete;
    SDLConsole& operator=(SDLConsole&&) = delete;


protected:
    friend class SDLConsole_impl;
    std::unique_ptr<SDLConsole_pshare> pshare;

private:
    void write_line_(std::string& line, std::optional<SDL_Color> color);
    template<typename F>
    void push_api_task(F&& func);
    std::shared_ptr<SDLConsole_impl> impl;

    SDLConsole();
    ~SDLConsole();
};

} // namespace sdl_console

#endif
