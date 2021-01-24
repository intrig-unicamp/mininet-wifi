# Parameter list for RiseV2G

This doc shows all the possible properties which can be used on RiseV2G version 1.2.6 (and probably also in the following).

All the properties can be set using `setProperty(<property-name>, <property-value>)` on MiniV2G. Furthermore, the most used properties have specific methods to make the setting more easy (e.g., `setExiCodec(<exiCodec-value>)`)

## Parameters for both EV and SE

- `network.interface`: the network interface name like en3 or eth1 of the network interface on which to communicate with the EVCC via a link-local IPv6 address

- The following are related to logs and can be modified on MiniV2G using the `setLogging()` function:
    - `exi.messages.showxml`: XML representation of messages. If 'true', the EXICodec will print each message's XML representation (for debugging purposes) .If no correct value is provided here, 'false' will be chosen.
    - `exi.messages.showhex`: Hexadecimal and Base64 representation of messages. If 'true', the EXICodec will print each message's hexadecimal and Base64 representation (for debugging purposes). If no correct value is provided here, 'false' will be chosen
    - `signature.verification.showlog`: Extended logging of signature verification. If 'true', extended logging will be printed upon verification of signatures (for debugging purposes). If no correct value is provided here, 'false' will be chosen
    
- `exi.codec`: EXI Codec. This (single!) value tells the program which EXI codec to use to en-/decode EXI messages. Possible values are:
    - exificient
    - open_exi

## SE's parameters

- `energy.transfermodes.supported`: supported energy transfer modes. Select one or a comma separated list of:
    - AC_single_phase_core
    - AC_three_phase_core
    - DC_core
    - DC_extended
    - DC_combo_core
    - DC_unique
    
- `charging.free`: is charging free? (true/false)
- `authentication.modes.supported`: select payment options from the following values:
    - Contract
    - ExternalPayment
    
    Take care that Contract is available only with TLS.
- `environment.private`: Is the SECC located in a private environment? In a private environment, TLS mechanisms work a bit differently than in a public environment. (true/false) 

- `implementation.secc.backend`, `implementation.secc.acevsecontroller`, `implementation.secc.dcevsecontroller`: 
  Implementation classes: if you want to replace the implementations then set the following values to the name of your classes. When omitted default dummy implementations will be used.


## EV's parameters
- `implementation.evcc.controller`: Implementation classes. If you want to replace the implementation then set the following value  to the name of your class. When omitted default dummy implementation will be used
- `voltage.accuracy`: Voltage accuracy. Used for the PreCharge target voltage. The present voltage indicated by the charging station in PreChargeRes can deviate from the present voltage set in PreChargeReq by an EV-specific deviation factor. This value is given in percent. Example: voltage.accuracy = 10 means: present voltage may deviate from target voltage by 10 percent in order to successfully stop PreCharge.
- `session.id`: Hexadecimal string representing a byte array. If this value is unequal to "00", then it represents a previously paused V2G communication session.
- `tls`: Security. If this value is set to 'false', TCP will be used on transport layer. If no correct value is provided here, 'false' will be chosen.
- `authentication.mode`: Selected payment option. This (single!) value needs to be provided from a previous charging session if charging has been paused. Possible values are:
    - Contract 
    - ExternalPayment
- `energy.transfermode.requested`: Requested energy transfer mode. This (single!) value needs to be provided from a previous charging session if charging has been paused. Possible values are:
    - AC_single_phase_core
    - AC_three_phase_core
    - DC_core
    - DC_extended
    - DC_combo_core
    - DC_unique
- `contract.certificate.update.timespan`: Contract certificate update time span. Integer value defining the time span in days which precedes the expiration of a contract certificate  and during which an update of the contract certificate needs to be performed