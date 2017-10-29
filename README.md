# Breadboard Recording Software

This repository is the operational distribution for the SEED 2017 Raspberry Pi's to hook into the breadboard-monitoring software we're testing. 

## Quick Setup:
This repo should live in the `pi` user's home directory.  Inside of the repo is a hidden file called `bb_login` that must have the user's credentials entered like so:

```
user
password
```

Code was written assuming >Python3.4, so no promises if it doesn't work on earlier versions. 

Firstly run to install the necessary hardware and imaging libraries:
```
pip3 install -r req
```
Make sure the most recent versions get installed.


One-time use of the code can be started simply by running the program with Python.  

For more standard operation, so that it is always running, we'll use `systemd/systemctl`.  

First cp the file `bb.service` file into `/lib/systemd/system/`.  Then give proper permissions with the following command (we'll call it `bb`, but you can call it whatever)

```
sudo chmod 644 /lib/systemd/system/bb.service
```

Then turn on daemon-reload:

```
sudo systemctl daemon-reload
```

Then enable the service:

```
sudo systemctl enable bb.service
```

Then you might as well reboot:

```
sudo reboot
```

When the computer comes back, check to see if stuff is running. No errors is a good thing:

```
sudo systemctl status bb.service
```

