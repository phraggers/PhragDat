//====================================
// PhragDat Datafile Manager
// phragdat_headers.h
// C++17 Windows 64-bit
//====================================
// (c) Phragware 2020
//====================================

// MSVCC likes to complain about safety and I don't care!
#define _CRT_SECURE_NO_WARNINGS

// C Headers
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

// C++ Storage
#include <array>
#include <vector>
#include <map>

// C++ Streams
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>

// C++ Other
#include <string>
#include <memory>
#include <iterator>
#include <filesystem>
#include <windows.h>
#include <shlwapi.h>
#include <random>
#include <fcntl.h>
#include <io.h>

// GLOBAL GENERATORS
std::string GETBASEPATH()
{
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL,buffer,sizeof(buffer));
	std::string bufr = buffer;
	while(bufr.back() != '\\') bufr.pop_back();
	return bufr;
}

// GLOBALS
std::string BASE_PATH = GETBASEPATH();
const uint64_t BUFFER_WRITE_SIZE = 0xffffffff; // 8-byte(64-bit) buffersize

// INTERNALS
#include "phragdat_funcs.h"
#include "phragdat_compile.h"