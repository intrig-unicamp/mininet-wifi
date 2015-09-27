#!/usr/bin/env python

"Setuptools params"

from setuptools import setup, find_packages
from os.path import join

# Get version number from source tree
import sys
sys.path.append( '.' )
from mininet.net import VERSION

scripts = [ join( 'bin', filename ) for filename in [ 'mn' ] ]

modname = distname = 'mininet'

setup(
    name=distname,
    version=VERSION,
    description='Process-based OpenFlow emulator',
    author='Bob Lantz - Modified by Ramon Fontes',
    author_email='rlantz@cs.stanford.edu - ramonrf@dca.fee.unicamp.br',
    packages=[ 'mininet', 'mininet.examples' ],
    long_description="""
        Mininet-WiFi is a network emulator which uses lightweight
        virtualization to create virtual networks for rapid
        prototyping of Software-Defined Wireless Network (SDWN) designs
        using OpenFlow. http://intrig.dca.fee.unicamp.br/index.php/projects/projects.html
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
