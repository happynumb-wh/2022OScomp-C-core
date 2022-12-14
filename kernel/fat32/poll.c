#include <fs/poll.h>
#include <os/ring_buffer.h>
#include <screen.h>
#include <assert.h>
#include <fs/pipe.h>
int32_t do_ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *tmo_p, const sigset_t *sigmask){
    if(sigmask != 0){
        printk("signal is not implied\n");
    }
    nfds_t ret = 0;
    int timer = 0;
    for (int i = 0; i < nfds; i++)
    {
        if(fds->fd < 0){
            fds->revents = 0;
            ret ++;
        } else{
            pcb_t * current_running = get_current_running();
            fd_t* fd = &current_running->pfd[get_fd_index(fds->fd, current_running)];
            if(fd->dev == DEV_STDIN){
                if (fds->events & POLLIN){
                    while (ring_buffer_empty(&stdin_buf)) do_scheduler();
                    fds->revents = POLLIN;
                    ret++;
                }
            } else if(fd->dev == DEV_PIPE){
                if(fds->events & POLLIN){
                    struct ring_buffer *rbuf = &(pipe[current_running->pfd[get_fd_index(fds->fd, current_running)].pip_num].rbuf);
                    while (ring_buffer_empty(&rbuf)) do_scheduler();
                    fds->revents = POLLIN;
                    ret++;
                }
            }
        }
        fds++;
    }
    return ret;
}
