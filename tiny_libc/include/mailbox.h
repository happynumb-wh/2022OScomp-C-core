#ifndef INCLUDE_MAIL_BOX_
#define INCLUDE_MAIL_BOX_

#define MAX_MBOX_LENGTH (32)

// TODO: please define mailbox_t;
// mailbox_t is just an id of kernel's mail box.
/*just define to compile for the test_shell*/
typedef int mailbox_t;

/* 根据名字返回一个对应的mailbox，
 * 如果不存在对应名字的mailbox则返回
 * 一个新的mailbox
 */
mailbox_t mbox_open(char *);
/* 关闭该mailbox，如果该mailbox的引用数为0，
 * 则释放该mailbox
*/
void mbox_close(mailbox_t);
/**
 * 向一个mailbox发送数据，如果mailbox满了
 * 则释放该mailbox
 */
int mbox_send(mailbox_t, void *, int);
/**
 * 从一个mailbox接收数据，如果mailbox空了，
 * 则产生一个阻塞，直到读取到数据
 */
int mbox_recv(mailbox_t, void *, int);

#endif
