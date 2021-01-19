#*******************************************************************************
# The MIT License (MIT)
#
# Copyright (c) 2015-2018  V2G Clarity (Dr. Marc Mültin) 
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#*******************************************************************************

# ===============================================================================================================
# This shell script can be used to create all necessary certificates and Keystores needed in order to
# - successfully perform a TLS handshake between the EVCC (TLSClient) and the SECC (TLSServer) and 
# - install/update a contract certificate in the EVCC.
# 
# This file shall serve you with all information needed to create your own certificate chains.
#
# Helpful information about using openssl is provided by Ivan Ristic's book "Bulletproof SSL and TLS".
# Furthermore, you should have openssl 1.0.2 (or above) installed to comply with all security requirements 
# imposed by ISO 15118. For example, openssl 0.9.8 does not come with SHA-2 for SHA-256 signature algorithms. 
# Some MacOS X installations unfortunately still use openssl < v1.0.2. You could use Homebrew to install openssl. 
# Be aware that you probably then need to use an absolute path for your openssl commands, such as 
# /usr/local/Cellar/openssl/1.0.2h_1/bin/openssl.
#
# Author: Dr. Marc Mültin (marc.mueltin@v2g-clarity.com) 
# ===============================================================================================================


# Some variables to create different outcomes of the PKI for testing purposes. Change the validity periods (given in number of days) to test 
# - valid certificates (e.g. contract certificate or Sub-CA certificate)
# - expired certificates (e.g. contract certificate or Sub-CA certificates) -> you need to reset your system time to the past to create expired certificates
# - a to be updated contract certificate
validity_contract_cert=730
validity_mo_subca1_cert=1460
validity_mo_subca2_cert=1460
validity_oem_prov_cert=1460
validity_oem_subca1_cert=1460
validity_oem_subca2_cert=1460
validity_cps_leaf_cert=90
validity_cps_subca1_cert=1460
validity_cps_subca2_cert=730
validity_secc_cert=60
validity_cpo_subca1_cert=1460
validity_cpo_subca2_cert=365
validity_v2g_root_cert=3650
validity_oem_root_cert=3650
validity_mo_root_cert=3650

# FURTHER REMARKS:
# 1. OpenSSL does not use the named curve 'secp256r1' but the equivalent 'prime256v1'. So this file uses only 'prime256v1'.
# 2. The password used is stored in the file passphrase.txt. In it, you'll find two lines. The first line is used for the -passin option, the second line for the -passout option (especially when dealing with PKCS12 containers). See also https://www.openssl.org/docs/man1.0.2/apps/openssl.html, section "Pass Phrase Arguments"


# 0) Create directories if not yet existing
rm -r keystores # the keystores in the keystores folder (if existing) need to be deleted at first, so delete the complete folder
mkdir -p certs
mkdir -p csrs
mkdir -p keystores
mkdir -p privateKeys


# 1) Create a self-signed V2GRootCA certificate
# 1.1) Create a private key
#	- private key -> -genkey
#	- with elliptic curve parameters -> ecparam
#	- using the named curve prime256v1 -> -name prime256v1
#	- encrypt the key with symmetric cipher AES-128-CBC using the 'ec' utility command -> ec -aes-128-cbc
#   - the passphrase for the encryption of the private key is provided in a file -> -passout file:passphrase.txt
#	- save the encrypted private key at the location provided -> -out 
openssl ecparam -genkey -name prime256v1 | openssl ec -aes-128-cbc -passout file:passphrase.txt -out privateKeys/v2gRootCA.key 
# 1.2) Create a CSR
#	- new -> -new
#	- certificate signing request -> req
# 	- using the previously created private key from which the public key can be derived -> -key
#	- use the passwort stored in the file to decrypt the private key -> -passin
#	- take the values needed for the Distinguished Name (DN) from the configuration file -> -config
#	- save the CSR at the location provided -> -out
openssl req -new -key privateKeys/v2gRootCA.key -passin file:passphrase.txt -config configs/v2gRootCACert.cnf -out csrs/v2gRootCA.csr
# 1.3) Create an X.509 certificate
#	- use the X.509 utility command -> x509
#	- requesting a new X.509 certificate ... -> -req
#	- ... using a CSR file that is located at -> -in
#	- we need an X.509v3 (version 3) certificate that allows for extensions. Those are specified in an extensions file ... -> -extfile
#	- ... that contains a section marked with 'ext' -> -extensions
# 	- self-sign the certificate with the previously generated private key -> -signkey
#	- use the passwort stored in the file to decrypt the private key -> -passin
#	- tell OpenSSL to use SHA-256 for creating the digital signature (otherwise SHA1 would be used) -> -sha256
#	- each issued certificate must contain a unique serial number assigned by the CA (must be unique within the issuers number range) -> -set_serial
#	- save the certificate at the location provided -> -out
# 	- make the certificate valid for 40 years (give in days) -> -days 
openssl x509 -req -in csrs/v2gRootCA.csr -extfile configs/v2gRootCACert.cnf -extensions ext -signkey privateKeys/v2gRootCA.key -passin file:passphrase.txt -sha256 -set_serial 12345 -out certs/v2gRootCACert.pem -days $validity_v2g_root_cert


# 2) Create an intermediate CPO sub-CA 1 certificate which is directly signed by the V2GRootCA certificate
# 2.1) Create a private key (same procedure as for V2GRootCA)
openssl ecparam -genkey -name prime256v1 | openssl ec -aes-128-cbc -passout file:passphrase.txt -out privateKeys/cpoSubCA1.key  
# 2.2) Create a CSR (same procedure as for V2GRootCA)
openssl req -new -key privateKeys/cpoSubCA1.key -passin file:passphrase.txt -config configs/cpoSubCA1Cert.cnf -out csrs/cpoSubCA1.csr
# 2.3) Create an X.509 certificate (same procedure as for V2GRootCA, but with the difference that we need the ‘-CA’ switch to point to the CA certificate, followed by the ‘-CAkey’ switch that tells OpenSSL where to find the CA’s private key. We need the private key to create the signature and the public key certificate to make sure that the CA’s certificate and private key match.
openssl x509 -req -in csrs/cpoSubCA1.csr -extfile configs/cpoSubCA1Cert.cnf -extensions ext -CA certs/v2gRootCACert.pem -CAkey privateKeys/v2gRootCA.key -passin file:passphrase.txt -set_serial 12345 -out certs/cpoSubCA1Cert.pem -days $validity_cpo_subca1_cert


# 3) Create a second intermediate CPO sub-CA certificate (sub-CA 2) just the way the previous intermedia certificate was created which is directly signed by the CPOSubCA1
# Differences to CPOSubCA1
#	- basicConstraints in config file sets pathlength to 0 (meaning that no further sub-CA certificates may be signed with this certificate, a leaf certificate must follow this certificate in a certificate chain)
#	- validity is set to 1 year (1 - 2 years are allowed according to ISO 15118)
openssl ecparam -genkey -name prime256v1 | openssl ec -aes-128-cbc -passout file:passphrase.txt -out privateKeys/cpoSubCA2.key
openssl req -new -key privateKeys/cpoSubCA2.key -passin file:passphrase.txt -config configs/cpoSubCA2Cert.cnf -out csrs/cpoSubCA2.csr
openssl x509 -req -in csrs/cpoSubCA2.csr -extfile configs/cpoSubCA2Cert.cnf -extensions ext -CA certs/cpoSubCA1Cert.pem -CAkey privateKeys/cpoSubCA1.key -passin file:passphrase.txt -set_serial 12345 -days $validity_cpo_subca2_cert -out certs/cpoSubCA2Cert.pem


# 4) Create an SECCCert certificate which is the leaf certificate belonging to the charging station which authenticates itself to the EVCC during a TLS handshake, signed by CPOSubCA2 certificate
# Differences to CPOSubCA1 and CPOSubCA2
# - basicConstraints sets CA to false, no pathlen is therefore set
# - keyusage is set to digitalSignature instead of keyCertSign and cRLSign
# - validity is set to 60 days (2 - 3 months are allowed according to ISO 15118)
openssl ecparam -genkey -name prime256v1 | openssl ec -aes-128-cbc -passout file:passphrase.txt -out privateKeys/secc.key 
openssl req -new -key privateKeys/secc.key -passin file:passphrase.txt -config configs/seccCert.cnf -out csrs/seccCert.csr
openssl x509 -req -in csrs/seccCert.csr -extfile configs/seccCert.cnf -extensions ext -CA certs/cpoSubCA2Cert.pem -CAkey privateKeys/cpoSubCA2.key -passin file:passphrase.txt -set_serial 12345 -days $validity_secc_cert -out certs/seccCert.pem


# 5) Create a self-signed OEMRootCA certificate (validity is up to the OEM, this example applies the same validity as the V2GRootCA)
openssl ecparam -genkey -name prime256v1 | openssl ec -aes-128-cbc -passout file:passphrase.txt -out privateKeys/oemRootCA.key 
openssl req -new -key privateKeys/oemRootCA.key -passin file:passphrase.txt -config configs/oemRootCACert.cnf -out csrs/oemRootCA.csr
openssl x509 -req -in csrs/oemRootCA.csr -extfile configs/oemRootCACert.cnf -extensions ext -signkey privateKeys/oemRootCA.key -passin file:passphrase.txt -sha256 -set_serial 12345 -out certs/oemRootCACert.pem -days $validity_oem_root_cert


# 6) Create an intermediate OEM sub-CA certificate which is directly signed by the OEMRootCA certificate (validity is up to the OEM, this example applies the same validity as the CPOSubCA1)
openssl ecparam -genkey -name prime256v1 | openssl ec -aes-128-cbc -passout file:passphrase.txt -out privateKeys/oemSubCA1.key 
openssl req -new -key privateKeys/oemSubCA1.key -passin file:passphrase.txt -config configs/oemSubCA1Cert.cnf -out csrs/oemSubCA1.csr
openssl x509 -req -in csrs/oemSubCA1.csr -extfile configs/oemSubCA1Cert.cnf -extensions ext -CA certs/oemRootCACert.pem -CAkey privateKeys/oemRootCA.key -passin file:passphrase.txt -set_serial 12345 -days $validity_oem_subca1_cert -out certs/oemSubCA1Cert.pem


# 7) Create a second intermediate OEM sub-CA certificate which is directly signed by the OEMSubCA1 certificate (validity is up to the OEM, this example applies the same validity as the CPOSubCA2)
openssl ecparam -genkey -name prime256v1 | openssl ec -aes-128-cbc -passout file:passphrase.txt -out privateKeys/oemSubCA2.key 
openssl req -new -key privateKeys/oemSubCA2.key -passin file:passphrase.txt -config configs/oemSubCA2Cert.cnf -out csrs/oemSubCA2.csr
openssl x509 -req -in csrs/oemSubCA2.csr -extfile configs/oemSubCA2Cert.cnf -extensions ext -CA certs/oemSubCA1Cert.pem -CAkey privateKeys/oemSubCA1.key -passin file:passphrase.txt -set_serial 12345 -days $validity_oem_subca2_cert -out certs/oemSubCA2Cert.pem


# 8) Create an OEM provisioning certificate which is the leaf certificate belonging to the OEM certificate chain (used for contract certificate installation)
openssl ecparam -genkey -name prime256v1 | openssl ec -aes-128-cbc -passout file:passphrase.txt -out privateKeys/oemProv.key 
openssl req -new -key privateKeys/oemProv.key -passin file:passphrase.txt -config configs/oemProvCert.cnf -out csrs/oemProvCert.csr
openssl x509 -req -in csrs/oemProvCert.csr -extfile configs/oemProvCert.cnf -extensions ext -CA certs/oemSubCA2Cert.pem -CAkey privateKeys/oemSubCA2.key -passin file:passphrase.txt -set_serial 12345 -days $validity_oem_prov_cert -out certs/oemProvCert.pem


# 9) Create a self-signed MORootCA (mobility operator) certificate (validity is up to the MO, this example applies the same validity as the V2GRootCA)
openssl ecparam -genkey -name prime256v1 | openssl ec -aes-128-cbc -passout file:passphrase.txt -out privateKeys/moRootCA.key 
openssl req -new -key privateKeys/moRootCA.key -passin file:passphrase.txt -config configs/moRootCACert.cnf -out csrs/moRootCA.csr
openssl x509 -req -in csrs/moRootCA.csr -extfile configs/moRootCACert.cnf -extensions ext -signkey privateKeys/moRootCA.key -passin file:passphrase.txt -sha256 -set_serial 12345 -out certs/moRootCACert.pem -days $validity_mo_root_cert


# 10) Create an intermediate MO sub-CA certificate which is directly signed by the MORootCA certificate (validity is up to the MO, this example applies the same validity as the CPOSubCA1)
openssl ecparam -genkey -name prime256v1 | openssl ec -aes-128-cbc -passout file:passphrase.txt -out privateKeys/moSubCA1.key 
openssl req -new -key privateKeys/moSubCA1.key -passin file:passphrase.txt -config configs/moSubCA1Cert.cnf -extensions ext -out csrs/moSubCA1.csr
openssl x509 -req -in csrs/moSubCA1.csr -extfile configs/moSubCA1Cert.cnf -extensions ext -CA certs/moRootCACert.pem -CAkey privateKeys/moRootCA.key -passin file:passphrase.txt -set_serial 12345 -days $validity_mo_subca1_cert -out certs/moSubCA1Cert.pem


# 11) Create a second intermediate MO sub-CA certificate which is directly signed by the MOSubCA1 certificate (validity is up to the MO, this example applies the same validity as the CPOSubCA2)
openssl ecparam -genkey -name prime256v1 | openssl ec -aes-128-cbc -passout file:passphrase.txt -out privateKeys/moSubCA2.key 
openssl req -new -key privateKeys/moSubCA2.key -passin file:passphrase.txt -config configs/moSubCA2Cert.cnf -out csrs/moSubCA2.csr
openssl x509 -req -in csrs/moSubCA2.csr -extfile configs/moSubCA2Cert.cnf -extensions ext -CA certs/moSubCA1Cert.pem -CAkey privateKeys/moSubCA1.key -passin file:passphrase.txt -set_serial 12345 -days $validity_mo_subca2_cert -out certs/moSubCA2Cert.pem


# 12) Create a contract certificate which is the leaf certificate belonging to the MO certificate chain (used for contract certificate installation)
# Validity can be between 4 weeks and 2 years (restricted by the contract lifetime), for testing purposes the validity will be set to 2 years
openssl ecparam -genkey -name prime256v1 | openssl ec -aes-128-cbc -passout file:passphrase.txt -out privateKeys/contract.key 
openssl req -new -key privateKeys/contract.key -passin file:passphrase.txt -config configs/contractCert.cnf -out csrs/contractCert.csr
openssl x509 -req -in csrs/contractCert.csr -extfile configs/contractCert.cnf -extensions ext -CA certs/moSubCA2Cert.pem -CAkey privateKeys/moSubCA2.key -passin file:passphrase.txt -set_serial 12345 -days $validity_contract_cert -out certs/contractCert.pem
cat certs/moSubCA2Cert.pem certs/moSubCA1Cert.pem > certs/intermediateMOCACerts.pem
openssl pkcs12 -export -inkey privateKeys/contract.key -in certs/contractCert.pem -certfile certs/intermediateMOCACerts.pem -aes128 -passin file:passphrase.txt -passout file:passphrase.txt -name contract_cert -out certs/moCertChain.p12


# 13) Create an intermediate provisioning service sub-CA certificate which is directly signed by the V2GRootCA certificate 
openssl ecparam -genkey -name prime256v1 | openssl ec -aes-128-cbc -passout file:passphrase.txt -out privateKeys/cpsSubCA1.key 
openssl req -new -key privateKeys/cpsSubCA1.key -passin file:passphrase.txt -config configs/cpsSubCA1Cert.cnf -out csrs/cpsSubCA1.csr
openssl x509 -req -in csrs/cpsSubCA1.csr -extfile configs/cpsSubCA1Cert.cnf -extensions ext -CA certs/v2gRootCACert.pem -CAkey privateKeys/v2gRootCA.key -passin file:passphrase.txt -set_serial 12345 -days $validity_cps_subca1_cert -out certs/cpsSubCA1Cert.pem


# 14) Create a second intermediate provisioning sub-CA certificate which is directly signed by the CPSSubCA1 certificate (validity 1 - 2 years, we make it 2 years)
openssl ecparam -genkey -name prime256v1 | openssl ec -aes-128-cbc -passout file:passphrase.txt -out privateKeys/cpsSubCA2.key 
openssl req -new -key privateKeys/cpsSubCA2.key -passin file:passphrase.txt -config configs/cpsSubCA2Cert.cnf -out csrs/cpsSubCA2.csr
openssl x509 -req -in csrs/cpsSubCA2.csr -extfile configs/cpsSubCA2Cert.cnf -extensions ext -CA certs/cpsSubCA1Cert.pem -CAkey privateKeys/cpsSubCA1.key -passin file:passphrase.txt -set_serial 12345 -days $validity_cps_subca2_cert -out certs/cpsSubCA2Cert.pem


# 15) Create a provisioning service certificate which is the leaf certificate belonging to the provisioning certificate chain (used for contract certificate installation)
# Validity can be between 2 - 3 months, we make it 3 months
openssl ecparam -genkey -name prime256v1 | openssl ec -aes-128-cbc -passout file:passphrase.txt -out privateKeys/cpsLeaf.key 
openssl req -new -key privateKeys/cpsLeaf.key -passin file:passphrase.txt -config configs/cpsLeafCert.cnf -out csrs/cpsLeafCert.csr
openssl x509 -req -in csrs/cpsLeafCert.csr -extfile configs/cpsLeafCert.cnf -extensions ext -CA certs/cpsSubCA2Cert.pem -CAkey privateKeys/cpsSubCA2.key -passin file:passphrase.txt -set_serial 12345 -days $validity_cps_leaf_cert -out certs/cpsLeafCert.pem
cat certs/cpsSubCA2Cert.pem certs/cpsSubCA1Cert.pem > certs/intermediateCPSCACerts.pem
openssl pkcs12 -export -inkey privateKeys/cpsLeaf.key -in certs/cpsLeafCert.pem -certfile certs/intermediateCPSCACerts.pem -aes128 -passin file:passphrase.txt -passout file:passphrase.txt -name cps_leaf_cert -out certs/cpsCertChain.p12


# 16) Finally we need to convert the certificates from PEM format to DER format (PEM is the default format, but ISO 15118 only allows DER format)
openssl x509 -inform PEM -in certs/v2gRootCACert.pem       -outform DER -out certs/v2gRootCACert.der
openssl x509 -inform PEM -in certs/cpsSubCA1Cert.pem      -outform DER -out certs/cpsSubCA1Cert.der
openssl x509 -inform PEM -in certs/cpsSubCA2Cert.pem      -outform DER -out certs/cpsSubCA2Cert.der
openssl x509 -inform PEM -in certs/cpsLeafCert.pem 		-outform DER -out certs/cpsLeafCert.der
openssl x509 -inform PEM -in certs/cpoSubCA1Cert.pem       -outform DER -out certs/cpoSubCA1Cert.der
openssl x509 -inform PEM -in certs/cpoSubCA2Cert.pem       -outform DER -out certs/cpoSubCA2Cert.der
openssl x509 -inform PEM -in certs/seccCert.pem        -outform DER -out certs/seccCert.der
openssl x509 -inform PEM -in certs/oemRootCACert.pem       -outform DER -out certs/oemRootCACert.der
openssl x509 -inform PEM -in certs/oemSubCA1Cert.pem       -outform DER -out certs/oemSubCA1Cert.der
openssl x509 -inform PEM -in certs/oemSubCA2Cert.pem       -outform DER -out certs/oemSubCA2Cert.der
openssl x509 -inform PEM -in certs/oemProvCert.pem     -outform DER -out certs/oemProvCert.der
openssl x509 -inform PEM -in certs/moRootCACert.pem        -outform DER -out certs/moRootCACert.der
openssl x509 -inform PEM -in certs/moSubCA1Cert.pem        -outform DER -out certs/moSubCA1Cert.der
openssl x509 -inform PEM -in certs/moSubCA2Cert.pem        -outform DER -out certs/moSubCA2Cert.der
openssl x509 -inform PEM -in certs/contractCert.pem    -outform DER -out certs/contractCert.der
# Since the intermediate certificates need to be in PEM format when putting them in a PKCS12 container and the resulting PKCS12 file is a binary format, it might be sufficient. Otherwise, I have currently no idea how to covert the intermediate certificates in DER without running into problems when creating the PKCS12 container.


# 17) In case you want the private keys in PKCS#8 file format and DER encoded, use this command. Especially necessary for the private key of MOSubCA2 in RISE V2G
openssl pkcs8 -topk8 -in privateKeys/moSubCA2.key -inform PEM -passin file:passphrase.txt -passout file:passphrase.txt -outform DER -out privateKeys/moSubCA2.pkcs8.der -v1 PBE-SHA1-3DES


# 18) Create the keystores for the EVCC and SECC. We need to first create the PKCS12 files and then import them into the JKS using the 'keytool' command.
# 	- create a PKCS12 file -> -export
#	- if no private keys are added, e.g. for a truststore that holds only root CA certificates -> -nokeys
# 	- add a private key to the newly created PKCS12 container; must be in PEM format (not DER) -> -inkey
#	- add any certificate (leaf certificate or root certificate); must be in PEM format (not DER) -> -in
#	- add additional certificats (intermediate sub-CA certificates or more root CA certificates); must be in PEM format (not DER) -> -certfile
#	- use AES to encrypt private keys before outputting -> -aes128
#	- passphrase source to decrypt any private keys that are to be imported -> -passin
#	- passphrase to encrypt any outputted private keys with -> -passout
#	- provide the "friendly name" for the leaf certificate, which is used as the alias in Java -> -name
# 	- provide the "friendly name" for CA certificates, which is used as the alias in Java;
#	  this option may be used multiple times to specify names for all certificates in the order they appear -> -caname
#	- the file location to store the PKCS12 data container -> -out
#
# 18.1) SECC keystore needs to hold initially hold the OEM provisioning certificate (contract certificate will be installed with ISO 15118 message exchange)
# First, concatenate the intermediate sub-CA certificates into one file intermediateCAs.pem
# IMPORTANT: Concatenate in such a way that the chain leads from the leaf certificate to the root (excluding), this means here: first parameter of the cat command is the sub-CA certificate which signs the leaf certificate (in this case cpoSubCA2.pem). Otherwise the Java method getCertificateChain() which is called on the keystore will only return the leaf certificate!
cat certs/cpoSubCA2Cert.pem certs/cpoSubCA1Cert.pem > certs/intermediateCPOCACerts.pem
# Put the seccCertificate, the private key associated with the seccCertificate as well as the intermediate sub-CA certificates in a PKCS12 container with the -certfile switch.
openssl pkcs12 -export -inkey privateKeys/secc.key -in certs/seccCert.pem -name secc_cert -certfile certs/intermediateCPOCACerts.pem -caname mo_subca_2 -caname mo_subca_1 -aes128 -passin file:passphrase.txt -passout file:passphrase.txt -out keystores/cpoCertChain.p12
keytool -importkeystore -srckeystore keystores/cpoCertChain.p12 -srcstoretype pkcs12 -srcstorepass:file passphrase.txt -srcalias secc_cert -destalias secc_cert -destkeystore keystores/seccKeystore.jks -storepass:file passphrase.txt -noprompt
#
# 18.2) EVCC keystore needs to initally hold the OEM provisioning certificate (contract certificate will be installed with ISO 15118 message exchange)
cat certs/oemSubCA2Cert.pem certs/oemSubCA1Cert.pem > certs/intermediateOEMCACerts.pem
openssl pkcs12 -export -inkey privateKeys/oemProv.key -in certs/oemProvCert.pem -name oem_prov_cert -certfile certs/intermediateOEMCACerts.pem -caname oem_subca_2 -caname oem_subca_1 -aes128 -passin file:passphrase.txt -passout file:passphrase.txt -out keystores/oemCertChain.p12
keytool -importkeystore -srckeystore keystores/oemCertChain.p12 -srcstoretype pkcs12 -srcstorepass:file passphrase.txt -srcalias oem_prov_cert -destalias oem_prov_cert -destkeystore keystores/evccKeystore.jks -storepass:file passphrase.txt -noprompt


# 19) Create truststores for EVCC and SECC. Storing trust anchors in PKCS12 is not supported in Java 8. For creating trusstores we need a Java KeyStore (JKS) and import the DER encoded root CA certificates.
#
# 19.1) EVCC truststore needs to hold the V2GRootCA certificate 
keytool -import -keystore keystores/evccTruststore.jks -alias v2g_root_ca -file certs/v2gRootCACert.der -storepass:file passphrase.txt -noprompt
#
# 19.2) SECC truststore needs to hold the V2G root CA certificate and MO root CA certificate. 
# According to ISO 15118-2, MO root CA is not necessarily needed as the MO sub-CA 1 could instead be signed by a V2G root CA.
keytool -import -keystore keystores/seccTruststore.jks -alias v2g_root_ca -file certs/v2gRootCACert.der -storepass:file passphrase.txt -noprompt
keytool -import -keystore keystores/seccTruststore.jks -alias mo_root_ca -file certs/moRootCACert.der -storepass:file passphrase.txt -noprompt


# Side notes for OCSP stapling in Java: see http://openjdk.java.net/jeps/249
