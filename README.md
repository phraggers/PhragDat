# Phragware Utilities: PhragDat
![PhragDatLogo](https://raw.githubusercontent.com/phraggers/PhragDat/master/PhragwareLogo.png)
<br>PhragDat DataFile Manager
<br/>C++17 for Windows 64-bit
<br/>(c) Phragware 2020


- Compile game or application data into a single .dat file for easy portability
- Generates .dat data file and .c contents file
- Takes a path to input directory and compiles all files and subdirectories which can then be accessed via contents C source code file
- Files and directories can be excluded from input using a list of exceptions written to a txt file

<hr/>

## Usage:
	phragdat -i"input/dir" -d"path/to/output/dir" -c"path/to/output/dir" -e"exclusions.txt"(optional)

### Compilation:
    compiles all contents of "path/to/input" and exports single .dat file.
    Output will be named [input directory].dat and .c respectively.
    optional exclusions text file: see below options for details

#### Contents .c:
    contains mapping info and functions for C/C++ code to access files within the .dat file
    list of files (path from .dat as root) with their address and length within .dat file

#### Exclusions File:
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

- v0.1: 02-06-2020:
	- basic functionality

- v1.0: 22-06-2020:
	- Compile mode implemented

- v1.1: 24-06-2020:
	- compile mode bug fixes
	- code cleanup

- v1.2: 25-06-2020:
	- implemented compile mode exclusions: can now pass -f"exclusions.txt" and these exclusions will not be compiled into the output.dat

- v2.0: 26-06-2020:
	- Extract mode implemented
	- much code cleanup
	- buffer options removed, simplified buffers, sticking with 64-bit addresses/buffers (means memory usage could go up to 4.1GB in certain situations, but no big deal for most systems and windows probably deals with it anyway)

- v2.1: 27-06-2020:
	- Added option to select specific file or directory for extract mode with option -f"file/dir"

- v3.0: 28-06-2020:
	- Read mode implemented

- v4.0: 29-06-2020:
	- Decided to remove read and array modes, they dont work or dont fit the functionality, so i have decided to cleanup and call this 'DONE'!

- v4.1: 29-06-2020:
	- Fixed bug in PHD_Compile/PHDC_Data::Write() which was discarding current .dat file data and rewriting whenever Write() was called instead of appending to end of file.
  - Added EOF file check to make sure file (seems) complete before reading. The .dat file length is added as a uin64_t to the last 8 bytes of the dat file, so when reading, read the last 8 bytes as a uint64_t and compare to the actual file size. If they don't match up then the .dat file being read is probably corrupted. A checksum would do this better but I wanted to implement a simple check of my own devise.

- v5.0: 01-07-2020:
	- re-worked functionality, now only compiles .dat files and generates contents as a .c source code file for use in c/c++ apps/games
  - fixed/simplified/removed a lot of code that was kinda pointless (I guess that just happens when you are trying to figure out a method for accomplishing some programming task, pseudo code does not always translate into useful code!)

- v5.1: 01-07-2020:
	- Quick re-compilation for release v5, accidentally released DEBUG build. Whoops.

- v5.2: 02-07-2020:
	- The generated c code was garbage so instead contents is now a simple .csv
  - cleaned up some unused code from phragdat_funcs.h

<hr/>

## Data.dat file composition:

### header (8 bytes)
- tag (6 bytes)
- version (2 bytes)

### file data
- binary data

## Contents.csv file composition:
- First line: PHRDAT, uint8 major version, uint8 minor version
- 1 line per file: "File Path within .dat", uint64 Address, uint64 Length

<hr/>