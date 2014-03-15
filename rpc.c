#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <jansson.h>

#include "rpc.h"


int sendCount = 0;
int sckt;

void initializeRPC() {
    int s;
    struct sockaddr_un local, remote;
    unsigned int len;
    int status;

    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if(s == -1) {
        fprintf(stderr, "'Could not create socket' near line %d.\n", __LINE__);
        exit(1);
    }

    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, "/Users/lucas/testsckt");
    unlink(local.sun_path);
    len = strlen(local.sun_path) + sizeof(local.sun_family) + 1;
    status = bind(s, (struct sockaddr *) &local, len);
    if(status == -1) {
        fprintf(stderr, "'Could not bind to socket' near line %d.\n", __LINE__);
        exit(1);
    }

    listen(s, 1);
    if(status == -1) {
        fprintf(stderr, "'Could not listen to socket' near line %d.\n", __LINE__);
        exit(1);
    }

    len = sizeof(struct sockaddr_un);
    sckt = accept(s, (struct sockaddr *) &remote, &len);
    if(sckt == -1) {
        fprintf(stderr, "'Failed accepting socket connection' near line %d.\n", __LINE__);
        exit(1);
    }
}

void sendCommand(json_t *cmd) {
    char *cmd_str = json_dumps(cmd, JSON_COMPACT|JSON_PRESERVE_ORDER);
    int cmd_length = strlen(cmd_str);

    char *buffer = malloc(cmd_length + 6); // 6 =  5 bytes length indicator + 1 newline

    // I'm aware that passing the length in ASCII is horribly inefficient, thank
    // you. But it makes it easy to debug because I can just `cat` the socket,
    // so I don't care. Feel free to optimize :)
    sprintf(buffer, "%05d%s\n", cmd_length+1, cmd_str);

    int status;
    sendCount++;
    status = send(sckt, buffer, strlen(buffer), 0);
    free(cmd_str);
    free(buffer);

    if(status == -1) {
        fprintf(stderr, "'Failed to send message to socket' near line %d.\n", __LINE__);
        perror("send()");
        exit(1);
    }
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

void drawLine(json_t *frame, float x1, float y1, float x2, float y2) {
    json_t *cmd = json_pack("{s:s, s:f, s:f, s:f, s:f}",
        "type", "line",
        "x1", x1,
        "y1", y1,
        "x2", x2,
        "y2", y2
    );
    json_array_append(frame, cmd);
    json_decref(cmd);
}
