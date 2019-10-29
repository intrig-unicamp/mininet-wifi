This example exercises the DPP functionality in hostapd / wpa\_supplicant

Please enable  INTERWORKING in your hostap/.config file and build it with this flag.

CONFIG\_INTERWORKING=y

Please ensure the following flag is set both in your .config file for wpa\_supplicant AND hostapd.

CONFIG\_DPP=y

The DPP protocol is used to onboard headless (IOT) devices.

Here is the sequence of commands we are trying to exercise

Configurator: add a configurator object
wpa\_cli -p /var/run/wpa\_supplicant1  dpp\_configurator\_add  

Configurator: get the configurator key using the returned ID
wpa\_cli -p /var/run/wpa\_supplicant1  dpp\_configurator\_get\_key 1

Configurator: self sign the configurator
wpa\_cli -p /var/run/wpa\_supplicant1  dpp\_configurator\_sign conf=sta-psk psk=29153c1e60c0e50afa47530eb7b6db1193b0131616c139e9f1785d174861cca7 ssid=012a configurator=1'

Enrollee: generate QR code for device
wpa\_cli -p /var/run/wpa\_supplicant2 dpp\_bootstrap\_gen  type=qrcode mac=00:00:00:00:00:82 key=.....

Enrollee: get the qr code using the returned bootstrap\_info\_id
wpa\_cli -p /var/run/wpa\_supplicant2 dpp\_bootstrap\_get\_uri 1

Enrollee: listen for dpp provisioning request
wpa\_cli -p /var/run/wpa\_supplicant2 dpp\_listen 2412

Configurator:  Enter the sta QR Code in the Configurator.
wpa\_cli -p /var/run/wpa\_supplicant1  dpp\_qr\_code 'DPP:.....'

Configurator: Send provisioning request to enrollee. 
wpa\_cli -p /var/run/wpa\_supplicant1  dpp\_auth\_init peer=1 conf=sta-psk ssid=012a psk=.... configurator=1

Enrollee: save the config file
wpa\_cli -p /var/run/wpa\_supplicant2 save\_config

Enrollee:  reload the config file
wpa\_cli -p /var/run/wpa\_supplicant2 reconfigure

In the provided example, sta1 is the configurator and sta2 is the enrollee
