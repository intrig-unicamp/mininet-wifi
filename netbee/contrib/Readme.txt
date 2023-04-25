This folder contains some of the external libraries needed to compile NetBee.

  1) pcre
  2) sqlite

Please note that this README file refers to Windows and aims at making the compilation of the NetBee library simpler. If you are on UNIX, we suppose you have those libraries installed in the standard folders of your operating system.
Any any case, project files (for NetBee sources, samples and tools) point to the right location and all the project should compile smoothly independently from your operating system.

These instructions should be used when you need to upgrade the version of those libraries required by NetBee present in the NetBee source folder.

Please note that those 'contrib' libraries are precompiled binaries; no source code is present here.

Finally, you need to copy the proper executables (e.g., DLLs) in the 'netbee/bin' folder in order to be able to execute your NetBee software.
The scripts in the root folder will reming you this; please remember to complete this step in order to make everything working on Windows.



Perl-compatible Regular Expressions (PCRE) - version 7.0 (March 2007)
=============================================================================
The PCRE library is a set of functions that implement regular expression 
pattern matching using the same syntax and semantics as Perl 5.

The Win32 precompiled binaries for pcre have been downloaded from the GnuWin project:

   http://sourceforge.net/projects/gnuwin32/files/pcre/7.0/

Downloaded packages:

   pcre-7.0-bin.zip
   pcre-7.0-lib.zip

All files have been extracted the same folder tree.

Some folders are not strictly required (e.g. docs) but they have been
kept in order to minimize the tasks to be done when downloading a new 
release

Please note that the 'pcre-nbeetest' folder contains a small project that
was used in the earlier development steps of NetBee.



SQLite Database (SQLite) - version 3.7.4. (December 2010)
=========================================================
Downloaded:
- http://www.sqlite.org/sqlite-amalgamation-3070400.zip
  (contains the header files)
- http://www.sqlite.org/sqlite-dll-win32-x86-3070400.zip
  (contains the precompiled SQLite DLL)

All files have been extracted the same folder tree.
Unfortunately, those ZIP files do not include the .lib file, which is required
by Visual Studio in order to link against the library.
You can create manually the .lib file starting from the .DEF file included
in previous packages, with the following procedure:

- Open the Visual Studio Command Prompt (Start - Programs - Microsoft Visual 
  Studio 2010 - Visual Studio Tools - Visual Studio Command Prompt)

- Execute the following command:

      lib /def:sqlite3.def /out:sqlite3.lib /machine:x86

In case of troubles, you can refer to the following web page:

  http://www.coderetard.com/2009/01/21/generate-a-lib-from-a-dll-with-visual-studio/