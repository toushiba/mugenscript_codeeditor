#include "common/HashString.h"
#include <random>


int rand(int mod)
{
	std::random_device dev;
	auto rand = dev();
	return rand % mod;
}

u32_t hash(const char* str)
{
	return hash(str,strlen(str));
}
