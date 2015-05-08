###About Mininet-WiFi
Mininet-WiFi is a release of Mininet (http://mininet.org/) which allows the using of both WiFi Stations and BaseStations. More informations and details about this release will be available in soon on Intrig's GitHub profile (https://github.com/intrig-unicamp/).  


###A Brief Demonstration
https://www.youtube.com/watch?v=H46EPuJDJhc  
You also may use the example file in /examples/simplewifitopology.py

##Installation  
####Option 1: via install.sh  
It is not work properly yet  
  
####Option 2: via apt  
step 1: apt-get install mininet  
step 2: git clone https://github.com/ramonfontes/mininet-wifi/  
step 3: cd mininet-wifi  
step 4: make install  

####Some known dependencies  
iw - apt-get install iw  
python-setuptools - apt-get install python-setuptools  
hostapd - apt-get install hostapd  

###Get Started  
sudo mn --wifi --ssid=new_ssid    
It will start a simple topology with 2 stations and 1 access point. The stations also includes a physical ethernet interface.  

###Team
Ramon dos Reis Fontes (ramonrf@dca.fee.unicamp.br)  
Samira Afzal (samira@dca.fee.unicamp.br)  
Samuel Henrique Bucke Brito (shbbrito@dca.fee.unicamp.br)  
Mateus A. Silva Santos (msantos@dca.fee.unicamp.br)  
Christian Rodolfo Esteve Rothenberg (chesteve@dca.fee.unicamp.br)  

We are members of INTRIG (Information & Networking Technologies Research & Innovation Group) at University of Campinas - Unicamp, SP, Brazil.


