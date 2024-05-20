import os
 
os.system("sudo util/install.sh -Wlnfv")

os.system("sudo apt install mininet")

os.system("sudo make install")

os.system("sudo apt install openvswitch-testcontroller")

os.system("sudo ln /usr/bin/ovs-testcontroller /usr/bin/controller")
