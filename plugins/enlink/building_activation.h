#ifndef UNKNBLCDR_BUILDING_ACTIVATION
#define UNKNBLCDR_BUILDING_ACTIVATION

#include <memory>
#include <utility>


#include "df/building_bars_floorst.h"
#include "df/building_bars_verticalst.h"
#include "df/building_bridgest.h"
#include "df/building_doorst.h"
#include "df/building_floodgatest.h"
#include "df/building_gear_assemblyst.h"
#include "df/building_grate_floorst.h"
#include "df/building_grate_wallst.h"
#include "df/building_hatchst.h"
#include "df/building_trapst.h"
#include "df/building_type.h"
#include "df/building_weaponst.h"
#include "df/trap_type.h"

#include "DataDefs.h"

namespace BuildingActivation
{
    namespace impl
    {
        template <class T>
        inline static void set_value_with_mask(T & buffer, const T& mask, const bool value)
        {
            buffer = (buffer & (~mask)) | (mask * value);
        }
          
        template <class T>
        inline static void toggle_value_with_mask(T & buffer, const T& mask)
        {
            buffer ^= mask;
        }

        template <class T>
        inline static bool get_value_with_mask(T & buffer, const T& mask)
        {
            return (buffer & mask);
        }
    }

    struct basic_activator
    {
        virtual void set(const bool) = 0;
        virtual void toggle() = 0;
        virtual bool get() const = 0;
    };

    struct door_flagged : basic_activator
    {
        df::door_flags *flags;
        static constexpr uint16_t mask = df::door_flags::mask_closed;
        void set (const bool st)
        {
            impl::set_value_with_mask(flags->whole, mask, !st);
        }
        void toggle()
        {
            impl::toggle_value_with_mask(flags->whole, mask);
        }
        bool get() const
        {
            return !impl::get_value_with_mask(flags->whole, mask);
        }
        template <class T>
        door_flagged(T * ptr):
        flags(&(ptr->door_flags))
        {
        }
    };

    struct gate_flagged : basic_activator
    {
        df::gate_flags *flags;
        static constexpr uint16_t mask = df::gate_flags::mask_closed;
        void set (const bool st)
        {
            flags->whole &= ~(df::gate_flags::mask_closing | df::gate_flags::mask_opening);
            impl::set_value_with_mask(flags->whole, mask, !st);
        }
        void toggle()
        {
            flags->whole &= ~(df::gate_flags::mask_closing | df::gate_flags::mask_opening);
            impl::toggle_value_with_mask(flags->whole, mask);
        }
        bool get() const
        {
            return !impl::get_value_with_mask(flags->whole, mask);
        }
        template <class T>
        gate_flagged(T * ptr):
        flags(&(ptr->gate_flags))
        {
        }
    };

    struct gear_flagged : basic_activator
    {
        df::building_gear_assemblyst::T_gear_flags *flags;
        static constexpr uint32_t mask = df::building_gear_assemblyst::T_gear_flags::mask_disengaged;
        void set (const bool st)
        {
            impl::set_value_with_mask(flags->whole, mask, !st);
        }
        void toggle ()
        {
            impl::toggle_value_with_mask(flags->whole, mask);
        }
        bool get() const
        {
            return !impl::get_value_with_mask(flags->whole, mask);
        }
        template <class T>
        gear_flagged(T * ptr):
        flags(&(ptr->gear_flags))
        {
        }
    };

    struct trapstate_flagged : basic_activator
    {
        uint8_t *flag;
        void set (const bool st)
        {
            *flag = st;
        }
        void toggle ()
        {
            *flag = !(*flag);
        }
        bool get() const
        {
            return *flag;
        }
        template <class T>
        trapstate_flagged(T * ptr):
        flag(&(ptr->state))
        {
        }
    };
    
    struct stop_flagged : basic_activator
    {
        df::building_trapst::T_stop_flags *flags;
        static constexpr uint16_t mask = df::building_trapst::T_stop_flags::mask_disabled;
        virtual void set (const bool st)
        {
            flags->whole &= ~(df::building_trapst::T_stop_flags::mask_disabling |
                              df::building_trapst::T_stop_flags::mask_enabling     );
            impl::set_value_with_mask(flags->whole, mask, !st);
        }
        virtual void toggle()
        {
            flags->whole &= ~(df::building_trapst::T_stop_flags::mask_disabling |
                              df::building_trapst::T_stop_flags::mask_enabling     );
            impl::toggle_value_with_mask(flags->whole, mask);
        }
        bool get() const
        {
            return !impl::get_value_with_mask(flags->whole, mask);
        }
        template <class T>
        stop_flagged(T * ptr):
        flags(&(ptr->stop_flags))
        {
        }
    };
    
    template <class TimerT, class Activator>
    //I should implement static_asserts to ensure
    //TimerT is a pointer to an int_something
    //and Activator is derived from basic_activator
    struct timed_activator : Activator
    {
        TimerT *timer;
        void set(const bool st)
        {
            (*timer) = 0;
            Activator::set(st);
        }
        void toggle()
        {
            (*timer) = 0;
            Activator::toggle();
        }
        template <class ... Args>
        timed_activator(TimerT &t_ptr, Args && ... args):
        Activator(std::forward<Args>(args)...), timer(&t_ptr)
        {
        }
    };
    
    inline static std::unique_ptr<basic_activator> get_activator(df::building * b_ptr)
    {
        if (!b_ptr)
        {
            return nullptr;
        }
        using namespace df::enums;
        switch (b_ptr->getType())
        {
          case building_type::Door:
            {
              df::building_doorst *ptr = DFHack::virtual_cast<df::building_doorst>(b_ptr);
              if (ptr) return dts::make_unique<door_flagged>(ptr);
              std::cerr << "d" << std::endl;
            }
          case building_type::Floodgate:
            {
              df::building_floodgatest *ptr = DFHack::virtual_cast<df::building_floodgatest>(b_ptr);
              if (ptr) return dts::make_unique<timed_activator<int8_t, gate_flagged>>(ptr->timer, ptr);
              std::cerr << "s" << std::endl;
            }
          case building_type::Bridge:
            {
              df::building_bridgest *ptr = DFHack::virtual_cast<df::building_bridgest>(b_ptr);
              if (ptr) return dts::make_unique<gate_flagged>(ptr);
              std::cerr << "a" << std::endl;
            }
          case building_type::Weapon:
            {
              df::building_weaponst *ptr = DFHack::virtual_cast<df::building_weaponst>(b_ptr);
              if (ptr) return dts::make_unique<timed_activator<int8_t, gate_flagged>>(ptr->timer, ptr);
              std::cerr << "m" << std::endl;
            }
          case building_type::Hatch:
            {
              df::building_hatchst *ptr = DFHack::virtual_cast<df::building_hatchst>(b_ptr);
              if (ptr) return dts::make_unique<door_flagged>(ptr);
              std::cerr << "n" << std::endl;
            }
          case building_type::GrateWall:
            {
              df::building_grate_wallst *ptr = DFHack::virtual_cast<df::building_grate_wallst>(b_ptr);
              if (ptr) return dts::make_unique<timed_activator<int8_t, gate_flagged>>(ptr->timer, ptr);
              std::cerr << "b" << std::endl;
            }
          case building_type::GrateFloor:
            {
              df::building_grate_floorst *ptr = DFHack::virtual_cast<df::building_grate_floorst>(b_ptr);
              if (ptr) return dts::make_unique<timed_activator<int8_t, gate_flagged>>(ptr->timer, ptr);
              std::cerr << "v" << std::endl;
            }
          case building_type::BarsVertical:
            {
              df::building_bars_verticalst *ptr = DFHack::virtual_cast<df::building_bars_verticalst>(b_ptr);
              if (ptr) return dts::make_unique<timed_activator<int8_t, gate_flagged>>(ptr->timer, ptr);
              std::cerr << "c" << std::endl;
            }
          case building_type::BarsFloor:
            {
              df::building_bars_floorst *ptr = DFHack::virtual_cast<df::building_bars_floorst>(b_ptr);
              if (ptr) return dts::make_unique<timed_activator<int8_t, gate_flagged>>(ptr->timer, ptr);
              std::cerr << "x" << std::endl;
            }
          case building_type::GearAssembly:
            {
              df::building_gear_assemblyst *ptr = DFHack::virtual_cast<df::building_gear_assemblyst>(b_ptr);
              if (ptr) return dts::make_unique<gear_flagged>(ptr);
              std::cerr << "z" << std::endl;
            }
          case building_type::Trap:
            {
              df::building_trapst *ptr = DFHack::virtual_cast<df::building_trapst>(b_ptr);
              if (ptr)
              {
                  switch (ptr->trap_type)
                  {
                    case trap_type::Lever:
                      return dts::make_unique<trapstate_flagged>(ptr);
                    case trap_type::PressurePlate:
                      return dts::make_unique<timed_activator<int16_t, trapstate_flagged>>(ptr->ready_timeout, ptr);
                    case trap_type::WeaponTrap:
                      return dts::make_unique< timed_activator< int16_t, timed_activator< int16_t, trapstate_flagged > > >
                                    (ptr->ready_timeout, ptr->fill_timer, ptr);
                    case trap_type::TrackStop:
                      return dts::make_unique<timed_activator<int8_t, stop_flagged>>(ptr->stop_trigger_timer, ptr);
                    default:
                      std::cerr << "INVALID TRAPTYPE!" << std::endl;
                      return nullptr;
                  }
                }
                std::cerr << "<>" << std::endl;
            }
          default:
            std::cerr << "INVALID TYPE!" << std::endl;
            return nullptr;
        }
    }
    
    inline static bool is_active(df::building * b_ptr)
    {
        std::unique_ptr<basic_activator> ptr = get_activator(b_ptr);
        if (!!ptr)
        {
            return ptr->get();
        }
        return false;
    }

    inline static void set_state(df::building * b_ptr, const bool state)
    {
        std::unique_ptr<basic_activator> ptr = get_activator(b_ptr);
        if (!!ptr)
        {
            ptr->set(state);
        }
    }

    inline static void toggle_state(df::building * b_ptr)
    {
        std::unique_ptr<basic_activator> ptr = get_activator(b_ptr);
        if (!!ptr)
        {
            ptr->toggle();
        }
    }
}

#endif
