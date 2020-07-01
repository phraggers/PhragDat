//====================================
// PhragDat Datafile Manager
// Compile/Extract/Read Phragdat files
// C++17 Windows 64-bit
//====================================
// (c) Phragware 2020
//====================================

#define VER_MAJ 5
#define VER_MIN 0
#define DEBUG_MODE 1

#include "phragdat_headers.h"

int main(int argc, char **argv)
{
	if(argc < 2 || argc > 5)
	{
		std::cerr << PHD_UsageStr << std::endl;
		return 1;
	}

	std::string arg_input; // -i"path"
	std::string arg_datpath; // -d"path"
	std::string arg_cpath; // -c"path"
	std::string arg_exclusions; // -e"path"
	std::map<int,bool> argisproc; // check all args processed

  // process args
	for(int i=1; i<argc; i++)
	{
    std::string thisarg = argv[i];
		argisproc[i] = 0;

		// set input
		if(thisarg[0] == '-' && thisarg[1] == 'i')
		{
			if(thisarg.length() > 2)
			{
        for(int j=2; j<thisarg.length(); j++)
				{
          arg_input.push_back(thisarg[j]);
					argisproc[i] = 1;
				}
			}
		}

		// set datpath
		if(thisarg[0] == '-' && thisarg[1] == 'd')
		{
			if(thisarg.length() > 2)
			{
				for(int j=2; j<thisarg.length(); j++)
				{
					arg_datpath.push_back(thisarg[j]);
					argisproc[i] = 1;
				}
			}
		}

		// set cpath
		if(thisarg[0] == '-' && thisarg[1] == 'c')
		{
			if(thisarg.length() > 2)
			{
				for(int j=2; j<thisarg.length(); j++)
				{
					arg_cpath.push_back(thisarg[j]);
					argisproc[i] = 1;
				}
			}
		}

		// set exclusions
		if(thisarg[0] == '-' && thisarg[1] == 'e')
		{
			if(thisarg.length() > 2)
			{
				for(int j=2; j<thisarg.length(); j++)
				{
					arg_exclusions.push_back(thisarg[j]);
					argisproc[i] = 1;
				}
			}
		}

		// remove duplicate slashes in args
		if(arg_input.length()) arg_input = PHD_EnsureSingleSlashes(arg_input);
		if(arg_datpath.length()) arg_datpath = PHD_EnsureSingleSlashes(arg_datpath);
		if(arg_cpath.length()) arg_cpath = PHD_EnsureSingleSlashes(arg_cpath);
		if(arg_exclusions.length()) arg_exclusions = PHD_EnsureSingleSlashes(arg_exclusions);

	}

	// error for unused args
	for(int i=1; i<argc; i++)
	{
		if(!argisproc[i])
		{
      std::cerr << "PhragDat Error: unknown argument: " << argv[i] << std::endl;
			return 1;
		}
	}

	// set console page 850 for standardized output
	UINT ConsolePage = GetConsoleOutputCP();
	if(ConsolePage != 850) SetConsoleOutputCP(850);

	int ecode = PHD_COMPILE(arg_input, arg_datpath, arg_cpath, arg_exclusions);

	// return console page back to what it was before phragdat ran, be nice to user :D
	if(GetConsoleOutputCP() != ConsolePage) SetConsoleOutputCP(ConsolePage);

	return ecode;
}