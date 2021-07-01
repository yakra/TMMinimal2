#include "Datacheck.h"

std::mutex Datacheck::mtx;
std::list<Datacheck> Datacheck::errors;

void Datacheck::add(Route *rte, std::string l1, std::string l2, std::string l3, std::string c, std::string i)
{	mtx.lock();
	errors.emplace_back(rte, l1, l2, l3, c, i);
	mtx.unlock();
}

Datacheck::Datacheck(Route *rte, std::string l1, std::string l2, std::string l3, std::string c, std::string i)
{	route = rte;
	label1 = l1;
	label2 = l2;
	label3 = l3;
	code = c;
	info = i;
	fp = 0;
}
