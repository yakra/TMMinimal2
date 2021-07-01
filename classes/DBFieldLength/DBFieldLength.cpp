#include "DBFieldLength.h"

// constants
const size_t DBFieldLength::abbrev = 3;
const size_t DBFieldLength::banner = 6;
const size_t DBFieldLength::city = 100;
const size_t DBFieldLength::color = 16;
const size_t DBFieldLength::continentCode = 3;
const size_t DBFieldLength::continentName = 15;
const size_t DBFieldLength::countryCode = 3;
const size_t DBFieldLength::countryName = 32;
const size_t DBFieldLength::label = 26;
const size_t DBFieldLength::regionCode = 8;
const size_t DBFieldLength::regionName = 48;
const size_t DBFieldLength::regiontype = 32;
const size_t DBFieldLength::root = 32;
const size_t DBFieldLength::route = 16;
const size_t DBFieldLength::systemFullName = 60;
const size_t DBFieldLength::systemName = 10;
const size_t DBFieldLength::traveler = 48;
// sums of other constants
const size_t DBFieldLength::dcErrValue = DBFieldLength::root + DBFieldLength::label + 1;;
