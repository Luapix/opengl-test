#pragma once

#include <string>

namespace PixCraft {
	const int VERSION_MAJOR = 0;
	const int VERSION_MINOR = 0;
	extern const char* COMMIT_HASH;
	
	std::string getVersionString();
}