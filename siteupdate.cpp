// Tab Width = 8

// Travel Mapping Project, Jim Teresco and Eric Bryant, 2015-2021
/* Code to read .csv and .wpt files and prepare for
adding to the Travel Mapping Project database.

(c) 2015-2021, Jim Teresco and Eric Bryant
Original Python version by Jim Teresco, with contributions from Eric Bryant and the TravelMapping team
C++ translation by Eric Bryant

This module defines classes to represent the contents of a
.csv file that lists the highways within a system, and a
.wpt file that lists the waypoints for a given highway.
*/

#include <cstring>
#include <dirent.h>
#include <thread>
#include "classes/Args/Args.h"
#include "classes/DBFieldLength/DBFieldLength.h"
#include "classes/ConnectedRoute/ConnectedRoute.h"
#include "classes/Datacheck/Datacheck.h"
#include "classes/ElapsedTime/ElapsedTime.h"
#include "classes/ErrorList/ErrorList.h"
#include "classes/HighwaySegment/HighwaySegment.h"
#include "classes/HighwaySystem/HighwaySystem.h"
#include "classes/Region/Region.h"
#include "classes/Route/Route.h"
#include "classes/TravelerList/TravelerList.h"
#include "classes/Waypoint/Waypoint.h"
#include "classes/WaypointQuadtree/WaypointQuadtree.h"
#include "functions/crawl_hwy_data.h"
#include "functions/split.h"
#include "functions/upper.h"
#ifdef threading_enabled
#include "functions/threads.h"
#endif
using namespace std;

int main(int argc, char *argv[])
{	ifstream file;
	string line;
	mutex list_mtx, term_mtx;

	// argument parsing
	if (Args::init(argc, argv)) return 1;
      #ifndef threading_enabled
	Args::numthreads = 1;
      #endif

	// start a timer for including elapsed time reports in messages
	ElapsedTime et(Args::timeprecision);
	time_t timestamp = time(0);
	cout << "Start: " << ctime(&timestamp);

	// create ErrorList
	ErrorList el;

	// Get list of travelers in the system
	TravelerList::ids = move(Args::userlist);
	if (TravelerList::ids.empty())
	{	DIR *dir;
		dirent *ent;
		if ((dir = opendir (Args::userlistfilepath.data())) != NULL)
		{	while ((ent = readdir (dir)) != NULL)
			{	string trav = ent->d_name;
				if (trav.size() > 5 && trav.substr(trav.size()-5, string::npos) == ".list")
					TravelerList::ids.push_back(trav);
			}
			closedir(dir);
		}
		else	el.add_error("Error opening user list file path \""+Args::userlistfilepath+"\". (Not found?)");
	}
	else for (string &id : TravelerList::ids) id += ".list";

	// read region, country, continent descriptions
	cout << et.et() << "Reading region, country, and continent descriptions." << endl;

	// continents
	vector<pair<string, string>> continents;
	file.open(Args::highwaydatapath+"/continents.csv");
	if (!file) el.add_error("Could not open "+Args::highwaydatapath+"/continents.csv");
	else {	getline(file, line); // ignore header line
		while(getline(file, line))
		{	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
			if (line.empty()) continue;
			size_t delim = line.find(';');
			if (delim == string::npos)
			{	el.add_error("Could not parse continents.csv line: [" + line
					   + "], expected 2 fields, found 1");
				continue;
			}
			string code = line.substr(0,delim);
			string name = line.substr(delim+1);
			if (name.find(';') != string::npos)
			{	el.add_error("Could not parse continents.csv line: [" + line
					   + "], expected 2 fields, found 3");
				continue;
			}
			// verify field lengths
			if (code.size() > DBFieldLength::continentCode)
				el.add_error("Continent code > " + std::to_string(DBFieldLength::continentCode)
					   + " bytes in continents.csv line " + line);
			if (name.size() > DBFieldLength::continentName)
				el.add_error("Continent name > " + std::to_string(DBFieldLength::continentName)
					   + " bytes in continents.csv line " + line);
			continents.emplace_back(pair<string, string>(code, name));
		}
	     }
	file.close();
	// create a dummy continent to catch unrecognized continent codes in .csv files
	continents.emplace_back(pair<string, string>("error", "unrecognized continent code"));

	// countries
	vector<pair<string, string>> countries;
	file.open(Args::highwaydatapath+"/countries.csv");
	if (!file) el.add_error("Could not open "+Args::highwaydatapath+"/countries.csv");
	else {	getline(file, line); // ignore header line
		while(getline(file, line))
		{	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
			if (line.empty()) continue;
			size_t delim = line.find(';');
			if (delim == string::npos)
			{	el.add_error("Could not parse countries.csv line: [" + line
					   + "], expected 2 fields, found 1");
				continue;
			}
			string code = line.substr(0,delim);
			string name = line.substr(delim+1);
			if (name.find(';') != string::npos)
			{	el.add_error("Could not parse countries.csv line: [" + line
					   + "], expected 2 fields, found 3");
				continue;
			}
			// verify field lengths
			if (code.size() > DBFieldLength::countryCode)
				el.add_error("Country code > " + std::to_string(DBFieldLength::countryCode)
					   + " bytes in countries.csv line " + line);
			if (name.size() > DBFieldLength::countryName)
				el.add_error("Country name > " + std::to_string(DBFieldLength::countryName)
					   + " bytes in countries.csv line " + line);
			countries.emplace_back(pair<string, string>(code, name));
		}
	     }
	file.close();
	// create a dummy country to catch unrecognized country codes in .csv files
	countries.emplace_back(pair<string, string>("error", "unrecognized country code"));

	//regions
	file.open(Args::highwaydatapath+"/regions.csv");
	if (!file) el.add_error("Could not open "+Args::highwaydatapath+"/regions.csv");
	else {	getline(file, line); // ignore header line
		while(getline(file, line))
		{	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
			if (line.empty()) continue;
			Region* r = new Region(line, countries, continents, el);
				    // deleted on termination of program
			if (r->is_valid)
			{	Region::allregions.push_back(r);
				Region::code_hash[r->code] = r;
			} else	delete r;
		}
	     }
	file.close();
	// create a dummy region to catch unrecognized region codes in .csv files
	Region::allregions.push_back(new Region("error;unrecognized region code;error;error;unrecognized region code", countries, continents, el));
	Region::code_hash[Region::allregions.back()->code] = Region::allregions.back();

	// Create a list of HighwaySystem objects, one per system in systems.csv file
	cout << et.et() << "Reading systems list in " << Args::highwaydatapath << "/" << Args::systemsfile << "." << endl;
	file.open(Args::highwaydatapath+"/"+Args::systemsfile);
	if (!file) el.add_error("Could not open "+Args::highwaydatapath+"/"+Args::systemsfile);
	else {	getline(file, line); // ignore header line
		list<string> ignoring;
		while(getline(file, line))
		{	if (line.back() == 0x0D) line.erase(line.end()-1);	// trim DOS newlines
			if (line.empty()) continue;
			if (line[0] == '#')
			{	ignoring.push_back("Ignored comment in " + Args::systemsfile + ": " + line);
				continue;
			}
			HighwaySystem *hs = new HighwaySystem(line, el, countries);
					    // deleted on termination of program
			if (!hs->is_valid) delete hs;
			else {	HighwaySystem::syslist.push_back(hs);
			     }
		}
		cout << endl;
		// at the end, print the lines ignored
		for (string& l : ignoring) cout << l << endl;
		ignoring.clear();
	     }
	file.close();

	// For tracking whether any .wpt files are in the directory tree
	// that do not have a .csv file entry that causes them to be
	// read into the data
	cout << et.et() << "Finding all .wpt files. " << flush;
	unordered_set<string> splitsystems;
	crawl_hwy_data(Args::highwaydatapath+"/hwy_data", Route::all_wpt_files, splitsystems, Args::splitregion, 0);
	cout << Route::all_wpt_files.size() << " files found." << endl;

	// For finding colocated Waypoints and concurrent segments, we have
	// quadtree of all Waypoints in existence to find them efficiently
	WaypointQuadtree all_waypoints(-90,-180,90,180);

	// Next, read all of the .wpt files for each HighwaySystem
	cout << et.et() << "Reading waypoints for all routes." << endl;
      #ifdef threading_enabled
	HighwaySystem::in_flight.assign(Args::numthreads,0);
	std::vector<std::thread> thr(Args::numthreads);
	HighwaySystem::it = HighwaySystem::syslist.begin();
	#define THREADLOOP for (unsigned int t = 0; t < thr.size(); t++)
	THREADLOOP thr[t] = thread(ReadWptThread, t, &list_mtx, &el, &all_waypoints);
	THREADLOOP thr[t].join();
	HighwaySystem::in_flight.clear();
      #else
	for (HighwaySystem* h : HighwaySystem::syslist)
	{	std::cout << h->systemname << std::flush;
		bool usa_flag = h->country->first == "USA";
		for (Route* r : h->route_list)
			r->read_wpt(0, &all_waypoints, &el, usa_flag);
		std::cout << "!" << std::endl;
	}
      #endif

	timestamp = time(0);
	cout << "Finish: " << ctime(&timestamp);
	cout << "Total run time: " << et.et() << endl;

}
