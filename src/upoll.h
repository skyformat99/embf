#ifndef UPOLL_H_
#define UPOLL_H_

#include "global.h"

#define UPOLL_MAX_NUM 30 //The number must more than maximum drivers number

#define UPOLL_OK      1
#define UPOLL_ERR    -1

#define UPOLL_CTL_ADD 1
#define UPOLL_CTL_DEL 2
#define UPOLL_CTL_MOD 3

typedef union upoll_data {
    void   *ptr;
    int32_t  filed;
    uint32_t ui32;
    uint64_t ui64;
} upoll_data_t;

typedef struct {
    int32_t   events;      // Epoll events
    int32_t   dev_flag;
    OS_EVENT *ev;
    upoll_data_t data;    // User data variable
}upoll_event;

int32_t upoll_create(int8_t size);
int32_t upoll_ctl(int32_t epfd, int32_t op, int32_t fd,upoll_event *event);
int32_t upoll_wait(int32_t epfd,upoll_event *event,int32_t maxevents,int16_t timeout);

#endif
