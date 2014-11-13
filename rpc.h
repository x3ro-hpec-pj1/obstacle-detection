#ifndef RPC_H_INCLUDED
#define RPC_H_INCLUDED

#define MAX_CMD_LENGTH 256

void initializeRPC();
void sendCommand(json_t *cmd);
void drawText(json_t *frame, char* text, float x, float y);
void drawLine(json_t *frame, float x1, float y1, float x2, float y2, unsigned int color);

#endif
