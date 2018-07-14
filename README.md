![](https://github.com/ramonfontes/miscellaneous/blob/master/mininet-wifi/mininet-wifi-logo.png)

### About Mininet-WiFi
Mininet-WiFi is a fork of Mininet (http://mininet.org/) which allows the using of both WiFi Stations and Access Points. Mininet-WiFi only add wifi features and you can work with it like you were working with Mininet.   

[![Build Status](https://travis-ci.org/intrig-unicamp/mininet-wifi.svg?branch=master)](https://travis-ci.org/intrig-unicamp/mininet-wifi)

### Things to keep in mind when working with Mininet-WiFi   
* You can use any wireless network tools (e.g. iw, iwconfig, wpa_supplicant, etc)    
* Please consider computer network troubleshooting steps to solve issues before making questions in the mailing list (e.g. is the station associated with ap? Is the OpenFlow rule working correctly? etc)   
* Do you need help? Be careful with questions in the mailing list and please providing as much information you can.

### User Manual  
[Access the User Manual](https://github.com/ramonfontes/manual-mininet-wifi/raw/master/mininet-wifi-draft-manual.pdf)

### Mailing List  
[https://groups.google.com/forum/#!forum/mininet-wifi-discuss](https://groups.google.com/forum/#!forum/mininet-wifi-discuss) 

### Use Cases Catalogue   
Please, let us know if you are doing research with Mininet-WiFi. A list of citations on Mininet-WiFi is available [here](https://docs.google.com/spreadsheets/d/1laEhejMg6th-Urgc-_RqBi2H6m308Rnh9uJpKZavEio/edit?usp=sharing).     

## Installation  
step 1: $ sudo apt-get install git  
step 2: $ git clone https://github.com/intrig-unicamp/mininet-wifi  
step 3: $ cd mininet-wifi  
step 4: $ sudo util/install.sh -Wlnfv  
#### install.sh options:   
-W: wireless dependencies   
-n: mininet-wifi dependencies    
-f: OpenFlow   
-v: OpenvSwitch   
-l: wmediumd   
optional:  
-6: wpan tools  
-P: python3

### Building Topologies with VND    

I'm a beginner, I do not know Python and I would like to create a customized topology. In this case, [VND](https://github.com/ramonfontes/vnd-sdn-version) can serve as support since it provides a GUI and automatically generates Python scripts. 

![](https://github.com/ramonfontes/vnd-sdn-version/blob/master/screenshot.png)

### Development
For instructions about easier development check [this helper file](doc/dev_help.md).

## Pre-configured Virtual Machine    
[Ubuntu 16.04 x64 :: Password: wifi](https://intrig.dca.fee.unicamp.br:8840/owncloud/index.php/s/7NgXfuPArXStSax)      
user: wifi   
pass: wifi   
   
## Note
Mininet-WiFi should work fine in any Ubuntu distribution from 14.04, but in some cases (only if you have problems when start it) you have to stop NetworkManager with `stop network-manager` (you can also use `sudo systemctl stop network-manager` or `sudo service network-manager stop`).    

### Team
Ramon dos Reis Fontes (ramonrf@dca.fee.unicamp.br)  
Christian Rodolfo Esteve Rothenberg (chesteve@dca.fee.unicamp.br)  

We are members of [INTRIG (Information & Networking Technologies Research & Innovation Group)](http://intrig.dca.fee.unicamp.br) at University of Campinas - Unicamp, SP, Brazil.
