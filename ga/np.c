#include "list.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>

//#define DEBUG

#ifdef DEBUG
#define debug(format,...) printf("%s(%d):\t\t"format"\n",__func__, __LINE__, ##__VA_ARGS__)
#else
#define debug(format,...)
#endif

#define TRUE	1
#define FALSE	0


struct np_s {
	char	chromosome;
	int		weight;
	int		value;
	struct list_head lh;
};

#define LENGTH		7		//chromosome length
#define POP_INIT	50		//init pop count
#define POP_MAX		50		//max pop count
#define OW_MAX		150		//overweight max
#define P_CROS		0.85	//cross fucking probability
#define P_MUTA		0.15	//mutant probablitiy
#define C_CNT		20		//choice times every living
#define L_CNT		100		//living times whole program
#define BREAK_CNT	20

static int w_a[LENGTH] = {35,30,60,50,40,10,25};
static int v_a[LENGTH] = {10,40,30,50,35,40,30};

static int pop_cnt=0;

static char	chromosome[20]; 		//buf for print
static int	last_max_value=0;
static int	suit_sum_value=0;
static struct np_s *min_np=NULL;
static struct np_s *max_np=NULL;

LIST_HEAD(np_list);

//genetic code
//0 0 0 0 0 0 0 0
//  ~ ~ ~ ~ ~ ~ ~
//0 1 1 1 1 1 1 1
//char 0~127

void np_clean()
{
	struct np_s *pos,*next;

	list_for_each_entry_safe(pos,next,&np_list,lh) {
		list_del(&pos->lh);
		free(pos);
	}
	pop_cnt = 0;
}

char generate()
{
	return (char)(rand()%127+1); 
}

int random_p(float p)
{
	return (rand()%100-p*100 < 0) ? TRUE : FALSE;
}

char* print_ch(char ch)
{
	memset(chromosome,0,sizeof(chromosome));
	sprintf(chromosome,"%d-%d-%d-%d-%d-%d-%d", \
			ch>>6&0x01, \
			ch>>5&0x01, \
			ch>>4&0x01, \
			ch>>3&0x01, \
			ch>>2&0x01, \
			ch>>1&0x01, \
			ch>>0&0x01  \
		);
	return chromosome;
}

void print_pop()
{
	struct np_s *pos,*next;

	list_for_each_entry_safe(pos,next,&np_list,lh) {
		debug("chromosome=%s\tw=%d\tv=%d",print_ch(pos->chromosome),pos->weight,pos->value);
	}
	printf("\npop_cnt=%d\tMVP=%s\tw=%d\tv=%d\n",pop_cnt,print_ch(max_np->chromosome),max_np->weight,max_np->value);
	printf("------------------------------------------------------------\n");
}

void calc_w_v(struct np_s *np_t)
{
	int i = 0;
	int weight	= 0;
	int value	= 0;

	for ( i = 0 ; i < LENGTH; i++) {
		if (np_t->chromosome >> i & 0x01) {
			weight += w_a[i];
			value += v_a[i];
		}
	}

	np_t->weight = weight;
	np_t->value = value;

	debug("weight=%d\tvalue=%d",np_t->weight,np_t->value);
}

void np_sync()
{
	int min_value = 1000;
	int max_value = 0;
	struct np_s *pos,*next;

	suit_sum_value = 0;
	pop_cnt = 0;

	list_for_each_entry_safe(pos,next,&np_list,lh) { 

		//record the sum of pop value
		suit_sum_value += pos->value;
		//record the count of pop
		pop_cnt += 1;

		//record the min value np
		if (pos->value < min_value) {
			min_value = pos->value;
			min_np = pos;
		}
		//recore the max value np
		if (pos->weight <= OW_MAX && pos->value > max_value ) {
			max_value = pos->value;
			max_np = pos;
		}
	}
}

char np_mutant(char ch)
{
	char cross;
	char after = ch;
	char tmp;

	if(random_p(P_MUTA)) {
		
		debug("oh god! mutant raised!");

		cross = 0x01 << rand()%LENGTH;
		tmp = ch | cross;
		after &= ~cross;
		if(tmp==0) {
			after |= cross;
		}
	}
	return after;
}

struct np_s* np_nature_mut(struct np_s *np_t)
{
	char tmp_ch = np_mutant(np_t->chromosome);

	if ( np_t->chromosome != tmp_ch ) {
		np_t->chromosome = tmp_ch;
		calc_w_v(np_t);
		np_sync();	
	}
	return np_t;
}

void np_cross_gen(struct np_s *f,struct np_s *m)
{
	int cross=0;
	char mask=0;
	char child1,child2;
	struct np_s *new;

	cross = rand()%(LENGTH-1)+1;
	mask = 0x7f >> cross;

	child1 = ( f->chromosome & mask ) | ( m->chromosome & ~mask );
	child2 = ( m->chromosome & mask ) | ( f->chromosome & ~mask );

	debug("child1 chromosome=%s",print_ch(child1));
	debug("child2 chromosome=%s",print_ch(child2));

	new = (struct np_s*)malloc(sizeof(struct np_s));
	new->chromosome = np_mutant(child1);
	calc_w_v(new);
	list_add_tail(&new->lh,&np_list);

	new = (struct np_s*)malloc(sizeof(struct np_s));
	new->chromosome = np_mutant(child2);
	calc_w_v(new);
	list_add_tail(&new->lh,&np_list);

	np_sync();
}

void np_del_min(int n)
{
	int i=0;
	for ( i = 0; i < n; i++) {
		list_del(&min_np->lh);
		free(min_np);
		np_sync();
	}
}

void np_choice()
{
	int cros_i;
	int rand_i;
	struct np_s *pos,*next;
	struct np_s *father=NULL;
	struct np_s *mother=NULL;

	for ( cros_i = 0; cros_i < C_CNT; cros_i++) {
		father = NULL;
		mother = NULL;

		//choice the father
		rand_i = rand()%suit_sum_value;
		list_for_each_entry_safe(pos,next,&np_list,lh) {
			rand_i -= pos->value;
			if ( rand_i <= 0 ) {
				father = pos;
				//father = np_nature_mut(pos);
				break;
			} else {
				continue;
			}
		}

		//choice the mother
		rand_i = rand() % ( suit_sum_value - father->value );
		list_for_each_entry_safe(pos,next,&np_list,lh) { 
			rand_i -= pos->value;
			if (pos->chromosome == father->chromosome) {
				continue;
			}
			if ( rand_i <= 0 ) {
				mother = pos;
				//mother = np_nature_mut(pos);
				break;
			} else {
				continue;
			}
		}

		if(father!=NULL && mother!=NULL && random_p(P_CROS)) {
			//debug print father and mother
			debug("choice father:%s",print_ch(father->chromosome));
			debug("choice mother:%s",print_ch(mother->chromosome));
			debug("fucking~");
			np_cross_gen(father,mother);

		} else {
			debug("fucking missed");
		}
	}
}

void suit_overweight(int n)
{
	int ow_cnt = n;
	struct np_s *pos,*next;

	list_for_each_entry_safe(pos,next,&np_list,lh) {
		if (pos->weight > OW_MAX) {
			list_del(&pos->lh);
			free(pos);
		}
		if(ow_cnt-- <= 0) {
			break;
		}
	}
	np_sync();
}

void np_suit()
{
	debug("before suit pop_cnt:%d",pop_cnt);
	//suit_overweight(POP_MAX-C_CNT);
	suit_overweight(POP_MAX);
	np_del_min(pop_cnt-POP_MAX);
	debug("after suit pop_cnt:%d",pop_cnt);
}

void np_init()
{
	int i;
	struct np_s *new;

	debug("init~");

	pop_cnt = 0;
	for( i = 0 ; i < POP_INIT ; i++) {
		new = (struct np_s*)malloc(sizeof(struct np_s));
		new->chromosome = generate();
		calc_w_v(new);
		list_add_tail(&new->lh,&np_list);
	}

	np_sync();
	debug("after init pop_cnt:%d",pop_cnt);
}

int main()
{
	int i=0;
	int break_cnt=BREAK_CNT;

	srand(time(0));
	debug("Time is %d now",(int)time(0));

	np_init();
	print_pop();

	for ( i = 0; i < L_CNT; i++) {
		last_max_value = max_np->value;
		np_choice();
		np_suit();
		print_pop();
		printf("living %d\n",i);

		if (last_max_value == max_np->value) {
			break_cnt--;
		} else if (last_max_value < max_np->value) {
			break_cnt = BREAK_CNT;
		}
		if (break_cnt < 0) {
			break;
		}

		sleep(1);
	}
	suit_overweight(POP_MAX);
	print_pop();
	np_clean();
	return 0;
}
