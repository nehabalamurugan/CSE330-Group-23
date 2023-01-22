# Project 1

## Step 4

`make` before calling the kernal module.

```bash
sudo insmod main.ko; sudo dmesg | tail -n 1
sudo rmmod main.ko; sudo dmesg | tail -n 1
```
