This example exercises the DPP functionality in hostapd / wpa\_supplicant

Please enable  INTERWORKING in your hostap/.config file and build it with this flag.

CONFIG\_INTERWORKING=y

Please ensure the following flag is set both in your .config file for wpa\_supplicant AND hostapd.

CONFIG\_DPP=y

