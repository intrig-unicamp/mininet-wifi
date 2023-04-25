#!/bin/bash

#Author: Ivano Cerrato

echo "Installing the library..."

#This script must be executed with sudo
if [[ $EUID -ne 0 ]]; then
  echo "You must be root to execute this script. Terminating." 2>&1 
  exit 1
fi

tmp=`ls bin | grep .so | wc -w`

if [[ $tmp = 0 ]]; then
	echo "Nothing to be installed. Please, compile netbee before running this script." 2>&1
	echo "Terminating." 2>&1
	exit 1
fi

cp bin/*.so /usr/lib 

echo "Installation completed."
