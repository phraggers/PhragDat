//====================================
// PhragDat Datafile Manager
// phragdat_funcs.h
// C++17 Windows 64-bit
//====================================
// (c) Phragware 2020
//====================================

//================================
//    Random
// returns int between min/max
//================================
std::mt19937 PHD_rng((unsigned int)time(NULL));
int PHD_RandomRoll(int _min, int _max)
{
  std::uniform_int_distribution<int> DiceRoll(_min, _max);
  return DiceRoll(PHD_rng);
}

//==============
// COUT Strings
//==============
std::string PHD_UsageStr =
"PhragDat Error: incorrect usage, use phragdat -h for help";

std::string PHD_HelpStr =
"\n## Usage:\
\n	phragdat -i\"input/dir\" -d\".dat output/dir\" -c\".c output/dir\" -e\"exclusions.txt\"(optional)\
\n\
\n### Compilation:\
\n    compiles all contents of \"path/to/input\" and exports single .dat file\
\n    Output will be named [input directory].dat and .c respectively\
\n    optional exclusions text file: see below options for details\
\n\
\n#### Contents .c:\
\n    contains mapping info for C/C++ code to access files within the .dat file\
\n    list of files (path from .dat as root) with their address and length within .dat file\
\n\
\n#### Exclusions File:\
\n    a simple text file with each new text line counting as an exclude.\
\n    Possible Exclusions:\
\n    Specific Files: \"filename.ext\"\
\n    Files with specific extension: \"*.ext\"\
\n    Directory and all sub-directories: \"Directory/\"\
\n    (directories must end in '/' and ANY directory with that name will be excluded)\
\n    for example:\
\n        File e.txt Contents:\
\n            *.txt\
\n            Thumbs.db\
\n            TestDirectory/\
\n    This will exclude all files ending in '.txt', all 'Thumbs.db' files, and all files\
\n    and directories within 'TestDirectory/'.";

//============================
// PHD_GetVersion
// returns version as string
//============================
std::string PHD_GetVersion()
{
  // expects IBM-850 (CP850) for (c) character 184
	std::stringstream ss;
	ss << "PhragDat Datafile Manager v" << VER_MAJ << "." << VER_MIN << " " << static_cast<char>(184) << "Phragware 2020";
	return ss.str();
}

//================================================
// exec : returns string from cmd line
//================================================
std::string PHD_Exec(const char *_cmd)
{
	if(DEBUG_MODE) std::cout << "CMD: " << _cmd << std::endl;

    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(_popen(_cmd, "r"), _pclose);

    while(!feof(pipe.get()))
    {
        if(fgets(buffer.data(), 128, pipe.get()) != nullptr)
        result += buffer.data();
    }

    return result;
}

//===================================================
// separate string : string > vector of strings by char
// (input string, separator, vector to output to)
//===================================================
void PHD_SeparateString(std::string _in, char _sep, std::vector<std::string> &_vecstr)
{
	if(!_in.length()) return;
	std::stringstream ss(_in);
	std::string outstr;
	if(_vecstr.size() > 0) _vecstr.clear();
	while(std::getline(ss, outstr, _sep)) _vecstr.push_back(outstr);
}

//================================
// EnsureSingleSlashes
// removes any duplicate \\ or /
//================================
std::string PHD_EnsureSingleSlashes(std::string _Input)
{
  if(!_Input.length()) return "";
  std::string Output;

  for(int i=0; i<_Input.length(); i++)
  {
    if(_Input[i]=='\\' || _Input[i]=='/')
    {
      Output.push_back('/');
      int sc = 0;
      while(_Input[i+sc+1]=='\\' || _Input[i+sc+1]=='/')
      {
        sc++;
        if(i+sc+2 > _Input.length()) break;
      }
      i = i+sc;
      continue;
    }

    else Output.push_back(_Input[i]);
  }

  return Output;
}

//==================================
// GetFileAndDirectoryList
// get file and dir list from path
//==================================
int PHD_GetFileAndDirectoryList(std::string _Input, std::vector<std::string> &_vecstrF, std::vector<std::string> &_vecstrD)
{
	if(!_Input.length())
	{
		std::cerr << "PhragDat error: GetFileAndDirectoryList: given string empty" << std::endl;
		return 1;
	}

	if(GetFileAttributesA(_Input.c_str()) != (DWORD)FILE_ATTRIBUTE_DIRECTORY)
  {
    std::cerr << "PhragDat error: " << _Input << " is not a valid directory, please see phragdat -h for help. Aborting." << std::endl;
    return 1;
  }

	if(!_vecstrF.empty()) _vecstrF.clear();
	if(!_vecstrD.empty()) _vecstrD.clear();

  std::stringstream ss;
  for(const auto &entry : std::filesystem::directory_iterator(_Input)) ss << entry.path();

  std::map<uint64_t,std::string> Paths;
  uint64_t PathCounter = 0;
  std::string PathStr = ss.str();
  PathStr = PHD_EnsureSingleSlashes(PathStr);
  bool ReadEntry = 0;

  // make Paths[0] the root path of current
  Paths[0] = _Input;

  // paths are contained in ""
  for(int i=0; i<PathStr.length(); i++)
  {
    if(PathStr[i] == '\"')
    {
      if(!ReadEntry)
      {
        ReadEntry = 1;
        PathCounter++;
        continue;
      }
      else
      {
        ReadEntry = 0;
        continue;
      }
    }

    else Paths[PathCounter].push_back(PathStr[i]);
  }

  for(int i=0; i<Paths.size(); i++)
  {
    DWORD ftyp = GetFileAttributesA(Paths[i].c_str());

    if(ftyp == INVALID_FILE_ATTRIBUTES)
    {
      if(DEBUG_MODE) std::cout << Paths[i] << " invalid attributes, skipping..." << std::endl;
    }

    else if(ftyp & FILE_ATTRIBUTE_DIRECTORY)
    {
      if(DEBUG_MODE) std::cout << "Found Directory: " << Paths[i] << std::endl;
      _vecstrD.push_back(Paths[i]);
    }

    else
    {
      if(DEBUG_MODE) std::cout << "Found File: " << Paths[i] << std::endl;
      _vecstrF.push_back(Paths[i]);
    }
  }

	return 0;
}

//==================================================
// GetDirectoryList
// returns string of directories separated by \n
//==================================================
std::string PHD_GetDirectoryList(std::string _Input)
{
  if(!_Input.length())
	{
		std::cerr << "PhragDat error: GetDirectoryList: given string empty" << std::endl;
		return "";
	}

	if(PathIsDirectoryEmptyA(_Input.c_str()))
  {
    std::cerr << "PhragDat error: GetDirectoryList: given directory is empty" << std::endl;
		return "";
  }

  std::string ReturnStr;

  std::stringstream ss;
  for(const auto &entry : std::filesystem::directory_iterator(_Input)) ss << entry.path();

  std::map<uint64_t,std::string> Paths;
  uint64_t PathCounter = 0;
  std::string PathStr = ss.str();
  PathStr = PHD_EnsureSingleSlashes(PathStr);
  bool ReadEntry = 0;

  // make Paths[0] the root path of current
  Paths[0] = _Input;

  // paths are contained in ""
  for(int i=0; i<PathStr.length(); i++)
  {
    if(PathStr[i] == '\"')
    {
      if(!ReadEntry)
      {
        ReadEntry = 1;
        PathCounter++;
        continue;
      }
      else
      {
        ReadEntry = 0;
        continue;
      }
    }

    else Paths[PathCounter].push_back(PathStr[i]);
  }

  for(int i=0; i<Paths.size(); i++)
  {
    DWORD ftyp = GetFileAttributesA(Paths[i].c_str());

    if(ftyp == INVALID_FILE_ATTRIBUTES)
    {
      if(DEBUG_MODE) std::cout << Paths[i] << " invalid attributes, skipping..." << std::endl;
    }

    else if(ftyp & FILE_ATTRIBUTE_DIRECTORY)
    {
      if(DEBUG_MODE) std::cout << "Found Directory: " << Paths[i] << std::endl;
      if(!ReturnStr.length()) ReturnStr = Paths[i];
      else
      {
        ReturnStr += "\n";
        ReturnStr += Paths[i];
      }
    }
  }

  return ReturnStr;
}

//==================================================
// GetFileList
// returns string of files separated by \n
//==================================================
std::string PHD_GetFileList(std::string _Input)
{
  if(!_Input.length())
	{
		std::cerr << "PhragDat error: GetFileList: given string empty" << std::endl;
		return "";
	}

	if(PathIsDirectoryEmptyA(_Input.c_str()))
  {
    std::cerr << "PhragDat error: GetFileList: given directory is empty" << std::endl;
		return "";
  }

  std::string ReturnStr;

  std::stringstream ss;
  for(const auto &entry : std::filesystem::directory_iterator(_Input)) ss << entry.path();

  std::map<uint64_t,std::string> Paths;
  uint64_t PathCounter = 0;
  std::string PathStr = ss.str();
  PathStr = PHD_EnsureSingleSlashes(PathStr);
  bool ReadEntry = 0;

  // make Paths[0] the root path of current
  Paths[0] = _Input;

  // paths are contained in ""
  for(int i=0; i<PathStr.length(); i++)
  {
    if(PathStr[i] == '\"')
    {
      if(!ReadEntry)
      {
        ReadEntry = 1;
        PathCounter++;
        continue;
      }
      else
      {
        ReadEntry = 0;
        continue;
      }
    }

    else Paths[PathCounter].push_back(PathStr[i]);
  }

  for(int i=0; i<Paths.size(); i++)
  {
    DWORD ftyp = GetFileAttributesA(Paths[i].c_str());

    if(ftyp == INVALID_FILE_ATTRIBUTES)
    {
      if(DEBUG_MODE) std::cout << Paths[i] << " invalid attributes, skipping..." << std::endl;
    }

    else if(ftyp & FILE_ATTRIBUTE_DIRECTORY)
    {
      if(DEBUG_MODE) std::cout << "Found Directory: " << Paths[i] << std::endl;
    }

    else
    {
      if(DEBUG_MODE) std::cout << "Found File: " << Paths[i] << std::endl;
      if(!ReturnStr.length()) ReturnStr = Paths[i];
      else
      {
        ReturnStr += "\n";
        ReturnStr += Paths[i];
      }
    }
  }

  return ReturnStr;
}

//=======================================
//    RemoveParentsFromPath
// takes path, removes any parent dirs
//=======================================
std::string PHD_RemoveParentsFromPath(std::string _Input)
{
  if(!_Input.length())
  {
    std::cerr << "PhragDat error: RemoveParentsFromPath: given string empty" << std::endl;
    return "";
  }

  std::string Input = "/";
  Input += _Input;
  std::string Path;

  if(Input.back()=='/') Input.pop_back();

  int cc = 0;
  for(int i=(int)Input.length(); i>0; i--)
  {
    if(Input[i]=='/') break;
    else cc++;
  }

  for(int i=(((int)Input.length())-cc); i<(int)Input.length(); i++) if(Input[i]!='/')Path.push_back(Input[i]);

  if(Path[0]=='/')
  {
    std::string Path2 = Path;
    Path.clear();
    for(int i=1; i<(int)Path2.length(); i++) Path.push_back(Path2[i]);
  }

  return Path;
}

//====================================
//    RemovePathFromParents
// takes path, returns only parents
//====================================
std::string PHD_RemovePathFromParents(std::string _Input)
{
  if(!_Input.length())
  {
    std::cerr << "PhragDat error: RemovePathFromParents: given string empty" << std::endl;
    return "";
  }

  std::string Input = "/";
  Input += _Input;
  std::string Path;

  if(Input.back()=='/') Input.pop_back();

  int cc = 0;
  for(int i=(int)Input.length(); i>0; i--)
  {
    if(Input[i]=='/') break;
    else cc++;
  }

  for(int i=0; i<(((int)Input.length())-cc); i++) Path.push_back(Input[i]);

  if(Path[0]=='/')
  {
    std::string Path2 = Path;
    Path.clear();
    for(int i=1; i<(int)Path2.length(); i++) Path.push_back(Path2[i]);
  }

  return Path;
}

//================================
// Convert_uint64_to_uint8
//================================
void PHD_Convert_uint64_to_uint8(uint64_t _u64, std::vector<uint8_t> &_vec)
{
  uint64_t num = _u64;
  uint8_t *p = (uint8_t*)&num;
  for(int i=0; i<8; i++) _vec.push_back(p[i]);
}

//================================
// Convert_uint32_to_uint8
//================================
void PHD_Convert_uint32_to_uint8(uint32_t _u32, std::vector<uint8_t> &_vec)
{
  uint32_t num = _u32;
  uint8_t *p = (uint8_t*)&num;
  for(int i=0; i<4; i++) _vec.push_back(p[i]);
}

//================================
// Convert_uint16_to_uint8
//================================
void PHD_Convert_uint16_to_uint8(uint16_t _u16, std::vector<uint8_t> &_vec)
{
  uint16_t num = _u16;
  uint8_t *p = (uint8_t*)&num;
  for(int i=0; i<2; i++) _vec.push_back(p[i]);
}

//================================
// Convert_uint8_to_uint64
//================================
uint64_t PHD_Convert_uint8_to_uint64(std::vector<uint8_t> &_vec)
{
  std::vector<uint8_t> num = _vec;
  uint64_t *p = (uint64_t*)&num[0];
  return *p;
}

//================================
// Convert_uint8_to_uint32
//================================
uint32_t PHD_Convert_uint8_to_uint32(std::vector<uint8_t> &_vec)
{
  std::vector<uint8_t> num = _vec;
  uint32_t *p = (uint32_t*)&num[0];
  return *p;
}

//================================
// Convert_uint8_to_uint16
//================================
uint16_t PHD_Convert_uint8_to_uint16(std::vector<uint8_t> &_vec)
{
  std::vector<uint8_t> num = _vec;
  uint16_t *p = (uint16_t*)&num[0];
  return *p;
}