//====================================
// PhragDat Datafile Manager
// Compile/Extract/Read Phragdat files
// C++17 Windows 64-bit
//====================================
// (c) Phragware 2020
//====================================

#define VER_MAJ 5
#define VER_MIN 4
#define DEBUG_MODE 0

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
static std::string
GETBASEPATH()
{
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL,buffer,sizeof(buffer));
	std::string bufr = buffer;
	while(bufr.back() != '\\') bufr.pop_back();
	return bufr;
}

// GLOBALS
static std::string BASE_PATH = GETBASEPATH();
static uint64_t BUFFER_WRITE_SIZE = 0xffffffff; // 8-byte(64-bit) buffersize
//#define BUFFER_WRITE_SIZE 4294967295U

//==============
// COUT Strings
//==============
static std::string PHD_UsageStr =
"PhragDat Error: incorrect usage, use phragdat -h for help";

static std::string PHD_HelpStr =
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
static std::string
PHD_GetVersion()
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
static std::string
PHD_EnsureSingleSlashes(std::string _Input)
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
static int
PHD_GetFileAndDirectoryList(std::string _Input,
                            std::vector<std::string> &_vecstrF,
                            std::vector<std::string> &_vecstrD)
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
  for(auto &entry : std::filesystem::directory_iterator(_Input))
  {
    ss << entry.path();
  }

  std::map<uint64_t,std::string> Paths;
  uint64_t PathCounter = 0;
  std::string PathStr = ss.str();
  PathStr = PHD_EnsureSingleSlashes(PathStr);
  bool ReadEntry = 0;

  // make Paths[0] the root path of current
  Paths[0] = _Input;

  // paths are contained in ""
  for(int iPathStr=0; iPathStr<PathStr.length(); ++iPathStr)
  {
    if(PathStr[iPathStr] == '\"')
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

    else
    {
      Paths[PathCounter].push_back(PathStr[iPathStr]);
    }
  }

  for(int iPaths=0; iPaths<Paths.size(); ++iPaths)
  {
    DWORD ftyp = GetFileAttributesA(Paths[iPaths].c_str());

    if(ftyp == INVALID_FILE_ATTRIBUTES)
    {
      if(DEBUG_MODE)
      {
        std::cout << Paths[iPaths] << " invalid attributes, skipping..." << std::endl;
      }
    }

    else if(ftyp & FILE_ATTRIBUTE_DIRECTORY)
    {
      if(DEBUG_MODE)
      {
        std::cout << "Found Directory: " << Paths[iPaths] << std::endl;
      }

      _vecstrD.push_back(Paths[iPaths]);
    }

    else
    {
      if(DEBUG_MODE)
      {
        std::cout << "Found File: " << Paths[iPaths] << std::endl;
      }

      _vecstrF.push_back(Paths[iPaths]);
    }
  }

	return 0;
}

//=======================================
//    RemoveParentsFromPath
// takes path, removes any parent dirs
//=======================================
static std::string
PHD_RemoveParentsFromPath(std::string _Input)
{
  if(!_Input.length())
  {
    std::cerr << "PhragDat error: RemoveParentsFromPath: given string empty" << std::endl;
    return "";
  }

  std::string Input = "/";
  Input += _Input;
  std::string Path;

  if(Input.back()=='/')
  {
    Input.pop_back();
  }

  int CharCount = 0;
  for(int iInputLength = (int)Input.length(); iInputLength > 0; --iInputLength)
  {
    if(Input[iInputLength]=='/')
    {
      break;
    }

    else
    {
      CharCount++;
    }
  }

  for(int iPathChar = ( ((int)Input.length()) - CharCount ); iPathChar < (int)Input.length(); ++iPathChar)
  {
    if(Input[iPathChar]!='/')
    {
      Path.push_back(Input[iPathChar]);
    }
  }

  if(Path[0] == '/')
  {
    std::string Path2 = Path;
    Path.clear();

    for(int iPath2Char = 1; iPath2Char < (int)Path2.length(); iPath2Char++)
    {
      Path.push_back(Path2[iPath2Char]);
    }
  }

  return Path;
}

/* Function not currently being used
//====================================
//    RemovePathFromParents
// takes path, returns only parents
//====================================
static std::string
PHD_RemovePathFromParents(std::string _Input)
{
  if(!_Input.length())
  {
    std::cerr << "PhragDat error: RemovePathFromParents: given string empty" << std::endl;
    return "";
  }

  std::string Input = "/";
  Input += _Input;
  std::string Path;

  if(Input.back()=='/')
  {
    Input.pop_back();
  }

  int CharCount = 0;
  for(int iInputLen = (int)Input.length(); iInputLen > 0; --iInputLen)
  {
    if(Input[iInputLen]=='/')
    {
      break;
    }

    else
    {
      CharCount++;
    }
  }

  for(int iInputChar = 0; iInputChar < (((int)Input.length()) - CharCount); ++iInputChar)
  {
    Path.push_back(Input[iInputChar]);
  }

  if(Path[0] == '/')
  {
    std::string Path2 = Path;
    Path.clear();

    for(int iPath2Char = 1; iPath2Char < (int)Path2.length(); ++iPath2Char)
    {
      Path.push_back(Path2[iPath2Char]);
    }
  }

  return Path;
}*/

//================================
// PHDC_File (Input File Info)
//================================
struct PHDC_File
{
  std::string InputPath; // full file path for input
  std::string DatPath; // path relative to .dat for contents
  uint64_t Address; // address inside .dat
  uint64_t Length; // file size in bytes
};

//================================
//    PHD_COMPILE
//================================
static int
PHD_COMPILE(std::string _Input,
            std::string _DatPath,
            std::string _CPath,
            std::string _Exclusions)
{
  // check input strings
  if(!_Input.length() || !_DatPath.length() || !_CPath.length())
  {
    std::cerr << "PhragDat Error: invalid input, see phragdat -h for help" << std::endl;
    return 1;
  }

  // check inputs are valid directories
  {
    DWORD ftyp = GetFileAttributesA(_Input.c_str());
    if(ftyp != FILE_ATTRIBUTE_DIRECTORY)
    {
      std::cerr << "PhragDat error: " << _Input << " is not a valid directory. Check read/write privileges or check the path is correct. Aborting." << std::endl;
      if(DEBUG_MODE) {std::cout << "ftyp reads: " << (int)ftyp << std::endl;}
      return 1;
    }
  }

  {
    DWORD ftyp = GetFileAttributesA(_DatPath.c_str());
    if(ftyp != FILE_ATTRIBUTE_DIRECTORY)
    {
      std::cerr << "PhragDat error: " << _DatPath << " is not a valid directory. Check read/write privileges or check the path is correct. Aborting." << std::endl;
      if(DEBUG_MODE) {std::cout << "ftyp reads: " << (int)ftyp << std::endl;}
      return 1;
    }
  }

  {
    DWORD ftyp = GetFileAttributesA(_CPath.c_str());
    if(ftyp != FILE_ATTRIBUTE_DIRECTORY)
    {
      std::cerr << "PhragDat error: " << _CPath << " is not a valid directory. Check read/write privileges or check the path is correct. Aborting." << std::endl;
      if(DEBUG_MODE) {std::cout << "ftyp reads: " << (int)ftyp << std::endl;}
      return 1;
    }
  }

  // output dat info
  std::string Dat_OutputPath; // full path & name
  std::string Dat_OutputName; // path minus parent dirs
  std::string Dat_SimpleName; // name with no path or ext
  std::string C_OutputPath; // full path & name

  Dat_OutputPath = _DatPath;
  std::filesystem::path p(_DatPath);
  Dat_OutputName = p.filename().string();

  Dat_OutputName = _Input;
  if(Dat_OutputName.back()=='/') {Dat_OutputName.pop_back();}
  Dat_OutputName.append(".dat");
  Dat_OutputName = PHD_RemoveParentsFromPath(Dat_OutputName);

  Dat_OutputPath = _DatPath;
  if(Dat_OutputPath.back()!='/') {Dat_OutputPath.push_back('/');}
  Dat_OutputPath.append(Dat_OutputName);

  Dat_SimpleName = Dat_OutputName;
  for(int i=0; i<4; i++) {Dat_SimpleName.pop_back();}
  for(int i=0; i<Dat_SimpleName.length(); i++) { if(Dat_SimpleName[i]==' ') {Dat_SimpleName[i] = '_';} }

  C_OutputPath = _CPath;
  if(C_OutputPath.back() != '/') {C_OutputPath.push_back('/');}
  C_OutputPath.append(Dat_SimpleName);
  C_OutputPath.append(".csv");

  // OutputDat DEBUG report
  if(DEBUG_MODE) {std::cout << "Dat_OutputPath: " << Dat_OutputPath << "\nDat_OutputName: " << Dat_OutputName << std::endl;}

  std::map<uint64_t, PHDC_File> MasterFileList;
  std::vector<std::string> MasterDirectoryList;
  uint64_t NewFileUID = 0;
  uint64_t CurrentDirectory = 0;

  // Populate Exclusions lists
  std::vector<std::string> FileExclusions;
  std::vector<std::string> ExtExclusions;
  std::vector<std::string> DirExclusions;

  if(_Exclusions.length())
  {
    std::string ExcludeStr;
    std::ifstream ExclusionFile;

    ExclusionFile.open(_Exclusions, std::ios::in | std::ios::binary);
    if(ExclusionFile.is_open())
    {
      std::cout << "Found exclusion list, importing..." << std::endl;
      char c;
      while(ExclusionFile.get(c))
      {
        if(c == '\n')
        {
          if(ExcludeStr[0]=='*') ExtExclusions.push_back(ExcludeStr);
          else if(ExcludeStr.back()=='/') DirExclusions.push_back(ExcludeStr);
          else FileExclusions.push_back(ExcludeStr);
          ExcludeStr.clear();
        }
        else ExcludeStr.push_back(c);
      }

      if(ExcludeStr.length())
      {
        if(ExcludeStr[0]=='*') ExtExclusions.push_back(ExcludeStr);
        else if(ExcludeStr.back()=='/') DirExclusions.push_back(ExcludeStr);
        else FileExclusions.push_back(ExcludeStr);
        ExcludeStr.clear();
      }

      ExclusionFile.close();

      // report to user
      for(int i=0; i<FileExclusions.size(); i++) {std::cout << "Adding File exclusion: " << FileExclusions[i] << std::endl;}
      for(int i=0; i<ExtExclusions.size(); i++) {std::cout << "Adding Extension exclusion: " << ExtExclusions[i] << std::endl;}
      for(int i=0; i<DirExclusions.size(); i++) {std::cout << "Adding Directory exclusion: " << DirExclusions[i] << std::endl;}

      // fix exclusion strings for string matching:

      // remove * from Ext (this is just for denoting exception type)
      for(int iExclusion = 0; iExclusion < ExtExclusions.size(); ++iExclusion)
      {
        std::string Str;

        for(int iChar = 0; iChar < ExtExclusions[iExclusion].length(); ++iChar)
        {
          if(ExtExclusions[iExclusion][iChar]!='*')
          {
            Str.push_back(ExtExclusions[iExclusion][iChar]);
          }
        }

        ExtExclusions[iExclusion].clear();
        ExtExclusions[iExclusion] = Str;
      }

      // remove '/' from directory end (this is just for denoting exception type)
      for(int iChar = 0; iChar < DirExclusions.size(); ++iChar)
      {
        if(DirExclusions[iChar].back() == '/')
        {
          DirExclusions[iChar].pop_back();
        }
      }

      // remove pesky carriage returns from windows encoded text
      for(int i=0; i<FileExclusions.size(); i++) if(FileExclusions[i].back()==0xd) FileExclusions[i].pop_back();
      for(int i=0; i<ExtExclusions.size(); i++) if(ExtExclusions[i].back()==0xd) ExtExclusions[i].pop_back();
      for(int i=0; i<DirExclusions.size(); i++) if(DirExclusions[i].back()==0xd) DirExclusions[i].pop_back();
    }

    else //ExclusionFile didnt open
    {
      std::cerr << "Unable to open Exclusions list, ignoring and continuing" << std::endl;
    }
  }

  // AddressCounter
  uint64_t AddressCounter = 8;

  // root directory
  MasterDirectoryList.push_back(_Input);

  // loop per directory
  bool doGetDirList = 1;
  while(doGetDirList)
  {
    std::vector<std::string> FileList;
    std::vector<std::string> DirectoryList;

    PHD_GetFileAndDirectoryList(MasterDirectoryList[CurrentDirectory], FileList, DirectoryList);

    // File Exclusions
    for(int iExclusion = 0; iExclusion < FileExclusions.size(); ++iExclusion)
    {
      std::vector<std::string>::iterator iFileList = FileList.begin();
      while(iFileList != FileList.end())
      {
        if((*iFileList).find(FileExclusions[iExclusion]) != std::string::npos)
        {
          if(DEBUG_MODE) {std::cout << "Removing exception: " << (*iFileList) << std::endl;}

          iFileList = FileList.erase(iFileList);
        }
        else iFileList++;
      }
    }

    // Ext Exclusions
    for(int iExclusion = 0; iExclusion < ExtExclusions.size(); ++iExclusion)
    {
      std::vector<std::string>::iterator iFileList = FileList.begin();
      while(iFileList != FileList.end())
      {
        bool erase = 0;
        size_t pos = (*iFileList).find(ExtExclusions[iExclusion]);

        if(pos != std::string::npos)
        {
          // make sure found .ext is at end of filename
          if(pos == ((*iFileList).length()) - (ExtExclusions[iExclusion].length()))
          {
            erase = 1;
          }
        }

        if(erase)
        {
          if(DEBUG_MODE) {std::cout << "Removing exception: " << (*iFileList) << std::endl;}

          iFileList = FileList.erase(iFileList);
        }

        else
        {
          iFileList++;
        }
      }
    }

    // Directory Exclusions
    for(int iExclusion = 0; iExclusion < DirExclusions.size(); ++iExclusion)
    {
      std::vector<std::string>::iterator iDirectoryList = DirectoryList.begin();
      while(iDirectoryList != DirectoryList.end())
      {
        if((*iDirectoryList).find(DirExclusions[iExclusion]) != std::string::npos)
        {
          if(DEBUG_MODE) {std::cout << "Removing exception: " << (*iDirectoryList) << std::endl;}

          iDirectoryList = DirectoryList.erase(iDirectoryList);
        }

        else
        {
          iDirectoryList++;
        }
      }
    }

    // add directories to masterlist
    for(int iDirectory = 0; iDirectory < DirectoryList.size(); ++iDirectory)
    {
      // skip empty
      if(PathIsDirectoryEmptyA(DirectoryList[iDirectory].c_str()))
      {
        if(DEBUG_MODE) {std::cout << DirectoryList[iDirectory] << " is empty, skipping..." << std::endl;}

        continue;
      }

      // make sure no duplicates
      bool SkipDir = 0;

      for(int iMasterDirectory = 0; iMasterDirectory < MasterDirectoryList.size(); ++iMasterDirectory)
      {
        if(MasterDirectoryList[iMasterDirectory] == DirectoryList[iDirectory])
        {
          SkipDir = 1;
        }
      }

      if(SkipDir && DEBUG_MODE)
      {
        std::cout << DirectoryList[iDirectory] << " exists in list, skipping..." << std::endl;
      }

      if(!SkipDir)
      {
        MasterDirectoryList.push_back(DirectoryList[iDirectory]);
      }
    }

    // add files to masterlist
    for(int iFile = 0; iFile < FileList.size(); ++iFile)
    {
      // skip 0 length
      uint64_t Length = (uint64_t)std::filesystem::file_size(FileList[iFile]);
      if(!Length)
      {
        if(DEBUG_MODE) {std::cout << FileList[iFile] << " is empty, skipping..." << std::endl;}

        continue;
      }

      MasterFileList[NewFileUID].InputPath = FileList[iFile];

      // remove _Input from DatPath
      std::string DatPath = FileList[iFile];
      for(int iChar = (int)_Input.length() + 1; iChar < (int)FileList[iFile].length(); ++iChar)
      {
        MasterFileList[NewFileUID].DatPath.push_back(FileList[iFile][iChar]);
      }

      MasterFileList[NewFileUID].Address = AddressCounter;
      MasterFileList[NewFileUID].Length = Length;

      // iterate for next file
      AddressCounter += Length+1; // fwrite puts 1 byte pad (0xff) between each file
      NewFileUID++;
    }

    // iterate or end loop
    if(CurrentDirectory+1 == MasterDirectoryList.size())
    {
      doGetDirList = 0;
    }

    else
    {
      CurrentDirectory++;
    }
  }

  // debug report
  if(DEBUG_MODE)
  {
    for(uint64_t iFile = 0; iFile < (uint64_t)MasterFileList.size(); ++iFile)
    {
      std::cout << "\nInput File: " << MasterFileList[iFile].InputPath
      << "\nDatPath: " << MasterFileList[iFile].DatPath
      << "\nLength: " << MasterFileList[iFile].Length
      << std::endl;
    }
  }

  // write .dat file
  // (using C's FILE* instead of C++ filestream
  // because its simpler when dealing with raw bytes,
  // C++ filestream tends to mess with signed/unsigned which I can't be bothered to work around)
  {
    FILE *OutputDatFile;
    OutputDatFile = fopen(Dat_OutputPath.c_str(), "wb");

    if(!OutputDatFile)
    {
      std::cerr << "PhragDat error: failed to write " << Dat_OutputPath << ", check read/write privileges or spelling and try again, exiting..." << std::endl;
      return 1;
    }

    // write header
    {
      char Header[8] = {0x50, 0x48, 0x52, 0x44, 0x41, 0x54, VER_MAJ, VER_MIN};
      fwrite(Header, 1, 8, OutputDatFile);
    }

    // write data
    for(int iFile = 0; iFile < MasterFileList.size(); ++iFile)
    {
      std::cout << "Writing: " << MasterFileList[iFile].InputPath << std::endl;

      FILE *InputFile;
      InputFile = fopen(MasterFileList[iFile].InputPath.c_str(), "rb");

      if(!InputFile)
      {
        std::cerr << "PhragDat error: failed reading " << MasterFileList[iFile].InputPath << ", exiting..." << std::endl;
        fclose(OutputDatFile);
        std::remove(Dat_OutputPath.c_str());
        return 1;
      }

      int c;
      do
      {
        c = fgetc (InputFile);
        fputc(c, OutputDatFile);
      }
      while (c != EOF);

      fclose(InputFile);
    }

    fclose(OutputDatFile);
  }

  std::cout << Dat_OutputPath << " written" << std::endl;
  std::cout << "Writing: " << C_OutputPath << "..." << std::endl;

  // write contents.csv file
  {
    FILE *OutputCSVFile;
    OutputCSVFile = fopen(C_OutputPath.c_str(), "wb");

    if(!OutputCSVFile)
    {
      std::cerr << "PhragDat error: failed writing " << C_OutputPath << ", exiting..." << std::endl;
      return 1;
    }

    // write header
    {
      std::stringstream ssHeader;
      ssHeader << "\"PHRDAT\"," << (int)VER_MAJ << "," << (int)VER_MIN << "\n";
      std::string FileContents = ssHeader.str();
      fwrite(&FileContents[0], 1, FileContents.length(), OutputCSVFile);
    }

    // write file contents
    for(int iFile = 0; iFile < MasterFileList.size(); ++iFile)
    {
      std::stringstream ssContents;
      ssContents << "\"" << MasterFileList[iFile].DatPath
      << "\"," << (uint64_t)MasterFileList[iFile].Address
      << "," << (uint64_t)MasterFileList[iFile].Length << "\n";
      std::string FileContents = ssContents.str();
      fwrite(&FileContents[0], 1, FileContents.length(), OutputCSVFile);
    }

    fclose(OutputCSVFile);

  }

  std::cout << C_OutputPath << " written" << std::endl;

  return 0;
}

/* // just testing some stuff, ignore this
struct DatFileMember {size_t Address; size_t Length;};
std::map<std::string,DatFileMember> nspdat =
{
	{"Days.ttf", {8,36012}}
};
*/

//================================
//    Main
//================================
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
  std::map<int,bool> ArgIsProcessed; // check all args processed

  // process args
  for(int iArg = 1; iArg < argc; ++iArg)
  {
    std::string ThisArg = argv[iArg];
    ArgIsProcessed[iArg] = 0;

    // print help
    if(ThisArg[0] == '-' && ThisArg[1] == 'h')
    {
      std::cout << PHD_HelpStr << std::endl;
      return 0;
    }

    // print version
    if(ThisArg[0] == '-' && ThisArg[1] == 'v')
    {
      // set console page 850
      UINT ConsolePage = GetConsoleOutputCP();
      if(ConsolePage != 850) SetConsoleOutputCP(850);

      //print version
      std::cout << PHD_GetVersion() << std::endl;

      // return console page back to what it was
      if(GetConsoleOutputCP() != ConsolePage) SetConsoleOutputCP(ConsolePage);
      return 0;
    }

    // set input
    if(ThisArg[0] == '-' && ThisArg[1] == 'i')
    {
      if(ThisArg.length() > 2)
      {
        for(int iChar= 2; iChar < ThisArg.length(); ++iChar)
        {
          arg_input.push_back(ThisArg[iChar]);
          ArgIsProcessed[iArg] = 1;
        }
      }
    }

    // set datpath
    if(ThisArg[0] == '-' && ThisArg[1] == 'd')
    {
      if(ThisArg.length() > 2)
      {
        for(int iChar = 2; iChar < ThisArg.length(); ++iChar)
        {
          arg_datpath.push_back(ThisArg[iChar]);
          ArgIsProcessed[iArg] = 1;
        }
      }
    }

    // set cpath
    if(ThisArg[0] == '-' && ThisArg[1] == 'c')
    {
      if(ThisArg.length() > 2)
      {
        for(int iChar = 2; iChar < ThisArg.length(); ++iChar)
        {
          arg_cpath.push_back(ThisArg[iChar]);
          ArgIsProcessed[iArg] = 1;
        }
      }
    }

    // set exclusions
    if(ThisArg[0] == '-' && ThisArg[1] == 'e')
    {
      if(ThisArg.length() > 2)
      {
        for(int iChar = 2; iChar < ThisArg.length(); ++iChar)
        {
          arg_exclusions.push_back(ThisArg[iChar]);
          ArgIsProcessed[iArg] = 1;
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
  for(int iArg = 1; iArg < argc; ++iArg)
  {
    if(!ArgIsProcessed[iArg])
    {
      std::cerr << "PhragDat Error: unknown argument: " << argv[iArg] << std::endl;
      return 1;
    }
  }

  int ecode = PHD_COMPILE(arg_input, arg_datpath, arg_cpath, arg_exclusions);

  return ecode;
}