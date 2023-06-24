#ifndef __WINDOWING_H__
#define __WINDOWING_H__

#include <stdint.h>
#include <stddef.h>

#include "networks.h"

typedef struct windowBuf {
   uint32_t buf_size;
   uint32_t seq_num;
   uint8_t buffer[MAX_LEN];
} windowBuf;

typedef struct CircularQwindow {
   uint32_t lower;
   uint32_t current;
   uint32_t upper;
   uint32_t win_sz;

   windowBuf *ptr;
   uint8_t *occupied;
} CircularQWindow;

void *getMemoryWindow(int num);
void insertIntoWindow(CircularQWindow *window, uint8_t *packet, int packetLen, int seq_num);
void windowreset(CircularQWindow *window, int window_size);
void loadFromWindow(CircularQWindow *window, uint8_t *packet, uint32_t *len_read, int seq_num);
void removeFromWindow(CircularQWindow *window, int seq_num);
void moveWindow(CircularQWindow *window, int new_lower);
void clearContents(CircularQWindow *window);









#endif