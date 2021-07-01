class HighwaySegment;
class HighwaySystem;
class Region;
class Route;
#include <list>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

class TravelerList
{
	public:
	std::unordered_set<HighwaySegment*> clinched_segments;
	std::string *update;
	std::unordered_set<Route*> routes;
	static std::mutex mtx;
	static std::list<std::string> ids;
};
