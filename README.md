DATCreator

This simple program takes 2 example files, test1.ttf and test2.ttf.
These are both TrueType Font files. If a single binary character is wrong, they are unreadable
(and are therefore great for testing this program).

This program takes the 2 example files, out spits them out as a single .DAT file.

It then reads the .DAT file and re-separates them into 2 new files, and they both work.

This can be used to package up game data so that libraries such as SDL2 can read using RWops,
and users don't have to have lots of sprites, images, fonts, etc, lying around inside game data directories.