#!/bin/bash

echo "Downloading CoppeliaSim"
cd examples/uav/
wget https://www.coppeliarobotics.com/files/CoppeliaSim_Edu_V4_1_0_Ubuntu18_04.tar.xz
tar -xJf CoppeliaSim_Edu_V4_1_0_Ubuntu18_04.tar.xz
rm CoppeliaSim_Edu_V4_1_0_Ubuntu18_04.tar.xz
cd ../../

