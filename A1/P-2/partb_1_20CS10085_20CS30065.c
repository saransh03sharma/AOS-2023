/* 
    Pranav Mehrotra 20CS10085
    Saransh Sharma  20CS30065
*/

/*
    front->| | | | | |<-rear
    the left end is pointed by front
    the right end is pointed by rear 
*/

//necessary imports
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

//module declarations
MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Pranav Mehrotra and Saransh Sharma");
MODULE_DESCRIPTION("Deque LKM");
MODULE_VERSION("0.1");

//macros for proc file name and buffer size
#define PROCESS_FILE_NAME "partb_1_20CS10085_20CS30065"
#define BUFFER_MAX_SIZE 512

//node structure for deque
struct node {
    int value;
};

//deque implementation using fixed size circular array
struct deque {
    struct node *circular_array;
    int max_size;
    //left end of deque where element can be inserted
    int front_index; 
    //right end of deque where element can be inserted
    int rear_index;  
};

//available process states
enum process_state {
    //process state at the beginning. The inital write operation writes just one byte i.e. the size of deque
    PROCESS_INIT,
    //process state after the size of deque is set, i.e. to indicate read/write operations are allowed
    PROCESS_R_W,
};

//node to store details about process opening proc file
struct process_node {
    pid_t pid;                              //pid of process
    enum process_state state;               //state of the process
    struct deque *process_deque;            //dedicated deque of the process
    struct process_node *next_proc_node;    //pointer to next process node
};

//to point to proc file containg process deques
static struct proc_dir_entry *proc_file;
//head of process linked list i.e. list of all processed which have proc file opened
static struct process_node *process_list_head = NULL;

//user process buffer to store data temporarily
static char process_buffer[BUFFER_MAX_SIZE];
static size_t process_buffer_size = 0;

//intitialize mutex 
DEFINE_MUTEX(mutex);

//create a deque object of size max_size
static struct deque *deque_init(int max_size) 
{
    //allocate space to a deque object
    struct deque *process_dq = kmalloc(sizeof(struct deque), GFP_KERNEL);
    if (process_dq == NULL) {
        printk(KERN_ALERT "Error: Memory allocation for deque failed!\n");
        return NULL;
    }

    //alloacte memory to the circular array
    process_dq->circular_array = kmalloc(max_size * sizeof(struct node), GFP_KERNEL);
    if (process_dq->circular_array == NULL) {
        printk(KERN_ALERT "Error: Memory allocation for circular_array failed! \n");
        return NULL;
    }

    //intialize the deque metadata
    process_dq->max_size = max_size;
    process_dq->front_index = -1;   
    process_dq->rear_index = 0; 
    
    return process_dq;
}

//function to free space allocated to deque and it's circular array
static void free_deque(struct deque *dq) 
{
    if (dq != NULL) {
        kfree(dq->circular_array);
        kfree(dq);
    }
}

//check if deque is empty
static bool isempty(struct deque *dq)
{
    return (dq->front_index == -1);
}

//check if deque is full
static bool isfull(struct deque *dq)
{
    if(dq->front_index == 0 && dq->rear_index == dq->max_size - 1)return true;
    else if (dq->front_index == dq->rear_index + 1) return true;
    else return false;
}
static int insert_front(struct deque *dq, int value, pid_t pid) 
{
    //to insert element at front
    if (isfull(dq)) {
        printk(KERN_ALERT "Error: deque overflow!\n");
        return -EACCES;
    }

    //if deque is empty
    if(dq->front_index == -1)
    {
        dq->front_index = 0;
        dq->rear_index = 0;
    }
    //front is at first index
    else if (dq->front_index == 0) 
    {
        dq->front_index = dq->max_size - 1;
    } 
    else 
    {
        //decrement the front index
        dq->front_index = (dq->front_index - 1);
    }
    
    //insert element at index pointed by front
    dq->circular_array[dq->front_index].value = value;
    printk(KERN_INFO "Value %d has been inserted at front end by process %d\n", value, pid);
    
    return 0;
}


static int insert_rear(struct deque *dq, int value, pid_t pid) 
{
    //to insert element at the rear end
    if (isfull(dq)) {
        printk(KERN_ALERT "Error: deque overflow!\n");
        return -EACCES;
    }

    //empty deque
    if(dq->front_index == -1)
    {
        dq->front_index = 0;
        dq->rear_index = 0;
    }
    //rear points to last index
    else if(dq->rear_index ==  dq->max_size -1)
    {
        dq->rear_index = 0;
    }
    //increment rear to next index
    else 
        dq->rear_index = (dq->rear_index + 1);
    
    //insert the element at the index pointed by rear
    dq->circular_array[dq->rear_index].value = value;
    printk(KERN_INFO "Value %d has been inserted at rear end by process %d\n", value, pid);

    return 0;
}

static int insert_dq(struct deque *dq, int value, struct process_node *curr) 
{
    if (isfull(dq)) {
        printk(KERN_ALERT "Error: deque overflow!\n");
        return -EACCES;
    }

    if (value % 2 == 0) 
    { 
        //even elements are inserted at right/rear end
        insert_rear(dq, value, curr->pid);
    } 
    else 
    { 
        //odd elements are inserted at left/front end
        insert_front(dq, value, curr->pid);
    }
    return 0;
}

//function to insert process node in process linked list
static struct process_node *insert_proc_node(pid_t pid) 
{
    //create a process node
    struct process_node *temp_node = kmalloc(sizeof(struct process_node), GFP_KERNEL);
    
    if (temp_node == NULL) 
    {
        printk(KERN_ALERT "Error: Memory allocation for process node failed!\n");
        return NULL;
    }
    
    //intiialize the metadata of process node
    temp_node->pid = pid;
    temp_node->state = PROCESS_INIT;
    temp_node->process_deque = NULL;

    //append it at head and update head of linked list
    temp_node->next_proc_node = process_list_head;
    process_list_head = temp_node;
    return temp_node;
}

//function to search for a node in process list with desired pid value
static struct process_node *search_pid(pid_t pid) {
    struct process_node *proc_node = process_list_head;
    //traverse the linked list until nodes pid matches target pid
    while (proc_node != NULL) 
    {
        if (proc_node->pid == pid) 
        {
            return proc_node;
        }
        proc_node = proc_node->next_proc_node;
    }
    return NULL;
}

//function to delete process node from linked list
static int delete_proc_node(pid_t pid)
{
    //find the node to be deleted and one node before it
    struct process_node *prev_node = NULL;
    struct process_node *proc_node = process_list_head;

    while (proc_node != NULL) 
    {
        if (proc_node->pid == pid) 
        {
            //first node needs to be deleted
            if (prev_node == NULL) 
            {
                //update head
                process_list_head = proc_node->next_proc_node;
            } 
            else
            {
                //change pointers 
                prev_node->next_proc_node = proc_node->next_proc_node;
            }
            //free the space occupied by process node
            free_deque(proc_node->process_deque);
            kfree(proc_node);

            return 0;
        }
        prev_node = proc_node;
        proc_node = proc_node->next_proc_node;
    }
    //node to be deleted doesn't exist in the list
    return -EACCES;
}

//function called when a process attempts to open proc file
static int open_file(struct inode *inode, struct file *file) {
    
    int return_value;
    pid_t target_pid;
    struct process_node *target_process;  

    target_pid = current->pid;
    mutex_lock(&mutex);

    printk(KERN_INFO "file open request made by process %d\n", target_pid);  
    target_process = search_pid(target_pid);

    //open called for first time
    if (target_process == NULL) 
    {
        //insert process to proc linked list
        target_process = insert_proc_node(target_pid);
        if (target_process == NULL) 
        {
            printk(KERN_ALERT "Error: process node creation failed!\n");
            return_value = -ENOMEM;
        } 
        else 
        {
            printk(KERN_INFO "Process %d added to list!\n", target_pid);
            return_value = 0;
        }
    } 
    else 
    {
        printk(KERN_ALERT "Error: file already opened by Process %d!\n", target_pid);
        return_value = -EACCES;
    }
    //the entire process is locked to ensure safe updation of kernel logs
    mutex_unlock(&mutex);
    return return_value;
}

static int close_file(struct inode *inode, struct file *file) {
    
    int return_value;
    pid_t target_pid;
    struct process_node *target_process;

    mutex_lock(&mutex);
    target_pid = current->pid;
    
    printk(KERN_INFO "File close request made by process %d!\n", target_pid);
    target_process = search_pid(target_pid);
    
    if (target_process == NULL) 
    {
        printk(KERN_ALERT "Error: Process %d doesn't have file opened!\n", target_pid);
        return_value = -EACCES;
    } 
    else 
    {
        delete_proc_node(target_pid);
        printk(KERN_INFO "File succesfully closed by process %d! \n", target_pid);
        return_value = 0;
    }

    mutex_unlock(&mutex);
    return return_value;
}

static ssize_t execute_read(struct process_node *curr) {

    int read_value;
    struct deque *dq;
    
    if (curr->state == PROCESS_INIT) {
        printk(KERN_ALERT "Error: size of deque not yet set by process %d \n", curr->pid);
        return -EACCES;
    }
    
    if (isempty(curr->process_deque)) {
        printk(KERN_ALERT "Error: Empty deque\n");
        return -EACCES;
    }


    read_value = curr->process_deque->circular_array[curr->process_deque->front_index].value;
    //printk(KERN_INFO "Reading from front index: %d\n", curr->process_deque->front_index);

    dq = curr->process_deque;

    //deque has only one element
    if(dq->front_index == dq->rear_index)
    {
        dq->front_index  = -1;
        dq->rear_index = -1;
    }
    else{
        //re-initialize front
        if(dq->front_index == dq->max_size - 1)
        {
            dq->front_index = 0;
        }
        else{
            //increment front by one index
            dq->front_index++;
        }
    }

    //copy the value read to process buffer
    strncpy(process_buffer, (const char *)&read_value, sizeof(int));
    process_buffer[sizeof(int)] = '\0';
    process_buffer_size = sizeof(int);
    return process_buffer_size;
}

//function called when process calls read operation
static ssize_t read_file(struct file *filep, char __user *buffer, size_t length, loff_t *offset) {
    pid_t pid;
    int return_value = 0;
    struct process_node *target_process;

    mutex_lock(&mutex);
    pid = current->pid;
    printk(KERN_INFO "Read request made by Process %d!\n", pid);
    
    target_process = search_pid(pid);
    if (target_process == NULL) 
    {
        printk(KERN_ALERT "Error: File not opened by process %d!\n", pid);
        return_value = -EACCES;
    } else {
        process_buffer_size = min(length, (size_t)BUFFER_MAX_SIZE);
        return_value = execute_read(target_process);
        if (return_value >= 0) {
            if (copy_to_user(buffer, process_buffer, process_buffer_size) != 0) {
                printk(KERN_ALERT "Error: could not copy data to user space\n");
                return_value = -EACCES;
            } else {
                return_value = process_buffer_size;
            }
        }
    }
    mutex_unlock(&mutex);
    return return_value;
}

static ssize_t execute_write(struct process_node *process) {
    size_t max_size;
    int value, return_value;

    //if the process is in init state that is the size of deque isn't set yet
    if (process->state == PROCESS_INIT) 
    {
        //if the process writes anything greater than 1 byte
        if (process_buffer_size != 1ul) 
        {
            printk(KERN_ALERT "Error: Buffer size must be of 1 byte\n");
            return -EINVAL;
        }

        //size entered should be from 1 to 100
        max_size = (size_t)process_buffer[0];
        if (max_size < 1 || max_size > 100) 
        {
            printk(KERN_ALERT "Error: Size must be between 1 and 100!\n");
            return -EINVAL;
        }
        //initialise a deque of the same size
        process->process_deque = deque_init(max_size); 
        if (process->process_deque == NULL) 
        {
            printk(KERN_ALERT "Error: Deque creation failed!\n");
            return -ENOMEM;
        }

        printk(KERN_INFO " Process %d initialized a deque of size %zu!\n", process->pid, max_size);
        process->state = PROCESS_R_W;
        //upon succesfull set up of size the next input should be of type int(4 bytes)
    } 
    else if (process->state == PROCESS_R_W) 
    {
        if (process_buffer_size != 4ul) 
        {
            printk(KERN_ALERT "Error: Input should be of 4 bytes\n");
            return -EINVAL;
        }
        //get input value
        value = *((int *)process_buffer);
       
        // Insert the value into the deque
        return_value = insert_dq(process->process_deque, value, process); 
        
        if (return_value < 0) {
            printk(KERN_ALERT "Error: Insertion failed!\n");
            return -EACCES;
        }
        process->state = PROCESS_R_W;
    }
    return process_buffer_size;
}

static ssize_t write_file(struct file *filep, const char __user *buffer, size_t length, loff_t *offset) {
    pid_t pid;
    int ret_value = 0;
    struct process_node *target_process;
    
    mutex_lock(&mutex);

    pid = current->pid;
    printk(KERN_INFO "Write request made by Process with pid %d! \n", pid);

    target_process = search_pid(pid);
    
    if (target_process == NULL) 
    {
        printk(KERN_ALERT "Error: Proc file not opened by process with pid %d. \n", pid);
        ret_value = -EACCES;
    } 
    else 
    {
        if (buffer == NULL || length == 0) {
            printk(KERN_ALERT "Error: Empty write operation called.\n");
            ret_value = -EINVAL;
        } 
        else {
            process_buffer_size = min(length, (size_t)BUFFER_MAX_SIZE);
            if (copy_from_user(process_buffer, buffer, process_buffer_size)) {
                printk(KERN_ALERT "Error: could not copy from user\n");
                ret_value = -EFAULT;
            } else {
                ret_value = execute_write(target_process);
            }
        }
    }
    mutex_unlock(&mutex);
    return ret_value;
}

static const struct proc_ops file_operations = {
    .proc_open = open_file,
    .proc_read = read_file,
    .proc_write = write_file,
    .proc_release = close_file,
};

static int lkm_init_module(void) {
    printk(KERN_INFO "LKM for deque loading...\n");

    proc_file = proc_create(PROCESS_FILE_NAME, 0666, NULL, &file_operations);
    
    
    if (proc_file == NULL) {
        printk(KERN_ALERT "Error: attempt to create proc file failed\n");
        return -ENOENT;
    }
    else{
        printk(KERN_INFO "/proc/%s created\n", PROCESS_FILE_NAME);
    }
    return 0;
}

static void delete_process_list(void) 
{
    struct process_node *iter_node = process_list_head;
    while (iter_node != NULL) 
    {
        struct process_node *x = iter_node;
        iter_node = iter_node->next_proc_node;
        free_deque(x->process_deque);
        kfree(x);
    }
}

static void lkm_exit_module(void) 
{
    delete_process_list();
    remove_proc_entry(PROCESS_FILE_NAME, NULL);
    printk(KERN_INFO "/proc/%s removed\n", PROCESS_FILE_NAME);
    printk(KERN_INFO "LKM for deque unloaded\n");
}

module_init(lkm_init_module);
module_exit(lkm_exit_module);