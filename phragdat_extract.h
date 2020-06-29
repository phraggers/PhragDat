//====================================
// PhragDat Datafile Manager
// phragdat_extract.h
// C++17 Windows 64-bit
//====================================
// (c) Phragware 2020
//====================================

// phragdat -e -i"input/dat.dat" -o"output/directory" -f"dat/file/or/dir"(optional)

//================================
//   WriteFile
//================================
int PHDE_WriteFile(std::string _DatFile, std::string _OutputPath, uint64_t Address, uint64_t Length)
{
  std::ifstream infile (_DatFile, std::ifstream::binary);
  std::ofstream outfile (_OutputPath, std::ofstream::binary);

  infile.seekg (Address,infile.beg);
  char* buffer = new char[Length];

  infile.read (buffer,Length);
  outfile.write (buffer,Length);
  delete[] buffer;

  outfile.close();
  infile.close();

  return 0;
}

//================================
//    PHD_EXTRACT
//================================
int PHD_EXTRACT(std::string _Input, std::string _Output, std::string _FileOrDir)
{
  // check inputs
  if(!_Input.length() || !_Output.length())
  {
    std::cerr << "PhragDat Error: invalid input, see phragdat -h for help" << std::endl;
    return 1;
  }

  // check output
  if(GetFileAttributesA(_Output.c_str()) != (DWORD)FILE_ATTRIBUTE_DIRECTORY)
  {
    if(!std::filesystem::create_directory(_Output))
    {
      std::cerr << "PhragDat error: Failed to create output directory " << _Output << ", check read/write permissions and try again. Aborting." << std::endl;
      return 1;
    }
    else std::cout << "Directory did not exist, created: " << _Output << std::endl;
  }

  uint64_t InputFileLength = 0;
  uint64_t DirListAddr = 0;
  uint64_t FileIndexAddr = 0;
  uint64_t FileDataAddr = 0;

  // check input file
  {
    switch(GetFileAttributesA(_Input.c_str()))
    {
      case INVALID_FILE_ATTRIBUTES:
      {
        std::cerr << "PhragDat error: Error reading " << _Input << " - Check filepath and try again" << std::endl;
        return 1;
      } break;

      case FILE_ATTRIBUTE_DIRECTORY:
      {
        std::cerr << "PhragDat error: " << _Input << " is a directory, expected a .dat file! exiting." << std::endl;
        return 1;
      } break;

      default:
      {
        InputFileLength = (uint64_t)std::filesystem::file_size(_Input);
        break;
      }
    }

    // EOF file length check
    {
      FILE *File;
      File = fopen(_Input.c_str(), "rb");
      fseek(File, -(long)ADDRESS_SIZE, SEEK_END);
      uint64_t DatSize = (uint64_t)std::filesystem::file_size(_Input);
      uint64_t DatSizeExpected;
      fread(&DatSizeExpected, sizeof(uint64_t), 1, File);
      fclose(File);

      if(DatSize != DatSizeExpected)
      {
        std::cerr << "Error reading " << _Input << ", unexpected EOF. File may be corrupted. Exiting." << std::endl;
        if(DEBUG_MODE) std::cout << "EOF check reads: " << DatSizeExpected << "\nActual File Size: " << DatSize << std::endl;
        return 1;
      }

      if(DEBUG_MODE) std::cout << "EOF Check success, file seems OK" << std::endl;
    }

    // I could hard-code these but its nice to have options to change later
    uint64_t HeadTagLen = 6;
    uint64_t HeadVerLen = 2;
    uint64_t ContentsDirListLen = ADDRESS_SIZE;
    uint64_t ContentsFileIndexLen = ADDRESS_SIZE;
    uint64_t ContentsFileDataLen = ADDRESS_SIZE;
    uint64_t HeaderLength = HeadTagLen + HeadVerLen + ContentsDirListLen + ContentsFileIndexLen + ContentsFileDataLen;

    // Min file size: Headersize
    if(InputFileLength < HeaderLength)
    {
      std::cerr << "PhragDat error: " << _Input << " is invalid, file too short! Exiting." << std::endl;
      return 1;
    }

    // report
    std::cout << "Input: " << _Input << "\nOutput: " << _Output << "\nFile/Dir: " << ((_FileOrDir.length()) ? _FileOrDir : "All") << std::endl;

    std::ifstream ifile;
    ifile.open(_Input, std::ios::in | std::ios::binary);
    if(!ifile.is_open())
    {
      std::cerr << "PhragDat error: " << _Input << " could not be opened. Exiting." << std::endl;
      return 1;
    }

    // Read Header+Contents
    std::string HeaderData;
    char c;
    while(ifile.get(c))
    {
      HeaderData.push_back(c);
      if(HeaderData.length() == HeaderLength) break;
    }
    ifile.close();

    // HeadTag
    {
      std::string ExpectedHeadtag = "PHRDAT";
      std::string ReadHeadTag;
      for(int i=0; i<6; i++) ReadHeadTag.push_back(HeaderData[i]);
      if(ReadHeadTag != ExpectedHeadtag)
      {
        std::cerr << "PhragDat error: Unexpected Error reading file! Exiting." << std::endl;
        if(DEBUG_MODE) std::cerr << "Debug: read HeadTag: " << ReadHeadTag << std::endl;
        return 1;
      }
    }

    // Version
    {
      uint8_t ReadVerMaj = 0;
      uint8_t ReadVerMin = 0;
      ReadVerMaj = static_cast<uint8_t>(HeaderData[6]);
      ReadVerMin = static_cast<uint8_t>(HeaderData[7]);

      // allow earlier minor patch of same major version
      if(ReadVerMaj != (uint8_t)VER_MAJ || ReadVerMin > (uint8_t)VER_MIN)
      {
        std::cerr << "PhragDat error: Version mis-match!\nCurrent version: PhragDat v" << VER_MAJ << "." << VER_MIN << "\n" << _Input << " was compiled with PhragDat v" << (int)ReadVerMaj << "." << (int)ReadVerMin << std::endl;
        return 1;
      }
    }

    // Contents
    {
      std::string DirListAddrStr = HeaderData.substr((size_t)(HeadTagLen + HeadVerLen), (size_t)ADDRESS_SIZE);
      std::string FileIndexAddrStr = HeaderData.substr((size_t)(HeadTagLen + HeadVerLen + ADDRESS_SIZE), (size_t)ADDRESS_SIZE);
      std::string FileDataAddrStr = HeaderData.substr((size_t)(HeadTagLen + HeadVerLen + ADDRESS_SIZE + ADDRESS_SIZE), (size_t)ADDRESS_SIZE);

      std::vector<uint8_t> DirListAddrVec;
      std::vector<uint8_t> FileIndexAddrVec;
      std::vector<uint8_t> FileDataAddrVec;

      for(int i=0; i<8; i++) DirListAddrVec.push_back(static_cast<uint8_t>(DirListAddrStr[i]));
      for(int i=0; i<8; i++) FileIndexAddrVec.push_back(static_cast<uint8_t>(FileIndexAddrStr[i]));
      for(int i=0; i<8; i++) FileDataAddrVec.push_back(static_cast<uint8_t>(FileDataAddrStr[i]));

      DirListAddr = PHD_Convert_uint8_to_uint64(DirListAddrVec);
      FileIndexAddr = PHD_Convert_uint8_to_uint64(FileIndexAddrVec);
      FileDataAddr = PHD_Convert_uint8_to_uint64(FileDataAddrVec);
    }
  }

  // Directory List
  {
    std::vector<std::string> DirectoryList;
    std::string DirListData;
    {
      std::ifstream ifile;
      ifile.open(_Input, std::ios::in | std::ios::binary);
      char c;
      uint64_t i = 0;
      while(ifile.get(c))
      {
        if(i == FileIndexAddr) break;
        if(i >= DirListAddr) DirListData.push_back(c);
        i++;
      }
      ifile.close();
    }

    // populate DirList
    uint16_t DirNameLen = 0;
    std::string DirNameLenStr;
    std::string DirName;
    bool DoReadName = 0; // 1 = ReadName, 0 = ReadLength

    for(int i=0; i<DirListData.length(); i++)
    {
      if(!DoReadName)
      {
        DirNameLenStr.push_back(DirListData[i]);

        if(DirNameLenStr.length()==2)
        {
          std::vector<uint8_t> DirNameLenVec = {static_cast<uint8_t>(DirNameLenStr[0]), static_cast<uint8_t>(DirNameLenStr[1])};
          DirNameLen = PHD_Convert_uint8_to_uint16(DirNameLenVec);
          DirNameLenStr.clear();
          DoReadName = 1;
        }
      }

      else
      {
        DirName.push_back(DirListData[i]);

        if(DirName.length()==DirNameLen)
        {
          std::string FullName = _Output;
          if(FullName.back()!='/') FullName.push_back('/');
          FullName += DirName;
          DirectoryList.push_back(FullName);
          DirName.clear();
          DirNameLen = 0;
          DoReadName = 0;
        }
      }
    }

    // create directories on disk

    // if no directory/file specified
    if(!_FileOrDir.length())
    {
      for(int i=0; i<DirectoryList.size(); i++)
      {
        if(std::filesystem::create_directory(DirectoryList[i])) std::cout << "Created Directory: " << DirectoryList[i] << std::endl;

        else
        {
          std::cerr << "PhragDat: Error creating directory: " << DirectoryList[i] << " Perhaps it already exists, else check read/write permissions and try again, exiting..." << std::endl;
          return 1;
        }
      }
    }

    // if directory/file specified
    else
    {
      std::string FileOrDir = _FileOrDir;

      // directory specified
      if(FileOrDir.back()=='/')
      {
        FileOrDir.pop_back();
        bool Found = 0;
        for(int i=0; i<DirectoryList.size(); i++)
        {
          if(DirectoryList[i].find(FileOrDir) != std::string::npos)
          {
            Found = 1;
            break;
          }
        }

        if(!Found)
        {
          std::cerr << "PhragDat error: " << FileOrDir << " Not found! Check spelling and try again. Exiting." << std::endl;
          return 1;
        }

        // create directory and parents
        if(Found)
        {
          std::string DirToCreate;
          FileOrDir.push_back('/');
          for(int i=0; i<FileOrDir.length(); i++)
          {
            if(FileOrDir[i]!='/') DirToCreate.push_back(FileOrDir[i]);

            else
            {
              DirToCreate.push_back('/');

              if(std::filesystem::create_directory(DirToCreate))
              {
                std::cout << "Created Directory: " << DirToCreate << std::endl;
              }

              else
              {
                std::cerr << "PhragDat: Error creating directory: " << DirToCreate << " Perhaps it already exists, else check read/write permissions and try again, exiting..." << std::endl;
                return 1;
              }
            }
          }
        }
      }
    }
  }

  // FileIndex
  struct File
  {
    std::string Name; // in this case Name = FullPath
    uint64_t Address;
    uint64_t Length;
  };
  std::vector<File> FileIndex;

  // populate FileIndex
  {
    std::string FileIndexData;
    {
      std::ifstream ifile;
      ifile.open(_Input, std::ios::in | std::ios::binary);
      char c;
      uint64_t i = 0;
      while(ifile.get(c))
      {
        if(i == FileDataAddr) break;
        if(i >= FileIndexAddr) FileIndexData.push_back(c);
        i++;
      }
      ifile.close();
    }

    // populate FileIndex
    File CurrentFile;
    uint16_t NameLength = 0;
    std::string NameLenStr;
    std::string AddressStr;
    std::string LengthStr;

    int ReadState = 0; // 0: NameLen, 1: Name, 2: Addr, 3: Length

    for(int i=0; i<FileIndexData.length(); i++)
    {
      if(ReadState==0) // name length
      {
        NameLenStr.push_back(FileIndexData[i]);

        if(NameLenStr.length()==2)
        {
          std::vector<uint8_t> FileNameLenVec = {static_cast<uint8_t>(NameLenStr[0]), static_cast<uint8_t>(NameLenStr[1])};
          NameLength = PHD_Convert_uint8_to_uint16(FileNameLenVec);
          NameLenStr.clear();
          ReadState = 1;
        }
      }

      else if(ReadState==1) // name
      {
        CurrentFile.Name.push_back(FileIndexData[i]);

        if(CurrentFile.Name.length()==NameLength)
        {
          std::string FullName = _Output;
          if(FullName.back()!='/') FullName.push_back('/');
          FullName += CurrentFile.Name;
          CurrentFile.Name = FullName;
          NameLength = 0;
          ReadState = 2;
        }
      }

      else if(ReadState==2) // address
      {
        AddressStr.push_back(FileIndexData[i]);

        if(AddressStr.length()==ADDRESS_SIZE)
        {
          std::vector<uint8_t> AddressVec;
          for(int j=0; j<ADDRESS_SIZE; j++) AddressVec.push_back(static_cast<uint8_t>(AddressStr[j]));
          CurrentFile.Address = PHD_Convert_uint8_to_uint64(AddressVec);
          AddressStr.clear();
          ReadState = 3;
        }
      }

      else if(ReadState==3) // length
      {
        LengthStr.push_back(FileIndexData[i]);

        if(LengthStr.length()==ADDRESS_SIZE)
        {
          std::vector<uint8_t> LengthVec;
          for(int j=0; j<ADDRESS_SIZE; j++) LengthVec.push_back(static_cast<uint8_t>(LengthStr[j]));
          CurrentFile.Length = PHD_Convert_uint8_to_uint64(LengthVec);
          LengthStr.clear();
          ReadState = 0;

          FileIndex.push_back(CurrentFile);
          CurrentFile.Name.clear();
          CurrentFile.Address = 0;
          CurrentFile.Length = 0;
        }
      }
    }
 }

 // Write Files

 // if no file specified
 if(!_FileOrDir.length())
 {
   for(int i=0; i<FileIndex.size(); i++)
   {
     std::cout << "Writing: " << FileIndex[i].Name << " (" << i+1 << "/" << FileIndex.size() << ")" << std::endl;
     if(PHDE_WriteFile(_Input, FileIndex[i].Name, FileIndex[i].Address, FileIndex[i].Length)) return 1;
   }

   std::cout << "Successfully extracted " << FileIndex.size() << " files from " << _Input << " to " << _Output << std::endl;
 }

 // file/dir specified
 else
 {
   // directory specified
   if(_FileOrDir.back()=='/')
   {
     // create files
     std::string FileOrDir = _FileOrDir;
     for(int i=0; i<FileIndex.size(); i++)
     {
       if(FileIndex[i].Name.find(_FileOrDir) != std::string::npos)
       {
         std::cout << "Writing: " << FileIndex[i].Name << std::endl;
         if(PHDE_WriteFile(_Input, FileIndex[i].Name, FileIndex[i].Address, FileIndex[i].Length)) return 1;
       }
     }
     std::cout << _FileOrDir << " successfully extracted." << std::endl;
   }

   // file specified
   else
   {
     bool Found = 0;
     std::string FileName = PHD_RemoveParentsFromPath(_FileOrDir);

     // check file doesnt exist
     {
       DWORD ftyp = GetFileAttributesA(FileName.c_str());
       if(ftyp != -1)
       {
         std::cerr << "PhragDat error: " << FileName << " already exists at output path! Will not overwrite. Exiting..." << std::endl;
         return 1;
       }
     }

     for(int i=0; i<FileIndex.size(); i++)
     {
       if(FileIndex[i].Name.find(_FileOrDir) != std::string::npos)
       {
         Found = 1;
         std::cout << "Writing: " << FileName << std::endl;
         if(PHDE_WriteFile(_Input, FileName, FileIndex[i].Address, FileIndex[i].Length)) return 1;
         break;
       }
     }

     if(Found)
     {
       std::cout << _FileOrDir << " successfully extracted." << std::endl;
     }

     if(!Found)
     {
       std::cerr << "PhragDat error: " << _FileOrDir << " Not found! Check spelling and try again. Exiting." << std::endl;
       return 1;
     }
   }
 }

 return 0;
}