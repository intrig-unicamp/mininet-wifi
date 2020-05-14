# BMv2 in Network Namespaces Example

Example where you can see the use of the [`P4RuntimeAP`](https://github.com/intrig-unicamp/mininet-wifi/blob/p4/mn_wifi/bmv2.py) class. This class manages the launch of the BMV2 and configures it via P4Runtime. Additionally, in this example you can see how it is possible to run the BMV2 in different Network Namespaces, this has been achieved by using the class [`Netns_mgmt`](https://github.com/intrig-unicamp/mininet-wifi/blob/p4/mn_wifi/node.py). This class allows us to execute python code at runtime on different Netns,  thus managing to carry out the configuration via P4Runtime on each Netns. As a future work, we propose to carry out the configuration via P4Runtime from the root-Netns, assigning a control IP to each softswitch and establishing the control plane logic so that all softswitches are reachable by a hypothetical external controller.



As already mentioned in the previous README, it is necessary to have the dependencies associated with the p4 environment installed to carry out the execution of this example.


To run the example, it is first necessary to compile the P4 program:

```bash

sudo make

```

Which will create a directory structure where useful information for the execution of the example will be stored, and it will compile our P4 program (storing it under the `build` directory).

Once the P4 program is compiled, you can run the `test.py` script which will build the topology in Mininet-Wifi.

```bash

sudo python test.py

```


 
