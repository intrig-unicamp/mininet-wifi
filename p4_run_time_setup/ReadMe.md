# Summary

The script `start.sh` installs all the necessary dependencies to run a P4 switch, it installs:
- Protobuf
- grpc
- PI
- Behavioral-model (BMv2)
- P4C

For a docker image with P4 already built from the official mn-wifi docker iamge see: https://hub.docker.com/r/davidcc73/mn-wifi-p4


# Instructions

To start installing the P4 elements, attach to the container, go to root, andrun:
```
./p4_run_time_setup/start.sh
```

`Warnings`: 
- Sometimes the script fails to install the behavioral model, and the P4C, but the installation process can be continued by running the remaining code manually and directly in the terminal.

- Not all packages have Debian 12 versions, so some (still compatible) Debian 11 packages were used and the remaining ones are compiled from source, ideally in the future they should be replaced by official Debian 12 ones.

- The P4C is compiled from source and every time it fails one or more build test, that is way said testes (`make -j4 check`) are commented in the `start.sh`, run your image tests to know which ones are failing. Good P4C containers already exist so it is easy to just pre-compile the code and then push it to the mininet container, either directly via a volume or using a SDN controller that can access the compiled P4 code.