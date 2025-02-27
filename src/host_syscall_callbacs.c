#include "monitor.h"

#define CB_NAME_LEN		20
#define MAX_CALLBACKS	(MAX_CVMS+1)

//#define CB_DEBUG 1

//todo: caps here should be sealed
struct sc_cb_store_s {
	char name[CB_NAME_LEN];
	void *__capability pcc;
	void *__capability ddc;
	void *__capability pcc2;
};

static struct sc_cb_store_s sc_cbs[MAX_CALLBACKS];
static pthread_mutex_t sc_cb_store_lock;

void init_cbs() {
	memset(sc_cbs, 0, MAX_CALLBACKS * sizeof(struct sc_cb_store_s));

	if(pthread_mutex_init(&sc_cb_store_lock, NULL) != 0) {
		printf("\n mutex init failed\n");
		while(1) ;
	}
}

//NB: I assume that libos name is always different even in the case of Unique compontnts in SCO

int host_syscall_handler_adv(char *libos, void *__capability pcc, void *__capability ddc, void *__capability pcc2) {
#ifdef CB_DEBUG
	printf("MON: SYSCALL advertise: %s, %p, %p\n", libos, pcc, ddc);
#endif
	pthread_mutex_lock(&sc_cb_store_lock);
	int i = 0;
	for(i = 0; i < MAX_CALLBACKS; i++) {
		if(sc_cbs[i].name[0] == 0)
			break;
	}

	if(i == MAX_CALLBACKS) {
		printf("need more sc_cbs, die\n");
		while(1) ;
	}

	snprintf(sc_cbs[i].name, CB_NAME_LEN, "%s", libos);
	sc_cbs[i].pcc = pcc;
	sc_cbs[i].pcc2 = pcc2;
	sc_cbs[i].ddc = ddc;

#ifdef CB_DEBUG
	printf("MON: sc_cbs[%d]: %s, %p, %p, %p\n", i, sc_cbs[i].name, sc_cbs[i].pcc, sc_cbs[i].ddc);
#endif
	pthread_mutex_unlock(&sc_cb_store_lock);
	return 0;
}

#if 0
//wrong
int host_syscall_handler_purge(char *libos) {
#ifdef CB_DEBUG
	printf("MON: SYSCALL purge: %s\n", libos);
#endif
	pthread_mutex_lock(&sc_cb_store_lock);
	int i = 0;
	for(i = 0; i < MAX_CALLBACKS; i++) {
		if(sc_cbs[i].name[0] == 0)
			break;
	}

	if(i == MAX_CALLBACKS) {
		pthread_mutex_unlock(&sc_cb_store_lock);
		return 0;	//there is no a callback, should it be?
	}

	memset(sc_cbs[i].name, 0, CB_NAME_LEN);
	sc_cbs[i].pcc = 0;
	sc_cbs[i].pcc2 = 0;
	sc_cbs[i].ddc = 0;

	pthread_mutex_unlock(&sc_cb_store_lock);
	return 0;
}
#endif

int host_syscall_handler_prb(char *key, void *ppc, void *ddc, void *ppc2) {
#ifdef CB_DEBUG
	printf("MON: probe syscall handler for key %s, PCC %p, DDC %p, PPC2 %p\n", key, ppc, ddc, ppc2);
#endif
	pthread_mutex_lock(&sc_cb_store_lock);
	int i;
	for(i = 0; i < MAX_CALLBACKS; i++) {
#ifdef CB_DEBUG
		printf("CHECK %d: '%s' vs '%s'\n", i, key, sc_cbs[i].name);
#endif
		if(strncmp(sc_cbs[i].name, key, CB_NAME_LEN) == NULL)
			break;
	}

	if(i == MAX_CALLBACKS) {
		printf("wrong SC key %s\n", key);
		while(1) ;
	}

	st_cap(ppc, sc_cbs[i].pcc);
	st_cap(ppc2, sc_cbs[i].pcc2);
	st_cap(ddc, sc_cbs[i].ddc);

	pthread_mutex_unlock(&sc_cb_store_lock);
	return i;
}
