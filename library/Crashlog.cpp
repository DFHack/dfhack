#include "DFHackVersion.h"
#include <csignal>
#include <thread>
#include <filesystem>
#include <fstream>

#include <execinfo.h>

const int BT_ENTRY_MAX = 25;
struct CrashInfo {
    int backtrace_entries = 0;
    void* backtrace[BT_ENTRY_MAX];
    int signal = 0;
};

CrashInfo crash_info;

/*
 * As of c++17 the only safe stdc++ methods are plain lock-free atomic methods
 * This sadly means that using std::semaphore *could* cause issues according to the standard.
 */
std::atomic_bool crashed = false;
std::atomic_bool crashlog_ready = false;
std::atomic_bool crashlog_complete = false;

void flag_set(std::atomic_bool &atom) {
    atom.store(true);
    atom.notify_all();
}
void flag_wait(std::atomic_bool &atom) {
    atom.wait(false);
}

std::thread crashlog_thread;
bool shutdown = false;

extern "C" void dfhack_crashlog_handle_signal(int sig) {
    if (crashed.exchange(true)) {
        // Crashlog already produced, bail thread.
        std::quick_exit(1);
    }
    crash_info.signal = sig;
    crash_info.backtrace_entries = backtrace(crash_info.backtrace, BT_ENTRY_MAX);
    
    // Signal saving of crashlog and wait for completion
    flag_set(crashlog_ready);
    flag_wait(crashlog_complete);
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

void dfhack_save_crashlog() {
    char** backtrace_strings = backtrace_symbols(crash_info.backtrace, crash_info.backtrace_entries);
    if (!backtrace_strings) {
        // Allocation failed, give up
        return;
    }
    std::filesystem::path crashlog_path = "./crash.txt";
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

    free(backtrace_strings);
}

void dfhack_crashlog_thread() {
    // Wait for activation signal
    flag_wait(crashlog_ready);
    if (shutdown) // Shutting down gracefully, end thread.
        return;

    dfhack_save_crashlog();

    flag_set(crashlog_complete);
    std::quick_exit(1);
}

const int desired_signals[3] = {SIGSEGV,SIGILL,SIGABRT};
namespace DFHack {
    void dfhack_crashlog_init() {
        for (int signal : desired_signals) {
            std::signal(signal, dfhack_crashlog_handle_signal);
        }
        std::set_terminate(dfhack_crashlog_handle_terminate);

        // https://sourceware.org/glibc/manual/latest/html_mono/libc.html#index-backtrace-1
        // backtrace is AsyncSignal-Unsafe due to dynamic loading of libgcc_s
        // Using it here ensures it is loaded before use in the signal handler.
        int _ = backtrace(crash_info.backtrace, 1);

        crashlog_thread = std::thread(dfhack_crashlog_thread);
    }

    void dfhack_crashlog_shutdown() {
        shutdown = true;
        flag_set(crashlog_ready);
        crashlog_thread.join();
        return;
    }
}
