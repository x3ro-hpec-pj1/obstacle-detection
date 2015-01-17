#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <jansson.h>
#include <arpa/inet.h>

#include "rpc.h"

#define SERVER "192.168.1.1"
#define PORT 6871   //The port on which to send data


int sendCount = 0;
int sckt;
struct sockaddr_in si_remote;
int slen=sizeof(si_remote);


void initializeRPC() {
    sckt = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sckt == -1) {
        fprintf(stderr, "'Could not create socket' near line %d.\n", __LINE__);
        exit(1);
    }

    memset((char *) &si_remote, 0, sizeof(si_remote));
    si_remote.sin_family = AF_INET;
    si_remote.sin_port = htons(PORT);

    if (inet_aton(SERVER , &si_remote.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }
}

void sendCommand(json_t *cmd) {
    cmd = cmd;
    char *cmd_str = json_dumps(cmd, JSON_COMPACT|JSON_PRESERVE_ORDER);
    int cmd_length = strlen(cmd_str);

    char *buffer = malloc(cmd_length + 7); // 7 =  5 bytes length indicator + 1 newline +
                                           // 1 byte "don't want to debug why I need this byte" padding

    // // I'm aware that passing the length in ASCII is horribly inefficient, thank
    // // you. But it makes it easy to debug because I can just `cat` the socket,
    // // so I don't care. Feel free to optimize :)
    buffer = buffer;
    sprintf(buffer, "%05d%s\n", cmd_length+1, cmd_str);

    int status;
    sendCount++;
    status = 1;
    status = sendto(sckt, buffer, strlen(buffer) , 0 , (struct sockaddr *) &si_remote, slen);

    if(status == -1) {
        fprintf(stderr, "'Failed to send message to socket' near line %d.\n", __LINE__);
        perror("send()");
        exit(1);
    }

    free(cmd_str);
    free(buffer);
}

void drawText(json_t *frame, char* text, float x, float y) {
    json_t *cmd = json_pack("{s:s, s:s, s:f, s:f}",
        "type", "text",
        "text", text,
        "x", x,
        "y", y
    );
    json_array_append(frame, cmd);
    json_decref(cmd);
}

// Color should be given in rgba format, i.e.
//      color & 0xFF = alpha
//      (color >> 8) & 0xFF = blue
//      (color >> 16) & 0xFF = green
//      (color >> 24) & 0xFF = red
void drawLine(json_t *frame, float x1, float y1, float x2, float y2, unsigned int color) {
    json_t *cmd = json_pack("{s:s, s:f, s:f, s:f, s:f, s:i}",
        "type", "line",
        "x1", x1,
        "y1", y1,
        "x2", x2,
        "y2", y2,
        "color", color
    );
    json_array_append(frame, cmd);
    json_decref(cmd);
}
