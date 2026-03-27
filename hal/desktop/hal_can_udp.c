/**
 * @file hal_can_udp.c
 * Desktop CAN HAL implementation using UDP multicast.
 *
 * CAN frames are serialized as raw can_frame_t structs and sent/received
 * via UDP multicast on 224.0.0.100:4200.  This allows multiple processes
 * on the same machine (or LAN) to simulate a CAN bus.
 */

#include "hal_can.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

#define CAN_UDP_MULTICAST_GROUP "224.0.0.100"
#define CAN_UDP_PORT            4200

static int sock_send = -1;
static int sock_recv = -1;
static struct sockaddr_in mcast_addr;

int hal_can_init(void)
{
    /* --- Send socket --- */
    sock_send = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_send < 0) {
        perror("hal_can_init: send socket");
        return -1;
    }

    /* Allow multicast loopback so we can test on a single machine */
    unsigned char loop = 1;
    setsockopt(sock_send, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop));

    /* Set TTL to 1 (link-local) */
    unsigned char ttl = 1;
    setsockopt(sock_send, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    memset(&mcast_addr, 0, sizeof(mcast_addr));
    mcast_addr.sin_family = AF_INET;
    mcast_addr.sin_port = htons(CAN_UDP_PORT);
    inet_pton(AF_INET, CAN_UDP_MULTICAST_GROUP, &mcast_addr.sin_addr);

    /* --- Receive socket --- */
    sock_recv = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_recv < 0) {
        perror("hal_can_init: recv socket");
        close(sock_send);
        sock_send = -1;
        return -1;
    }

    /* Allow multiple processes to bind the same port */
    int reuse = 1;
    setsockopt(sock_recv, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    setsockopt(sock_recv, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));

    struct sockaddr_in bind_addr;
    memset(&bind_addr, 0, sizeof(bind_addr));
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_port = htons(CAN_UDP_PORT);
    bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock_recv, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) < 0) {
        perror("hal_can_init: bind");
        close(sock_send);
        close(sock_recv);
        sock_send = sock_recv = -1;
        return -1;
    }

    /* Join multicast group */
    struct ip_mreq mreq;
    inet_pton(AF_INET, CAN_UDP_MULTICAST_GROUP, &mreq.imr_multiaddr);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(sock_recv, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                   &mreq, sizeof(mreq)) < 0) {
        perror("hal_can_init: join multicast");
        close(sock_send);
        close(sock_recv);
        sock_send = sock_recv = -1;
        return -1;
    }

    return 0;
}

int hal_can_send(const can_frame_t *frame)
{
    if (sock_send < 0 || !frame) return -1;

    ssize_t sent = sendto(sock_send, frame, sizeof(can_frame_t), 0,
                          (struct sockaddr *)&mcast_addr, sizeof(mcast_addr));
    return (sent == sizeof(can_frame_t)) ? 0 : -1;
}

int hal_can_receive(can_frame_t *frame, uint32_t timeout_ms)
{
    if (sock_recv < 0 || !frame) return -1;

    if (timeout_ms == 0) {
        /* Non-blocking: use MSG_DONTWAIT instead of SO_RCVTIMEO,
         * because a zero-valued SO_RCVTIMEO means "block forever"
         * on most platforms. */
        ssize_t n = recvfrom(sock_recv, frame, sizeof(can_frame_t),
                             MSG_DONTWAIT, NULL, NULL);
        if (n == (ssize_t)sizeof(can_frame_t)) {
            return 0;
        }
        return -1; /* no data available */
    }

    /* Set receive timeout */
    struct timeval tv;
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(sock_recv, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    ssize_t n = recvfrom(sock_recv, frame, sizeof(can_frame_t), 0, NULL, NULL);
    if (n == (ssize_t)sizeof(can_frame_t)) {
        return 0;
    } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return -1; /* timeout */
    }
    return -2; /* error */
}

void hal_can_deinit(void)
{
    if (sock_recv >= 0) {
        /* Leave multicast group */
        struct ip_mreq mreq;
        inet_pton(AF_INET, CAN_UDP_MULTICAST_GROUP, &mreq.imr_multiaddr);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        setsockopt(sock_recv, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                   &mreq, sizeof(mreq));
        close(sock_recv);
        sock_recv = -1;
    }
    if (sock_send >= 0) {
        close(sock_send);
        sock_send = -1;
    }
}
