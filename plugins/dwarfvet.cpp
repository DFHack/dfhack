/**
 * Copyright (c) 2015, Michael Casadevall
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 **/

#include "Console.h"
#include "Core.h"
#include "DataDefs.h"
#include "Export.h"
#include "PluginManager.h"
#include "modules/EventManager.h"
#include "modules/Units.h"
#include "modules/Buildings.h"
#include "modules/Maps.h"
#include "modules/Job.h"

#include "df/animal_training_level.h"
#include "df/building_type.h"
#include "df/caste_raw.h"
#include "df/caste_raw_flags.h"
#include "df/creature_raw.h"
#include "df/job.h"
#include "df/general_ref_unit_workerst.h"
#include "df/profession.h"
#include "df/ui.h"
#include "df/unit.h"
#include "df/unit_health_info.h"
#include "df/unit_health_flags.h"
#include "df/world.h"

#include <map>
#include <vector>

using namespace DFHack;
using namespace DFHack::Units;
using namespace DFHack::Buildings;

using namespace std;

DFHACK_PLUGIN("dwarfvet");
DFHACK_PLUGIN_IS_ENABLED(dwarfvet_enabled);

REQUIRE_GLOBAL(ui);
REQUIRE_GLOBAL(world);

static vector<int32_t> tracked_units;
static int32_t howOften = 100;

struct hospital_spot {
    int32_t x;
    int32_t y;
    int32_t z;
};

class Patient {
  public:
    // Constructor/Deconstrctor
    Patient(int32_t id, int spot_index, int32_t x, int32_t y, int32_t z);
    int32_t getID() { return this->id; };
    int32_t getSpotIndex() { return this->spot_index; };
    int32_t returnX() { return this->spot_in_hospital.x; };
    int32_t returnY() { return this->spot_in_hospital.y; };
    int32_t returnZ() { return this->spot_in_hospital.z; };

  private:
    struct hospital_spot spot_in_hospital;
    int id;
    int spot_index;
};

Patient::Patient(int32_t id, int32_t spot_index, int32_t x, int32_t y, int32_t z){
    this->id = id;
    this->spot_in_hospital.x = x;
    this->spot_in_hospital.y = y;
    this->spot_in_hospital.z = z;
}

class AnimalHospital {

  public:
    // Constructor
    AnimalHospital(df::building *, color_ostream &out);
    ~AnimalHospital();
    int32_t getID() { return id; }
    bool acceptPatient(int32_t id, color_ostream&);
    void processPatients(color_ostream &out);
    void dischargePatient(Patient * patient, color_ostream &out);
    void calculateHospital(bool force, color_ostream &out);
    void reportUsage(color_ostream &out);

    // GC
    bool to_be_deleted;

  private:
    int spots_open;
    int32_t id;
    int32_t x1;
    int32_t x2;
    int32_t y1;
    int32_t y2;
    int32_t z;
    int height;
    int length;

    // Doing an actual array in C++ is *annoying*, bloody copy constructors */
    vector<bool> spots_in_use;
    vector<int32_t> building_in_hospital_notification; /* If present, we already notified about this */
    vector<Patient*> accepted_patients;
};

AnimalHospital::AnimalHospital(df::building * building, color_ostream &out) {
    // Copy in what we need to know
    id = building->id;
    x1 = building->x1;
    x2 = building->x2;
    y1 = building->y1;
    y2 = building->y2;
    z = building->z;

    // Determine how many spots we have for animals
    this->length = x2-x1+1;
    this->height = y2-y1+1;

    // And calculate the hospital!
    this->calculateHospital(true, out);
}

AnimalHospital::~AnimalHospital() {
    // Go through and delete all the patients
    for (vector<Patient*>::iterator accepted_patient = this->accepted_patients.begin(); accepted_patient != this->accepted_patients.end(); accepted_patient++) {
        delete (*accepted_patient);
   }
}

bool AnimalHospital::acceptPatient(int32_t id, color_ostream &out) {
    // This function determines if we can accept a patient, and if we will.
    this->calculateHospital(true, out);

    // First, do we have room?
    if (!spots_open) return false;

    // Yup, let's find the next open spot,
    // and give it to our patient
    int spot_cur = 0; // fuck the STL for requiring a second counter to make this usable
    for (vector<bool>::iterator spot = this->spots_in_use.begin(); spot != this->spots_in_use.end(); spot++) {
        if (*spot == false) {
            *spot = true;
            break;
        }
        spot_cur++;
    }

    spots_open--;

    // Convert the spot into x/y/z cords.
    int offset_y = spot_cur/length;
    int offset_x =  spot_cur%length;

    // Create the patient!
    Patient * patient = new Patient(id,
        spot_cur,
        this->x1+offset_x,
        this->y1+offset_y,
        this->z
    );

    accepted_patients.push_back(patient);
    return true;
}

// Before any use of the hospital, we need to make calculate open spots
// and such. This can change (i.e. stuff built in hospital) and
// such so it should be called on each function.
void AnimalHospital::reportUsage(color_ostream &out) {
    // Debugging tool to see parts of the hospital in use
    int length_cursor = this->length;

    for (vector<bool>::iterator spot = this->spots_in_use.begin(); spot != this->spots_in_use.end(); spot++) {
        if (*spot) out.print("t");
        if (!(*spot)) out.print("f");
        length_cursor--;
        if (length_cursor < 0) {
            out.print("\n");
            length_cursor = this->length;
        }
    }
    out.print("\n");

}

void AnimalHospital::calculateHospital(bool force, color_ostream &out) {
    // Only calculate out the hospital if we actually have a patient in it
    // (acceptPatient will forcibly rerun this to make sure everything OK

    // Should reduce FPS impact of each calculation tick when the hospitals
    // are not in use
    //if (!force || (spots_open == length*height)) {
        // Hospital is idle, don't recalculate
    //    return;
    //}

    // Calculate out the total area of the hospital
    // This can change if a hospital has been resized
    this->spots_open = length*height;
    this->spots_in_use.assign(this->spots_open, false);

    // The spots_in_use maps one to one with a spot
    // starting at the upper-left hand corner, then
    // across, then down. i.e.
    //
    // given hospital zone:
    //
    // UU
    // uU
    //
    // where U is in use, and u isn't, the array
    // would be t,t,f,t

    // Walk the building array and see what stuff is in the hospital,
    // then walk the patient array and remark those spots as used.

    // If a patient is in an invalid spot, reassign it
    for (df::building *building : world->buildings.all) {

        // Check that we're not comparing ourselves;
        if (building->id == this->id) {
            continue;
        }

        // Check if the building is on our z level, if it isn't
        // then it can't overlap the hospital (until Toady implements
        // multi-z buildings
        if (building->z != this->z) {
            continue;
        }

        // DF defines activity zones multiple times in the building structure
        // If axises agree with each other, we're looking at a reflection of
        // ourselves
        if (building->x1 == this->x1 &&
            building->x2 == this->x2 &&
            building->y1 == this->y1 &&
            building->y2 == this->y2) {
            continue;
        }

        // Check for X/Y overlap
        // I can't believe I had to look this up -_-;
        // http://stackoverflow.com/questions/306316/determine-if-two-rectangles-overlap-each-other
        if ((this->x1 > building->x2 ||
              building->x1 > this->x2 ||
              this->y1 > building->y2 ||
              building->y1 > this->y2)) {
            continue;
        }

        // Crap, building overlaps, we need to figure out where it is in the hospital
        // NOTE: under some conditions, this generates a false warning. Not a lot I can do about it

        // Mark spots used by that building as used; FIXME: handle special logic for traction benches and such
        int building_offset_x = building->x1 - this->x1;
        int building_offset_y = building->y1 - this->y1;
        int building_length = building->x2 - building->x1 + 1;
        int building_height = building->y2 - building->y1 + 1;

        // Cap the used calculation to only include the part in the hospital
        if (this->x1 > building->x1) {
            building_offset_x -= (this->x1 - building->x1);
        }

        if (this->y1 > building->y1) {
            building_offset_y -= (building->y1 - this->y1);
        }

        if ((this->x2 < building->x2) && building_offset_x) {
            building_length -= (this->x2 - building->x2) + 1;
        }

        if ((this->y2 < building->y2) && building_offset_y) {
            building_height = (building->y2 - this->y2) + 1;
        }

        // Quick explination, if a building is north or east of the activity zone,
        // we get a negative offset, we'll just skip those lines below. If its
        // south or west, we make the building length/height lower to compinsate.

        /* if we have a negative x offset, we correct that */
        if (building_offset_x < 0) {
            building_height += building_offset_x;
            building_offset_x = 0;
        }

        /* Handle negative y offset */
        if (building_offset_y < 0) {
            building_length += building_offset_y;
            building_offset_y = 0;
        };

        /* Advance the pointer to first row we need to mark */
        int spot_cur = 0;
        if (building_offset_y) {
            spot_cur = (length+1) * building_offset_y;
        }

        spot_cur += building_offset_x;
        /* Start marking! */
        for (int i = 0; i < building_height; i++) {
            for (int j = 0; j < building_length; j++) {
                spots_in_use[spot_cur+j] = true;
            }

            // Wind the cursor to the start of the next row
            spot_cur += length+1;
        }

        // *phew*, done. Now repeat the process for the next building!
    }

}

// Self explanatory
void AnimalHospital::dischargePatient(Patient * patient, color_ostream &out) {
    int32_t id = patient->getID();

    // Remove them from the hospital

    // We can safely iterate here because once we delete the unit
    // we no longer use the iterator
    for (vector<Patient*>::iterator accepted_patient = this->accepted_patients.begin(); accepted_patient != this->accepted_patients.end(); accepted_patient++) {
        if ( (*accepted_patient)->getID() == id) {
            out.print("Discharging unit %d from hospital %d\n", id, this->id);
            // Reclaim their spot
            spots_in_use[(*accepted_patient)->getSpotIndex()] = false;
            this->spots_open++;
            delete (*accepted_patient);
            this->accepted_patients.erase(accepted_patient);
            break;
        }
    }

    // And the master list
    for (vector<int32_t>::iterator it = tracked_units.begin(); it != tracked_units.end(); it++) {
        if ((*it) == id) {
            tracked_units.erase(it);
            break;
        }
    }

    return;
}

void AnimalHospital::processPatients(color_ostream &out) {
    // Where the magic happens
    for (vector<Patient*>::iterator patient = this->accepted_patients.begin(); patient != this->accepted_patients.end(); patient++) {
        int id = (*patient)->getID();
        df::unit * real_unit = nullptr;
        // Appears the health bits can get freed/realloced too -_-;, Find the unit from the main
        // index and check it there.
        auto units = world->units.all;

        for ( size_t a = 0; a < units.size(); a++ ) {
            real_unit = units[a];
            if (real_unit->id == id) {
                break;
            }
        }

        // Check to make sure the unit hasn't expired before assigning a job, or if they've been healed
        if (!real_unit || !Units::isActive(real_unit) || !real_unit->health->flags.bits.needs_healthcare) {
            // discharge the patient from the hospital
            this->dischargePatient(*patient, out);
            return;
        }

        // Give the unit a job if they don't have any
        if (!real_unit->job.current_job) {
            // Create REST struct
            df::job * job = new df::job;
            DFHack::Job::linkIntoWorld(job);

            job->pos.x = (*patient)->returnX();
            job->pos.y = (*patient)->returnY();
            job->pos.z = (*patient)->returnZ();
            job->flags.bits.special = 1;
            job->job_type = df::enums::job_type::Rest;
            df::general_ref *ref = df::allocate<df::general_ref_unit_workerst>();
            ref->setID(real_unit->id);
            job->general_refs.push_back(ref);
            real_unit->job.current_job = job;
            job->wait_timer = 1600;
            out.print("Telling intelligent unit %d to report to the hospital!\n", real_unit->id);
        }
    }
}


static vector<AnimalHospital*> animal_hospital_zones;

void delete_animal_hospital_vector(color_ostream &out) {
    if (dwarfvet_enabled) {
        out.print("Clearing all animal hospitals\n");
    }
    for (vector<AnimalHospital*>::iterator animal_hospital = animal_hospital_zones.begin(); animal_hospital != animal_hospital_zones.end(); animal_hospital++) {
        delete (*animal_hospital);
    }
    animal_hospital_zones.clear();
}

command_result dwarfvet(color_ostream &out, std::vector <std::string> & parameters);

DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{
    commands.push_back(PluginCommand(
        "dwarfvet",
        "Allows animals to be cared for in animal hospitals (activity zones that are animal training + hospital combined).",
        dwarfvet,
        false, //allow non-interactive use
        "dwarfvet enable\n"
        " enables animals to use animal hospitals (requires dwarf with Animal Caretaker labor enabled)\n"
        "dwarfvet report\n"
        " displays all zones dwarfvet considers animal hospitals and their current location on the map\n"
    ));
    return CR_OK;
}

DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    return CR_OK;
}

bool isActiveAnimalHospital(df::building * building) {
    if (Buildings::isHospital(building) && Buildings::isAnimalTraining(building) && Buildings::isActive(building)) {
        return true;
    }

    return false;
}

bool compareAnimalHospitalZones(df::building * hospital1, df::building * hospital2) {
    // We compare hospitals by checking if positions are identical, not by ID
    // since activity zones can easily be changed in size

    if ( hospital1->x1 == hospital2->x1 &&
         hospital1->x2 == hospital2->x2 &&
         hospital1->y1 == hospital2->y1 &&
         hospital1->y2 == hospital2->y2 &&
         hospital1->z == hospital2->z) {
        return true;
    }

    return false;
}

void tickHandler(color_ostream& out, void* data) {
    if ( !dwarfvet_enabled )
        return;
    CoreSuspender suspend;
    int32_t own_race_id = df::global::ui->race_id;
    int32_t own_civ_id = df::global::ui->civ_id;
    auto units = world->units.all;

    /**
     * Generate a list of animal hospitals on the map
     *
     * Since activity zones can change any instant given user interaction
     * we need to be constantly on the lookout for changed zones, and update
     * our cached list on the fly if necessary.
     **/

    vector<df::building*> hospitals_on_map;

    // Because C++ iterators suck, we're going to build a temporary vector with the AHZ, and then
    // copy it for my own bloody sanity (and compilance with the STL spec)
    vector<AnimalHospital*> ahz_scratch;

    // Holding area for things to be added to the scratch
    vector<df::building*> to_be_added;


    // Walk the building tree, and generate a list of animal hospitals on the map
    for (size_t b =0 ; b < world->buildings.all.size(); b++) {
        df::building* building = world->buildings.all[b];
        if (isActiveAnimalHospital(building)) {
            hospitals_on_map.push_back(building);
        }
    }

    int count_of_hospitals = hospitals_on_map.size();
    int hospitals_cached = animal_hospital_zones.size();
    //out.print ("count_of_Hospitals: %d, hospitals_cached: %d\n", count_of_hospitals, hospitals_cached);
    // It's possible our hospital cache is empty, if so, simply copy it, and jump to the main logic
    if (!hospitals_cached && count_of_hospitals) {
        out.print("Populating hospital cache:\n");
        for (df::building *current_hospital : hospitals_on_map) {
            AnimalHospital * hospital = new AnimalHospital(current_hospital, out);
            out.print("  Found animal hospital %d  at x1: %d, y1: %d, z: %d from valid hospital list\n",
                        hospital->getID(),
                        current_hospital->x1,
                        current_hospital->y1,
                        current_hospital->z
            );
            animal_hospital_zones.push_back(hospital);
        }

        goto processUnits;
    }

    if (!count_of_hospitals && !hospitals_cached) {
        // No hospitals found, delete any cache, and return
        delete_animal_hospital_vector(out);
        out.print("No hospitals found, plugin sleeping ...\n");
        goto cleanup;
    }

    // Now walk our list of known hospitals, do a bit of checking, then compare
    // TODO: this doesn't handle zone resizes at all

    for (vector<AnimalHospital*>::iterator animal_hospital = animal_hospital_zones.begin(); animal_hospital != animal_hospital_zones.end(); animal_hospital++) {
        // If a zone is changed at all, DF seems to reallocate it.
        //
        // Each AnimalHospital has a "to_be_deleted" bool. We're going to set that to true, and clear it if we can't
        // find a matching hospital. This limits the number of times we need to walk through the AHZ list to twice, and
        // lets us cleanly report it later
        //
        // Surviving hospitals will be copied to scratch which will become the new AHZ vector

        (*animal_hospital)->to_be_deleted = true;
        for (vector<df::building*>::iterator current_hospital = hospitals_on_map.begin(); current_hospital != hospitals_on_map.end(); current_hospital++) {

            /* Keep the hospital if its still valid */
            if ((*animal_hospital)->getID() == (*current_hospital)->id) {
                ahz_scratch.push_back(*animal_hospital);
                (*animal_hospital)->to_be_deleted = false;
                break;
            }

        }
    }

    // Report what we're deleting by checking the to_be_deleted bool.
    //
    // Whatsever left is added to the pending add list
    for (vector<AnimalHospital*>::iterator animal_hospital = animal_hospital_zones.begin(); animal_hospital != animal_hospital_zones.end(); animal_hospital++) {
        if ((*animal_hospital)->to_be_deleted) {
            out.print("Hospital #%d removed\n", (*animal_hospital)->getID());
            delete *animal_hospital;
        }
    }

    /* Now we need to walk the scratch and add anything that is a hospital and wasn't in the vector */

    for (vector<df::building*>::iterator current_hospital = hospitals_on_map.begin(); current_hospital != hospitals_on_map.end(); current_hospital++) {
        bool new_hospital = true;

        for (vector<AnimalHospital*>::iterator animal_hospital = ahz_scratch.begin(); animal_hospital != ahz_scratch.end(); animal_hospital++) {
            if ((*animal_hospital)->getID() == (*current_hospital)->id) {
                // Next if we're already here
                new_hospital = false;
                break;
            }
        }

        // Add it if its new
        if (new_hospital == true) to_be_added.push_back(*current_hospital);
    }

    /* Now add it to the scratch AHZ */
    for (vector<df::building*>::iterator current_hospital = to_be_added.begin(); current_hospital != to_be_added.end(); current_hospital++) {
        // Add it to the vector
        out.print("Adding new hospital #id: %d at x1 %d y1: %d z: %d\n",
                (*current_hospital)->id,
                (*current_hospital)->x1,
                (*current_hospital)->y1,
                (*current_hospital)->z
                );
        AnimalHospital * hospital = new AnimalHospital(*current_hospital, out);
        ahz_scratch.push_back(hospital);
    }

    /* Copy the scratch to the AHZ */
    animal_hospital_zones = ahz_scratch;

    // We always recheck the cache instead of counts because someone might have removed then added a hospital
/*    if (hospitals_cached != count_of_hospitals) {
        out.print("Hospitals on the map changed, rebuilding cache\n");

        for (vector<df::building*>::iterator current_hospital = hospitals_on_map.begin(); current_hospital != hospitals_on_map.end(); current_hospital++) {
            bool add_hospital = true;

            for (vector<AnimalHospital*>::iterator map_hospital = animal_hospital_zones.begin(); map_hospital != animal_hospital_zones.end(); map_hospital++) {
                if (compareAnimalHospitalZones(*map_hospital, *current_hospital)) {
                    // Same hospital, we're good
                    add_hospital = false;
                    break;
                }
            }

            // Add it to the list
            if (add_hospital) {
                 out.print("Adding zone at x1: %d, y1: %d to valid hospital list\n", (*current_hospital)->x1, (*current_hospital)->y1);
                animal_hospital_zones.push_back(*current_hospital);
            }
        }
    }
*/
processUnits:
    /* Code borrowed from petcapRemover.cpp */
    for ( size_t a = 0; a < units.size(); a++ ) {
        df::unit* unit = units[a];

        /* As hilarious as it would be, lets not treat FB :) */
        if ( !Units::isActive(unit) || unit->flags1.bits.active_invader || unit->flags2.bits.underworld || unit->flags2.bits.visitor_uninvited || unit->flags2.bits.visitor ) {
            continue;
       }

        if ( !Units::isTamable(unit)) {
            continue;
       }

      /**
       * So, for a unit to be elligable for the hospital, all the following must be true
       *
       * 1. It must be a member of our civilization
       * 2. It must be tame (semi-wild counts for this)
       * 2.1 If its not a dwarf, AND untame clear its civ out so traps work
       * 3. It must have a health struct (which is generated by combat)
       * 4. health->needs_healthcare must be set to true
       * 5. If health->requires_recovery is set, the creature can't move under its own power
       *    and a Recover Wounded or Pen/Pasture job MUST be created by hand - TODO
       * 6. An open spot in the "Animal Hospital" (activity zone with hospital+animal training set)
       *     must be available
       *
       * I apologize if this excessively verbose, but the healthcare system is stupidly conplex
       * and there's tons of edgecases to watch out for, and I want someone else to ACTUALLY
       * beside me able to understand what's going on
       */

        // 1. Make sure its our own civ
        if (!Units::isOwnCiv(unit)) {
            continue;
        }

        // 2. Check for tameness
        if (unit->training_level == df::animal_training_level::WildUntamed) {
            // We don't IMMEDIATELY continue here, if the unit is
            // part of our civ, it indiciates it WAS tamed, and reverted
            // from SemiWild. Clear its civ flag so it looses TRAPAVOID
            //
            // Unfortunately, dwarves (or whatever is CIV_SELECTABLE)
            // also have a default taming level of WildUntamed so
            // check for this case
            //
            // Furthermore, it MIGHT be a werebeast, so check THAT too
            // and exclude those as well.
            //
            // Finally, this breaks makeown. I might need to write a patch
            // to set the tameness of "makeowned" units so dwarfvet can notice
            // it

            if (unit->race == own_race_id ||  unit->enemy.normal_race == own_race_id) {
                continue;
            } else {
                unit->civ_id = -1;
                out.print ("Clearing civ on unit: %d", unit->id);
            }
        }

        // 3. Check for health struct
        if (!unit->health) {
            // Unit has not been injured ever; health struct MIA
            continue;
        }

        // 4. Check the healthcare flags
        if (unit->health->flags.bits.needs_healthcare) {
            /**
             * So, for dwarves to care for a unit it must be resting in
             * in a hospital zone. Since non-dwarves never take jobs
             * this why animal healthcare doesn't work for animals despite
             * animal caretaker being coded in DF itself
             *
             * How a unit gets there is dependent on several factors. If
             * a unit can move under its own power, it will take the rest
             * job, with a position of a bed in the hospital, then move
             * into that bed and fall asleep. This triggers a doctor to
             * treat the unit.
             *
             * If a unit *can't* move, it will set needs_recovery, which
             * creates a "Recover Wounded" job in the job list, and then
             * create the "Rest" job as listed above. Another dwarf with
             * the right labors will go recover the unit, then the above
             * logic kicks off.
             *
             * The necessary flags seem to be properly set for all units
             * on the map, so in theory, we just need to make the jobs and
             * we're in business, but from a realism POV, I don't think
             * non-sentient animals would be smart enough to go to the
             * hospital on their own, so instead, we're going to do the following
             *
             * If a unit CAN_THINK, and can move let it act like a dwarf,
             * it will try and find an open spot in the hospital, and if so,
             * go there to be treated. In vanilla, the only tamable animal
             * with CAN_THINK are Gremlins, so this is actually an edge case
             * but its the easiest to code.
             *
             * TODO: figure out exact logic for non-thinking critters.
             */

            // Now we need to find if this unit can be accepted at a hospital
            bool awareOfUnit = false;
            for (vector<int32_t>::iterator it = tracked_units.begin(); it != tracked_units.end(); it++) {
                if ((*it) == unit->id) {
                    awareOfUnit = true;
                }
            }
            // New unit for dwarfvet to be aware of!
            if (!awareOfUnit) {
                // The master list handles all patients which are accepted
                // Check if this is a unit we're already aware of

                for (auto animal_hospital : animal_hospital_zones) {
                    if (animal_hospital->acceptPatient(unit->id, out)) {
                        out.print("Accepted patient %d at hospital %d\n", unit->id, animal_hospital->getID());
                        tracked_units.push_back(unit->id);
                        break;
                    }
                }
            }
        }
    }

    // The final step, process all patients!
    for (vector<AnimalHospital*>::iterator animal_hospital = animal_hospital_zones.begin(); animal_hospital != animal_hospital_zones.end(); animal_hospital++) {
        (*animal_hospital)->calculateHospital(true, out);
        (*animal_hospital)->processPatients(out);
    }

cleanup:
    EventManager::unregisterAll(plugin_self);
    EventManager::EventHandler handle(tickHandler, howOften);
    EventManager::registerTick(handle, howOften, plugin_self);
}

command_result dwarfvet (color_ostream &out, std::vector <std::string> & parameters)
{
    CoreSuspender suspend;

    for ( size_t a = 0; a < parameters.size(); a++ ) {
        if ( parameters[a] == "enable" ) {
            out.print("dwarfvet enabled!\n");
            dwarfvet_enabled = true;
        }
        if ( parameters[a] == "disable") {
            out.print("dwarvet disabled!\n");
            dwarfvet_enabled = false;
        }
        if ( parameters[a] == "report") {
            out.print("Current animal hospitals are:\n");
            for (size_t b =0 ; b < world->buildings.all.size(); b++) {
                df::building* building = world->buildings.all[b];
                if (isActiveAnimalHospital(building)) {
                    out.print("  at x1: %d, x2: %d, y1: %d, y2: %d, z: %d\n", building->x1, building->x2, building->y1, building->y2, building->z);
                }
            }
            return CR_OK;
        }
        if ( parameters[a] == "report-usage") {
            out.print("Current animal hospitals are:\n");
            for (vector<AnimalHospital*>::iterator animal_hospital = animal_hospital_zones.begin(); animal_hospital != animal_hospital_zones.end(); animal_hospital++) {
                (*animal_hospital)->calculateHospital(true, out);
                (*animal_hospital)->reportUsage(out);
            }
            return CR_OK;
        }
    }

    if ( !dwarfvet_enabled ) {
        return CR_OK;
    }

    EventManager::unregisterAll(plugin_self);
    EventManager::EventHandler handle(tickHandler, howOften);
    EventManager::registerTick(handle, howOften, plugin_self);

    return CR_OK;
}

DFhackCExport command_result plugin_enable(color_ostream &out, bool enable)
{
    if (enable && !dwarfvet_enabled) {
        dwarfvet_enabled = true;
    }
    else if (!enable && dwarfvet_enabled) {
        delete_animal_hospital_vector(out);
        dwarfvet_enabled = false;
    }

    return CR_OK;
}

DFhackCExport command_result plugin_onstatechange(color_ostream &out, state_change_event event)
{
    switch (event)
    {
    case DFHack::SC_MAP_LOADED:
        break;
    case DFHack::SC_MAP_UNLOADED:
        delete_animal_hospital_vector(out);
        dwarfvet_enabled = false;
        break;
    default:
        break;
    }
    return CR_OK;
}
