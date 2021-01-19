@echo off
REM This is a useful small shell script to automatically copy the Java Keystores (.jks files), .p12 containers and the DER encoded Mobility Operator Sub-CA private key to the places in the RISE V2G project where they belong. Execute this script after you executed the generateCertificates.sh script.

copy keystores\evccKeystore.jks ..\RISE-V2G-EVCC
copy keystores\evccTruststore.jks ..\RISE-V2G-EVCC
copy keystores\seccKeystore.jks ..\RISE-V2G-SECC
copy keystores\seccTruststore.jks ..\RISE-V2G-SECC

copy certs\cpsCertChain.p12 ..\RISE-V2G-SECC
copy certs\moCertChain.p12 ..\RISE-V2G-SECC

copy privateKeys\moSubCA2.pkcs8.der ..\RISE-V2G-SECC