#!/usr/bin/env python

"Setuptools params"

from setuptools import setup
from os.path import join

# Get version number from source tree
import sys
sys.path.append( '.' )
from mininet.net import VERSION

scripts = [ join( 'bin', filename ) for filename in [ 'mn' ] ]

modname = distname = 'mininet-wifi'

setup(
    name=distname,
    version=VERSION,
    description='Process-based OpenFlow emulator',
    author='Bob Lantz; Ramon Fontes',
    author_email='rlantz@cs.stanford.edu; ramonrf@dca.fee.unicamp.br',
    packages=[ 'mn_wifi', 'mn_wifi.sixLoWPAN', 'mn_wifi.data', 'mn_wifi.examples', 'mn_wifi.sumo', 'mn_wifi.sumo.sumolib',
               'mn_wifi.sumo.traci', 'mn_wifi.sumo.data', 'mn_wifi.sumo.sumolib.net', 'mn_wifi.sumo.sumolib.output',
               'mn_wifi.sumo.sumolib.shapes', 'mn_wifi.utils' ],
    package_data={'mn_wifi.sumo.data': ['*.xml', '*.sumocfg'], 'mn_wifi.data': ['signal_table_ieee80211ax']},
    long_description="""
        Mininet-WiFi is a network emulator which uses lightweight
        virtualization to create virtual networks for rapid
        prototyping of Software-Defined Wireless Network (SDWN) designs
        using OpenFlow.
        """,
    classifiers=[
          "License :: OSI Approved :: BSD License",
          "Programming Language :: Python",
          "Development Status :: 5 - Production/Stable",
          "Intended Audience :: Developers",
          "Topic :: System :: Emulators",
    ],
    keywords='networking emulator protocol Internet OpenFlow SDN',
    license='BSD',
    install_requires=[
        'setuptools'
    ],
    scripts=scripts,
)