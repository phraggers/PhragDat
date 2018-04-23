//==============
// Molivos
// DATCreator
//==============

// Takes 2 TrueType Font files test1.ttf and test2.ttf
// packs them into a single file: testout.dat
// then reads testout.dat and re-separates them to testout1.ttf and testout2.ttf

#include <fstream>
#include <string>

int main()
{
  char inputchar;

  // TEST1 IN
  std::ifstream ifile1;
  ifile1.open("test1.ttf", std::ifstream::in | std::ifstream::binary);
  std::string string1;
  while(ifile1.get(inputchar))
    {
      string1.push_back(inputchar);
    }
  ifile1.close();

  // TEST2 IN
  std::ifstream ifile2;
  ifile2.open("test2.ttf", std::ifstream::in | std::ifstream::binary);
  std::string string2;
  while(ifile2.get(inputchar))
    {
      string2.push_back(inputchar);
    }
  ifile2.close();

  // SEPARATOR
  std::string separator = "__SEP__";

  // TEST DAT OUT
  std::ofstream ofile;
  ofile.open("testout.dat", std::ofstream::out | std::ofstream::binary);
  ofile << string1 << separator << string2;
  ofile.close();

  // TEST DAT IN
  std::ifstream ifiledat;
  ifiledat.open("testout.dat", std::ifstream::in | std::ifstream::binary);
  std::string stringdat;
  while(ifiledat.get(inputchar))
    {
      stringdat.push_back(inputchar);
    }
  ifiledat.close();

  // TEST DAT SEPARATE
  std::string bufferstring;
  std::string stringfile1out;
  std::string stringfile2out;
  int iterator = 0;
  bool scan = true;

  while(scan)
    {
      bufferstring.push_back(stringdat[iterator]);
      if(stringdat[iterator+1] == '_')
	{
	  if(stringdat[iterator+2] == '_')
	    {
	      if(stringdat[iterator+3] == 'S')
		{
		  if(stringdat[iterator+4] == 'E')
		    {
		      if(stringdat[iterator+5] == 'P')
			{
			  if(stringdat[iterator+6] == '_')
			    {
			      if(stringdat[iterator+7] == '_')
				{
				  scan = false;
				  iterator += 7;
				}
			    }
			}
		    }
		}
	    }
	}
      iterator++;
    }

  stringfile1out = bufferstring;
  bufferstring.clear();
  
  for(int i = iterator; i <= stringdat.length(); i++)
    {
      stringfile2out.push_back(stringdat[i]);
    }
  
  // TEST DAT: RE-SEPARATE FILES
  std::ofstream file1out;
  file1out.open("test1out.ttf", std::ofstream::out | std::ofstream::binary);
  file1out << stringfile1out;
  file1out.close();

  std::ofstream file2out;
  file2out.open("test2out.ttf", std::ofstream::out | std::ofstream::binary);
  file2out << stringfile2out;
  file2out.close();
  
  return 0;
}
