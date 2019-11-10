#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket() and bind() */
#include <arpa/inet.h>  /* for sockaddr_in */
#include <stdlib.h>     /* for atoi(), srand() and exit() */
#include <string.h>     /* for memset() */
#include <unistd.h>     /* for close() */
#include <time.h>       /* for time() */
#include <sys/time.h>

int main(){

    int sock;                         /* Socket */
    struct sockaddr_in broadcastAddr; /* Broadcast address */
    struct sockaddr_in broadcastAddrRPi; /* Broadcast address */
    unsigned short broadcastPort;     /* Server port */
    int broadcastPermission;          /* Socket opt to set permission to broadcast */
    unsigned int sendStringLen;       /* Length of string to broadcast */
    unsigned char byteToSend;
    struct timeval t1;
    
    gettimeofday(&t1, NULL);
    srand(t1.tv_usec * t1.tv_sec);

    char *broadcastIPBBB = "10.42.0.255"; //"192.168.7.255"; //"127.0.0.1";
    char *broadcastIPRPi = "10.42.0.255";
    broadcastPort = 37020;

    /* Create socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        perror("socket() failed\n");
        exit(EXIT_FAILURE);
    }
    /* Set socket to allow broadcast */
    broadcastPermission = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission, sizeof(broadcastPermission)) < 0){
        perror("setsockopt() failed\n");
        exit(EXIT_FAILURE);
    }
    /* Construct local address structure */
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));   /* Zero out structure */
    broadcastAddr.sin_family = AF_INET;                 /* Internet address family */
    broadcastAddr.sin_addr.s_addr = inet_addr(broadcastIPBBB);/* Broadcast IP address */
    broadcastAddr.sin_port = htons(broadcastPort);         /* Broadcast port */

    memset(&broadcastAddrRPi, 0, sizeof(broadcastAddrRPi));   /* Zero out structure */
    broadcastAddrRPi.sin_family = AF_INET;                 /* Internet address family */
    broadcastAddrRPi.sin_addr.s_addr = inet_addr(broadcastIPRPi);/* Broadcast IP address */
    broadcastAddrRPi.sin_port = htons(broadcastPort);         /* Broadcast port */

    sendStringLen = 1; //strlen(byteToSend);  /* Find length of sendString */

    while(1)
    {
        byteToSend = (rand() % 256) + 0; /* random byte with value : 0b00000000 up to 0b11111111 */
        /* Broadcast sendString in datagram to clients every 3 seconds*/
        /*if (sendto(sock, &byteToSend, sendStringLen, 0, (struct sockaddr *) &broadcastAddrRPi, sizeof(broadcastAddrRPi)) != sendStringLen){
            perror("sendto() sent a different number of bytes than expected");
            exit(EXIT_FAILURE);
        }*/
        if (sendto(sock, &byteToSend, sendStringLen, 0, (struct sockaddr *) &broadcastAddr, sizeof(broadcastAddr)) != sendStringLen){
            perror("sendto() sent a different number of bytes than expected");
            exit(EXIT_FAILURE);
        }
        //sleep(0.001);
    }
}
