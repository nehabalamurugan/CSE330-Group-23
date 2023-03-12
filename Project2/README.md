# Project 2

`make` first

```bash
sudo insmod producer_consumer.ko buffSize=5 prod=1 cons=1 uuid=1000; sudo dmesg | tail -n 50
sudo rmmod producer_consumer.ko buffSize=5 prod=1 cons=1 uuid=1000; sudo dmesg | tail -n 50
```
