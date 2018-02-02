#! /bin/bash
make > /dev/null
filename=$1
if [[ $1 == "c" ]]; then
  ./user_udp/user_client ${2:-"127.0.0.1"} ${3:-"3000"}
fi

if [[ $1 == "s" ]]; then
  ./user_udp/user_server ${2:-"3000"}
fi
