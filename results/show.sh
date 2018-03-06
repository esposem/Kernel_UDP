#! /bin/bash

filename=kernel_result.txt
foldername=kernel_data

if [ -z ${1} ]; then
  echo "Usage:
    [k | u]
    folder_data file_output"
  kill $$
fi

if [[ $1 == "k" ]]; then
  filename=kernel_result.txt
  foldername=kernel_data
elif [[ $1 == "u" ]]; then
  filename=user_result.txt
  foldername=user_data
else
  filename=$1
  foldername=$2
fi

rm read_results
make read_results
./read_results $foldername $filename

gnuplot --persist -c plot.p $filename
