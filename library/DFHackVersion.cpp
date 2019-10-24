#define NO_DFHACK_VERSION_MACROS
#include "DFHackVersion.h"
#include "git-describe.h"
namespace DFHack {
    namespace Version {
        int dfhack_abi_version()
        {
            return DFHACK_ABI_VERSION;
        }
        const char *dfhack_version()
        {
            return DFHACK_VERSION;
        }
        const char *df_version()
        {
            return DF_VERSION;
        }
        const char *dfhack_release()
        {
            return DFHACK_RELEASE;
        }
        const char *dfhack_build_id()
        {
            return DFHACK_BUILD_ID;
        }
        const char *git_description()
        {
            return DFHACK_GIT_DESCRIPTION;
        }
        const char *git_commit()
        {
            return DFHACK_GIT_COMMIT;
        }
        const char *git_xml_commit()
        {
            return DFHACK_GIT_XML_COMMIT;
        }
        const char *git_xml_expected_commit()
        {
            return DFHACK_GIT_XML_EXPECTED_COMMIT;
        }
        const char *git_xml_description()
        {
            return DFHACK_GIT_XML_DESCRIPTION;
        }

        bool git_xml_match()
        {
        #ifdef DFHACK_GIT_XML_MATCH
            return true;
        #else
            return false;
        #endif
        }

        bool is_release()
        {
        #ifdef DFHACK_GIT_TAGGED
            return true;
        #else
            return false;
        #endif
        }

        bool is_prerelease()
        {
        #ifdef DFHACK_PRERELEASE
            return true;
        #else
            return false;
        #endif
        }
    }
}
