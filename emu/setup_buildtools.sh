#! /bin/bash

echo "-----------------------------"
echo "set up tools for building xv6"

sudo apt-get update
sudo apt-get upgrade -y
sudo apt-get install git -y
sudo apt-get install gcc -y
sudo apt-get install make -y
sudo apt-get install gdb -y
sudo apt-get install qemu -y

echo "tools was installed"


echo "-----------------------------"
echo "git clone xv6"

git clone git://github.com/mit-pdos/xv6-public.git

echo "git clone was finished"
