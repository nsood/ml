#include "list.h"
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

//#define DEBUG
#ifdef DEBUG
#define debug(format,...) printf("%s\t(%d):\t"format"\n",__func__, __LINE__, ##__VA_ARGS__)
#else
#define debug(format,...)
#endif

#define TRUE	1
#define FALSE	0

#define LENGTH		15
#define DATA_INIT	50
#define C_CNT		10
#define L_CNT		100
#define C_MUTA		0.85
#define P_MUTA		0.35

struct max_v_s {
	uint16_t	ch_x;
	uint16_t	ch_y;
	double 		x;
	double		y;
	double		z;
	struct list_head lh;
};

static char chromosome[32];
static struct max_v_s *max_v_min;
static struct max_v_s *max_v_max;
static int data_cnt;

LIST_HEAD(max_v_list);

int random_p(float p)
{
	return (rand()%100-p*100 < 0) ? TRUE : FALSE;
}

uint16_t gen_ch()
{
	int flag = random_p(0.5);
	uint16_t tmp = rand()%32768;

	return ((flag<<15) | (tmp&0x7fff));
}

uint16_t mut_ch(uint16_t ch)
{
	uint16_t cross;
	uint16_t after = ch;
	uint16_t tmp;

	if(random_p(P_MUTA)) {
		
		debug("oh god! mutant raised!");

		cross = 0x0001 << rand()%LENGTH;
		tmp = ch | cross;
		after &= ~cross;
		if(tmp==0) {
			after |= cross;
		}
	}
	return after;
}

char* print_ch(uint16_t ch)
{
	memset(chromosome,0,sizeof(chromosome));
	sprintf(chromosome,"%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d%d", \
			ch>>15&0x0001, \
			ch>>14&0x0001, \
			ch>>13&0x0001, \
			ch>>12&0x0001, \
			ch>>11&0x0001, \
			ch>>10&0x0001, \
			ch>>9&0x0001, \
			ch>>8&0x0001, \
			ch>>7&0x0001, \
			ch>>6&0x0001, \
			ch>>5&0x0001, \
			ch>>4&0x0001, \
			ch>>3&0x0001, \
			ch>>2&0x0001, \
			ch>>1&0x0001, \
			ch>>0&0x0001  \
		);
	return chromosome;
}

void print_data()
{
	struct max_v_s *pos,*next;

	list_for_each_entry_safe(pos,next,&max_v_list,lh) {
		debug("ch_x:%s ch_y:%s x:%f y:%f z:%f", \
		print_ch(pos->ch_x), \
		print_ch(pos->ch_y), \
		pos->x, \
		pos->y, \
		pos->z  \
		);
	}
	printf("data_cnt=%d x=%f y=%f z=%f\n",data_cnt,max_v_max->x,max_v_max->y,max_v_max->z);
}

void calc_xyz(struct max_v_s* tmp)
{
	int flag_x = (tmp->ch_x>>15 & 0x0001) ? -1 : 1; 
	int flag_y = (tmp->ch_y>>15 & 0x0001) ? -1 : 1; 

	tmp->x = flag_x*10*((double)(tmp->ch_x & 0x7fff)+1)/32768;
	tmp->y = flag_y*10*((double)(tmp->ch_y & 0x7fff)+1)/32768;
	//tmp->z = tmp->y * sin(tmp->x) + tmp->x * cos(tmp->y);
	tmp->z = tmp->y * cos(tmp->x) + tmp->x * sin(tmp->y);

	//debug("X=%f\tY=%f\tZ=%f",tmp->x,tmp->y,tmp->z);
}

void max_v_sync()
{
	double min=1000;
	double max=0;
	struct max_v_s *pos,*next;

	data_cnt = 0;

	list_for_each_entry_safe(pos,next,&max_v_list,lh) {
		data_cnt++;
		if (pos->z < min) {
			min = pos->z;
			max_v_min = pos;
		}
		if (pos->z > max) {
			max = pos->z;
			max_v_max = pos;
		}
	}
}

void data_del_min()
{
	list_del(&max_v_min->lh);
	free(max_v_min);
	max_v_sync();
}

void data_del(int del_num)
{
	int i;
	for( i =0; i < del_num; i++) {
		data_del_min();
	}
}

void data_init(int init_num)
{
	int i=0;
	struct max_v_s *new;

	for ( i = 0; i < init_num; i++ ) {
		new = (struct max_v_s*)malloc(sizeof(struct max_v_s));
		new->ch_x = gen_ch();
		new->ch_y = gen_ch();		
		calc_xyz(new);

		list_add_tail(&new->lh,&max_v_list);
	}
	max_v_sync();
}

void data_clean()
{
	struct max_v_s *pos,*next;

	list_for_each_entry_safe(pos,next,&max_v_list,lh) {
		list_del(&pos->lh);
		free(pos);
	}
}

void max_v_cross(struct max_v_s *f,struct max_v_s *m)
{
	uint16_t cross,mask;
	uint16_t x1,x2,y1,y2;
	struct max_v_s *new;

	cross = rand()%(LENGTH-7)+7;
	mask = 0x7fff >> cross;

	debug("cross mask %04X",mask);

	x1 = (f->ch_x & mask) | (m->ch_x & ~mask);
	x2 = (m->ch_x & mask) | (f->ch_x & ~mask);
	y1 = (f->ch_y & mask) | (m->ch_y & ~mask);
	y2 = (m->ch_y & mask) | (f->ch_y & ~mask);

	new = (struct max_v_s*)malloc(sizeof(struct max_v_s));
	new->ch_x = mut_ch(x1);
	new->ch_y = mut_ch(y1);
	calc_xyz(new);
	debug("child :x=%f y=%f z=%f",new->x,new->y,new->z);
	if (new->z > f->z && new->z > m->z)
		list_add_tail(&new->lh,&max_v_list);

	new = (struct max_v_s*)malloc(sizeof(struct max_v_s));
	new->ch_x = mut_ch(x1);
	new->ch_y = mut_ch(y2);
	calc_xyz(new);
	debug("child :x=%f y=%f z=%f",new->x,new->y,new->z);
	if (new->z > f->z && new->z > m->z)
		list_add_tail(&new->lh,&max_v_list);

	new = (struct max_v_s*)malloc(sizeof(struct max_v_s));
	new->ch_x = mut_ch(x2);
	new->ch_y = mut_ch(y1);
	calc_xyz(new);
	debug("child :x=%f y=%f z=%f",new->x,new->y,new->z);
	if (new->z > f->z && new->z > m->z)
		list_add_tail(&new->lh,&max_v_list);

	new = (struct max_v_s*)malloc(sizeof(struct max_v_s));
	new->ch_x = mut_ch(x2);
	new->ch_y = mut_ch(y2);
	calc_xyz(new);
	debug("child :x=%f y=%f z=%f",new->x,new->y,new->z);
	if (new->z > f->z && new->z > m->z)
		list_add_tail(&new->lh,&max_v_list);

	max_v_sync();
}

void max_v_choice()
{
	int i;
	int rand_f,rand_m;
	struct max_v_s *pos,*next;
	struct max_v_s *f=NULL;
	struct max_v_s *m=NULL;

	for( i = 0; i < C_CNT; i++) {
		if (data_cnt < 2) return;

		rand_f = rand()%data_cnt+1;
		do {
			rand_m = rand()%data_cnt+1;
		} while(rand_f==rand_m);

		debug("f_i:%d m_i:%d",rand_f,rand_m);

		list_for_each_entry_safe(pos,next,&max_v_list,lh) {
			if (rand_f > 0) rand_f--;
			if (rand_m > 0) rand_m--;
			if (rand_f == 0 && f==NULL ) f = pos;
			if (rand_m == 0 && m==NULL ) m = pos;
			if (f!=NULL && m!=NULL) break;
		}

		if (f!=NULL && m!=NULL && f->z!=m->z && random_p(P_MUTA)) {
			debug("father:x=%f y=%f z=%f",f->x,f->y,f->z);
			debug("mother:x=%f y=%f z=%f",m->x,m->y,m->z);
			max_v_cross(f,m);
		}
	}
}

int main()
{
	int i=0;
	srand(time(0));
	debug("Time is %d now",(int)time(0));

	data_init(DATA_INIT);
	print_data();

	for ( i = 0; i < L_CNT; i++) {
		max_v_choice();
		data_del(data_cnt-DATA_INIT);
		print_data();
		sleep(2);
	}

	print_data();
	data_clean();

	return 0;
}
