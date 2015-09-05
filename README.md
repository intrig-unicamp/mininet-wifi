###About Mininet-WiFi
Mininet-WiFi is a fork of Mininet (http://mininet.org/) which allows the using of both WiFi Stations and Access Points. Mininet-WiFi only add wifi features and you can work with it like you were working with Mininet. More informations and details about it  will be available in soon.        

##Installation  
step 1: git clone https://github.com/intrig-unicamp/mininet-wifi  
step 2: cd mininet-wifi  
step 3: util/install.sh -Wnfv      

##Download an Image  
Download: [Virtual Disk (1.3GB)](http://intrig.dca.fee.unicamp.br/index.php/projects/projects.html)    

##Mininet-wifi on Docker  
Image: [(ramonfontes/mininet-wifi)](https://registry.hub.docker.com/u/ramonfontes/mininet-wifi/)      
  
###Get Started
sudo mn --wifi --ssid=new_ssid  
It should start a simple topology with 2 stations and 1 access point. 

###A Brief Demonstration
Video 01: https://www.youtube.com/watch?v=TUu_Wun6TQs  
Video 02: https://www.youtube.com/watch?v=_PtSmhf7Z8s  
Video 03: https://www.youtube.com/watch?v=H46EPuJDJhc  
Video 04: https://www.youtube.com/watch?v=WH6bSOKC7Lk  
  
###You also can use the examples:   

/examples/2AccessPoints.py  
/examples/adhoc.py  
/examples/simplewifitopology.py  
/examples/wifiMobility.py  
/examples/wifiMobilityModel.py  
/examples/wifiPosition.py  
/examples/wifiStationsAndHosts.py  

##Note
Mininet-WiFi should work fine in any Ubuntu distribution from 14.04, but in some cases (only if you have problems when start it) you have to stop NetworkManager typing `stop network-manager`.  

###Team
Ramon dos Reis Fontes (ramonrf@dca.fee.unicamp.br)  
Christian Rodolfo Esteve Rothenberg (chesteve@dca.fee.unicamp.br)  

We are members of [INTRIG (Information & Networking Technologies Research & Innovation Group)](http://intrig.dca.fee.unicamp.br) at University of Campinas - Unicamp, SP, Brazil.


