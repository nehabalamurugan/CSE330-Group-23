#!/usr/bin/env bash
make -B

sudo insmod producer_consumer.ko buffSize=5 prod=1 cons=1 uuid=1000; sudo dmesg -wH
sleep 1
sudo rmmod producer_consumer.ko buffSize=5 prod=1 cons=1 uuid=1000; sudo dmesg -wH

make clean
