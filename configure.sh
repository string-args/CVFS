#!/bin/bash

# install requirements
apt-get install open-iscsi
apt-get install apache2
apt-get install sqlite3
apt-get install php5-sqlite
apt-get install php5
apt-get install lvm2
apt-get install nmap
apt-get install samba
apt-get install libapache2-mod-php5

# make directories being used by the cvfs2
mkdir -m 777 /mnt/Share
mkdir -m 777 /mnt/CVFSTemp
mkdir -m 777 /mnt/CVFSCache
mkdir -m 777 /mnt/CVFStorage
mkdir -m 777 /mnt/CVFSFStorage
