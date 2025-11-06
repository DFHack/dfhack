#include "DFHackVersion.h"
#include <csignal>
#include <thread>
#include <semaphore>
#include <filesystem>
#include <fstream>

#include <execinfo.h>

const int BT_ENTRY_MAX = 25;
int bt_entries = 0;
void* bt[BT_ENTRY_MAX];
int crash_signal = 0;

std::binary_semaphore crashlog_ready{0};
std::binary_semaphore crashlog_complete{0};

std::thread crashlog_thread;
volatile bool shutdown = false;

extern "C" void dfhack_crashlog_handle_signal(int sig) {
    crash_signal = sig;
    bt_entries = backtrace(bt, BT_ENTRY_MAX);
    
    // Signal saving of crashlog and wait for completion
    crashlog_ready.release();
    crashlog_complete.acquire();
    std::quick_exit(1);
}

void dfhack_save_crashlog() {
    char** backtrace_strings = backtrace_symbols(bt, bt_entries);
    if (!backtrace_strings) {
        // Something has gone terribly wrong
        return;
    }
    std::filesystem::path crashlog_path = "./crash.txt";
    std::ofstream crashlog(crashlog_path);

    crashlog << "Dwarf Fortress has crashed!" << "\n";
    crashlog << "DwarfFortress Version " << DFHack::Version::df_version() << "\n";
    crashlog << "DFHack Version " << DFHack::Version::dfhack_version() << "\n\n";

    for (int i = 0; i < bt_entries; i++) {
        crashlog << i << "> " << backtrace_strings[i] << "\n";
    }

    free(backtrace_strings);
}

void dfhack_crashlog_thread() {
    // Wait for crash or shutdown signal
    crashlog_ready.acquire();
    if (shutdown)
        return;

    dfhack_save_crashlog();
    crashlog_complete.release();
    std::quick_exit(1);
}

const int desired_signals[3] = {SIGSEGV,SIGILL,SIGABRT};
namespace DFHack {
void dfhack_crashlog_init() {
    for (int signal : desired_signals) {
        std::signal(signal, dfhack_crashlog_handle_signal);
    }

    // Ensure the library is initialized to avoid AsyncSignal-Unsafe init during crash
    int _ = backtrace(bt, 1);

    crashlog_thread = std::thread(dfhack_crashlog_thread);
}

void dfhack_crashlog_shutdown() {
    shutdown = true;
    crashlog_ready.release();
    crashlog_thread.join();
    return;
}
}
