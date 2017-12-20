![](https://github.com/ramonfontes/miscellaneous/blob/master/mininet-wifi/mininet-wifi-logo.png)

### About Mininet-WiFi
Mininet-WiFi is a fork of Mininet (http://mininet.org/) which allows the using of both WiFi Stations and Access Points. Mininet-WiFi only add wifi features and you can work with it like you were working with Mininet.   

[![Build Status](https://travis-ci.org/intrig-unicamp/mininet-wifi.svg?branch=master)](https://travis-ci.org/intrig-unicamp/mininet-wifi)


### User Manual  
[Access the User Manual](https://github.com/ramonfontes/manual-mininet-wifi/raw/master/mininet-wifi-draft-manual.pdf)

### Mailing List  
[https://groups.google.com/forum/#!forum/mininet-wifi-discuss](https://groups.google.com/forum/#!forum/mininet-wifi-discuss) 
  
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

### Development
For instructions about easier development check [this helper file](doc/dev_help.md).

## Pre-configured Virtual Machine    
[Ubuntu 16.04 x64 :: Password: wifi](https://intrig.dca.fee.unicamp.br:8840/owncloud/index.php/s/91mCANMB4LobPmQ)      
user: wifi   
pass: wifi   
   
## Note
Mininet-WiFi should work fine in any Ubuntu distribution from 14.04, but in some cases (only if you have problems when start it) you have to stop NetworkManager with `stop network-manager` (you can also use `sudo systemctl stop network-manager` or `sudo service network-manager stop`).    

### Team
Ramon dos Reis Fontes (ramonrf@dca.fee.unicamp.br)  
Christian Rodolfo Esteve Rothenberg (chesteve@dca.fee.unicamp.br)  

We are members of [INTRIG (Information & Networking Technologies Research & Innovation Group)](http://intrig.dca.fee.unicamp.br) at University of Campinas - Unicamp, SP, Brazil.
