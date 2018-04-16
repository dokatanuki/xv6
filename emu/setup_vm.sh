#! /bin/bash

echo "initialize Vagrantfile and boot vm"
echo "-----------------------------------"
vagrant plugin install vagrant-vbguest
vagrant init ubuntu/xenial64
vagrant up --provider virtualbox
vagrant ssh-config > ssh.config
scp -F ssh.config setup_buildtools.sh vagrant@default:~/
echo "-----------------------------------"
echo "run \"vagrant ssh\" and set up environment"
