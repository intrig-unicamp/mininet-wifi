# This is a useful small shell script to automatically copy the Java Keystores (.jks files), .p12 containers and the DER encoded Mobility Operator Sub-CA private key to the places in the RISE V2G project where they belong. Execute this script after you executed the generateCertificates.sh script.

cp keystores/evccKeystore.jks ../RISE-V2G-EVCC/
cp keystores/evccTruststore.jks ../RISE-V2G-EVCC/
cp keystores/seccKeystore.jks ../RISE-V2G-SECC/
cp keystores/seccTruststore.jks ../RISE-V2G-SECC/

cp certs/cpsCertChain.p12 ../RISE-V2G-SECC/
cp certs/moCertChain.p12 ../RISE-V2G-SECC/

cp privateKeys/moSubCA2.pkcs8.der ../RISE-V2G-SECC/