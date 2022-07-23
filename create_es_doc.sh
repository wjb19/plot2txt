#!/bin/bash

if [[ ! -f $1 ]] ;
then

echo "can't locate "$1
exit
fi

rm -rf /tmp/scratch
mkdir -p /tmp/scratch
cp $1 /tmp/scratch

f=$(basename -s .pdf $1)
ff=$(ls /tmp/scratch/*pdf)
pdftoppm $ff /tmp/scratch/$f -png
files=$(ls /tmp/scratch/*png)

ds=''

rm -rf t.txt
var=1
for x in $files
do

  /opt/bin/text_lines $x | /opt/bin/san >> t.txt

  plt=$(/opt/bin/sp2txt --im $x --s $var )
  np=$(echo $plt | grep "Couldn't" | wc -c)

  if [ $np -lt 1 ]; then
    ds+=$plt" "
  fi

  let var++

done

data=( $(echo $ds) )

len=$(echo "${#data[@]} -1" | bc)
var=0;

echo -n "{\"linplots\":["


if [ $len -lt 0 ]; then
  echo "],"
fi

if [ $len -eq 0 ]; then
  echo ${data[$len]}"],"
fi

if [ $len -gt 0 ]; then
  for x in $data
  do
    if [ $var -ge $len ]; then
      break
    fi
    echo $x","
    let var++
  done

  echo ${data[$len]}"],"
fi

cat t.txt | /opt/bin/wc

echo "}"
