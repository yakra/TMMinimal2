class Route;
class TravelerList;
class Waypoint;
#include <list>
#include <mutex>
#include <unordered_set>

class HighwaySegment
{   /* This class represents one highway segment: the connection between two
    Waypoints connected by one or more routes */

	public:
	Waypoint *waypoint1;
	Waypoint *waypoint2;
	Route *route;
	double length;
	std::list<HighwaySegment*> *concurrent;
	std::unordered_set<TravelerList*> clinched_by;
	std::mutex clin_mtx;
	unsigned char system_concurrency_count;
	unsigned char active_only_concurrency_count;
	unsigned char active_preview_concurrency_count;

	HighwaySegment(Waypoint *, Waypoint *, Route *);
};
