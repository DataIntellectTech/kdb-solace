#define mxi(x,y) pthread_mutex_init(&x->y, NULL)
#define mxl(x,y) pthread_mutex_lock(&x->y)
#define mxu(x,y) pthread_mutex_unlock(&x->y)
#define cdi(x,y) pthread_cond_init(&x->y, NULL)
#define cdw(x,y,z) pthread_cond_wait(&x->y, &x->z)
#define cds(x,y) pthread_cond_signal(&x->y)

typedef struct {
	uint8_t * d;
	int h,t,n;
	pthread_mutex_t k;
	pthread_cond_t u, o;
} cb;

void cb_init(cb *b, int n);
int cb_size(cb *b);
int cb_space(cb *b);
int cb_write(cb *b, uint8_t *d, int n, int w);
int cb_read(cb *b, uint8_t *t, int n, int w);
