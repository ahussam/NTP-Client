#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define NTP_TIMESTAMP_DELTA 2208988800ull // Detla of NTP epoch
#define PORT 123                          // NTP UDP port number.

// Structure that defines the 48 byte NTP packet protocol.

typedef struct
{

    uint8_t li_vn_mode; // Leap indicator, 2 bits; Version number, 3 bits; Mode, 3 bits

    //Leap indicator: This is a two-bit code warning of an impending leap second to be inserted in the NTP timescale. The bits are set before 23:59 on the day of insertion and reset after 00:00 on the following day. This causes the number of seconds (rollover interval) in the day of insertion to be increased or decreased by one.
    //Version: This is a three-bit code used to label the version of the NTP.
    //Mode: This is a three-bit code used to set the mode(3 for client).

    uint8_t stratum;   // Stratum level (0-15) indicates the device's distance to the reference clock. Stratum 0 means a device is directly connected to e.g., a GPS antenna.
    uint8_t poll;      // Poll interval:  Maximum interval between successive messages. Default 2^10s = 1024s.
    uint8_t precision; // Precision of the local clock.

    uint32_t rootDelay;      // Total round trip delay time.
    uint32_t rootDispersion; // This is a number indicating the maximum error relative to the primary reference source at the root of the synchronization subnet, in seconds. Only positive values greater than zero are possible.
    uint32_t refId;          // This is a 32-bit code identifying the particular reference clock. In the case of stratum 0 (unspecified) or stratum 1 (primary reference source), this is a four-octet, left-justified, zero-padded ASCII string.

    uint32_t refTm_s; // Reference time-stamp seconds.
    uint32_t refTm_f; // Reference time-stamp fraction of a second.

    uint32_t orginTm_s; // Originate time-stamp seconds.
    uint32_t orginTm_f; // Originate time-stamp fraction of a second.

    uint32_t rxTm_s; // Received time-stamps seconds.
    uint32_t rxTm_f; // Received time-stamps fraction of a second.

    uint32_t txTm_s; // Transmit time-stamps seconds (the most important)
    uint32_t txTm_f; // Transmit time-stamp fraction of a second.

} ntp_packet;

void debugger(char *msg)
{

    perror(msg);
    exit(0);
}

int main()
{

    int sockfd;
    int status;
    char hostname[30];

    ntp_packet packet = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    memset(&packet, 0, sizeof(ntp_packet));

    // Set the first byte's bits to 00,011,011 for li = 0, vn = 3, and mode = 3. The rest will be left set to zero.

    *((char *)&packet + 0) = 0x1b; // 0x1b = 00011011 in base 2

    printf("--------------------NTP Implementation--------------------\n");

    printf("Enter hostname(NTP server hostname):\n");

    scanf("%s", hostname); // Get the hostname to create a UDP connection.

    // Create a UDP socket, convert the host-name to an IP address, set the port number,
    // connect to the server, send the packet, and then read in the return packet.

    struct sockaddr_in servaddr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // Create a UDP socket.

    if (sockfd < 0)
    {
        debugger("Unable to open a UDP socket.");
    }

    server = gethostbyname(hostname); // Hostname to IP address

    if (server == NULL)
    {
        debugger("No such a host!");
    }

    // Zero out the server address

    bzero((char *)&servaddr, sizeof(servaddr));

    servaddr.sin_family = AF_INET;

    // Copy the server address to the server address structure

    bcopy((char *)server->h_addr, (char *)&servaddr.sin_addr.s_addr, server->h_length);

    // Convert the port number integer to network big-endian style and save it to the server address structure.

    servaddr.sin_port = htons(PORT);

    status = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    if (status < 0)
    {
        debugger("Unable to connect to the UDP server!");
    }

    // Send the NTP packet. If n == -1, it failed.

    status = write(sockfd, (char *)&packet, sizeof(ntp_packet));

    if (status < 0)
    {
        debugger("ERROR writing to socket");
    }

    // Wait and receive the packet back from the server. If n == -1, it failed.

    status = read(sockfd, (char *)&packet, sizeof(ntp_packet));

    if (status < 0)
    {
        debugger("Unable to read socket");
    }

    // These two fields contain the time-stamp seconds as the packet left the NTP server.
    // The number of seconds correspond to the seconds passed since 1900.
    // ntohl() converts the bit/byte order from the network's to host's "endianness".

    packet.txTm_s = ntohl(packet.txTm_s);
    packet.txTm_f = ntohl(packet.txTm_f);

    // Extract the 32 bits that represent the time-stamp seconds (since NTP epoch) from when the packet left the server.
    // Subtract 70 years worth of seconds from the seconds since 1900.
    // This leaves the seconds since the UNIX epoch of 1970.
    // (1900)------------------(1970)**************************************(Time Packet Left the Server)

    time_t txTm = (time_t)(packet.txTm_s - NTP_TIMESTAMP_DELTA);

    printf("The time received from server is: %s", ctime((const time_t *)&txTm));

    return 0;
}
