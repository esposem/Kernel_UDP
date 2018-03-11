#! /bin/bash
server=myip=127.0.0.3' 'myport=3000
client=myip=127.0.0.3' 'myport=4000' 'destip=127.0.0.3' 'destport=3000

ns=0
tsec=0
nclients=0
ntests=10

if [[ $1 == "s" ]]; then
  filename="udp_server"
  options=$server
fi

if [[ $1 == "c" ]]; then
  filename="udp_client"
  options=$client
  if [ ! -z ${3} ] && [ "$3" != "x" ]; then
      ns=$3
  fi
  if [ ! -z ${4} ] && [ "$4" != "x" ]; then
    tsec=$4
  fi
  if [ ! -z ${5} ] && [ "$5" != "x" ]; then
    ntests=$5
  fi
  if [ ! -z ${6} ] && [ "$6" != "x" ]; then
    nclients=$6
  fi
fi

if [ -z ${2} ]; then
  echo "Usage [s or c] [l or p or t] wait_ns "
  kill $$
fi

path=$(dirname $filename)

if sudo rmmod $filename.ko 2> /dev/null; then
  echo "Module was already present. Removed it"
fi

if cd $path > /dev/null && make > /dev/null && cd -  > /dev/null;then
  echo "Module Successfully complied"
else
  echo "Could not compile the module"
  kill $$
fi

if sudo insmod ./$filename.ko $options opt=$2 ns=$ns tsec=$tsec ntests=$ntests nclients=$nclients; then
  echo "Successfully loaded Module"

  read -rp "Press enter to remove the module or Ctrl+C to exit..." key
  echo "Closing"

  if sudo rmmod $filename.ko; then
    echo "Successfuly unloaded Module"

  else
    echo "Error in unloading the module\nTerminated"
  fi

else
  echo "Terminated"
fi
