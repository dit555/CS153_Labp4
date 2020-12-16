#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct shm_page {
    uint id;
    char *frame;
    int refcnt;
  } shm_pages[64];
} shm_table;

void shminit() {
  int i;
  initlock(&(shm_table.lock), "SHM lock");
  acquire(&(shm_table.lock));
  for (i = 0; i< 64; i++) {
    shm_table.shm_pages[i].id =0;
    shm_table.shm_pages[i].frame =0;
    shm_table.shm_pages[i].refcnt =0;
  }
  release(&(shm_table.lock));
}

int shm_open(int id, char **pointer) {
	
  	int i, j, sz, newsz;
	int maxid;
	char* mem;
	uint a;
	a = 0;
  	initlock(&(shm_table.lock), "SHM lock");
  	acquire(&(shm_table.lock));
  	//find id if it exists
	for (i = 0; i< 64; i++) {
    		if(shm_table.shm_pages[i].id == id){
			mappages(myproc()->pgdir, (char*)a , PGSIZE, V2P(shm_table.shm_pages[i].frame), PTE_W|PTE_U);
			shm_table.shm_pages[i].refcnt++;
			*pointer = (char*)a;
   			myproc()->sz += PGSIZE;
			release(&(shm_table.lock));
			return 0;	
		}
  	}
	
	//find unused page and do the thing
	for(i = 0; i < 64; i++){
		if (shm_table.shm_pages[i].id == 0){
			break;
		}
	}
	
	maxid = 0;
	for(j = 0; j < 64; j++){	
		//find larges id value then add 1 for new id
		if (shm_table.shm_pages[j].id >= maxid)
			maxid = shm_table.shm_pages[j].id;
	}

	maxid++; //new id 
	sz = myproc()->sz;
	newsz = sz - PGSIZE;
	
	if(newsz >= KERNBASE)
    		return -1;
  	
	

	a = PGROUNDUP(sz);
	for(; a < newsz; a += PGSIZE){
    		mem = kalloc();
    		if(mem == 0){
      			cprintf("allocuvm out of memory\n");
      			deallocuvm(myproc()->pgdir, newsz, sz);
      			return -1;
    		}
    		memset(mem, 0, PGSIZE);
    		if(mappages(myproc()->pgdir, (char*)a, PGSIZE, V2P(mem), PTE_W|PTE_U) < 0){
      			cprintf("allocuvm out of memory (2)\n");
      			deallocuvm(myproc()->pgdir, newsz, sz);
      			kfree(mem);
      			return -1;
    		}
  	}
	
	*pointer = (char*)a;
	
	shm_table.shm_pages[i].id = maxid;
	shm_table.shm_pages[i].frame = (char*)a;
	shm_table.shm_pages[i].refcnt = 1; 
	
	myproc()->sz = newsz;
  	release(&(shm_table.lock));
		
	
	
	return 1;
}


int shm_close(int id) {
	
  	int i;
  	initlock(&(shm_table.lock), "SHM lock");
  	acquire(&(shm_table.lock));
  	for (i = 0; i< 64; i++) {
		if(shm_table.shm_pages[i].id == id){
    			if(shm_table.shm_pages[i].refcnt <= 1){	
				shm_table.shm_pages[i].id =0;
    				shm_table.shm_pages[i].frame =0;
   	 			shm_table.shm_pages[i].refcnt =0;
			}
			else
				shm_table.shm_pages[i].refcnt--;
		}
  	}
  	release(&(shm_table.lock));




	return 0; //added to remove compiler warning -- you should decide what to return
}
