nbextractor and SQLite support
==============================

nbextractor can dump its data on screen (standard output) or on a file.
Optionally, nbextractor can dump data on a SQLite database.

In order to enable sqlite3 support:

 *	install the sqlite3 library.
	E.g, in Ubuntu you have to type: 
		sudo apt-get install libsqlite3
		sudo apt-get install libsqlite3-dev

 *	change the ENABLE_SQLITE3_SUPPORT flag in tools\CMakeLists.txt to ON.
	Note: you need to modify the main CMakeLists.txt file present in the
	'tools' directory, and not the one in this local directory.


After compiling nbextractor, launch 'nbextractor -h' to find out the 
available new command line options.


NOTE: the NetBee source tree already includes the sqlite3 libraries for 
Windows in the 'contrib/sqlite3' folder. Hence on this platform you need
just to turn the ENABLE_SQLITE3_SUPPORT flag in CMakeLists.txt file.
