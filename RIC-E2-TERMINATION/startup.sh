#!/bin/sh

dockerIp=$(ifconfig eth0 | awk '{ print $2}' | grep -E -o "([0-9]{1,3}[\.]){3}[0-9]{1,3}")
echo "docker ip: $dockerIp"
sed -i "s/external-fqdn=e2t.com/external-fqdn=$dockerIp/g" "/opt/e2/config/config.conf"
./e2 -p config -f config.conf
