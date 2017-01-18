rm bond0.data
rm s10-eth4.data
rm bond0Th.data
rm s10-eth4Th.data

echo "applying first rule"
ovs-ofctl mod-flows s10 in_port=1,actions=output:4
ovs-ofctl mod-flows s10 in_port=2,actions=drop
ovs-ofctl mod-flows s10 in_port=3,actions=drop

end=$((SECONDS+60))
while [ $SECONDS -lt $end ]; do
    sleep 0.45
    #cat /sys/class/net/wlan0/statistics/tx_bytes >> wlan0.data
    /home/alpha/Dropbox/mininet-wifi/util/m car0 ifconfig bond0 | grep "TX packets" | awk -F' ' '{print $3}' >> bond0.data
    /home/alpha/Dropbox/mininet-wifi/util/m s10 ifconfig s10-eth4 | grep "TX packets" | awk -F' ' '{print $3}' >> s10-eth4.data

    /home/alpha/Dropbox/mininet-wifi/util/m car0 ifconfig bond0 | grep "bytes" | awk -F' ' 'NR==2{print $5}' >> bond0Th.data
    /home/alpha/Dropbox/mininet-wifi/util/m s10 ifconfig s10-eth4 | grep "bytes" | awk -F' ' 'NR==2{print $5}' >> s10-eth4Th.data
    :
done

echo "applying second rule"
ovs-ofctl mod-flows s10 in_port=1,actions=output:4
ovs-ofctl mod-flows s10 in_port=2,actions=output:4
ovs-ofctl mod-flows s10 in_port=3,actions=output:4

end=$((SECONDS+60))
while [ $SECONDS -lt $end ]; do
    sleep 0.45
    /home/alpha/Dropbox/mininet-wifi/util/m car0 ifconfig bond0 | grep "TX packets" | awk -F' ' '{print $3}' >> bond0.data
    /home/alpha/Dropbox/mininet-wifi/util/m s10 ifconfig s10-eth4 | grep "TX packets" | awk -F' ' '{print $3}' >> s10-eth4.data
    /home/alpha/Dropbox/mininet-wifi/util/m car0 ifconfig bond0 | grep "bytes" | awk -F' ' 'NR==2{print $5}' >> bond0Th.data
    /home/alpha/Dropbox/mininet-wifi/util/m s10 ifconfig s10-eth4 | grep "bytes" | awk -F' ' 'NR==2{print $5}' >> s10-eth4Th.data
    :
done


echo "applying third rule"
ovs-ofctl mod-flows s10 in_port=1,actions=output:4
ovs-ofctl mod-flows s10 in_port=2,actions=drop
ovs-ofctl mod-flows s10 in_port=3,actions=drop

end=$((SECONDS+60))
while [ $SECONDS -lt $end ]; do
    sleep 0.45
    /home/alpha/Dropbox/mininet-wifi/util/m car0 ifconfig bond0 | grep "TX packets" | awk -F' ' '{print $3}' >> bond0.data
    /home/alpha/Dropbox/mininet-wifi/util/m s10 ifconfig s10-eth4 | grep "TX packets" | awk -F' ' '{print $3}' >> s10-eth4.data
    /home/alpha/Dropbox/mininet-wifi/util/m car0 ifconfig bond0 | grep "bytes" | awk -F' ' 'NR==2{print $5}' >> bond0Th.data
    /home/alpha/Dropbox/mininet-wifi/util/m s10 ifconfig s10-eth4 | grep "bytes" | awk -F' ' 'NR==2{print $5}' >> s10-eth4Th.data
    :
done
