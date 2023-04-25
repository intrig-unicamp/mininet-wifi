In order to create a new release of NetBee, you have to follow these steps:

- Update version number (and release date) in:
    - 'download' oage, on the web site
    - 'nbee.h' file
- Clean all the projects
- Compile everything in Release mode
- Launch scripts in the following order (and *follow the instructions on screen*):
    - createbin.sh
    - createdoc.sh
    - createdevpack.sh
    - createsrc.sh


<!--
Final Steps
- Create the installer: launch script 'installer/installer-config.iss' to create the analyzer setup
- Upload on the Analyzer website:
   - the setup
   - the source pack
- Update all the website
-->
   