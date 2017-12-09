#!/usr/bin/python3

from subprocess import check_output as co
from sys import exit

# Actually run bin/mn rather than importing via python path
version = 'Mininet-WiFi ' + co( 'PYTHONPATH3=. bin/mn --wifi --version 2>&1', shell=True ).decode('utf-8')
version = version.strip()

# Find all Mininet path references
lines = co( "egrep -or 'Mininet-WiFi [0-9\.\+]+\w*' *", shell=True ).decode('utf-8')

error = False

for line in lines.split( '\n' ):
    if line and 'Binary' not in line:
        fname, fversion = line.split( ':' )
        if version != fversion:
            print( "%s: incorrect version '%s' (should be '%s')" % (
                fname, fversion, version ) )
            error = True

if error:
    exit( 1 )
