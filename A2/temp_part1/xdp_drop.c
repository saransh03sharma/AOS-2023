#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <arpa/inet.h>
#include <bpf/bpf_helpers.h>

SEC("xdp_drop")
int packet_dropper(struct __sk_buff *skb) {
    struct ethhdr *eth = bpf_hdr_pointer(skb);
    struct iphdr *ip = (struct iphdr *)(eth + 1);
    struct tcphdr *tcp = (struct tcphdr *)(ip + 1);

    if (ip->protocol == IPPROTO_TCP && ntohs(tcp->dest) == 8080) {
        // Check if the TCP sequence number is even
        if (tcp->seq % 2 == 0) {
            // Drop the packet
            return XDP_DROP;
        }
    }

    // Allow the packet to pass through
    return XDP_PASS;
}

char _license[] SEC("license") = "GPL";
