#! /bin/sh
filename=$1

if [ -z ${1} ]; then
  echo "Usage file_name.ko [param_name=value ...]"
  kill $$
fi

path=$(dirname $filename)

if sudo rmmod $filename.ko 2> /dev/null; then
  echo "Module was already present. Removed it"
fi

shift # discard first parameter
clean="0"
if [ "$1" = "-c" ]; then
  echo "You set not to clean the log"
  clean="1"
  shift
fi

if cd $path > /dev/null && make > /dev/null && cd -  > /dev/null;then
  echo "Module Successfully complied"
  if sudo insmod ./$filename.ko $@; then
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
else
  echo "Could not compile the module"
fi
