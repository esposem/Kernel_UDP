#! /bin/bash
make > /dev/null

# ${2:-"127.0.0.1"} means that second arg ($2) get set to "127.0.0.1" if not set
if [[ $1 == "c" ]]; then
  ./user_udp/user_client ${2:-"127.0.0.1"} ${3:-"3000"}
fi

if [[ $1 == "s" ]]; then
  ./user_udp/user_server ${2:-"3000"}
fi
