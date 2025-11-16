#include "DFHackVersion.h"
#include <csignal>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <thread>

#include <sys/eventfd.h>
#include <execinfo.h>
#include <unistd.h>

const int BT_ENTRY_MAX = 25;
struct CrashInfo {
    int backtrace_entries = 0;
    void* backtrace[BT_ENTRY_MAX];
    int signal = 0;
};

CrashInfo crash_info;

std::atomic<bool> crashed = false;
std::atomic<bool> crashlog_ready = false;
std::atomic<bool> shutdown = false;

// Use eventfd for async-signal safe waiting
int crashlog_complete = -1;

void flag_set(std::atomic_bool &atom) {
    atom.store(true);
    atom.notify_all();
}
void flag_wait(std::atomic_bool &atom) {
    atom.wait(false);
}

void signal_crashlog_complete() {
    if (crashlog_complete == -1)
        return;
    uint64_t v = 1;
    [[maybe_unused]] auto _ = write(crashlog_complete, &v, sizeof(v));
}

std::thread crashlog_thread;

extern "C" void dfhack_crashlog_handle_signal(int sig) {
    if (shutdown.load() || crashed.exchange(true) || crashlog_ready.load()) {
        // Ensure the signal handler doesn't try to write a crashlog
        // whilst the crashlog thread is unavailable.
        std::quick_exit(1);
    }
    crash_info.signal = sig;
    crash_info.backtrace_entries = backtrace(crash_info.backtrace, BT_ENTRY_MAX);

    // Signal saving of crashlog
    flag_set(crashlog_ready);
    // Wait for completion via eventfd read, if fd isn't valid, bail
    if (crashlog_complete != -1) {
        [[maybe_unused]] uint64_t v;
        [[maybe_unused]] auto _ = read(crashlog_complete, &v, sizeof(v));
    }
    std::quick_exit(1);
}

void dfhack_crashlog_handle_terminate() {
    dfhack_crashlog_handle_signal(0);
}

std::string signal_name(int sig) {
    switch (sig) {
        case SIGINT:
            return "SIGINT";
        case SIGILL:
            return "SIGILL";
        case SIGABRT:
            return "SIGABRT";
        case SIGFPE:
            return "SIGFPE";
        case SIGSEGV:
            return "SIGSEGV";
        case SIGTERM:
            return "SIGTERM";
    }
    return "";
}

std::filesystem::path get_crashlog_path() {
    std::time_t time = std::time(nullptr);
    std::tm* tm = std::localtime(&time);

    std::string timestamp = "unknown";
    if (tm) {
        char stamp[64];
        std::size_t out = strftime(&stamp[0], 63, "%Y-%m-%d-%H-%M-%S", tm);
        if (out != 0)
            timestamp = stamp;
    }

    std::filesystem::path dir = "crashlog";
    std::error_code err;
    std::filesystem::create_directories(dir, err);

    std::filesystem::path log_path = dir / ("crash_" + timestamp + ".txt");
    return log_path;
}

void dfhack_save_crashlog() {
    char** backtrace_strings = backtrace_symbols(crash_info.backtrace, crash_info.backtrace_entries);
    if (!backtrace_strings) {
        // Allocation failed, give up
        return;
    }
    try {
        std::filesystem::path crashlog_path = get_crashlog_path();
        std::ofstream crashlog(crashlog_path);

        crashlog << "Dwarf Fortress Linux has crashed!" << "\n";
        crashlog << "Dwarf Fortress Version " << DFHack::Version::df_version() << "\n";
        crashlog << "DFHack Version " << DFHack::Version::dfhack_version() << "\n\n";

        std::string signal = signal_name(crash_info.signal);
        if (!signal.empty()) {
            crashlog << "Signal " << signal << "\n";
        }

        for (int i = 0; i < crash_info.backtrace_entries; i++) {
            crashlog << i << "> " << backtrace_strings[i] << "\n";
        }
    } catch (...) {}

    free(backtrace_strings);
}

void dfhack_crashlog_thread() {
    // Wait for activation signal
    flag_wait(crashlog_ready);
    if (shutdown.load()) // Shutting down gracefully, end thread.
        return;

    dfhack_save_crashlog();
    signal_crashlog_complete();
    std::quick_exit(1);
}

std::terminate_handler term_handler = nullptr;

const int desired_signals[3] = {SIGSEGV,SIGILL,SIGABRT};
namespace DFHack {
    void dfhack_crashlog_init() {
        // Initialize eventfd flag
        crashlog_complete = eventfd(0, EFD_CLOEXEC);

        crashlog_thread = std::thread(dfhack_crashlog_thread);

        for (int signal : desired_signals) {
            std::signal(signal, dfhack_crashlog_handle_signal);
        }
        term_handler = std::set_terminate(dfhack_crashlog_handle_terminate);

        // https://sourceware.org/glibc/manual/latest/html_mono/libc.html#index-backtrace-1
        // backtrace is AsyncSignal-Unsafe due to dynamic loading of libgcc_s
        // Using it here ensures it is loaded before use in the signal handler.
        [[maybe_unused]] int _ = backtrace(crash_info.backtrace, 1);
    }

    void dfhack_crashlog_shutdown() {
        shutdown.exchange(true);
        for (int signal : desired_signals) {
            std::signal(signal, SIG_DFL);
        }
        std::set_terminate(term_handler);

        // Shutdown the crashlog thread.
        flag_set(crashlog_ready);
        crashlog_thread.join();

        // If the signal handler is somehow running whilst here, let it terminate
        signal_crashlog_complete();
        if (crashlog_complete != -1)
            close(crashlog_complete); // Close fd
        return;
    }
}
