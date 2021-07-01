#include "ConnectedRoute.h"
#include "../DBFieldLength/DBFieldLength.h"
#include "../ErrorList/ErrorList.h"
#include "../HighwaySystem/HighwaySystem.h"
#include "../Route/Route.h"
#include "../../functions/lower.h"
#include "../../functions/split.h"

ConnectedRoute::ConnectedRoute(std::string &line, HighwaySystem *sys, ErrorList &el)
{	mileage = 0;

	// parse chopped routes csv line
	system = sys;
	size_t NumFields = 5;
	std::string sys_str, roots_str;
	std::string* fields[5] = {&sys_str, &route, &banner, &groupname, &roots_str};
	split(line, fields, NumFields, ';');
	if (NumFields != 5)
	{	el.add_error("Could not parse " + sys->systemname
			   + "_con.csv line: [" + line + "], expected 5 fields, found " + std::to_string(NumFields));
		return;
	}
	// system
	if (system->systemname != sys_str)
		el.add_error("System mismatch parsing " + system->systemname
			   + "_con.csv line [" + line + "], expected " + system->systemname);
	// route
	if (route.size() > DBFieldLength::route)
		el.add_error("route > " + std::to_string(DBFieldLength::route)
			   + " bytes in " + system->systemname + "_con.csv line: " + line);
	// banner
	if (banner.size() > DBFieldLength::banner)
		el.add_error("banner > " + std::to_string(DBFieldLength::banner)
			   + " bytes in " + system->systemname + "_con.csv line: " + line);
	// groupname
	if (groupname.size() > DBFieldLength::city)
		el.add_error("groupname > " + std::to_string(DBFieldLength::city)
			   + " bytes in " + system->systemname + "_con.csv line: " + line);
	// roots
	lower(roots_str.data());
	int rootOrder = 0;
	size_t l = 0;
	for (size_t r = 0; r != -1; l = r+1)
	{	r = roots_str.find(',', l);
		try {	Route *root = Route::root_hash.at(roots_str.substr(l, r-l));
			roots.push_back(root);
			if (root->con_route)
			  el.add_error("Duplicate root in " + sys->systemname + "_con.csv: " + root->root +
				       " already in " + root->con_route->system->systemname + "_con.csv");
			if (system != root->system)
			  el.add_error("System mismatch: chopped route " + root->root + " from " + root->system->systemname +
				       ".csv in connected route in " + system->systemname + "_con.csv");
			root->con_route = this;
			// save order of route in connected route
			root->rootOrder = rootOrder;
			rootOrder++;
		    }
		catch (std::out_of_range& oor)
		    {	el.add_error("Could not find Route matching ConnectedRoute root " + roots_str.substr(l, r-l) +
					" in system " + system->systemname + '.');
		    }
	}
	if (roots.size() < 1) el.add_error("No valid roots in " + system->systemname + "_con.csv line: " + line);
}
