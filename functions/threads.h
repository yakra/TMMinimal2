class ErrorList;
class WaypointQuadtree;
#include <mutex>

void ReadWptThread   (unsigned int, std::mutex*, ErrorList*, WaypointQuadtree*);
