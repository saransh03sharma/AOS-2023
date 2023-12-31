# Part - B
## 1. eBPF Program
### 1.1. Commands to compile and load the eBPF program
```
clang -O2 -Wall -target bpf -c load_balancer.c -o load_balancer.o
sudo ip link set dev lo xdpgeneric off
sudo ip link set dev lo xdpgeneric obj load_balancer.o sec xdp_load_balancer
```

## 2. Client and Server
### 2.1. Commands to compile and run the client and server
#### Execute the following commands in 4 different terminals
Run 3 servers on ports 13000, 13001 and 13002
```
gcc -o server server.c -lpthread
./server

<Input Port Number(13000, 13001 or 13002)>
```
Run the client
```
gcc -o client client.c
./client
```

## 3. eBPF Trace output
#### Execute the following command
```
sudo cat /sys/kernel/debug/tracing/trace_pipe
```

