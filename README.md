<br/>Phragware Utlities
<br/>PhragDat DataFile Manager
<br/>C++ for Windows 64-bit
<br/>(c) Phragware 2020
<br/>
<br/>- Compile game or application data into a single .dat file for easy portability
<br/>- Takes a path to input directory and compiles all files and subdirectories which can then be extracted later back to their original state
<br/>- Individual files and directories with their subdirectories can be extracted individually
<br/>- Files and directories can be excluded from input using a list of exceptions written to a txt file
<br/>
<br/>-----------------------------------------------------------------------
<br/>
<br/>Usage:
<br/>Compile:  phragdat -c -i"input/dir" -o"path/to/output" (-f"exclusions.txt")
<br/>Extract:  phragdat -e -i"input/dat.dat" -o"output/dir" (-f"dat/file/or/dir")
<br/>
<br/>PhragDat modes of operation:
<br/>
<br/>Compile:
<br/>    phragdat.exe -c -i"input/dir" -o"path/to/output" (-f"exclusions.txt")
<br/>    compiles all contents of "path/to/input" and exports single .dat file
<br/>    if output ends in .dat then output will write to the specified path
<br/>    otherwise output will be named [input directory].dat
<br/>    optional exclusions text file: see below options for details
<br/>
<br/>Extract:
<br/>    phragdat.exe -e -i"input.dat" -o"output/dir" -f"dat/file/or/dir"(optional)
<br/>    extracts contents of .dat file to "path/to/directory"
<br/>    Note: If you want to specify a specific file to extract with -f then the path
<br/>    must be relative to the original compiled directory root, eg: -f"Directory/file.ext"
<br/>    If you wish to specify a directory IT MUST END IN '/' TO SPECIFY A DIRECTORY!
<br/>    eg: -f"Directory/SubDirectory1/TargetDirectory/"
<br/>
<br/>Exclusions File:
<br/>    a simple text file with each new text line counting as an exclude.
<br/>    Possible Exclusions:
<br/>    Specific Files: "filename.ext"
<br/>    Files with specific extension: "*.ext"
<br/>    Directory and all sub-directories: "Directory/"
<br/>    (directories must end in '/' and ANY directory with that name will be excluded)
<br/>    for example:
<br/>        File e.txt Contents:
<br/>            *.txt
<br/>            Thumbs.db
<br/>            TestDirectory/
<br/>    This will exclude all files ending in '.txt', all 'Thumbs.db' files, and all files
<br/>    and directories within 'TestDirectory/'."
<br/>
<br/>-----------------------------------------------------------------------
<br/>
<br/>Version History:
<br/>
<br/>v0.1: 02-06-2020
<br/>		- basic functionality
<br/>
<br/>v1.0: 22-06-2020
<br/>		- Compile mode implemented
<br/>
<br/>v1.1: 24-06-2020
<br/>		- compile mode bug fixes
<br/>		- code cleanup
<br/>
<br/>v1.2: 25-06-2020
<br/>		- implemented compile mode exclusions: can now pass -f"exclusions.txt" and these exclusions will not be compiled into the output.dat
<br/>
<br/>v2.0: 26-06-2020
<br/>		- Extract mode implemented
<br/>		- much code cleanup
<br/>		- buffer options removed, simplified buffers, sticking with 64-bit addresses/buffers (means memory usage could go up to 4.1GB in certain situations, but no big deal for most systems and windows probably deals with it anyway)
<br/>
<br/>v2.1: 27-06-2020
<br/>		- Added option to select specific file or directory for extract mode with option -f"file/dir"
<br/>
<br/>v3.0: 28-06-2020
<br/>		- Read mode implemented
<br/>
<br/>v4.0: 29-06-2020
<br/>		- Decided to remove read and array modes, they dont work or dont fit the functionality, so i have decided to cleanup and call this 'DONE'!
<br/>
<br/>v4.1: 29-06-2020
<br/>		- Fixed bug in PHD_Compile/PHDC_Data::Write() which was discarding current .dat file data and rewriting whenever Write() was called instead of appending to end of file.
<br/>    - Added EOF file check to make sure file (seems) complete before reading. The .dat file length is added as a uin64_t to the last 8 bytes of the dat file, so when reading, read the last 8 bytes as a uint64_t and compare to the actual file size. If they don't match up then the .dat file being read is probably corrupted. A checksum would do this better but I wanted to implement a simple check of my own devise.
<br/>
<br/>-----------------------------------------------------------------------
<br/>
<br/>dat file composition:
<br/>
<br/>header
<br/>- tag
<br/>- version
<br/>- contents addresses
<br/>
<br/>directory list
<br/>- pathlength
<br/>- paths
<br/>
<br/>file index
<br/>- pathlength
<br/>- path
<br/>- startaddress
<br/>- length
<br/>
<br/>file data
<br/>- binary data
<br/>
<br/>-----------------------------------------------------------------------
<br/>
<br/>OUTPUT DAT FILE COMPOSITION:
<br/>
<br/>HEADER: 32 bytes
<br/>{
<br/>	TAG: 6 bytes
<br/>	{
<br/>		0x50 0x48 0x52 0x44 0x41 0x54 //ascii: "PHRDAT"
<br/>	}
<br/>	VERSION: 2 bytes
<br/>	{
<br/>		uint8_t VER_MAJ (1 byte)
<br/>		uint8_t VER_MIN (1 byte)
<br/>	}
<br/>	CONTENTS_ADDRESSES: 24 bytes
<br/>	{
<br/>		uint64_t DirectoryList_StartAddress (8 bytes)
<br/>		uint64_t FileIndex_StartAddress (8 bytes)
<br/>		uint64_t FileData_StartAddress (8 bytes)
<br/>	}
<br/>}
<br/>
<br/>DIRECTORY_LIST:
<br/>{
<br/>	PER DIRECTORY: ( (2 bytes + x bytes) * numDirectories)
<br/>	{
<br/>		2 bytes: uint16_t directory_path_length = x
<br/>		x bytes: char directory_path[x] (ascii)
<br/>	}
<br/>}
<br/>
<br/>FILE_INDEX:
<br/>{
<br/>	PER FILE: ( (2 bytes + x bytes + 8 bytes + 8 bytes) * numFiles)
<br/>	{
<br/>		2 bytes: uint16_t file_path_length = x
<br/>		x bytes: char file_path[x] (ascii)
<br/>		8 bytes: uint64_t file_address
<br/>		8 bytes: uint64_t file_length
<br/>	}
<br/>}
<br/>
<br/>FILE_DATA:
<br/>{
<br/>	PER FILE:
<br/>	{
<br/>		binary file data
<br/>	}
<br/>}
<br/>
<br/>first byte written is at address 0x00 (index 0, like C array)
<br/>
<br/>-----------------------------------------------------------------------