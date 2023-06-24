
#include "windowing.h"
#include <stdlib.h>


void *getMemoryWindow(int num) {
	void *mem = NULL;
	if ((mem = malloc(num)) == NULL) {
		perror("Memmory not initialized");
		exit(-1);
	}
	return mem;
}

void insertIntoWindow(CircularQWindow *window, uint8_t *packet, int packetLen, int seq_num) {

	int idx = 0;
	idx = seq_num % window->win_sz;
	memcpy((window->ptr)[idx].buffer, packet, packetLen);
	(window->ptr)[idx].buf_size = packetLen;
	(window->occupied)[idx] = 1;
	(window->ptr)[idx].seq_num = seq_num;
}

void loadFromWindow(CircularQWindow *window, uint8_t *packet, uint32_t *len_read, int seq_num) {
	int idx = seq_num % window->win_sz;

	memcpy(packet, (window->ptr)[idx].buffer, (window->ptr)[idx].buf_size);
	*len_read = (window->ptr)[idx].buf_size;
}

void removeFromWindow(CircularQWindow *window, int seq_num) {
	int idx = seq_num % window->win_sz;
	(window->occupied)[idx] = 0;
}

void moveWindow(CircularQWindow *window, int new_lower) {
	int itr;
	int idx;

	for (itr = window->lower; itr < new_lower; itr++) {
		idx = itr % window->win_sz;
		(window->occupied)[idx] = 0;
	}
    window->upper += new_lower - window->lower;
	window->lower = new_lower;
	if (window->lower > window->current) {
		window->current = window->lower;
	}
}

void clearContents(CircularQWindow *window) {
	free(window->ptr);
	free(window->occupied);
}

void windowreset(CircularQWindow *window, int window_size) {
	window->win_sz = window_size;
	window->lower = 1;
	window->upper = window_size + 1;
	window->current = 1;

	window->occupied = getMemoryWindow(sizeof(uint8_t) * window_size);
	window->ptr = getMemoryWindow(2000 * window_size);
}
