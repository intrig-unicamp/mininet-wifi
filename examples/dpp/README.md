This example exercises the DPP functionality in hostapd / wpa_supplicant

Please enable  INTERWORKING in your hostap/.config file and build it with this flag.

        CONFIG_INTERWORKING=y

Please ensure the following flag is set both in your .config file for wpa\_supplicant AND hostapd.

        CONFIG_DPP=y

In wpa\_supplicant .config file, you need the following 

        CONFIG_IEEE80211W=y

Build hostapd and wpa\_supplicant with these flags enabled.

The DPP protocol is used to onboard headless (IOT) devices.

Here is the sequence of commands we are trying to exercise (this is in the script, you don't need to do this manually):

Configurator: add a configurator object

    wpa_cli -p /var/run/wpa_supplicant1  dpp_configurator_add  

Configurator: get the configurator key using the returned ID

    wpa_cli -p /var/run/wpa_supplicant1  dpp_configurator_get_key 1

Configurator: self sign the configurator

    wpa_cli -p /var/run/wpa_supplicant1  dpp_configurator_sign conf=sta-psk psk=29153c1e60c0e50afa47530eb7b6db1193b0131616c139e9f1785d174861cca7 ssid=012a configurator=1'

Enrollee: generate QR code for device

    wpa_cli -p /var/run/wpa_supplicant2 dpp_bootstrap_gen  type=qrcode mac=00:00:00:00:00:82 key=.....

Enrollee: get the qr code using the returned bootstrap\_info\_id

    wpa_cli -p /var/run/wpa_supplicant2 dpp_bootstrap_get_uri 1

Enrollee: listen for dpp provisioning request

    wpa_cli -p /var/run/wpa_supplicant2 dpp_listen 2412

Configurator:  Enter the sta QR Code in the Configurator.

    wpa_cli -p /var/run/wpa_supplicant1  dpp_qr_code 'DPP:.....'

Configurator: Send provisioning request to enrollee. 

    wpa_cli -p /var/run/wpa_supplicant1  dpp_auth_init peer=1 conf=sta-psk ssid=012a psk=.... configurator=1

Enrollee: save the config file

    wpa_cli -p /var/run/wpa_supplicant2 save_config

Enrollee:  reload the config file
wpa_cli -p /var/run/wpa_supplicant2 reconfigure

In the provided example, sta1 is the configurator and sta2 is the enrollee
