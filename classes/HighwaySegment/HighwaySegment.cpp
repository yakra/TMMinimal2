#include "HighwaySegment.h"
#include "../Waypoint/Waypoint.h"

HighwaySegment::HighwaySegment(Waypoint *w1, Waypoint *w2, Route *rte)
{	waypoint1 = w1;
	waypoint2 = w2;
	route = rte;
	length = waypoint1->distance_to(waypoint2);
	concurrent = 0;
	system_concurrency_count = 1;
	active_only_concurrency_count = 1;
	active_preview_concurrency_count = 1;
}
