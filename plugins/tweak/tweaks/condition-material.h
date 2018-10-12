#include "df/viewscreen_workquota_conditionst.h"

using namespace DFHack;

struct condition_material_hook : df::viewscreen_workquota_conditionst {
    typedef df::viewscreen_workquota_conditionst interpose_base;
    typedef df::viewscreen_workquota_conditionst T_screen;

    struct T_order_mat_data {
        std::vector<std::string*> list_entries;
        std::vector<int16_t> mat_types;
        std::vector<int32_t> mat_indices;
        std::vector<int16_t> list_unk3;
        std::vector<int16_t> list_visible;
    };

    static std::map<df::viewscreen_workquota_conditionst*, T_order_mat_data*> order_mat_data;

    static void register_screen(T_screen *scr)
    {
        if (order_mat_data.find(scr) != order_mat_data.end())
        {
            unregister_screen(scr);
        }
        auto data = new T_order_mat_data;
        data->list_entries = scr->list_entries;
        data->mat_types    = scr->mat_types;
        data->mat_indices  = scr->mat_indices;
        data->list_unk3    = scr->list_unk3;
        data->list_visible = scr->list_visible;
        order_mat_data[scr] = data;
    }

    static void unregister_screen(T_screen *scr)
    {
        if (order_mat_data.find(scr) != order_mat_data.end() && order_mat_data[scr])
        {
            T_order_mat_data *data = order_mat_data[scr];
            scr->list_entries = data->list_entries;
            scr->mat_types    = data->mat_types;
            scr->mat_indices  = data->mat_indices;
            scr->list_unk3    = data->list_unk3;
            scr->list_visible = data->list_visible;
            delete data;
            order_mat_data.erase(scr);
        }
    }

    void apply_filter()
    {
        if (order_mat_data.find(this) != order_mat_data.end() && order_mat_data[this])
        {
            list_idx = 0;
            T_order_mat_data *data = order_mat_data[this];
            // keep the first item ("no material") around, because attempts to delete it
            // result in it still being displayed first, regardless of list_entries[0]
            list_entries.resize(1);
            mat_types.resize(1);
            mat_indices.resize(1);
            list_unk3.resize(1);
            list_visible.resize(1);
            // skip "no material" here
            for (size_t i = 1; i < data->list_entries.size(); i++)
            {
                // cap it at 32767 elements to be safe
                if (list_entries.size() >= INT16_MAX)
                {
                    break;
                }
                std::string *s = data->list_entries[i];
                if (s->find(filter) != std::string::npos)
                {
                    list_entries.push_back(data->list_entries[i]);
                    mat_types.push_back(data->mat_types[i]);
                    mat_indices.push_back(data->mat_indices[i]);
                    list_unk3.push_back(data->list_unk3[i]);
                    // this should be small enough to fit in an int16_t
                    list_visible.push_back(int16_t(list_entries.size() - 1));
                }
            }
        }
    }

    DEFINE_VMETHOD_INTERPOSE(void, feed, (std::set<df::interface_key> *input))
    {
        using namespace df::enums::interface_key;
        if (mode == T_mode::Material)
        {
            for (auto key : *input)
            {
                if (key == LEAVESCREEN || key == SELECT)
                {
                    INTERPOSE_NEXT(feed)(input);
                    unregister_screen(this);
                    return;
                }
                else if (key == STANDARDSCROLL_UP || key == STANDARDSCROLL_DOWN ||
                         key == STANDARDSCROLL_PAGEUP || key == STANDARDSCROLL_PAGEDOWN)
                {
                    INTERPOSE_NEXT(feed)(input);
                }
                int ch = Screen::keyToChar(key);
                if (ch != -1)
                {
                    if (ch == 0)
                    {
                        if (!filter.empty())
                        {
                            filter.erase(filter.size() - 1);
                        }
                    }
                    else
                    {
                        filter += tolower(char(ch));
                    }
                    apply_filter();
                }
            }
        }
        else
        {
            INTERPOSE_NEXT(feed)(input);
            if (mode == T_mode::Material)
            {
                register_screen(this);
                apply_filter();
            }
        }
    }
};

std::map<df::viewscreen_workquota_conditionst*, condition_material_hook::T_order_mat_data*> condition_material_hook::order_mat_data;

IMPLEMENT_VMETHOD_INTERPOSE(condition_material_hook, feed);
