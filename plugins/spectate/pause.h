#pragma once
#include <unordered_set>
#include <string>

namespace DFHack {
    ////////////
    // Locking mechanisms for control over pausing
    namespace Pausing
    {
        class Lock
        {
            bool locked = false;
            std::string name;
        public:
            explicit Lock(const char* name) { this->name = name;};
            virtual ~Lock()= default;
            virtual bool isAnyLocked() const = 0;
            virtual bool isOnlyLocked() const = 0;
            bool isLocked() const   { return locked; }
            virtual void lock()     { locked = true; }; //simply locks the lock
            void unlock()           { locked = false; };
        };

        // non-blocking lock resource used in conjunction with the announcement functions in World
        class AnnouncementLock : public Lock
        {
            static std::unordered_set<Lock*> locks;
        public:
            explicit AnnouncementLock(const char* name): Lock(name) { locks.emplace(this); }
            ~AnnouncementLock() override { locks.erase(this); }
            bool captureState(); // captures the state of announcement settings, iff this is the only locked lock (note it does nothing if 0 locks are engaged)
            void lock() override; // locks and attempts to capture state
            bool isAnyLocked() const override; // returns true if any instance of AnnouncementLock is locked
            bool isOnlyLocked() const override; // returns true if locked and no other instance is locked
        };

        // non-blocking lock resource used in conjunction with the Player pause functions in World
        class PlayerLock : public Lock
        {
            static std::unordered_set<Lock*> locks;
        public:
            explicit PlayerLock(const char* name): Lock(name) { locks.emplace(this); }
            ~PlayerLock() override { locks.erase(this); }
            bool isAnyLocked() const override; // returns true if any instance of PlayerLock is locked
            bool isOnlyLocked() const override; // returns true if locked and no other instance is locked
        };
    }
    namespace World {
        bool DisableAnnouncementPausing(); // disable announcement pausing if all locks are open
        bool SaveAnnouncementSettings(); // save current announcement pause settings if all locks are open
        bool RestoreAnnouncementSettings(); // restore saved announcement pause settings if all locks are open

        bool EnablePlayerPausing(); // enable player pausing if all locks are open
        bool DisablePlayerPausing(); // disable player pausing if all locks are open

        void Update();
    }
}