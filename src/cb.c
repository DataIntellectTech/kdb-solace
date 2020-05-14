#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include "cb.h"

void cb_init(cb *b,int n) {

	uint8_t* d = malloc(n);

	b->d = d;
	b->h = 0;
	b->t = 0;
	b->n = n;

	mxi(b,k);
	cdi(b,u);
	cdi(b,o);

}


int cb_size(cb *b) {

	int h,t,n;

	h = b->h;
	t = b->t;
	n = b->n;

	return h<=t?t-h:t+(n-h);

}


int cb_space(cb *b) {

	return b->n-(1 + cb_size(b));

}

int cb_write(cb *b, uint8_t *d, int n, int w) {

	mxl(b,k);

	while(n > cb_space(b)) {

		if(!w) {

			mxu(b,k);
			return -1;

		}

		cdw(b,o,k);

	}

	if(b->n <= b->t+n) {

		int e = b->n-b->t;
		memcpy(&b->d[b->t],d,e);
		d += e;
		n -= e;
		b->t = 0;

	}

	memcpy(&b->d[b->t],d,n);
	b->t += n;
	mxu(b,k);
	cds(b,u);

	return 0;

}


int cb_read(cb *b, uint8_t *t, int n, int w) {

	mxl(b,k);

	while(n > cb_size(b)) {

		if(!w) {

			mxu(b,k);
			return -1;

		}

		cdw(b,u,k);

	}

	if(b->n <= b->h+n) {

		int e = b->n - b->h;
		memcpy(t,&b->d[b->h],e);
		t += e;
		n -= e;
		b->h = 0;

	}

	memcpy(t, &b->d[b->h], n);
	b->h += n;
	mxu(b,k);
	cds(b,o);

	return 0;

}
