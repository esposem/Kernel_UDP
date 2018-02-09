#! /bin/bash
make > /dev/null

if [[ $1 == "c" ]]; then
  shift
  ./user_client 127.0.0.1 4000 127.0.0.1 3000 $@
fi

if [[ $1 == "s" ]]; then
  shift
  ./user_server 127.0.0.1 3000 $@
fi
