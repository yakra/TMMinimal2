#define debug
void ReadWptThread
(	unsigned int id, std::mutex* hs_mtx,
	ErrorList* el,WaypointQuadtree* all_waypoints
)
{	//printf("Starting ReadWptThread %02i\n", id); fflush(stdout);
	while (HighwaySystem::it != HighwaySystem::syslist.end())
	{	hs_mtx->lock();
		if (HighwaySystem::it == HighwaySystem::syslist.end())
		{	hs_mtx->unlock();
			return;
		}
		HighwaySystem* h(*HighwaySystem::it);
		//printf("ReadWptThread %02i assigned %s\n", id, h->systemname.data()); fflush(stdout);
		HighwaySystem::it++;
	      #ifdef debug
		HighwaySystem::in_flight[id] = h;
		TravelerList::mtx.lock();
		for (HighwaySystem* sys : HighwaySystem::in_flight)
		  if (sys)
		  {	std::cout << "| " << sys->systemname;
			for (size_t i = sys->systemname.size(); i < 11; i++)
			  std::cout << ' '; // pad to 11 chars with spaces
		  }
		  else std::cout << "           ";
		std::cout << " |" << std::endl;
		TravelerList::mtx.unlock();
	      #endif
		hs_mtx->unlock();
		std::cout << h->systemname << std::flush;
		bool usa_flag = h->country->first == "USA";
		for (Route *r : h->route_list)
			r->read_wpt(id, all_waypoints, el, usa_flag);
		std::cout << "!" << std::endl;
	}
}
