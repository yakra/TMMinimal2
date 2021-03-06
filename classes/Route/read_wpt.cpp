#define debug
#ifdef debug
  #include "../TravelerList/TravelerList.h"
  #define COND if (system->systemname == "usaca" || system->systemname == "usaal" || system->systemname == "usafl")
  #define DEBUG(WHAT) WHAT
  #define   LOCK TravelerList::mtx.lock() /* repurpose a mutex not doing anything */
  #define UNLOCK TravelerList::mtx.unlock() /* else ATM for locking terminal */
#else
  #define DEBUG(WHAT)
#endif

#include "Route.h"
#include "../Args/Args.h"
#include "../Datacheck/Datacheck.h"
#include "../ErrorList/ErrorList.h"
#include "../HighwaySegment/HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Waypoint/Waypoint.h"
#include <cstring>
#include <fstream>
#include <unordered_set>

void Route::read_wpt(unsigned int threadnum, ErrorList *el, bool usa_flag)
{	/* read data into the Route's waypoint list from a .wpt file */
	//cout << "read_wpt on " << str() << endl;
	std::string filename = Args::highwaydatapath + "/hwy_data" + "/" + rg_str + "/" + system->systemname + "/" + root + ".wpt";
	// remove full path from all_wpt_files list
	awf_mtx.lock();
	all_wpt_files.erase(filename);
	awf_mtx.unlock();
	std::vector<char*> lines;
	std::ifstream file(filename);
	if (!file)
	{	el->add_error("[Errno 2] No such file or directory: '" + filename + '\'');
		file.close();
		return;
	}
	file.seekg(0, std::ios::end);
	unsigned long wptdatasize = file.tellg();
	file.seekg(0, std::ios::beg);
	char *wptdata = new char[wptdatasize+1];
	file.read(wptdata, wptdatasize);
	wptdata[wptdatasize] = 0; // add null terminator
	file.close();

	DEBUG(COND{LOCK; std::cout << "ReadWptThread " << threadnum << ' ' << root << " slurped" << std::endl; UNLOCK;})

	// split file into lines
	size_t spn = 0;
	for (char* c = wptdata; *c; c += spn)
	{	for (spn = strcspn(c, "\n\r"); c[spn] == '\n' || c[spn] == '\r'; spn++) c[spn] = 0;
		lines.emplace_back(c);
	}

	lines.push_back(wptdata+wptdatasize+1); // add a dummy "past-the-end" element to make lines[l+1]-2 work
	// set to be used for finding duplicate coordinates
	std::unordered_set<Waypoint*> coords_used;
	double vis_dist = 0;
	Waypoint *last_visible = 0;
	char fstr[112];

	DEBUG(COND{LOCK; std::cout << "ReadWptThread " << threadnum << ' ' << root << " split into lines" << std::endl; UNLOCK;})

	for (unsigned int l = 0; l < lines.size()-1; l++)
	{	DEBUG(COND{LOCK; std::cout << "ReadWptThread " << threadnum << "   " << lines[l] << std::endl; UNLOCK;})
		// strip whitespace
		while (lines[l][0] == ' ' || lines[l][0] == '\t') lines[l]++;
		char * endchar = lines[l+1]-2; // -2 skips over the 0 inserted while splitting wptdata into lines
		while (*endchar == 0) endchar--;  // skip back more for CRLF cases, and lines followed by blank lines
		while (*endchar == ' ' || *endchar == '\t')
		{	*endchar = 0;
			endchar--;
		}
		DEBUG(COND{LOCK; std::cout << "ReadWptThread " << threadnum << "     whitespace stripped" << std::endl; UNLOCK;})
		if (lines[l][0] == 0) continue;
		Waypoint *w = new Waypoint(lines[l], this);
			      // deleted on termination of program, or immediately below if invalid
		DEBUG(COND{LOCK; std::cout << "ReadWptThread " << threadnum << "     new Waypoint" << std::endl; UNLOCK;})
		bool malformed_url = w->lat == 0 && w->lng == 0;
		bool label_too_long = w->label_too_long();
		DEBUG(COND{LOCK; std::cout << "ReadWptThread " << threadnum << "     label_too_long() returned" << std::endl; UNLOCK;})
		if (malformed_url || label_too_long)
		{	delete w;
			continue;
		}
		point_list.push_back(w);
		DEBUG(COND{LOCK; std::cout << "ReadWptThread " << threadnum << "     " << w->str() << " point_list.push_back(w)" << std::endl; UNLOCK;})

		// single-point Datachecks, and HighwaySegment
		w->out_of_bounds(fstr);
		w->duplicate_coords(coords_used, fstr);
		w->label_invalid_char();
		if (point_list.size() > 1)
		{	w->distance_update(fstr, vis_dist, point_list[point_list.size()-2]);
			// add HighwaySegment, if not first point
			segment_list.push_back(new HighwaySegment(point_list[point_list.size()-2], w, this));
					       // deleted on termination of program
		}
		DEBUG(COND{LOCK; std::cout << "ReadWptThread " << threadnum << "     new HighwaySegment" << std::endl; UNLOCK;})
		// checks for visible points
		if (!w->is_hidden)
		{	const char *slash = strchr(w->label.data(), '/');
			if (usa_flag && w->label.size() >= 2)
			{	w->bus_with_i();
				w->interstate_no_hyphen();
				w->us_letter();
			}
			w->label_invalid_ends();
			w->label_looks_hidden();
			w->label_parens();
			w->label_selfref(slash);
			w->label_slashes(slash);
			w->lacks_generic();
			w->underscore_datachecks(slash);
			w->visible_distance(fstr, vis_dist, last_visible);
		}
		DEBUG(COND{LOCK; std::cout << "ReadWptThread " << threadnum << "     checks for visible points OK" << std::endl; UNLOCK;})
	}
	DEBUG(COND{LOCK; std::cout << "ReadWptThread " << threadnum << " delete[] " << std::flush; UNLOCK;})
	delete[] wptdata;
	DEBUG(COND{LOCK; std::cout << "wptdata;" << std::endl; UNLOCK;})

	// per-route datachecks
	if (point_list.size() < 2) el->add_error("Route contains fewer than 2 points: " + str());
	else {	// look for hidden termini
		if (point_list.front()->is_hidden)	Datacheck::add(this, point_list.front()->label, "", "", "HIDDEN_TERMINUS", "");
		if (point_list.back()->is_hidden)	Datacheck::add(this, point_list.back()->label, "", "", "HIDDEN_TERMINUS", "");

		// angle check is easier with a traditional for loop and array indices
		for (unsigned int i = 1; i < point_list.size()-1; i++)
		{	//cout << "computing angle for " << point_list[i-1].str() << ' ' << point_list[i].str() << ' ' << point_list[i+1].str() << endl;
			if (point_list[i-1]->same_coords(point_list[i]) || point_list[i+1]->same_coords(point_list[i]))
				Datacheck::add(this, point_list[i-1]->label, point_list[i]->label, point_list[i+1]->label, "BAD_ANGLE", "");
			else {	double angle = point_list[i]->angle(point_list[i-1], point_list[i+1]);
				if (angle > 135)
				{	sprintf(fstr, "%.2f", angle);
					Datacheck::add(this, point_list[i-1]->label, point_list[i]->label, point_list[i+1]->label, "SHARP_ANGLE", fstr);
				}
			     }
		}
	     }
	DEBUG(LOCK;) // repurpose a mutex not doing anything ATM for locking terminal
	std::cout << '.' << std::flush;
	DEBUG(UNLOCK;)
	//std::cout << str() << std::flush;
	//print_route();
}
