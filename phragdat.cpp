//====================================
// PhragDat Datafile Manager
// Compile/Extract/Read Phragdat files
// C++17 Windows 64-bit
//====================================
// (c) Phragware 2020
//====================================

#define VER_MAJ 4
#define VER_MIN 1
#define DEBUG_MODE 0

#include "phragdat_headers.h"

int main(int argc, char **argv)
{
	if(argc < 2)
	{
		std::cerr << PHD_UsageStr << std::endl;
		return 1;
	}

	std::string arg_input; // -i"path"
	std::string arg_output; // -o"path"
	std::string arg_file; // -f"path"
	std::map<int,bool> argisproc; // check all args processed
	char select_mode = 0;

  // process args
	for(int i=1; i<argc; i++)
	{
    std::string thisarg = argv[i];
		argisproc[i] = 0;

    // set mode
		if(thisarg[0] == '-' && thisarg.length() == 2)
		{
      select_mode = thisarg[1];
			argisproc[i] = 1;
		}

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

		// set output
		if(thisarg[0] == '-' && thisarg[1] == 'o')
		{
			if(thisarg.length() > 2)
			{
				for(int j=2; j<thisarg.length(); j++)
				{
					arg_output.push_back(thisarg[j]);
					argisproc[i] = 1;
				}
			}
		}

		// set file
		if(thisarg[0] == '-' && thisarg[1] == 'f')
		{
			if(thisarg.length() > 2)
			{
				for(int j=2; j<thisarg.length(); j++)
				{
					arg_file.push_back(thisarg[j]);
					argisproc[i] = 1;
				}
			}
		}

		// remove duplicate slashes in args
		if(arg_input.length()) arg_input = PHD_EnsureSingleSlashes(arg_input);
		if(arg_output.length()) arg_output = PHD_EnsureSingleSlashes(arg_output);
		if(arg_file.length()) arg_file = PHD_EnsureSingleSlashes(arg_file);

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

	int ecode = 0;

	// mode switch
	switch(select_mode)
	{
		#if(DEBUG_MODE)
		case 'd': // DEBUG TEST
		{
			std::cerr << "DEBUG_TEST" << std::endl;
			return 0;
    } break;
		#endif //DEBUG_MODE

		case 'h': // print help
		{
      std::cout << PHD_HelpStr << std::endl;
      ecode = 0;
		} break;

		case 'v': // print version
		{
      std::cout << PHD_GetVersion() << std::endl;
      ecode = 0;
		} break;

		case 'c': // compile mode
		{
      ecode = PHD_COMPILE(arg_input, arg_output, arg_file);
		} break;

		case 'e': // extract mode
		{
      ecode = PHD_EXTRACT(arg_input, arg_output, arg_file);
		} break;

		default:
		{
			std::cerr << "PhragDat Error: Invalid Mode Switch -" << select_mode << ' : ' << "See phragdat -h for help" << std::endl;
			return 1;
		} break;
	}

	// return console page back to what it was before phragdat ran, be nice to user :D
	if(GetConsoleOutputCP() != ConsolePage) SetConsoleOutputCP(ConsolePage);

	return ecode;
}