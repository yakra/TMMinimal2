#include "TravelerList.h"
std::mutex TravelerList::mtx;
std::list<std::string> TravelerList::ids;
