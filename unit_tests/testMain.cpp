//======================================================================
// 
//======================================================================

//======================================================================

#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "rlutil.h"

//======================================================================


void Warning(std::string warningMessage)
{
	rlutil::saveDefaultColor();
	rlutil::setColor(rlutil::LIGHTMAGENTA);
	std::cout << "[WARNING] " << warningMessage << std::endl;
	rlutil::resetColor();
}

//======================================================================

int main(int argc, char* argv[]) 
{
	int result = Catch::Session().run(argc, argv);

	// global clean-up...

	return result;
}

//----------------------------------------------------------------------
//======================================================================
