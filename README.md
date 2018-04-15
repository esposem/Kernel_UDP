# Kernel_UDP
Two kernel modules that implement UDP client - server communication. <br>
Tested using Ubuntu 17.4 kernel version 4.10.0-26-generic. <br>

Once running, the client send HELLO to server, that answers OK.

## Files
`kern_udp/udp_client.c`: kernel module for client <br>
`kern_udp/udp_server.c`: kernel module for server <br>

Each module has the equivalent in user space under the folder used_udp. They also have the same parameters and perform the same
operations

## Usage
1. Compile ( `make` in this folder)
2. Load server with `sudo insmod udp_server.ko` (see [Parameters](#parameters) for optional parameters)
3. Load client with `sudo insmod udp_client.ko` (see [Parameters](#parameters) for optional parameters)
4. Observe in /var/log/kern.log the message passing:<br>
Load server
```
[  +4.514179] k_server: opt: p
[  +0.000006] Socket created
[  +0.000008] Socket is bind to 127.0.0.1:4000
[  +0.000057] k_server: Thread running
[  +0.000031] k_server: Performing a simple test: this module will wait to receive HELLO from client, replying with OK
```
Load client
```
[ +11.079634] k_client: Destination is 127.0.0.1:4000
[  +0.000003] k_client: opt: p, ns: 1, tsec: -1, ntests:1
[  +0.000003] Socket created
[  +0.000007] Socket is bind to 127.0.0.1:3000
[  +0.000104] k_client: Thread running
[  +0.000046] k_client: Performing a simple test: this module will send OK and will wait to receive HELLO from server
[  +0.000103] k_client: Sent HELLO (size 6) to 127.0.0.1 : 4000
[  +0.000096] k_server: Received HELLO (size 6) from 127.0.0.1:3000
[  +0.000009] k_server: Sent OK (size 6) to 127.0.0.1:3000
[  +0.000002] k_client: Received OK (size 6) from 127.0.0.1:4000
[  +0.000001] k_client: All done, terminating client
```
If you want to have a dynamic view of the kern.log file, use the command `tail -f /var/log/kern.log` or `dmesg -wH`

## Parameters:
On client, the server ip, server ip and port can be specified as module parameters
when loading the module: <br> `sudo insmod udp_client.ko myip=123.12.1.0 myport=2900 destip=123.12.1.2 destport=3000`

On server, the server ip and port can be specified as module parameters
when loading the module: <br> `sudo insmod udp_server.ko myip=123.12.1.2 port=3000`

For the other options, type `modinfo module_name.ko` in the terminal to get these informations.

The module features 4 operation types:<br>
- PRINT simple HELLO-OK passing<br>
- TROUGHPUT checks how many messages a client can send and how
many the server can receive<br>
- LATENCY checks the average latency<br>
- SIMULATION simulates n clients sending messages and counting
how many can be received back each second. Prints the result in
a file.


## u_run.sh and k_run
To make the loading / unloading of modules easier, I also created a script `k_run.sh`. This script calls `make`, `insmod`,
wait a command to terminate and call `rmmod`.

1. Give appropriate executing permission using `chmod`
2. Run it `./k_run module_name [other optionals parameters]`
3. Once unloaded the module, it will wait for `enter` or `ctrl-c` to either continue and unload the module or terminate.

If the module is already running and this script is called, the module is unloaded first, recompiled, and loaded again.
```
emanuele@emanuele-MacBookPro:~/Desktop/Kernel_Modules/Kernel_UDP$ ./run.sh udp_client
Module Successfully complied
Successfully loaded Module
Press enter to remove the module or Ctrl+C to exit...
Closing
Successfuly unloaded Module
```

Same is with `u_run.sh` for the user space applications.

## Disclaimer
These are my first kernel modules, if you experience bugs, or simply want to improve it, any suggestion is appreciated. <br>

In case the module blocks (impossible to unload it, `lsmod` returns `Used by -1`, whole screen freezes),
the only thing you can do is force restart the computer (hold power button, or unplug it). If you manage to shut it down
normally, but the interface still freeze and it does not shut down (ubuntu logo with moving dots), do a force shut down.<br>
On restart, the kernel will be restored and module unloaded. <br>
I am not responsible of any use you do or any damage it might create.
