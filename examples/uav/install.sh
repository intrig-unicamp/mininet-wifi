#!/bin/bash

distro=`lsb_release -is`
release=`lsb_release -rs`

if [[ $distro == "Ubuntu" ]]; then

		if [[ $release == "20.10" || $release == "20.04" || $release == "18.10" || r$elease == "18.04" ]]; then

			echo "Downloading CoppeliaSim"
			cd examples/uav/
			wget https://www.coppeliarobotics.com/files/CoppeliaSim_Edu_V4_1_0_Ubuntu18_04.tar.xz
			tar -xJf CoppeliaSim_Edu_V4_1_0_Ubuntu18_04.tar.xz
			mv CoppeliaSim_Edu_V4_1_0_Ubuntu18_04 CoppeliaSim_Edu
			rm CoppeliaSim_Edu_V4_1_0_Ubuntu18_04.tar.xz
			cd ../../
		elif [[ $release == "24.04" || $release == "24.10" ]]; then
			echo "Downloading CoppeliaSim"
			cd examples/uav/
			wget https://downloads.coppeliarobotics.com/V4_9_0_rev6/CoppeliaSim_Edu_V4_9_0_rev6_Ubuntu24_04.tar.xz
			tar -xJf CoppeliaSim_Edu_V4_9_0_rev6_Ubuntu24_04.tar.xz
			mv CoppeliaSim_Edu_V4_9_0_rev6_Ubuntu24_04 CoppeliaSim_Edu
			rm CoppeliaSim_Edu_V4_9_0_rev6_Ubuntu24_04.tar.xz
			cd ../../
		elif [[ $release == "16.04" || $release == "16.10" ]]; then
			echo "Downloading CoppeliaSim"
			cd examples/uav/
			wget https://www.coppeliarobotics.com/files/CoppeliaSim_Edu_V4_1_0_Ubuntu16_04.tar.xz
			tar -xJf CoppeliaSim_Edu_V4_1_0_Ubuntu16_04.tar.xz
			mv CoppeliaSim_Edu_V4_1_0_Ubuntu16_04 CoppeliaSim_Edu
			rm CoppeliaSim_Edu_V4_1_0_Ubuntu16_04.tar.xz
			cd ../../
		else
			echo "Non-compatible OS."
		fi
else
		echo "Non-compatible OS."
fi
