# Phragware Utlities
- PhragDat DataFile Manager
- C++ for Windows 64-bit
- (c) Phragware 2020

- Compile game or application data into a single .dat file for easy portability
- Takes a path to input directory and compiles all files and subdirectories which can then be extracted later back to their original state
- Individual files and directories with their subdirectories can be extracted individually
- Files and directories can be excluded from input using a list of exceptions written to a txt file

<hr/>

## Usage:
	- Compile:  phragdat -c -i"input/dir" -o"path/to/output" (-f"exclusions.txt")
	- Extract:  phragdat -e -i"input/dat.dat" -o"output/dir" (-f"dat/file/or/dir")

## PhragDat modes of operation:

### Compile:
    compiles all contents of "path/to/input" and exports single .dat file
    if output ends in .dat then output will write to the specified path
    otherwise output will be named [input directory].dat
    optional exclusions text file: see below options for details

### Extract:
    extracts contents of .dat file to "path/to/directory"
    Note: If you want to specify a specific file to extract with -f then the path
    must be relative to the original compiled directory root, eg: -f"Directory/file.ext"
    If you wish to specify a directory IT MUST END IN '/' TO SPECIFY A DIRECTORY!
    eg: -f"Directory/SubDirectory1/TargetDirectory/"

### Exclusions File:
    a simple text file with each new text line counting as an exclude.
    Possible Exclusions:
    Specific Files: "filename.ext"
    Files with specific extension: "*.ext"
    Directory and all sub-directories: "Directory/"
    (directories must end in '/' and ANY directory with that name will be excluded)
    for example:
        File e.txt Contents:
            *.txt
            Thumbs.db
            TestDirectory/
    This will exclude all files ending in '.txt', all 'Thumbs.db' files, and all files
    and directories within 'TestDirectory/'."

<hr/>

## Version History:<br/>

- v0.1: 02-06-2020
		- basic functionality

- v1.0: 22-06-2020
		- Compile mode implemented

- v1.1: 24-06-2020
		- compile mode bug fixes
		- code cleanup

- v1.2: 25-06-2020
		- implemented compile mode exclusions: can now pass -f"exclusions.txt" and these exclusions will not be compiled into the output.dat

- v2.0: 26-06-2020
		- Extract mode implemented
		- much code cleanup
		- buffer options removed, simplified buffers, sticking with 64-bit addresses/buffers (means memory usage could go up to 4.1GB in certain situations, but no big deal for most systems and windows probably deals with it anyway)

- v2.1: 27-06-2020
		- Added option to select specific file or directory for extract mode with option -f"file/dir"

- v3.0: 28-06-2020
		- Read mode implemented

- v4.0: 29-06-2020
		- Decided to remove read and array modes, they dont work or dont fit the functionality, so i have decided to cleanup and call this 'DONE'!

- v4.1: 29-06-2020
		- Fixed bug in PHD_Compile/PHDC_Data::Write() which was discarding current .dat file data and rewriting whenever Write() was called instead of appending to end of file.
    - Added EOF file check to make sure file (seems) complete before reading. The .dat file length is added as a uin64_t to the last 8 bytes of the dat file, so when reading, read the last 8 bytes as a uint64_t and compare to the actual file size. If they don't match up then the .dat file being read is probably corrupted. A checksum would do this better but I wanted to implement a simple check of my own devise.

<hr/>

## dat file composition:<br/>

### header
- tag
- version
- contents addresses

### directory list
- pathlength
- paths

### file index
- pathlength
- path
- startaddress
- length

### file data
- binary data

<hr/>

## OUTPUT DAT FILE COMPOSITION:<br/>

- HEADER: 32 bytes
	- TAG: 6 bytes
		- 0x50 0x48 0x52 0x44 0x41 0x54 //ascii: "PHRDAT"
	- VERSION: 2 bytes
		- uint8_t VER_MAJ (1 byte)
		- uint8_t VER_MIN (1 byte)
	- CONTENTS_ADDRESSES: 24 bytes
		- uint64_t DirectoryList_StartAddress (8 bytes)
		- uint64_t FileIndex_StartAddress (8 bytes)
		- uint64_t FileData_StartAddress (8 bytes)

- DIRECTORY_LIST:
	- PER DIRECTORY: ( (2 bytes + x bytes) * numDirectories)
		- 2 bytes: uint16_t directory_path_length = x
		- x bytes: char directory_path[x] (ascii)

- FILE_INDEX:
	- PER FILE: ( (2 bytes + x bytes + 8 bytes + 8 bytes) * numFiles)
		- 2 bytes: uint16_t file_path_length = x
		- x bytes: char file_path[x] (ascii)
		- 8 bytes: uint64_t file_address
		- 8 bytes: uint64_t file_length

- FILE_DATA:
	- PER FILE:
		- binary file data
<br/>
first byte written is at address 0x00 (index 0, like C array)
<hr/>