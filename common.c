#include "common.h"

void sendData(int fd, char *data, unsigned length, struct sockaddr_in *remote) {
    int ret;
    if (remote != NULL) {
	ret = sendto(fd, data, length, 0, (struct sockaddr *) remote, sizeof(*remote));
    } else {
	ret = send(fd, data, length, 0);
    }
    if (ret < 0) {
	perror("send");
	return;
    }
}
