#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <arpa/inet.h>
#include <bpf/bpf_helpers.h>
// #include <linux/pkt_cls.h>
// #include <sys/resource.h>


SEC("xdp_drop")
int packet_dropper(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    // struct ethhdr *eth = bpf_hdr_pointer(skb);
    struct ethhdr *eth = data;
    if (eth + 1 > (struct ethhdr *)ctx->data_end) {
        return XDP_PASS;
    }
    // Check if the packet is an IPv4 packet
    if (eth->h_proto != htons(ETH_P_IP)) {
        return XDP_PASS;
    }
    struct iphdr *ip = (struct iphdr *)(eth + 1);
    if (ip + 1 > (struct iphdr *)ctx->data_end) {
        return XDP_PASS;
    }
    if (ip->protocol == IPPROTO_UDP){
        struct udphdr *udp = (struct udphdr *)(ip + 1);
        if (udp + 1 > (struct udphdr *)ctx->data_end) {
            return XDP_PASS;
        }
        if (ntohs(udp->dest) == 8080) {
            void *payload = (void *)(udp + 1);
            if (payload + sizeof(int) > (void *)ctx->data_end) {
                return XDP_PASS;
            }
            int *payloadData = (int *)payload;
            bpf_printk("Recieved %d", *payloadData);
            if ((*payloadData) % 2 == 0) {
                bpf_printk("Dropping\n");
                return XDP_DROP;
            }
            bpf_printk("Sending\n");
        }
    }

    // Allow the packet to pass through
    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
