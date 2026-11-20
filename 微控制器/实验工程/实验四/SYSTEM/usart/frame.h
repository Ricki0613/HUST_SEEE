#ifndef LAB_FRAME_H
#define LAB_FRAME_H
#include <stdint.h>
#define LIMIT 4096u
typedef struct {uint8_t data[LIMIT];uint16_t n;uint8_t cr,bad,started;} Frame;
/* 0=incomplete, 1=ready, 2=discard through the next CRLF */
static void clear_frame(Frame *f){f->n=0;f->cr=f->bad=f->started=0;}
static void append(Frame *f,uint8_t c){if(f->n<LIMIT)f->data[f->n++]=c;else f->bad=1;}
static int feed(Frame *f,uint8_t c){
 f->started=1;
 if(f->cr){f->cr=0;if(c==10)return f->bad?2:1;append(f,13);}
 if(c==13)f->cr=1;else append(f,c);
 return 0;
}
#endif
