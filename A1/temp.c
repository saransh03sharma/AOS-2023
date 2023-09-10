#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pranav Mehrotra and Saransh Sharma");
MODULE_DESCRIPTION("LKM for a deque");
MODULE_VERSION("0.1");

#define PROCFS_NAME "partb_1_20CS10085_20CS30065"
#define PROCFS_MAX_SIZE 1024

struct node {
    int val;
};

struct deque {
    struct node *circular_array;
    int size;
    int capacity;
    int front; 
    int rear;  
};

enum process_state {
    PROC_FILE_OPEN,
    PROC_READ_VALUE,
    PROC_READ_PRIORITY,
};

struct process_node {
    pid_t pid;
    enum process_state state;
    struct deque *process_deque;
    struct process_node *next_prt;
    int last_read;
};

static struct proc_dir_entry *proc_file;
static char procfs_buffer[PROCFS_MAX_SIZE];
static size_t procfs_buffer_size = 0;
static struct process_node *process_list = NULL;

DEFINE_MUTEX(mutex);

static struct deque *create_dq(int capacity) {
    struct deque *dq = kmalloc(sizeof(struct deque), GFP_KERNEL);
    if (dq == NULL) {
        printk(KERN_ALERT "Error: could not allocate memory for deque\n");
        return NULL;
    }
    dq->circular_array = kmalloc(capacity * sizeof(struct node), GFP_KERNEL);
    if (dq->circular_array == NULL) {
        printk(KERN_ALERT "Error: could not allocate memory for deque circular_array array\n");
        return NULL;
    }
    dq->size = 0;
    dq->capacity = capacity;
    dq->front = -1;
    dq->rear = 0; 
    return dq;
}


static int insert_dq(struct deque *dq, int val, struct process_node *curr) {
    if (dq->size == dq->capacity) {
        printk(KERN_ALERT "Error: deque is full\n");
        return -EACCES;
    }

    // Check if the value is even or odd
    if (val % 2 == 0) { // Even integers go to the right (rear)
        if(dq->front == -1)
        {
            dq->front=0;
            dq->rear=0;
        }
        else if(dq->rear ==  dq->capacity -1)
        {
            dq->rear=0;
        }
        else dq->rear = (dq->rear + 1) % dq->capacity;
        dq->circular_array[dq->rear].val = val;
        printk(KERN_INFO "(right) Value %d has been written to the proc file for process %d\n", val, curr->pid);
    
    } else { // Odd integers go to the left (front)
        // Check if the front is at the beginning of the circular array
        if (dq->front == 0) {
            dq->front = dq->capacity - 1;
        } else {
            dq->front = (dq->front - 1) % dq->capacity;
        }
        dq->circular_array[dq->front].val = val;
        printk(KERN_INFO "(left) Value %d has been written to the proc file for process %d\n", val, curr->pid);
    }

    dq->size++;
    return 0;
}


static void print_dq(struct deque *dq) {
#ifdef DEBUG
    printk(KERN_INFO "Deque for process %d:\n", current->pid);
    if (dq != NULL && dq->deque != NULL) {
        int i;
        int index = dq->front;  // Start from the front of the deque
        for (i = 0; i < dq->size; i++) {
            printk(KERN_INFO "%d  [%d]\n", i, dq->deque[index].val);
            index = (index + 1) % dq->capacity;  // Move to the next_prt node in the circular buffer
        }
    }
    printk("\n");
#endif
}

static void delete_dq(struct deque *dq) {
    if (dq != NULL) {
        kfree(dq->circular_array);
        kfree(dq);
    }
}

static struct process_node *find_process(pid_t pid) {
    struct process_node *curr = process_list;
    while (curr != NULL) {
        if (curr->pid == pid) {
            return curr;
        }
        curr = curr->next_prt;
    }
    return NULL;
}

static struct process_node *insert_process(pid_t pid) {
    struct process_node *node = kmalloc(sizeof(struct process_node), GFP_KERNEL);
    if (node == NULL) {
        return NULL;
    }
    node->pid = pid;
    node->state = PROC_FILE_OPEN;
    node->process_deque = NULL;
    node->next_prt = process_list;
    node->last_read = 0;
    process_list = node;
    return node;
}

static void delete_process_node(struct process_node *node) {
    if (node != NULL) {
        delete_dq(node->process_deque);
        kfree(node);
    }
}

static int delete_process(pid_t pid) {
    struct process_node *prev = NULL;
    struct process_node *curr = process_list;
    while (curr != NULL) {
        if (curr->pid == pid) {
            if (prev == NULL) {
                process_list = curr->next_prt;
            } else {
                prev->next_prt = curr->next_prt;
            }
            delete_process_node(curr);
            return 0;
        }
        prev = curr;
        curr = curr->next_prt;
    }
    return -EACCES;
}

static void delete_process_list(void) {
    struct process_node *curr = process_list;
    while (curr != NULL) {
        struct process_node *temp = curr;
        curr = curr->next_prt;
        delete_process_node(temp);
    }
}


static int procfile_open(struct inode *inode, struct file *file) {
    pid_t pid;
    int ret;
    struct process_node *curr;

    mutex_lock(&mutex);

    pid = current->pid;
    printk(KERN_INFO "procfile_open() invoked by process %d\n", pid);
    ret = 0;

    curr = find_process(pid);
    if (curr == NULL) {
        curr = insert_process(pid);
        if (curr == NULL) {
            printk(KERN_ALERT "Error: could not allocate memory for process node\n");
            ret = -ENOMEM;
        } else {
            printk(KERN_INFO "Process %d has been added to the process list\n", pid);
        }
    } else {
        printk(KERN_ALERT "Error: process %d has the proc file already open\n", pid);
        ret = -EACCES;
    }

    mutex_unlock(&mutex);
    return ret;
}

// Close handler for proc file
static int procfile_close(struct inode *inode, struct file *file) {
    pid_t pid;
    int ret;
    struct process_node *curr;

    mutex_lock(&mutex);

    pid = current->pid;
    printk(KERN_INFO "procfile_close() invoked by process %d\n", pid);
    ret = 0;

    curr = find_process(pid);
    if (curr == NULL) {
        printk(KERN_ALERT "Error: process %d does not have the proc file open\n", pid);
        ret = -EACCES;
    } else {
        delete_process(pid);
        printk(KERN_INFO "Process %d has been removed from the process list\n", pid);
    }

    mutex_unlock(&mutex);
    return ret;
}

static int extract_leftmost(struct deque *dq) {
    int leftmost_val;

    if (dq->size == 0) {
        printk(KERN_ALERT "Error: deque is empty\n");
        return -EACCES;
    }

    leftmost_val = dq->circular_array[dq->front].val;

    return leftmost_val;
}

static ssize_t handle_read(struct process_node *curr) {
    int min_val;
    
    if (curr->state == PROC_FILE_OPEN) {
        printk(KERN_ALERT "Error: process %d has not yet written anything to the proc file\n", curr->pid);
        return -EACCES;
    }
    
    if (curr->process_deque->size == 0) {
        printk(KERN_ALERT "Error: deque is empty\n");
        return -EACCES;
    }

    // Get the value at the last_read position
    min_val = curr->process_deque->circular_array[(curr->process_deque->front+curr->last_read)%curr->process_deque->capacity].val;

    // Update last_read for the next read call
    curr->last_read = (curr->last_read + 1);


    strncpy(procfs_buffer, (const char *)&min_val, sizeof(int));
    procfs_buffer[sizeof(int)] = '\0';
    procfs_buffer_size = sizeof(int);
    return procfs_buffer_size;
}

// Read handler for proc file
static ssize_t procfile_read(struct file *filep, char __user *buffer, size_t length, loff_t *offset) {
    pid_t pid;
    int ret;
    struct process_node *curr;

    mutex_lock(&mutex);

    pid = current->pid;
    printk(KERN_INFO "procfile_read() invoked by process %d\n", pid);
    ret = 0;

    curr = find_process(pid);
    if (curr == NULL) {
        printk(KERN_ALERT "Error: process %d does not have the proc file open\n", pid);
        ret = -EACCES;
    } else {
        procfs_buffer_size = min(length, (size_t)PROCFS_MAX_SIZE);
        ret = handle_read(curr);
        if (ret >= 0) {
            if (copy_to_user(buffer, procfs_buffer, procfs_buffer_size) != 0) {
                printk(KERN_ALERT "Error: could not copy data to user space\n");
                ret = -EACCES;
            } else {
                ret = procfs_buffer_size;
            }
        }
        print_dq(curr->process_deque);
    }
    mutex_unlock(&mutex);
    return ret;
}

static ssize_t handle_write(struct process_node *curr) {
    size_t capacity;
    int value, ret;

    if (curr->state == PROC_FILE_OPEN) {
        if (procfs_buffer_size > 1ul) {
            printk(KERN_ALERT "Error: Buffer size for capacity must be 1 byte\n");
            return -EINVAL;
        }
        capacity = (size_t)procfs_buffer[0];
        if (capacity < 1 || capacity > 100) {
            printk(KERN_ALERT "Error: Capacity must be between 1 and 100\n");
            return -EINVAL;
        }
        curr->process_deque = create_dq(capacity); // Create a deque with the specified capacity
        if (curr->process_deque == NULL) {
            printk(KERN_ALERT "Error: deque initialization failed\n");
            return -ENOMEM;
        }
        printk(KERN_INFO "Deque with capacity %zu has been initialized for process %d\n", capacity, curr->pid);
        curr->state = PROC_READ_VALUE;
    } else if (curr->state == PROC_READ_VALUE) {
        if (procfs_buffer_size > 4ul) {  // sizeof(int)
            printk(KERN_ALERT "Error: Buffer size for value must be 4 bytes\n");
            return -EINVAL;
        }
        value = *((int *)procfs_buffer);
       
        // Insert the value into the deque based on odd/even value
        ret = insert_dq(curr->process_deque, value, curr); // true for front insertion
        

        if (ret < 0) {
            printk(KERN_ALERT "Error: deque insertion failed\n");
            return -EACCES;
        }
        curr->state = PROC_READ_VALUE;
    }
    return procfs_buffer_size;
}

// Write handler for proc file
static ssize_t procfile_write(struct file *filep, const char __user *buffer, size_t length, loff_t *offset) {
    pid_t pid;
    int ret;
    struct process_node *curr;
    
    mutex_lock(&mutex);

    pid = current->pid;
    printk(KERN_INFO "procfile_write() invoked by process %d\n", pid);
    ret = 0;

    curr = find_process(pid);
    if (curr == NULL) {
        printk(KERN_ALERT "Error: process %d does not have the proc file open\n", pid);
        ret = -EACCES;
    } else {
        if (buffer == NULL || length == 0) {
            printk(KERN_ALERT "Error: empty write\n");
            ret = -EINVAL;
        } else {
            procfs_buffer_size = min(length, (size_t)PROCFS_MAX_SIZE);
            if (copy_from_user(procfs_buffer, buffer, procfs_buffer_size)) {
                printk(KERN_ALERT "Error: could not copy from user\n");
                ret = -EFAULT;
            } else {
                ret = handle_write(curr);
            }
        }
        print_dq(curr->process_deque);
    }
    mutex_unlock(&mutex);
    return ret;
}

static const struct proc_ops proc_fops = {
    .proc_open = procfile_open,
    .proc_read = procfile_read,
    .proc_write = procfile_write,
    .proc_release = procfile_close,
};

// Module initialization
static int __init lkm_init(void) {
    printk(KERN_INFO "LKM for partb_1_20CS10085_20CS30065 loaded\n");

    proc_file = proc_create(PROCFS_NAME, 0666, NULL, &proc_fops);
    if (proc_file == NULL) {
        printk(KERN_ALERT "Error: could not create proc file\n");
        return -ENOENT;
    }
    printk(KERN_INFO "/proc/%s created\n", PROCFS_NAME);
    return 0;
}

// Module cleanup
static void __exit lkm_exit(void) {
    delete_process_list();
    remove_proc_entry(PROCFS_NAME, NULL);
    printk(KERN_INFO "/proc/%s removed\n", PROCFS_NAME);
    printk(KERN_INFO "LKM for partb_1_20CS10085_20CS30065 unloaded\n");
}

module_init(lkm_init);
module_exit(lkm_exit);