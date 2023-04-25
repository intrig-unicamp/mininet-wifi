PROJECT(NETBEE)

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

#NOTE [OM]: temporarily removed the NetVM related files from the build system

# Set source files
SET(SOURCES
	../../include/nbee.h
	../../include/nbee_initcleanup.h
	../../include/nbee_netpdlutils.h
	#../../include/nbee_netvm.h
	#../../include/nbee_netvmports.h
	../../include/nbee_packetdecoder.h
	../../include/nbee_packetdecoderutils.h
	../../include/nbee_packetdumpfiles.h
	#../../include/nbee_packetprocess.h
	../../include/nbee_pxmlreader.h
	../../include/nbee_packetengine.h
	../../include/nbee_extractedfieldreader.h
	../../include/nbee_profiler.h

	globals/globals.h
	globals/debug.h
	globals/debug.c
	globals/utils.h
	globals/utils.c
	globals/profiling.h
	globals/profiling.cpp
	globals/profiling-functions.h
	globals/profiling-functions.c

	decoder/netpdldecoder.h
	decoder/netpdldecoder.cpp
	decoder/netpdldecoderutils.h
	decoder/netpdldecoderutils.cpp
	decoder/netpdlexpression.h
	decoder/netpdlexpression.cpp
	decoder/netpdllookuptables.h
	decoder/netpdllookuptables.cpp
	decoder/netpdlprotodecoder.h
	decoder/netpdlprotodecoder.cpp
	decoder/netpdlstandardvars.h
	decoder/netpdlstandardvars.cpp
	decoder/netpdlvariables.h
	decoder/netpdlvariables.cpp
	decoder/pdmlmaker.h
	decoder/pdmlmaker.cpp
	decoder/pdmlreader.h
	decoder/pdmlreader.cpp
	decoder/psmlmaker.h
	decoder/psmlmaker.cpp
	decoder/psmlreader.h
	decoder/psmlreader.cpp
	decoder/pxmlreader.h
	decoder/pxmlreader.cpp

	decoder/decoderplugin/decoder_plugin.h
	decoder/decoderplugin/decoder_plugin.cpp
	decoder/decoderplugin/decoder_dnsdomainname.cpp

	decoder/showplugin/show_plugin.h
	decoder/showplugin/show_plugin.cpp
	decoder/showplugin/show_ip46name.cpp
	decoder/showplugin/show_dnsdomainname.cpp
	decoder/showplugin/show_netbiosname.cpp
	decoder/showplugin/native_showfunctions.h
	decoder/showplugin/native_showfunctions.cpp

	misc/os_utils.h
	misc/os_utils.cpp
	misc/exports.cpp
	misc/initialize.h
	misc/initialize.cpp
	misc/update_netpdlfile.cpp
	#netbee.def
	misc/resource.rc

	#netvm/netpe.cpp
	#netvm/netvm.cpp
	#netvm/netvmbytecode.cpp
	#netvm/netvmportapplication.cpp
	#netvm/netvmportbase.cpp
	#netvm/netvmportlocaladapter.cpp
	#netvm/netvmportremoteadapter.cpp

	nbpacketengine/nbpacketengine.cpp
	nbpacketengine/fieldreader.cpp
	nbpacketengine/nbeepacketengine.h
	nbpacketengine/nbeefieldreader.h
	
	#packetprocessing/packetcapture.cpp
	packetprocessing/packetdumpfile.h
	packetprocessing/packetdumpfile.cpp
	#packetprocessing/packetprocess.cpp
	packetprocessing/packet_pcapdumpfile.h
	packetprocessing/packet_pcapdumpfile.cpp
	#packetprocessing/savefile.c

	utils/asciibuffer.h
	utils/asciibuffer.cpp
	utils/netpdlutils.h
	utils/netpdlutils.c
)

IF(WIN32)
	SET(XERCES_INCLUDE_DIR ${NETBEE_SOURCE_DIR}/../../../xerces-c/include)
	SET(XERCES_LIBRARY_DIR ${NETBEE_SOURCE_DIR}/../../../xerces-c/lib)
	SET(PCRE_INCLUDE_DIR ${NETBEE_SOURCE_DIR}/../../contrib/pcre70/include)
	SET(PCRE_LIBRARY_DIR ${NETBEE_SOURCE_DIR}/../../contrib/pcre70/lib)
	SET(XERCES_LIBRARIES xerces-c_3.lib)
	SET(PCRE_LIBRARIES pcre.lib)
ELSE(WIN32)
	IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		MESSAGE (STATUS "Mac OS X detected!")
		SET (CMAKE_C_FLAGS "-fno-common")
		SET (CMAKE_CXX_FLAGS "-fno-common")
	ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	FIND_PATH(XERCES_INCLUDE_DIR 
	  NAMES
		dom/DOM.hpp
	  PATHS
  		/usr/local/include
		/usr/include
		/opt/local/include
		/opt/include
	  PATH_SUFFIXES xercesc)

	FIND_LIBRARY(XERCES_LIBRARY
	  NAMES
		xerces-c
	  PATHS
		/usr/local/lib
		/usr/lib
		/opt/local/lib
		/opt/lib
	    ${XERCES_INCLUDE_DIR}/../lib
	)


	  IF(XERCES_LIBRARY)
	    SET( XERCES_LIBRARIES ${XERCES_LIBRARY} )
	    SET( XERCES_FOUND "YES" )
	  ENDIF(XERCES_LIBRARY)

	FIND_PATH(PCRE_INCLUDE_DIR pcre.h
  		/usr/local/include
  		/usr/include
  		/opt/local/include
  		/opt/include
	)

	IF(PCRE_INCLUDE_DIR)
		FIND_LIBRARY(PCRE_LIBRARY
		  NAMES
		    pcre
		    pcrecpp
		  PATHS
		    /usr/local/lib
		    /usr/lib
		    /opt/local/lib
		    /opt/lib
		    ${PCRE_INCLUDE_DIR}/../lib
		)


		  IF(PCRE_LIBRARY)
		    SET( PCRE_LIBRARIES ${PCRE_LIBRARY} )
		    SET( PCRE_FOUND "YES" )
		  ENDIF(PCRE_LIBRARY)
	ENDIF(PCRE_INCLUDE_DIR)
ENDIF(WIN32)


# Default directories for include files
INCLUDE_DIRECTORIES(
	${NETBEE_SOURCE_DIR}/../../include
	${XERCES_INCLUDE_DIR}/../
	${PCRE_INCLUDE_DIR}
)

# Add libraries that are required for linking
LINK_LIBRARIES(	${XERCES_LIBRARIES}
		${PCRE_LIBRARIES}
		)

# Default directories for linking
IF(WIN32)
	LINK_DIRECTORIES( ${NETBEE_SOURCE_DIR}/../../lib )
	LINK_DIRECTORIES( ${NETBEE_SOURCE_DIR}/../../../WpdPack/Lib )
	LINK_DIRECTORIES( ${XERCES_LIBRARY_DIR} )
	LINK_DIRECTORIES( ${PCRE_LIBRARY_DIR} )
ENDIF(WIN32)


IF(WIN32)
# Default directories for include files
INCLUDE_DIRECTORIES(
	${NETBEE_SOURCE_DIR}/../../include
	${NETBEE_SOURCE_DIR}/../../../WPdPack/Include
	${XERCES_INCLUDE_DIR}
	${PCRE_INCLUDE_DIR}
)
ENDIF(WIN32)

# Platform-specific definitions
IF(WIN32)
	ADD_DEFINITIONS(
		-D_CRT_SECURE_NO_WARNINGS
		-D_CRT_SECURE_NO_DEPRECATE
		-D_CRT_NONSTDC_NO_DEPRECATE 
		-DWIN32_LEAN_AND_MEAN
		-DNBEE_EXPORTS
#		-DWPCAP
		-DHAVE_REMOTE
	)
ENDIF(WIN32)


# Add libraries that are required for linking
IF(WIN32)
  LINK_LIBRARIES( ${XERCES_LIBRARIES} wpcap.lib packet.lib pcre.lib nbprotodb.lib nbpflcompiler.lib nbsockutils.lib nbnetvm.lib)
ELSE(WIN32)
IF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  LINK_LIBRARIES(${XERCES_LIBRARIES} ${PCRE_LIBRARIES} pcap nbprotodb nbpflcompiler nbsockutils nbnetvm)
ELSE(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  LINK_LIBRARIES(${XERCES_LIBRARIES} ${PCRE_LIBRARIES} dl pcap nbprotodb nbpflcompiler nbsockutils nbnetvm)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
ENDIF(WIN32)




# Create output library
ADD_LIBRARY(
	nbee
	SHARED
	${SOURCES}
)


# Copy generated files in the right place
IF(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET nbee
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbee.dll ../../bin/.
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbee.lib ../../lib/.
		COMMAND cp ${CMAKE_CFG_INTDIR}/nbee.exp ../../lib/.
	)
ELSE(WIN32)
	ADD_CUSTOM_COMMAND(
		TARGET nbee
		POST_BUILD
		COMMAND cp ${CMAKE_CFG_INTDIR}/libnbee* ../../bin/.
	)
ENDIF(WIN32)


