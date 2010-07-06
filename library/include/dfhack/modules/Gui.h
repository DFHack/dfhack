#ifndef CL_MOD_GUI
#define CL_MOD_GUI

/*
* Gui: Query the DF's GUI state
*/
#include "dfhack/DFExport.h"
#include "dfhack/DFModule.h"

namespace DFHack
{
    class DFContextShared;
    struct t_viewscreen;
    class DFHACK_EXPORT Gui: public Module
    {
        public:
        
        Gui(DFHack::DFContextShared * d);
        ~Gui();
        bool Start();
        bool Finish();
        
        ///true if paused, false if not
        bool ReadPauseState(); 
        /// read the DF menu view state (stock screen, unit screen, other screens
        bool ReadViewScreen(t_viewscreen &);
        /// read the DF menu state (designation menu ect)
        uint32_t ReadMenuState();
        
        private:
        struct Private;
        Private *d;
    };
}
#endif

