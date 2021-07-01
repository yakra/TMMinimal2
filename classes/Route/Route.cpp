#include "Route.h"
#include "../DBFieldLength/DBFieldLength.h"
#include "../ErrorList/ErrorList.h"
#include "../HighwaySegment/HighwaySegment.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Region/Region.h"
#include "../TravelerList/TravelerList.h"
#include "../../functions/lower.h"
#include "../../functions/split.h"
#include "../../functions/upper.h"
#include <cstring>
#include <fstream>

std::unordered_map<std::string, Route*> Route::root_hash, Route::pri_list_hash, Route::alt_list_hash;
std::unordered_set<std::string> Route::all_wpt_files;
std::mutex Route::awf_mtx;

Route::Route(std::string &line, HighwaySystem *sys, ErrorList &el)
{	/* initialize object from a .csv file line,
	but do not yet read in waypoint file */
	con_route = 0;
	mileage = 0;
	rootOrder = -1; // order within connected route
	region = 0;	// if this stays 0, setup has failed due to bad .csv data
	is_reversed = 0;
	last_update = 0;

	// parse chopped routes csv line
	size_t NumFields = 8;
	std::string sys_str, arn_str;
	std::string* fields[8] = {&sys_str, &rg_str, &route, &banner, &abbrev, &city, &root, &arn_str};
	split(line, fields, NumFields, ';');
	if (NumFields != 8)
	{	el.add_error("Could not parse " + sys->systemname
			   + ".csv line: [" + line + "], expected 8 fields, found " + std::to_string(NumFields));
		root.clear(); // in case it was filled by a line with 7 or 9+ fields
		return;
	}
	// system
	system = sys;
	if (system->systemname != sys_str)
		el.add_error("System mismatch parsing " + system->systemname
			   + ".csv line [" + line + "], expected " + system->systemname);
	// region
	try {	region = Region::code_hash.at(rg_str);
	    }
	catch (const std::out_of_range& oor)
	    {	el.add_error("Unrecognized region in " + system->systemname
			   + ".csv line: " + line);
		region = Region::code_hash.at("error");
	    }
	// route
	if (route.size() > DBFieldLength::route)
		el.add_error("Route > " + std::to_string(DBFieldLength::route)
			   + " bytes in " + system->systemname + ".csv line: " + line);
	// banner
	if (banner.size() > DBFieldLength::banner)
		el.add_error("Banner > " + std::to_string(DBFieldLength::banner)
			   + " bytes in " + system->systemname + ".csv line: " + line);
	// abbrev
	if (abbrev.size() > DBFieldLength::abbrev)
		el.add_error("Abbrev > " + std::to_string(DBFieldLength::abbrev)
			   + " bytes in " + system->systemname + ".csv line: " + line);
	// city
	if (city.size() > DBFieldLength::city)
		el.add_error("City > " + std::to_string(DBFieldLength::city)
			   + " bytes in " + system->systemname + ".csv line: " + line);
	// root
	if (root.size() > DBFieldLength::root)
		el.add_error("Root > " + std::to_string(DBFieldLength::root)
			   + " bytes in " + system->systemname + ".csv line: " + line);
	lower(root.data());
	// alt_route_names
	upper(arn_str.data());
	size_t len;
	for (size_t pos = 0; pos < arn_str.size(); pos += len+1)
	{	len = strcspn(arn_str.data()+pos, ",");
		alt_route_names.emplace_back(arn_str, pos, len);
	}

	// insert into root_hash, checking for duplicate root entries
	if (!root_hash.insert(std::pair<std::string, Route*>(root, this)).second)
		el.add_error("Duplicate root in " + system->systemname + ".csv: " + root +
			     " already in " + root_hash.at(root)->system->systemname + ".csv");
	// insert list name into pri_list_hash, checking for duplicate .list names
	std::string list_name(readable_name());
	upper(list_name.data());
	if (alt_list_hash.find(list_name) != alt_list_hash.end())
		el.add_error("Duplicate main list name in " + root + ": '" + readable_name() +
			     "' already points to " + alt_list_hash.at(list_name)->root);
	else if (!pri_list_hash.insert(std::pair<std::string,Route*>(list_name, this)).second)
		el.add_error("Duplicate main list name in " + root + ": '" + readable_name() +
			     "' already points to " + pri_list_hash.at(list_name)->root);
	// insert alt names into alt_list_hash, checking for duplicate .list names
	for (std::string& a : alt_route_names)
	{   list_name = rg_str + ' ' + a;
	    upper(list_name.data());
	    if (pri_list_hash.find(list_name) != pri_list_hash.end())
		el.add_error("Duplicate alt route name in " + root + ": '" + region->code + ' ' + a +
			     "' already points to " + pri_list_hash.at(list_name)->root);
	    else if (!alt_list_hash.insert(std::pair<std::string, Route*>(list_name, this)).second)
		el.add_error("Duplicate alt route name in " + root + ": '" + region->code + ' ' + a +
			     "' already points to " + alt_list_hash.at(list_name)->root);
	    // populate unused set
	    system->unusedaltroutenames.insert(list_name);
	}
}

std::string Route::str()
{	/* printable version of the object */
	return root + " (" + std::to_string(point_list.size()) + " total points)";
}

std::string Route::readable_name()
{	/* return a string for a human-readable route name */
	return rg_str + " " + route + banner + abbrev;
}

std::string Route::list_entry_name()
{	/* return a string for a human-readable route name in the
	format expected in traveler list files */
	return route + banner + abbrev;
}

std::string Route::name_no_abbrev()
{	/* return a string for a human-readable route name in the
	format that might be encountered for intersecting route
	labels, where the abbrev field is often omitted */
	return route + banner;
}

void Route::store_traveled_segments(TravelerList* t, std::ofstream& log, unsigned int beg, unsigned int end)
{	// store clinched segments with traveler and traveler with segments
	for (unsigned int pos = beg; pos < end; pos++)
	{	HighwaySegment *hs = segment_list[pos];
		hs->add_clinched_by(t);
		t->clinched_segments.insert(hs);
	}
	if (t->routes.insert(this).second && last_update && t->update && last_update[0] >= *t->update)
		log << "Route updated " << last_update[0] << ": " << readable_name() << '\n';
}
