#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <bpf/bpf_helpers.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <linux/udp.h>

#define NUM_SERVERS 3
#define NUM_THREADS 5
#define MAX_QUEUE_SIZE 50
#define LOAD_BALANCER_PORT 11000
#define SERVER_PORTS 13000


struct bpf_map_def SEC("maps") queue = {
    .type = BPF_MAP_TYPE_ARRAY,
    .key_size = sizeof(int),
    .value_size = sizeof(int),
    .max_entries = MAX_QUEUE_SIZE + 2
};


struct bpf_map_def SEC("maps") free_threads_map = {
    .type = BPF_MAP_TYPE_ARRAY,
    .key_size = sizeof(int),
    .value_size = sizeof(int),
    .max_entries = NUM_SERVERS + 1
};

SEC("xdp_load_balancer")
int load_balancer(struct xdp_md* ctx){
    void *data = (void*)(long)ctx->data;
    struct ethhdr *eth = data;
    if(eth + 1 > (struct ethhdr*)ctx->data_end){
        return XDP_PASS;
    }
    // Check if the packet is an IP packet
    if(eth->h_proto != htons(ETH_P_IP)){
        return XDP_PASS;
    }
    struct iphdr *ip = (struct iphdr*)(eth + 1);
    if(ip + 1 > (struct iphdr*)ctx->data_end){
        return XDP_PASS;
    }
    if(ip->protocol != IPPROTO_UDP){
        return XDP_PASS;
    }
    struct udphdr *udp = (struct udphdr*)(ip + 1);
    if(udp + 1 > (struct udphdr*)ctx->data_end){
        return XDP_PASS;
    }
    if(ntohs(udp->dest) != LOAD_BALANCER_PORT){
        return XDP_PASS;
    }
    void *payload = (void *)(udp + 1);
    if(payload + sizeof(int) > (void *)ctx->data_end){
        return XDP_PASS;
    }
    int *payloadData = (int *)payload;
    bpf_printk("Recieved %d", *payloadData);
    int index = NUM_SERVERS;
    int *init_flag = bpf_map_lookup_elem(&free_threads_map, &index);
    if(init_flag == NULL){
        bpf_printk("Error in looking up free_threads_map\n");
        return XDP_DROP;
    }
    if(*init_flag == 0){
        bpf_printk("Initializing free_threads_map, with threads per server = %d\n", NUM_THREADS);
        for(int i = 0;i < NUM_SERVERS;i++){
            index = i;
            int *server_thread = bpf_map_lookup_elem(&free_threads_map, &index);
            if(server_thread == NULL){
                bpf_printk("Error in looking up free_threads_map\n");
                return XDP_DROP;
            }
            *server_thread = NUM_THREADS;
        }
        *init_flag = 1;
    }
    
    // Check if packet from servers
    for(int i=0; i<NUM_SERVERS; i++){
        if(ntohs(udp->source) == SERVER_PORTS + i){
            bpf_printk("Packet from server %d at port %d\n", i, ntohs(udp->source));
            // Check queue, if empty, increment free_threads_map of this server
            index = MAX_QUEUE_SIZE;
            int *front = bpf_map_lookup_elem(&queue, &index);
            if(front == NULL){
                bpf_printk("Error in looking up queue\n");
                return XDP_DROP;
            }
            index = MAX_QUEUE_SIZE + 1;
            int *back = bpf_map_lookup_elem(&queue, &index);
            if(back == NULL){
                bpf_printk("Error in looking up queue\n");
                return XDP_DROP;
            }
            if(*front == *back){
                bpf_printk("Queue empty, incrementing number of free threads of server %d\n", i);
                index = i;
                int *server_thread = bpf_map_lookup_elem(&free_threads_map, &index);
                if(server_thread == NULL){
                    bpf_printk("Error in looking up free_threads_map\n");
                    return XDP_DROP;
                }
                *server_thread = *server_thread + 1;
                return XDP_DROP;
            }
            // Else, send the queued packet to this server
            index = *front;
            int *queueData = bpf_map_lookup_elem(&queue, &index);
            if(queueData == NULL){
                bpf_printk("Error in looking up queue\n");
                return XDP_DROP;
            }
            bpf_printk("Sending queued packet %d to server %d\n", *queueData, i);
            udp->dest = udp->source;
            udp->source = htons(LOAD_BALANCER_PORT);
            *payloadData = *queueData;
            *front = *front + 1;
            return XDP_TX;
        }
    }

    // Packet is from a client
    bpf_printk("Packet from client at port %d\n", ntohs(udp->source));
    // Check if any server has a free thread
    for(int i=0; i<NUM_SERVERS; i++){
        index = i;
        int *server_thread = bpf_map_lookup_elem(&free_threads_map, &index);
        if(server_thread == NULL){
            bpf_printk("Error in looking up free_threads_map\n");
            return XDP_DROP;
        }
        if(*server_thread > 0){
            bpf_printk("Sending packet to server %d\n", i);
            udp->dest = htons(SERVER_PORTS + i);
            udp->source = htons(LOAD_BALANCER_PORT);
            *server_thread = *server_thread - 1;
            return XDP_TX;
        }
    }
    // No server has a free thread, queue the packet, if queue is not full, else drop the packet
    bpf_printk("No server has a free thread, queueing packet\n");
    index = MAX_QUEUE_SIZE;
    int *front = bpf_map_lookup_elem(&queue, &index);
    if(front == NULL){
        bpf_printk("Error in looking up queue\n");
        return XDP_DROP;
    }
    index = MAX_QUEUE_SIZE + 1;
    int *back = bpf_map_lookup_elem(&queue, &index);
    if(back == NULL){
        bpf_printk("Error in looking up queue\n");
        return XDP_DROP;
    }
    int nextBack = *back + 1;
    if(nextBack == MAX_QUEUE_SIZE)
        nextBack = 0;
    if(nextBack == *front){
        bpf_printk("Queue full, dropping packet\n");
        return XDP_DROP;
    }
    index = *back;
    int *queueData = bpf_map_lookup_elem(&queue, &index);
    if(queueData == NULL){
        bpf_printk("Error in looking up queue\n");
        return XDP_DROP;
    }
    *queueData = *payloadData;
    *back = nextBack;
    return XDP_DROP;
}

char _license[] SEC("license") = "GPL";


