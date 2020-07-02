//====================================
// PhragDat Datafile Manager
// phragdat_funcs.h
// C++17 Windows 64-bit
//====================================
// (c) Phragware 2020
//====================================

//==============
// COUT Strings
//==============
std::string PHD_UsageStr =
"PhragDat Error: incorrect usage, use phragdat -h for help";

std::string PHD_HelpStr =
"\n## Usage:\
\n	phragdat -i\"input/dir\" -d\"dat/output/dir\" -c\"csv/output/dir\" -e\"exclusions.txt\"(optional)\
\n\
\n### Compilation:\
\n    compiles all contents of \"path/to/input\" and exports single .dat file\
\n    Output will be named [input].dat and [input].csv respectively\
\n    optional exclusions text file: see below options for details\
\n\
\n#### Contents .csv:\
\n    contains contents/address info to access files within the .dat file\
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