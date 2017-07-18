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

    # sleep .5
    # echo "------------------ /var/log/kern.log ------------------"
    # gnome-terminal -x sh -c "tail -f /var/log/kern.log; bash"
    # echo "--------------------------------------------------------"

    read -rp "Press enter to remove the module or Ctrl+C to exit..." key
    # sleep 1
    echo "Closing"

    if sudo rmmod $filename.ko; then
      echo "Successfuly unloaded Module"

      # sleep .5
      # echo "------------------ /var/log/kern.log ------------------"
      # cat /var/log/kern.log
      # gnome-terminal -x sh -c "cat /var/log/kern.log; bash"

      # gnome-terminal -x sh -c "tail -f /var/log/kern.log; bash"
      # echo "--------------------------------------------------------"
    else
      echo "Error in unloading the module\nTerminated"
    fi

    # if [ $clean = "0" ] && sudo tee /var/log/kern.log; then
    #   echo "Cleaned kernel log"
    # else
    #   echo "Didn't clean the kernel log\nTerminated"
    # fi

  else
    echo "Terminated"
  fi
else
  echo "Could not compile the module"
fi
