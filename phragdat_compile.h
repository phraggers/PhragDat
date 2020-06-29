//====================================
// PhragDat Datafile Manager
// phragdat_compile.h
// C++17 Windows 64-bit
//====================================
// (c) Phragware 2020
//====================================

// phragdat -c -i"input/directory" -o"output/directory" -f"exclusions.txt"(optional)
// inputs expected: -i directory, -o directory unless .dat specified, -f path to exceptions text file
// if no .dat specified in -o then output is inputdirectory.dat

// PHDC: PHragDat Compile

//================================
// PHDC_File (Input File Info)
//================================
struct PHDC_File
{
  uint64_t UID;
  std::string Name;
  std::string Ext; // .ext (includes dot)
  std::string FullPath; // full file path for input
  std::string PathRoot; // full input path minus filename
  std::string DatRootPath; // where file will be inside dat
  std::string DatFullPath; // DatRootPath + Name + Ext
  uint64_t Length; // file size in bytes
};

//================================
// PHDC_Directory (Input Dir Info)
//================================
struct PHDC_Directory
{
  uint64_t UID;
  std::string InputFullPath; // path with input as root
  std::string DatFullPath; // path with dat as root
};

//===================================
// PHDC_OutputDat (output file info)
//===================================
struct PHDC_OutputDat
{
  std::string OFileName; // output.dat
  std::string OPathRoot; // where .dat is to be written
  std::string OFullPath; // pathroot + filename to be written
  std::string DatRoot; // relative root for all dat contents
  std::string IParent; // reference for where DatRoot is for compilation
};

//================================
//  Data (struct to write data)
//================================
struct PHDC_Data
{
  std::string FullPath;
  std::vector<uint8_t> Data;

  void Write()
  {
    if(!Data.size()) return;
    FILE *File;
    File = fopen(FullPath.c_str(), "ab");
    fwrite(&Data[0], 1, Data.size(), File);
    fclose(File);
    Data.clear();
  }

  // write full size at EOF for filecheck when reading
  void WriteEnd()
  {
    // NOTE: Writing the code below made me realize that using "ADDRESS_SIZE" is kinda pointless without retyping ALL of my uint64_t to a new type, Address_t or something, but oh well.
    Write();
    uint64_t DatLength = (uint64_t)std::filesystem::file_size(FullPath);
    DatLength += ADDRESS_SIZE;
    PHD_Convert_uint64_to_uint8(DatLength, Data);
    Write();
  }

  // read file to buffer
  void file(std::string _path)
  {
    std::ifstream ifile;
    ifile.open(_path, std::ios::in | std::ios::binary);
    char c;
    while(ifile.get(c)) sc(c);
    ifile.close();
  }

  // add String to buffer
  void str(std::string _str)
  {
    for(int i=0; i<_str.length(); i++) sc(_str[i]);
  }

  // add uint8 vector to buffer
  void vec(std::vector<uint8_t> &_vec)
  {
    for(int i=0; i<_vec.size(); i++) uc(_vec[i]);
  }

  // add signed char to buffer
  void sc(char _c)
  {
    uint8_t u8 = static_cast<uint8_t>(_c);
    Data.push_back(u8);
    if(Data.size()>=BUFFER_WRITE_SIZE) Write();
  }

  // add uint8 to buffer
  void uc(uint8_t _c)
  {
    Data.push_back(_c);
    if(Data.size()>=BUFFER_WRITE_SIZE) Write();
  }
};

//================================
//    PHD_COMPILE
//================================
int PHD_COMPILE(std::string _Input, std::string _Output, std::string _Exclude)
{
  // check inputs
  if(!_Input.length() || !_Output.length())
  {
    std::cerr << "PhragDat Error: invalid input, see phragdat -h for help" << std::endl;
    return 1;
  }

  {
    DWORD ftyp = GetFileAttributesA(_Input.c_str());
    if(ftyp != FILE_ATTRIBUTE_DIRECTORY)
    {
      std::cerr << "PhragDat error: " << _Input << " is not a valid directory. Check read/write priviledges or check the path is correct. Aborting." << std::endl;
      if(DEBUG_MODE) std::cout << "ftyp reads: " << (int)ftyp << std::endl;
      return 1;
    }
  }

  // output dat info
  PHDC_OutputDat OutputDat;
  bool OutputIsDat = 0;

  // output: check if end of string == .dat (allow lowercase & uppercase)
  if(_Output[_Output.length()-4]=='.' && (_Output[_Output.length()-3]=='d' || _Output[_Output.length()-3]=='D') && (_Output[_Output.length()-2]=='a' || _Output[_Output.length()-2]=='A') && (_Output[_Output.length()-1]=='t' || _Output[_Output.length()-1=='T']))
  {
    // ensure filename isnt just ".dat"
    if(_Output.find("/.dat")!=std::string::npos || _Output.length()<5)
    {
      std::cerr << "PhragDat error: .dat not given a name! Exiting..." << std::endl;
      return 1;
    }

    // check writable
    std::ofstream ofile;
    ofile.open(_Output.c_str(), std::ios::out | std::ios::binary);
    if(!ofile.is_open())
    {
      std::cerr << "PhragDat error: failed to write " << _Output << ", please check write permissions or that target directory exists, see phragdat -h for help. Aborting." << std::endl;
      return 1;
    }

    else
    {
      ofile.close();
      std::remove(_Output.c_str());
      OutputIsDat = 1;
    }
  }

  //output: if not dat, check is directory
  else // not .dat
  {
    if(GetFileAttributesA(_Output.c_str()) != (DWORD)FILE_ATTRIBUTE_DIRECTORY)
    {
      std::cerr << "PhragDat error: " << _Output << " is not a valid directory, please see phragdat -h for help. Aborting." << std::endl;
      return 1;
    }

    else // check writable
    {
      std::string ofilepath = _Output;
      if(ofilepath.back()!='/') ofilepath.push_back('/');

      std::string inputname = PHD_RemoveParentsFromPath(_Input);
      ofilepath.append(inputname);

      if(ofilepath.back()=='/') ofilepath.pop_back();
      ofilepath.append(".dat");

      std::ofstream ofile;
      ofile.open(ofilepath, std::ios::out | std::ios::binary);

      if(!ofile.is_open())
      {
        std::cerr << "PhragDat error: failed to write " << ofilepath << ", please check write permissions or that target directory exists, see phragdat -h for help. Aborting." << std::endl;
        return 1;
      }

      else
      {
        ofile.close();
        std::remove(ofilepath.c_str());
      }
    }
  }

  // Populate OutputDat struct
  {

    // IParent : parent of input directory
    {
      std::string InputDir;
      if(_Input[0] != '.') InputDir = "./" + _Input;
      else InputDir = _Input;
      OutputDat.IParent = PHD_RemovePathFromParents(InputDir);
      if(OutputDat.IParent.back()!= '/') OutputDat.IParent.push_back('/');
    }

    if(OutputIsDat)
    {
      // populate OutputDat info
      OutputDat.OFullPath = _Output;
      std::filesystem::path p(_Output);
      OutputDat.OFileName = p.filename().string();
      OutputDat.OPathRoot = _Output;
      for(int j=0; j<OutputDat.OFileName.length(); j++) OutputDat.OPathRoot.pop_back();

      // if pathroot is empty give it a relative path
      if(!OutputDat.OPathRoot.length())
      {
        OutputDat.OPathRoot = "./";
        OutputDat.OFullPath = "./" + OutputDat.OFileName;
      }

      // DatRoot
      {
        OutputDat.DatRoot = PHD_RemoveParentsFromPath(_Input);
        if(OutputDat.DatRoot.back()!= '/') OutputDat.DatRoot.push_back('/');
      }
    }

    else // Output is directory
    {
      // populate outputdat info
      // Filename
      std::string InputDir = _Input;
      if(InputDir.back() == '/') InputDir.pop_back();

      int cc = 0;
      for(int i=(int)InputDir.length(); i>0; i--)
      {
        if(InputDir[i] == '/')
        {
          cc = i+1;
          break;
        }
      }

      for(int i=cc; i<InputDir.length(); i++) OutputDat.DatRoot.push_back(InputDir[i]);
      OutputDat.OFileName = OutputDat.DatRoot + ".dat";
      OutputDat.DatRoot.push_back('/');

      // PathRoot
      OutputDat.OPathRoot = _Output;
      if(OutputDat.OPathRoot.back() != '/') OutputDat.OPathRoot.push_back('/');

      // FullPath
      OutputDat.OFullPath = OutputDat.OPathRoot + OutputDat.OFileName;
    }
  }

  // OutputDat DEBUG report
  if(DEBUG_MODE) std::cout << "OutputDat OFileName: " << OutputDat.OFileName << "\nOutputDat OPathRoot: " << OutputDat.OPathRoot << "\nOutputDat OFullPath: " << OutputDat.OFullPath << "\nOutputDat DatRoot: " << OutputDat.DatRoot << "\nOutputDat IParent: " << OutputDat.IParent << std::endl;

  // make sure output is not within scope of input
  std::string TestPath;
  std::string TestFileName;

  {
    // create random file 8xHexChars name
    std::string RandomFileName;
    {
      std::vector<int> RandomNumbers;
      for(int i=0; i<8; i++) RandomNumbers.push_back(PHD_RandomRoll(1,255));
      std::stringstream ss;
      for(int i=0; i<8; i++) ss << std::hex << RandomNumbers[i];
      ss.flush();
      std::string FileName = ss.str();
      for(int i=0; i<8; i++) RandomFileName.push_back(FileName[i]);
    }

    // Create test file
    TestPath = OutputDat.OPathRoot + RandomFileName;
    std::ofstream ofile;
    ofile.open(TestPath, std::ios::out | std::ios::binary);
    ofile << "THIS IS A TEST" << std::endl;
    ofile.close();

    TestFileName = RandomFileName;
  }

  std::map<uint64_t, PHDC_File> MasterFileList;
  std::map<uint64_t, PHDC_Directory> MasterDirectoryList;
  uint64_t NewFileUID = 0;
  uint64_t NewDirectoryUID = 0;
  uint64_t CurrentDirectory = 0;

  // Populate Exclusions lists
  std::vector<std::string> FileExclusions;
  std::vector<std::string> ExtExclusions;
  std::vector<std::string> DirExclusions;

  if(_Exclude.length())
  {
    std::string ExcludeStr;
    std::ifstream ifile;

    ifile.open(_Exclude, std::ios::in | std::ios::binary);
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

  // root directory
  MasterDirectoryList[0].UID = 0;
  MasterDirectoryList[0].InputFullPath = _Input;
  MasterDirectoryList[0].DatFullPath = OutputDat.DatRoot;
  NewDirectoryUID++;

  // loop per directory
  bool doGetDirList = 1;
  while(doGetDirList)
  {
    std::vector<std::string> FileList;
    std::vector<std::string> DirectoryList;

    PHD_GetFileAndDirectoryList(MasterDirectoryList[CurrentDirectory].InputFullPath, FileList, DirectoryList);

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

      for(int j=0; j<MasterDirectoryList.size(); j++) if(MasterDirectoryList[j].InputFullPath == DirectoryList[i]) SkipDir = 1;

      if(SkipDir) if(DEBUG_MODE) std::cout << DirectoryList[i] << " exists in list, skipping..." << std::endl;

      if(!SkipDir)
      {
        // get Directory.InputFullPath
        if(DirectoryList[i][0]!='.') MasterDirectoryList[NewDirectoryUID].InputFullPath = "./" + DirectoryList[i];
        else MasterDirectoryList[NewDirectoryUID].InputFullPath = DirectoryList[i];

        // get Directory.DatFullPath
        MasterDirectoryList[NewDirectoryUID].DatFullPath = OutputDat.DatRoot;

        for(int j=((int)OutputDat.IParent.length()+(int)OutputDat.DatRoot.length()); j<MasterDirectoryList[NewDirectoryUID].InputFullPath.length(); j++)
        {
          MasterDirectoryList[NewDirectoryUID].DatFullPath.push_back(MasterDirectoryList[NewDirectoryUID].InputFullPath[j]);
        }

        NewDirectoryUID++;
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

      std::string Name;
      {
        std::filesystem::path p(FileList[i]);
        std::string name1 = p.stem().string();
        for(int j=0; j<name1.length(); j++) if(name1[j]!='\"') Name.push_back(name1[j]);
      }

      std::string Ext;
      {
        std::filesystem::path p(FileList[i]);
        std::string ext1 = p.extension().string();
        for(int j=0; j<ext1.length(); j++) if(!(ext1[j]=='\"')) Ext.push_back(ext1[j]);
        if(Ext.length()) for(int j=0; j<Ext.length(); j++) Ext[j] = (char)tolower(Ext[j]);
      }

      std::string FullPath;
      {
        if(FileList[i][0]!='.') FullPath = "./" + FileList[i];
        else FullPath = FileList[i];
      }

      std::string PathRoot;
      {
        if(FileList[i][0]=='.') PathRoot = FileList[i];
        else
        {
          PathRoot = "./";
          PathRoot.append(FileList[i]);
        }
        for(int j=0; j<(Name.length() + Ext.length()); j++) PathRoot.pop_back();
      }

      std::string DatRootPath;
      {
        for(int j=(int)OutputDat.IParent.length(); j<(int)PathRoot.length(); j++) DatRootPath.push_back(PathRoot[j]);
      }

      std::string DatFullPath;
      {
        DatFullPath = DatRootPath + Name + Ext;
      }

      MasterFileList[NewFileUID].UID = NewFileUID;
      MasterFileList[NewFileUID].Name = Name;
      MasterFileList[NewFileUID].Ext = Ext;
      MasterFileList[NewFileUID].FullPath = FullPath;
      MasterFileList[NewFileUID].PathRoot = PathRoot;
      MasterFileList[NewFileUID].DatRootPath = DatRootPath;
      MasterFileList[NewFileUID].DatFullPath = DatFullPath;
      MasterFileList[NewFileUID].Length = Length;
      NewFileUID++;
    }

    // iterate or end loop
    if(CurrentDirectory+1 == MasterDirectoryList.size()) doGetDirList = 0;
    else CurrentDirectory++;
  }

  // remove test file
  std::remove(TestPath.c_str());
  for(int i=0; i<(int)MasterFileList.size(); i++) if(MasterFileList[i].Name==TestFileName)
  {
    std::cerr << "PhragDat error: Output is within Input path! Exiting." << std::endl;
    return 1;
  }

  // debug report
  if(DEBUG_MODE)
  {
    for(uint64_t i=0; i<(uint64_t)MasterDirectoryList.size(); i++)
    {
      std::cout << "\nDirectory UID: " << i << "\nInputPath: " << MasterDirectoryList[i].InputFullPath << "\nDatPath: " << MasterDirectoryList[i].DatFullPath << std::endl;
    }

    for(uint64_t i=0; i<(uint64_t)MasterFileList.size(); i++)
    {
      std::cout << "\nFile UID: " << MasterFileList[i].UID
      << "\nName: " << MasterFileList[i].Name
      << "\nExt: " << MasterFileList[i].Ext
      << "\nFullPath: " << MasterFileList[i].FullPath
      << "\nPathRoot: " << MasterFileList[i].PathRoot
      << "\nDatRootPath: " << MasterFileList[i].DatRootPath
      << "\nDatFullPath: " << MasterFileList[i].DatFullPath
      << "\nLength: " << MasterFileList[i].Length
      << std::endl;
    }
  }

  std::cout << "\nFound " << MasterDirectoryList.size() << " Directories, " << MasterFileList.size() << " Files..." << std::endl;

  PHDC_Data Data;
  Data.FullPath = OutputDat.OFullPath;

  // Header: Tag (6)
  {
    std::vector<uint8_t> HeaderTag = {0x50, 0x48, 0x52, 0x44, 0x41, 0x54};
    Data.vec(HeaderTag);
  }

  // Header: Version (2)
  Data.uc((uint8_t)VER_MAJ);
  Data.uc((uint8_t)VER_MIN);

  // Header: Contents (24)
  uint64_t DirListAddr;
  {
    DirListAddr = 32; // 6+2+24
    std::vector<uint8_t> DirListAddrVec;
    PHD_Convert_uint64_to_uint8(DirListAddr, DirListAddrVec);
    Data.vec(DirListAddrVec);
  }

  uint64_t FileIndexAddr;
  {
    // DirListAddr + NumDir*2 + All Dir DatPath lengths
    FileIndexAddr = DirListAddr;
    FileIndexAddr += ((uint64_t)MasterDirectoryList.size()*2);
    for(int i=0; i<MasterDirectoryList.size(); i++) FileIndexAddr += (uint64_t)MasterDirectoryList[i].DatFullPath.length();
    std::vector<uint8_t> FileIndexAddrVec;
    PHD_Convert_uint64_to_uint8(FileIndexAddr, FileIndexAddrVec);
    Data.vec(FileIndexAddrVec);
  }

  uint64_t FileDataAddr;
  {
    // FileIndexAddr + numfiles*2 + numfiles*8 + numfiles*8 + allfiles.datfullpath.length
    FileDataAddr = FileIndexAddr;
    FileDataAddr += ((uint64_t)MasterFileList.size()*2) + ((uint64_t)MasterFileList.size()*8) + ((uint64_t)MasterFileList.size()*8);
    for(int i=0; i<MasterFileList.size(); i++) FileDataAddr += (uint64_t)MasterFileList[i].DatFullPath.length();
    std::vector<uint8_t> FileDataAddrVec;
    PHD_Convert_uint64_to_uint8(FileDataAddr, FileDataAddrVec);
    Data.vec(FileDataAddrVec);
  }

  // DirectoryList
  // per directory: PathLength(2) PathStr(x)
  for(int i=0; i<MasterDirectoryList.size(); i++)
  {
    uint16_t PathLength = (uint16_t)MasterDirectoryList[i].DatFullPath.length();
    std::vector<uint8_t> PathLenVec;
    PHD_Convert_uint16_to_uint8(PathLength, PathLenVec);
    Data.vec(PathLenVec);
    Data.str(MasterDirectoryList[i].DatFullPath);
  }

  // FileIndex
  // fileindex: (per file) PathLength(2), PathStr(x), FileAddr(8), FileLen(8)
  for(int i=0; i<MasterFileList.size(); i++)
  {
    // PathLen, PathStr
    {
      uint16_t PathLength = (uint16_t)MasterFileList[i].DatFullPath.length();
      std::vector<uint8_t> PathLenVec;
      PHD_Convert_uint16_to_uint8(PathLength, PathLenVec);
      Data.vec(PathLenVec);
      Data.str(MasterFileList[i].DatFullPath);
    }

    // FileAddr
    {
      // address = FileDataAddr + all prev file lengths
      uint64_t FileAddr = FileDataAddr;
      for(int j=0; j<i; j++) FileAddr += MasterFileList[j].Length;
      std::vector<uint8_t> FileAddrVec;
      PHD_Convert_uint64_to_uint8(FileAddr, FileAddrVec);
      Data.vec(FileAddrVec);
    }

    // FileLength
    {
      std::vector<uint8_t> FileLength;
      PHD_Convert_uint64_to_uint8(MasterFileList[i].Length, FileLength);
      Data.vec(FileLength);
    }
  }

  // body: raw file bytes
  for(int i=0; i<MasterFileList.size(); i++)
  {
    std::cout << "Writing " << MasterFileList[i].FullPath << " (" << i+1 << "/" << MasterFileList.size() << ")" << std::endl;
    Data.file(MasterFileList[i].FullPath);
  }

  Data.WriteEnd();

  std::cout << Data.FullPath << " written" << std::endl;
  return 0;
}