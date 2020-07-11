#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include "opt-A2.h"
#include "opt-A3.h"
#include <mips/trapframe.h>
#include <vfs.h>
#include <kern/fcntl.h>
#include <limits.h>

  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
  (void)exitcode;


  #if OPT_A2
  //lock_acquire(p->procLock);
  //p->exitcode = exitcode;
  //p->exited = true;
  //lock_release(p->procLock);

  int length = array_num(p->children);
  for(int i = 0; i < length; ++i) {
          struct proc *child = array_get(p->children, i);
	  lock_acquire(child->procLock);
	  child->parent = NULL;
	  lock_release(child->procLock);
  }

  if(p->parent != NULL) {
	  length = array_num(p->parent->zombie);
	  lock_acquire(p->parent->procLock);
	  for(int j = 0; j < length; j++) {
		  struct zombie *zombieInfo = array_get(p->parent->zombie, j);
		  if(zombieInfo->pid == p->pid) {
			  zombieInfo->exitcode = _MKWAIT_EXIT(exitcode);
			  break;
		  }
	  }
	  lock_release(p->parent->procLock);
  }

  cv_broadcast(p->proc_cv, p->procLock);

  //if(p->parent == NULL) {
  //	  proc_destroy(p);
  //}


  #endif

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}


/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
  /* for now, this is just a stub that always returns a PID of 1 */
  /* you need to fix this to make it work properly */
	#if OPT_A2
	*retval = curproc->pid;
	#else
  *retval = 1;
  #endif
  return(0);
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  int exitstatus;
  int result;

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */

  if (options != 0) {
    return(EINVAL);
  }
  #if OPT_A2
  if(curproc->pid == pid || pid < 0) {
	  return EINVAL;
  }

  int length = array_num(curproc->zombie);
  for(int i = 0; i < length; ++ i) {
          struct zombie *zombieInfo = array_get(curproc->zombie, i);
          if(zombieInfo->pid == pid) {
                  while(zombieInfo->exitcode == -1) {
			  int childLength = array_num(curproc->children);
			  for(int j = 0; j < childLength; ++j) {
				  struct proc *child = array_get(curproc->children, j);
				  if(child->pid == pid) {
					  lock_acquire(curproc->procLock);
					  cv_wait(child->proc_cv, curproc->procLock);
					  lock_release(curproc->procLock);
					  break;
				  }
			  }
                  }
                  KASSERT(zombieInfo->exitcode != -1);
		  exitstatus = zombieInfo->exitcode;

                  result = copyout((void *)&exitstatus,status,sizeof(int));
                  if(result) return(result);

                  *retval = pid;
                  return(0);

          }
  }

  //pid does not exist in curproc
  *retval = -1;
  return ESRCH;

  #else

  /* for now, just pretend the exitstatus is 0 */
  exitstatus = 0;

  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
  #endif

}

#if OPT_A2
void thread_fork_func(void * tp, unsigned long random);
void thread_fork_func(void * tp, unsigned long random) {
	(void)random;
	enter_forked_process(tp);
}

int sys_fork(struct trapframe *tf, pid_t *retval) {
	KASSERT(curproc != NULL);
	struct proc *child = proc_create_runprogram(curproc->p_name);
	KASSERT(child != NULL);

	int addressErr = as_copy(curproc_getas(), &(child->p_addrspace));

	if(addressErr) {
		return ENOMEM;
	}

	struct trapframe *childtf = kmalloc(sizeof(struct trapframe));
	KASSERT(childtf != NULL);
	memcpy(childtf, tf, sizeof(struct trapframe));
	int tfErr = thread_fork("creating thread", 
			child, thread_fork_func, childtf, 0);
	if(tfErr) {
		return ENOMEM;
	}

	//add child here
	child->parent = curproc;

	struct zombie *info = kmalloc(sizeof(struct zombie));
	info->pid = child->pid;
	info->exitcode = -1;

	array_add(curproc->children, child, NULL);
	array_add(curproc->zombie, info, NULL);
	
	*retval = child->pid;

	return 0;

}

int sys_execv(const char *program, char **args, int *retval) {

	if(program == NULL) {
		return ENODEV;
	}

	(void) args;
	(void) retval;

	struct addrspace *as;
        struct vnode *v;
        vaddr_t entrypoint, stackptr;
        int result;

	/* copy program name */
	int nameLen = strlen(program) + 1;

	if(nameLen == 1) {
		return ENOENT;
	}

	if(nameLen > PATH_MAX) {
		return E2BIG;
	}

	char *nameSpace = kmalloc(nameLen * sizeof(char));
	KASSERT(nameSpace != NULL);

	int err = copyinstr((userptr_t) program, nameSpace, nameLen, NULL);
	KASSERT(err == 0);

        /* Open the file. */
        result = vfs_open(nameSpace, O_RDONLY, 0, &v);
        if (result) {
                return result;
        }

        /* Create a new address space. */
        as = as_create();
        if (as ==NULL) {
                vfs_close(v);
                return ENOMEM;
        }

        /* Switch to it and activate it. */
        curproc_setas(as);
        as_activate();

        /* Load the executable. */
        result = load_elf(v, &entrypoint);
        if (result) {
                /* p_addrspace will go away when curproc is destroyed */
                vfs_close(v);
                return result;
        }

        /* Done with the file now. */
        vfs_close(v);

        /* Define the user stack in the address space */
        result = as_define_stack(as, &stackptr);
        if (result) {
                /* p_addrspace will go away when curproc is destroyed */
                return result;
        }

        /* Warp to user mode. */
        enter_new_process(0 /*argc*/, NULL /*userspace addr of argv*/,
                          stackptr, entrypoint);

	 /* enter_new_process does not return. */
        panic("enter_new_process returned\n");
        return EINVAL;
}
#endif

