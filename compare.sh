#! /bin/bash

if [ $# -eq 0 ]
  then
    echo "Missing argument: path of dump"
    exit 2
fi

mkdir -p testing

# limited to 50MB, just in case
cat $1 | ./crepe_stand | head -c 50000000 > ./testing/ours_raw.txt
cat $1 | /home/cs1511/bin/crepe_stand | head -c 50000000 > ./testing/reference_raw.txt

# show what commands were entered
python3 show_inputs.py $1 ./testing/ours_raw.txt > ./testing/ours.txt
python3 show_inputs.py $1 ./testing/reference_raw.txt > ./testing/reference.txt

if [[ $* == *--silent* ]]
  then
    sdiff -t -w 200 ./testing/ours.txt ./testing/reference.txt > ./testing/comparison.txt
    exit 0
fi

echo -e '\nTest Complete!'
echo 'Full diff shown below (ours on the left, reference on the right)'
colordiff -y -W 200 ./testing/ours.txt ./testing/reference.txt

echo -e '\nEnd of full diff. Incorrect lines, if any, are shown below.'
colordiff --suppress-common-lines ./testing/ours.txt ./testing/reference.txt
