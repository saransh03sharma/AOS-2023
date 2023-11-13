# Part - A
## 1. eBPF Program
### 1.1. Commands to compile and load the eBPF program
```
clang -O2 -Wall -target bpf -c xdp_drop.c -o xdp_drop.o
sudo ip link set dev lo xdpgeneric off
sudo ip link set dev lo xdpgeneric obj xdp_drop.o sec xdp_drop
```

## 2. Client and Server
### 2.1. Commands to compile and run the client and server
Server:
```
gcc -o server server.c
./server
```
Client:
```
gcc -o client client.c
./client
```

## 3. eBPF Trace output
#### Execute the following command
```
sudo cat /sys/kernel/debug/tracing/trace_pipe
```

