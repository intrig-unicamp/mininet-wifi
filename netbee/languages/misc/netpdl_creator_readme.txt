How to create or modify NetPDL dissectors
=========================================

NetPDL dissectors are in the '../netpdl-dissectors' folder.
You can either modify existing dissectors or add new ones, for other 
protocols.

In case you want to add new dissectors, please remember that you need
to modify the 'netpdl_creator.sh' script as well, in order to tell
the script to include the new dissector.

The 'netpdl_creator.sh' script will collate all the dissectors listed
in the script itself into one single XML file, ready to use. The output
of this script will be available in ../../bin/netpdl.xml file.

Please note that the output file will not be validated against the NetPDL
schema. Hence, no errors are given even in case a dissector is wrong.
We suggest to validate the NetPDL file against the schema before using it.
