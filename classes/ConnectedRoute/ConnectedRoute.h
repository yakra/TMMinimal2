class ErrorList;
class HighwaySystem;
class Route;
#include <iostream>
#include <vector>

class ConnectedRoute
{   /* This class encapsulates a single 'connected route' as given
    by a single line of a _con.csv file
    */

	public:
	HighwaySystem *system;
	std::string route;
	std::string banner;
	std::string groupname;
	std::vector<Route*> roots;

	double mileage; // will be computed for routes in active & preview systems

	ConnectedRoute(std::string &, HighwaySystem *, ErrorList &);
};
