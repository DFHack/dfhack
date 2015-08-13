#pragma once
namespace DFHack {
    namespace Version {
        const char *dfhack_version();
        const char *df_version();
        const char *dfhack_release();
        const char *git_description();
        const char *git_commit();
    }
}

#ifndef NO_DFHACK_VERSION_MACROS
#define DF_VERSION DFHack::Version::df_version()
#define DFHACK_RELEASE DFHack::Version::dfhack_release()
#define DFHACK_VERSION DFHack::Version::dfhack_version()
#define DFHACK_GIT_DESCRIPTION DFHack::Version::git_description()
#define DFHACK_GIT_COMMIT DFHack::Version::git_commit()
#endif
