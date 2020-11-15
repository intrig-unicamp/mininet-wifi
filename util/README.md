![](https://github.com/ramonfontes/miscellaneous/blob/master/mininet-wifi/mininet-wifi-logo.png)

### About Mininet-WiFi
Mininet-WiFi is a fork of Mininet (http://mininet.org/) which allows the using of both WiFi Stations and Access Points. Mininet-WiFi only add wifi features and you can work with it like you were working with Mininet.   

[![Build Status](https://travis-ci.org/intrig-unicamp/mininet-wifi.svg?branch=master)](https://travis-ci.org/intrig-unicamp/mininet-wifi)

### Things to keep in mind when working with Mininet-WiFi   
* You can use any wireless network tools (e.g. iw, iwconfig, wpa_supplicant, etc)    
* Please consider computer network troubleshooting steps to solve issues before making questions in the mailing list (e.g. is the station associated with ap? Is the OpenFlow rule working correctly? etc)   
* Do you need help? Be careful with questions in the mailing list and please providing as much information you can.

### Mailing List  
[https://groups.google.com/forum/#!forum/mininet-wifi-discuss](https://groups.google.com/forum/#!forum/mininet-wifi-discuss) 

### Use Cases Catalogue   
Please, let us know if you are doing research with Mininet-WiFi. A list of citations on Mininet-WiFi is available [here](https://docs.google.com/spreadsheets/d/1laEhejMg6th-Urgc-_RqBi2H6m308Rnh9uJpKZavEio/edit?usp=sharing).     

## Installation  
**We highly recommend using Ubuntu version 16.04 or higher. Some new hostapd features might not work on Ubuntu 14.04.**  
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

### Building Topologies with GUI

![](https://github.com/ramonfontes/vnd/blob/master/miniedit.png)

I'm a beginner, I do not know Python and I would like to create a customized topology. In this case, both [MiniEdit](https://github.com/intrig-unicamp/mininet-wifi/blob/master/examples/miniedit.py) and [VND](https://github.com/ramonfontes/vnd) can serve as support since they provide a GUI to generate Python scripts. 


### Development
For instructions about easier development check [this helper file](doc/dev_help.md).

## Pre-configured Virtual Machine    
[Ubuntu 16.04 x64 :: Password: wifi](https://intrig.dca.fee.unicamp.br:8840/owncloud/index.php/s/1RS9FM5gWI43NDT)      
user: wifi   
pass: wifi   
   
## Note
Mininet-WiFi should work fine in any Ubuntu distribution from 14.04, but in some cases (only if you have problems when start it) you have to stop NetworkManager with `stop network-manager` (you can also use `sudo systemctl stop network-manager` or `sudo service network-manager stop`).    

## Book  
We are glad to announce that the Mininet-WiFi book has been published!   
  
While the 1st edition (237 pages) is in Portuguese, affordably available in Printed and E-Book versions, we are about to start to work on the 2nd edition in English. Hard to say when it will be out but we target 2019!   

Here are the pointers to the book:   

Printed version: https://www.clubedeautores.com.br/livro/emulando-redes-sem-fio-com-mininet-wifi   

Printed and e-Book version: https://www.amazon.com.br/dp/B07QM2BBRF/

Github repository with all use cases presented in the book:   
https://github.com/ramonfontes/mn-wifi-book-pt   

### Team
Ramon dos Reis Fontes (ramonrf@dca.fee.unicamp.br)  
Christian Rodolfo Esteve Rothenberg (chesteve@dca.fee.unicamp.br)  

We are members of [INTRIG (Information & Networking Technologies Research & Innovation Group)](http://intrig.dca.fee.unicamp.br) at University of Campinas - Unicamp, SP, Brazil.
