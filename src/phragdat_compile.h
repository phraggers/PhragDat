//====================================
// PhragDat Datafile Manager
// phragdat_compile.h
// C++17 Windows 64-bit
//====================================
// (c) Phragware 2020
//====================================

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
int PHD_COMPILE(std::string _Input, std::string _DatPath, std::string _CPath, std::string _Exclusions)
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
      if(DEBUG_MODE) std::cout << "ftyp reads: " << (int)ftyp << std::endl;
      return 1;
    }
  }

  {
    DWORD ftyp = GetFileAttributesA(_DatPath.c_str());
    if(ftyp != FILE_ATTRIBUTE_DIRECTORY)
    {
      std::cerr << "PhragDat error: " << _DatPath << " is not a valid directory. Check read/write privileges or check the path is correct. Aborting." << std::endl;
      if(DEBUG_MODE) std::cout << "ftyp reads: " << (int)ftyp << std::endl;
      return 1;
    }
  }

  {
    DWORD ftyp = GetFileAttributesA(_CPath.c_str());
    if(ftyp != FILE_ATTRIBUTE_DIRECTORY)
    {
      std::cerr << "PhragDat error: " << _CPath << " is not a valid directory. Check read/write privileges or check the path is correct. Aborting." << std::endl;
      if(DEBUG_MODE) std::cout << "ftyp reads: " << (int)ftyp << std::endl;
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
  if(Dat_OutputName.back()=='/') Dat_OutputName.pop_back();
  Dat_OutputName.append(".dat");
  Dat_OutputName = PHD_RemoveParentsFromPath(Dat_OutputName);

  Dat_OutputPath = _DatPath;
  if(Dat_OutputPath.back()!='/') Dat_OutputPath.push_back('/');
  Dat_OutputPath.append(Dat_OutputName);

  Dat_SimpleName = Dat_OutputName;
  for(int i=0; i<4; i++) Dat_SimpleName.pop_back();
  for(int i=0; i<Dat_SimpleName.length(); i++) if(Dat_SimpleName[i]==' ') Dat_SimpleName[i] = '_';

  C_OutputPath = _CPath;
  if(C_OutputPath.back() != '/') C_OutputPath.push_back('/');
  C_OutputPath.append(Dat_SimpleName);
  C_OutputPath.append(".c");

  // OutputDat DEBUG report
  if(DEBUG_MODE) std::cout << "Dat_OutputPath: " << Dat_OutputPath << "\nDat_OutputName: " << Dat_OutputName << std::endl;

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
    std::ifstream ifile;

    ifile.open(_Exclusions, std::ios::in | std::ios::binary);
    if(ifile.is_open())
    {
      std::cout << "Found exclusion list, importing..." << std::endl;
      char c;
      while(ifile.get(c))
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

      ifile.close();

      // report to user
      for(int i=0; i<FileExclusions.size(); i++) std::cout << "Adding File exclusion: " << FileExclusions[i] << std::endl;
      for(int i=0; i<ExtExclusions.size(); i++) std::cout << "Adding Extension exclusion: " << ExtExclusions[i] << std::endl;
      for(int i=0; i<DirExclusions.size(); i++) std::cout << "Adding Directory exclusion: " << DirExclusions[i] << std::endl;

      // fix exclusion strings for string matching:

      // remove * from Ext (this is just for denoting exception type)
      for(int i=0; i<ExtExclusions.size(); i++)
      {
        std::string Str;
        for(int j=0; j<ExtExclusions[i].length(); j++) if(ExtExclusions[i][j]!='*') Str.push_back(ExtExclusions[i][j]);
        ExtExclusions[i].clear();
        ExtExclusions[i] = Str;
      }

      // remove '/' from directory end (this is just for denoting exception type)
      for(int i=0; i<DirExclusions.size(); i++) if(DirExclusions[i].back()=='/') DirExclusions[i].pop_back();

      // remove pesky carriage returns from windows encoded text
      for(int i=0; i<FileExclusions.size(); i++) if(FileExclusions[i].back()==0xd) FileExclusions[i].pop_back();
      for(int i=0; i<ExtExclusions.size(); i++) if(ExtExclusions[i].back()==0xd) ExtExclusions[i].pop_back();
      for(int i=0; i<DirExclusions.size(); i++) if(DirExclusions[i].back()==0xd) DirExclusions[i].pop_back();
    }

    else std::cerr << "Unable to open Exclusions list, ignoring and continuing" << std::endl;
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
    for(int i=0; i<FileExclusions.size(); i++)
    {
      std::vector<std::string>::iterator it = FileList.begin();
      while(it != FileList.end())
      {
        if((*it).find(FileExclusions[i]) != std::string::npos)
        {
          if(DEBUG_MODE) std::cout << "Removing exception: " << (*it) << std::endl;
          it = FileList.erase(it);
        }
        else it++;
      }
    }

    // Ext Exclusions
    for(int i=0; i<ExtExclusions.size(); i++)
    {
      std::vector<std::string>::iterator it = FileList.begin();
      while(it != FileList.end())
      {
        bool erase = 0;
        size_t pos = (*it).find(ExtExclusions[i]);
        if(pos != std::string::npos)
        {
          // make sure found .ext is at end of filename
          if(pos == ((*it).length())-(ExtExclusions[i].length())) erase = 1;
        }
        if(erase)
        {
          if(DEBUG_MODE) std::cout << "Removing exception: " << (*it) << std::endl;
          it = FileList.erase(it);
        }
        else it++;
      }
    }

    // Directory Exclusions
    for(int i=0; i<DirExclusions.size(); i++)
    {
      std::vector<std::string>::iterator it = DirectoryList.begin();
      while(it != DirectoryList.end())
      {
        if((*it).find(DirExclusions[i]) != std::string::npos)
        {
          if(DEBUG_MODE) std::cout << "Removing exception: " << (*it) << std::endl;
          it = DirectoryList.erase(it);
        }
        else it++;
      }
    }

    // add directories to masterlist
    for(int i=0; i<DirectoryList.size(); i++)
    {
      // skip empty
      if(PathIsDirectoryEmptyA(DirectoryList[i].c_str()))
      {
        if(DEBUG_MODE) std::cout << DirectoryList[i] << " is empty, skipping..." << std::endl;
        continue;
      }

      // make sure no duplicates
      bool SkipDir = 0;

      for(int j=0; j<MasterDirectoryList.size(); j++) if(MasterDirectoryList[j] == DirectoryList[i]) SkipDir = 1;

      if(SkipDir && DEBUG_MODE) std::cout << DirectoryList[i] << " exists in list, skipping..." << std::endl;

      if(!SkipDir)
      {
        MasterDirectoryList.push_back(DirectoryList[i]);
      }
    }

    // add files to masterlist
    for(int i=0; i<FileList.size(); i++)
    {
      // skip 0 length
      uint64_t Length = (uint64_t)std::filesystem::file_size(FileList[i]);
      if(Length == 0)
      {
        if(DEBUG_MODE) std::cout << FileList[i] << " is empty, skipping..." << std::endl;
        continue;
      }

      MasterFileList[NewFileUID].InputPath = FileList[i];

      // remove _Input from DatPath
      std::string DatPath = FileList[i];
      for(int j=(int)_Input.length()+1; j<(int)FileList[i].length(); j++) MasterFileList[NewFileUID].DatPath.push_back(FileList[i][j]);

      MasterFileList[NewFileUID].Address = AddressCounter;
      MasterFileList[NewFileUID].Length = Length;

      // iterate for next file
      AddressCounter += Length+1; // fwrite puts 1 byte pad (0xff) between each file
      NewFileUID++;
    }

    // iterate or end loop
    if(CurrentDirectory+1 == MasterDirectoryList.size()) doGetDirList = 0;
    else CurrentDirectory++;
  }

  // debug report
  if(DEBUG_MODE)
  {
    for(uint64_t i=0; i<(uint64_t)MasterFileList.size(); i++)
    {
      std::cout << "\nInput File: " << MasterFileList[i].InputPath
      << "\nDatPath: " << MasterFileList[i].DatPath
      << "\nLength: " << MasterFileList[i].Length
      << std::endl;
    }
  }

  // write .dat file (using C FILE* because its easier with raw data)
  {
    FILE *ODatFile;
    ODatFile = fopen(Dat_OutputPath.c_str(), "wb");

    if(ODatFile == NULL)
    {
      std::cerr << "PhragDat error: failed to write " << Dat_OutputPath << ", check read/write privileges or spelling and try again, exiting..." << std::endl;
      return 1;
    }

    // write header
    {
      const char Header[8] = {0x50, 0x48, 0x52, 0x44, 0x41, 0x54, VER_MAJ, VER_MIN};
      fwrite(Header, 1, 8, ODatFile);
    }

    // write data
    for(int i=0; i<MasterFileList.size(); i++)
    {
      std::cout << "Writing: " << MasterFileList[i].InputPath << std::endl;

      FILE *IFile;
      IFile = fopen(MasterFileList[i].InputPath.c_str(), "rb");

      if(IFile == NULL)
      {
        std::cerr << "PhragDat error: failed reading " << MasterFileList[i].InputPath << ", exiting..." << std::endl;
        fclose(ODatFile);
        std::remove(Dat_OutputPath.c_str());
        return 1;
      }

      int c;
      do
      {
        c = fgetc (IFile);
        fputc(c, ODatFile);
      }
      while (c != EOF);

      fclose(IFile);
    }

    fclose(ODatFile);
  }

  std::cout << Dat_OutputPath << " written" << std::endl;
  std::cout << "Writing: " << C_OutputPath << "..." << std::endl;

  // write contents.c file (using fstream because its easier with c++ strings)
  {
    std::string DatIDTag = "PHR_DAT_" + Dat_SimpleName;
    std::string Instructions = "/*\nTo use:\nint PHR_DatLoadFile(std::string _DatPath, std::string _File, std::string &Buffer);\nReturns 1 if there is an error, else Returns 0 on success and puts file data in Buffer\n*/\n";

    std::string PHR_DatLoadFile_Str;
    {
      std::stringstream ss;
      ss << "int PHR_DatLoadFile(std::string _DatPath, std::string _File, std::string &Buffer) { std::string DatName; FILE *IDat; IDat = fopen(_DatPath.c_str(), \"rb\"); if(IDat == NULL) return 1; unsigned char HeaderTag[6]; unsigned char ExpectedHeaderTag[] = \"PHRDAT\"; fread(HeaderTag, 1, 6, IDat); for(int i=0; i<6; i++) if(HeaderTag[i] != ExpectedHeaderTag[i]) { fclose(IDat); return 1; } unsigned char Version[2]; unsigned char ExpectedVersion[] = {";
      ss << DatIDTag << "_VER_MAJ, " << DatIDTag << "_VER_MIN}; fread(Version, 1, 2, IDat); for(int i=0; i<2; i++) if(Version[i] != ExpectedVersion[i]) { fclose(IDat); return 1; } fseek(IDat, PHR_Dats[DatName][_File].FileAddress, SEEK_SET); fread(&Buffer[0], 1, PHR_Dats[DatName][_File].FileLength, IDat); fclose(IDat); return 0;}";
      PHR_DatLoadFile_Str = ss.str();
    }

    std::ofstream ofile;
    ofile.open(C_OutputPath, std::ios::out | std::ios::binary);
    if(!ofile.is_open())
    {
      std::cerr << "PhragDat error: failed to write " << C_OutputPath << ", check read/write privileges or spelling and try again, exiting..." << std::endl;
      return 1;
    }

    ofile << Instructions << "\n#ifndef " << DatIDTag << "_c"
    << "\n#define " << DatIDTag << "_c"
    << "\n#define " << DatIDTag << "_VER_MAJ " << VER_MAJ
    << "\n#define " << DatIDTag << "_VER_MIN " << VER_MIN
    << "\n#ifdef PHR_DAT_MANAGER"
    << "\nint " << DatIDTag << "_Included = " << DatIDTag << "AddToList();"
    << "\n#endif //ifdef PHR_DAT_MANAGER"
    << "\n#ifndef PHR_DAT_MANAGER\n#define PHR_DAT_MANAGER\n#include <string>\n#include <map>\n#include <cstdio>"
    << "\nstruct PHR_DatFileContent { unsigned long long FileAddress; unsigned long long FileLength;};"
    << "\nstd::map<std::string, std::map<std::string, PHR_DatFileContent>> PHR_Dats;"
    << "\nint " << DatIDTag << "_Included = " << DatIDTag << "AddToList();"
    << "\n" << PHR_DatLoadFile_Str << "\n#endif //ifndef PHR_DAT_MANAGER"
    << "\nint " << DatIDTag << "AddToList() {";

    for(int i=0; i<MasterFileList.size(); i++)
    {
      ofile << "\nPHR_Dats[\"" << Dat_OutputName << "\"][\"" << MasterFileList[i].DatPath << "\"].FileAddress = 0x" << std::hex << (uint64_t)MasterFileList[i].Address << ";\nPHR_Dats[\"" << Dat_OutputName << "\"][\"" << MasterFileList[i].DatPath << "\"].FileLength = 0x" << std::hex << (uint64_t)MasterFileList[i].Length << ";";
    }

    ofile << "\nreturn 1;}\n#endif //" << DatIDTag << "_c" << std::endl;

    ofile.close();
  }

  std::cout << C_OutputPath << " written" << std::endl;

  return 0;
}

/*
int PHR_DatLoadFile(std::string _DatPath, std::string _File, std::string &Buffer)
{
  std::string DatName;
  FILE *IDat;
  IDat = fopen(_DatPath.c_str(), "rb");
  if(IDat == NULL) return 1;
  unsigned char HeaderTag[6];
  unsigned char ExpectedHeaderTag[] = "PHRDAT";
  fread(HeaderTag, 1, 6, IDat);
  for(int i=0; i<6; i++) if(HeaderTag[i] != ExpectedHeaderTag[i]) { fclose(IDat); return 1; }
  unsigned char Version[2];
  unsigned char ExpectedVersion[] = {VER_MAJ, VER_MIN};
  fread(Version, 1, 2, IDat);
  for(int i=0; i<2; i++) if(Version[i] != ExpectedVersion[i]) { fclose(IDat); return 1; }
  fseek(IDat, PHR_Dats[DatName][_File].FileAddress, SEEK_SET);
  fread(&Buffer[0], 1, PHR_Dats[DatName][_File].FileLength, IDat);
  fclose(IDat);
  return 0;
}
*/