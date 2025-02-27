#include "monitor.h"

struct c_tread *get_cur_thread() {
#if (defined riscv) || (defined riscv_hyb) || (defined arm_hyb)
	int cid = (long) getSP() / CVM_MAX_SIZE;
#else
	unsigned long *tp = (__cheri_fromcap unsigned long *) getTP();
	int cid = tp[1];
#endif
	if((cid <= 0) || (cid >= MAX_CVMS)) {
//this will never work because at this moment TP is c_tp but not m_tp
		printf("wrong cvm id %d, sp = %p, die\n", cid, (long) getSP());
		while(1) ;
	}
// would be nice to check something
	struct c_thread *ret = &cvms[cid].threads[(cvms[cid].cmp_end - (((long) getSP() / STACK_SIZE) * STACK_SIZE)) / STACK_SIZE - 1];

	return ret;
}

unsigned long mon_to_comp(unsigned long addr, struct s_box *sbox) {
#if (defined arm_sim) || (defined x86_sim)
	return (addr);
#else
	return (addr - (!sbox->pure) * sbox->cmp_begin);
#endif
}

unsigned long comp_to_mon(unsigned long addr, struct s_box *sbox) {
#if (defined arm_sim) || (defined x86_sim)
	return (addr);
#else
	return (addr + (!sbox->pure) * sbox->cmp_begin);
#endif
}

unsigned long comp_to_mon_force(unsigned long addr, struct s_box *sbox) {
#if (defined arm_sim) || (defined x86_sim)
//intptr_t?
	return (addr + (unsigned long) sbox->cmp_begin);
#else
	return (addr + sbox->cmp_begin);
#endif
}

#ifndef SIM

#ifndef CHERI_PERM_CCALL
#define CHERI_PERM_CCALL CHERI_PERM_INVOKE
#endif

/*
 *	lwec: loading write-enabled caps
 */

void *__capability codecap_create(void *sandbox_base, void *sandbox_end, int disable_lwec) {
	void *__capability codecap;

	unsigned long extra_perm = CHERI_PERM_SW0;
	if(disable_lwec)
		extra_perm = 0;

	codecap = cheri_codeptrperm(sandbox_base, ((size_t) sandbox_end - (size_t) sandbox_base),
				    CHERI_PERM_GLOBAL | CHERI_PERM_LOAD | CHERI_PERM_STORE | CHERI_PERM_EXECUTE | CHERI_PERM_CCALL | CHERI_PERMS_HWALL | extra_perm);

	return (codecap);
}

//permissions here need a revision
void *__capability pure_codecap_create(void *sandbox_base, void *sandbox_end, int disable_lwec) {
	void *__capability codecap;

	unsigned long extra_perm = CHERI_PERM_SW0;
	if(disable_lwec)
		extra_perm = 0;

	codecap = cheri_codeptrperm(sandbox_base, ((size_t) sandbox_end - (size_t) sandbox_base),
				    CHERI_PERM_GLOBAL | CHERI_PERM_LOAD | CHERI_PERM_LOAD_CAP | CHERI_PERM_STORE | CHERI_PERM_EXECUTE | CHERI_PERM_CCALL | CHERI_PERMS_HWALL | extra_perm);

	codecap = cheri_capmode(codecap);

	return (codecap);
}

void *__capability datacap_create(void *sandbox_base, void *sandbox_end, int disable_lwec) {
	void *__capability datacap;

	unsigned long extra_perm = CHERI_PERM_SW0;
	if(disable_lwec)
		extra_perm = 0;

	datacap = cheri_ptrperm(sandbox_base, (size_t) sandbox_end - (size_t) sandbox_base, CHERI_PERM_GLOBAL | CHERI_PERM_LOAD | CHERI_PERM_STORE | CHERI_PERM_LOAD_CAP |
// we remove these two permisions to avoid using of revoked caps
				CHERI_PERM_STORE_CAP | CHERI_PERM_STORE_LOCAL_CAP | CHERI_PERM_CCALL | CHERI_PERMS_HWALL | extra_perm);

	return (datacap);
}

#endif

void place_canaries(unsigned long *begin, long size, long magic) {
	for(int i = 0; i < size / STACK_SIZE; i++) {
		begin[i * STACK_SIZE / 8] = magic;
		if(i > 0) {
#ifndef NC
//                      printf("mprotecting %p\n", &begin[i * STACK_SIZE / 8]);
#else
//                      NCPRINT("mprotecting %p\n", &begin[i * STACK_SIZE / 8]);
#endif
			if(mprotect(&begin[i * STACK_SIZE / 8], 4096, PROT_READ) == -1) {
				perror("mprotect");
				while(1) ;
			}
		}
	}
}

static char fbuf[1024];

void check_canaries(unsigned long *begin, long size, long magic) {
	for(int i = 0; i < size / STACK_SIZE; i++) {
		if(begin[i * STACK_SIZE / 8] != magic) {
//                      printf("CORRUPTED CANARY %d, %lx %lx\n", i, begin[i * STACK_SIZE / 8], magic);
			snprintf(fbuf, 1024, "CORRUPTED CANARY thread %d, addr %p, read %lx, expected %lx\n", i, &begin[i * STACK_SIZE / 8], begin[i * STACK_SIZE / 8], magic);
			write(2, fbuf, 1024);
			while(1) ;
		}
	}

}

int create_console(int cid) {

#if 0
	int fd;
//pipe. libvirt does like it
	char *fifo[20];
	snprintf(fifo, 20, "/tmp/cvm%d", cid);

	unlink(fifo);

	mkfifo(fifo, 0666);

	fd = open(fifo, O_WRONLY | O_NONBLOCK);

	printf("use #cat %s\n", fifo);

	return fd;

#endif
#if 0
//file
	int fd;
	char *fifo[20];
	snprintf(fifo, 20, "/tmp/cvm%d", cid);

	unlink(fifo);

	fd = open(fifo, O_WRONLY | O_CREAT | O_NONBLOCK, 0644);
	if(fd == -1) {
		perror("open");
		return -1;
	}

	printf("use #cat %s\n", fifo);

	return fd;
#endif
#if 1
	char *path;
	int fd = posix_openpt(O_RDWR | O_NOCTTY);
	grantpt(fd);
	unlockpt(fd);

	path = ptsname(fd);
	printf("pty path = %s\n", path);
	return fd;
#endif
}

int host_reg_cap(void *ptr, long size, void *location) {
#ifndef SIM
#warning host_reg_cap requires updating for CLEAN_ROOM
//todo: internally, datacap_create default cap of monitor. it might be slightly better to use caps of the target isolation layer
	void *__capability dcap = datacap_create(ptr, (void *) ((unsigned long) ptr + size), 0);
//      CHERI_CAP_PRINT(dcap);
#else
	void *dcap = ptr;
#endif

	st_cap(location, dcap);
}

int host_purge_cap(void *location) {
	memset(location, 0, 16);
}

long get_file_size(char *filename) {
	FILE *file = fopen(filename, "rb");	// Open the file in binary mode
	if(file == NULL) {
		// Error handling if the file cannot be opened
		perror("Error opening file");
		while(1) ;
	}

	fseek(file, 0, SEEK_END);	// Move the file pointer to the end of the file
	long fileSize = ftell(file);	// Get the current position of the file pointer
	fclose(file);		// Close the file

	return fileSize;
}

long copy_file_into_cvm(char *filename, long *where, long size) {
	printf("WARRNING: %s is unsafe and doesnt check where and what it loads\n", __func__);

	FILE *file = fopen(filename, "rb");
	if(file == NULL) {
		perror("Error opening file");
		while(1) ;
	}

	size_t bytesRead = fread(where, 1, size, file);
	if(bytesRead != size) {
		fclose(file);
		perror("Error reading file");
		while(1) ;
	}

	fclose(file);

	return bytesRead;
}
