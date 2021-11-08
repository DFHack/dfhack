#ifndef UNKNBLCDR_LOKTHUULS
#define UNKNBLCDR_LOKTHUULS	

#include<algorithm>
#include<utility>

#include "df/global_objects.h"
#include "df/ui.h"
#include "df/world.h"
#include "df/world_data.h"
#include "df/world_site.h"


namespace LocTools
{

    inline int32_t x_loc2glob(const int32_t lx)
    {
        return lx + df::global::world->map.region_x * 48;
    }

    inline int32_t y_loc2glob(const int32_t ly)
    {
        return ly + df::global::world->map.region_y * 48;
    }

    inline int32_t z_loc2glob(const int32_t lz)
    {
        return lz + df::global::world->map.region_z;
    }

    inline int32_t x_glob2loc(const int32_t gx)
    {
        return gx - df::global::world->map.region_x * 48;
        return gx - df::global::world->map.region_x * 48;
    }

    inline int32_t y_glob2loc(const int32_t gy)
    {
        return gy - df::global::world->map.region_y * 48;
    }

    inline int32_t z_glob2loc(const int32_t gz)
    {
        return gz - df::global::world->map.region_z;
    }

    inline void loc2glob(int32_t &gx, int32_t &gy, int32_t &gz)
    {
        gx = x_loc2glob(gx);
        gy = y_loc2glob(gy);
        gz = z_loc2glob(gz);
    }

    inline void glob2loc(int32_t &lx, int32_t &ly, int32_t &lz)
    {
        lx = x_loc2glob(lx);
        ly = y_loc2glob(ly);
        lz = z_loc2glob(lz);
    }


    inline void get_possibly_visible_sites(std::vector<int32_t> &v)
    {
        v.clear();
        if (DFHack::World::isFortressMode())
        {
            v.push_back(df::global::ui->site_id);
        }
        std::vector<std::pair<uint32_t, int32_t>> dlist;

        const int32_t x = df::global::world->map.region_x, y = df::global::world->map.region_y,
                      lo_x = x - 1, lo_y = x - 1, hi_x = x + 1, hi_y = y + 1;

        for (const auto & site : df::global::world->world_data->sites)
        {
            const int32_t site_lo_x = site->global_min_x, site_lo_y = site->global_min_y,
                          site_hi_x = site->global_max_x, site_hi_y = site->global_max_y,
                          center_x = (site_lo_x + site_hi_x) / 2,
                          center_y = (site_lo_y + site_hi_y) / 2;

            if (std::max(site_lo_x, lo_x) <= std::min(site_hi_x, hi_x) &&
                std::max(site_lo_y, lo_y) <= std::min(site_hi_y, hi_y)    )
            {
                const uint32_t deltax = (x - center_x), deltay = (y - center_y);
                const uint32_t dist =  deltax * deltax + deltay * deltay;
                dlist.push_back(std::pair<uint32_t, int32_t>(dist, site->id));
            }
        }

        std::sort(dlist.begin(), dlist.end());

        v.reserve(dlist.size());

        for (const auto& p : dlist)
        {
            v.push_back(p.second);
        }
    }

    inline int32_t get_current_site()
    {
        if (DFHack::World::isFortressMode())
        {
            return df::global::ui->site_id;
        }
        else if (DFHack::World::isAdventureMode())
        {
            std::vector<int32_t> v;
            get_possibly_visible_sites(v);
            if (v.size() > 0)
            {
                return v[0];
            }
        }
        return 0;
    }

}

#endif
