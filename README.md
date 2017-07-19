# Kernel_UDP
A kernel module that implements client - server communication in UDP. <br>
Tested using Ubuntu 17.4 kernel version 4.10.0-26-generic. <br>
Once running, the client send HELLO to server, that answers GOT IT.


## Files
`udp_client`: kernel module for client <br>
`udp_server`: kernel module for server <br>
`user_client`: a c user application to test that server receives message from any udp client. <br> <br>
If you want to test only the server, you could also uset `netcat -u [ipaddress] [port]` to send message.


## Usage
1. Compile ( `make` in the folder where these files are) 
2. Load server with `sudo insmod udp_server.ko` (see [Parameters](#parameters)
 for optional parameters)
3. Load client with `sudo insmod udp_client.ko` (see [Parameters](#parameters) for optional parameters)
4. Observe in /var/log/kern.log the message passing:
```
emanuele-MacBookPro kernel: [ 1560.383981] UDP Server: Server initialized [network_server_init]
emanuele-MacBookPro kernel: [ 1560.384003] UDP Server: Thread running [udp_server_start]
emanuele-MacBookPro kernel: [ 1560.384009] UDP Server: Created socket [udp_server_listen]
emanuele-MacBookPro kernel: [ 1560.384013] UDP Server: Socket is bind to any IP address of machine [udp_server_listen]
emanuele-MacBookPro kernel: [ 1566.800022] UDP Client: Client initialized [network_client_init]
emanuele-MacBookPro kernel: [ 1566.800103] UDP Client: Thread running [udp_client_start]
emanuele-MacBookPro kernel: [ 1566.800106] UDP Client: Server IP: 127.0.0.1 [udp_client_connect]
emanuele-MacBookPro kernel: [ 1566.800129] UDP Client: Created socket [udp_client_connect]
emanuele-MacBookPro kernel: [ 1566.800153] UDP Client: Connected to server [udp_client_connect]
emanuele-MacBookPro kernel: [ 1566.800156] UDP Client: Sent HELLO to server [udp_client_connect]
emanuele-MacBookPro kernel: [ 1566.800191] UDP Client: Waiting to receive a message [udp_client_connect]
emanuele-MacBookPro kernel: [ 1566.800203] UDP Server: Received message from 127.0.0.1 saying HELLO [udp_server_receive]
emanuele-MacBookPro kernel: [ 1566.800204] UDP Server: Got HELLO [connection_handler]
emanuele-MacBookPro kernel: [ 1566.800206] UDP Server: Sent message to 127.0.0.1 [udp_server_send]
emanuele-MacBookPro kernel: [ 1566.800228] UDP Client: Received message GOT IT from 127.0.0.1 (server) [udp_client_receive]
emanuele-MacBookPro kernel: [ 1566.800230] UDP Client: Got GOT IT [udp_client_connect]
emanuele-MacBookPro kernel: [ 1585.939102] UDP Client: Released socket [udp_client_receive]
emanuele-MacBookPro kernel: [ 1585.939140] UDP Client: Terminated thread [network_client_exit]
emanuele-MacBookPro kernel: [ 1585.939143] UDP Client: Module unloaded [network_client_exit]
emanuele-MacBookPro kernel: [ 1593.011248] UDP Server: Released socket [udp_server_receive]
emanuele-MacBookPro kernel: [ 1593.011295] UDP Server: Terminated thread [network_server_exit]
```
<b>Tip:</b> If you want to have a dynamic view of the kern.log file, use this command `tail -f /var/log/kern.log`

By default, server connects on localhost on port 3000.<br> The socket is bind to any IP address of machine, if you want to change it change this line `server.sin_addr.s_addr = INADDR_ANY;`

## Parameters:
On client, the server ip, server port and message length can be specified as module parameters
when loading the module: `sudo insmod udp_client.ko destip=123.12.1.2 port=3000 len=49`

On server, the server port and message length can be specified as module parameters
when loading the module: `sudo insmod udp_server.ko port=3000 len=49`

## run.sh
To fasten the loading / unloading of modules, I also created a script `run.sh`. This script calls `make`, `insmod`,
wait a command to terminate and call `rmmod`.

1. Give appropriate executing permission using `chmod`
2. Run it `./run udp_client destip=xxx.xxx.xxx.xxx port=xxxx len=xxx`
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

## Support
This is my first module, if you experience bugs, or simply want to improve it, any suggestion is appreciated. <br>
In case the module blocks (impossible to unload it, `lsmod` returns `Used by -1`, whole screen freezes),
the only thing you can do is force restart the computer (hold power button, or unplug it). If you manage to shut it down
normally, but the interface still freeze and it does not shut down (ubuntu logo with moving dots), do a force shut down.<br>
On restart, the kernel will be restored and module unloaded. <br>
I am not responsible of any use or issue you get.
