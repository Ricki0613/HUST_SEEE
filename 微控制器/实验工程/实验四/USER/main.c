#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "lcd.h"
#include "led.h"
#include <stdio.h>
#include <string.h>
static uint32_t baud=115200,peer,sequence;
volatile uint32_t last_len,last_us;
static uint8_t sample[121];
/* Move the complete interface down by four 16-pixel text rows. */
#define LCD_TOP_OFFSET 64U
static void line(unsigned y,char *s){
 y+=LCD_TOP_OFFSET;
 if(y+16<=lcddev.height)LCD_ShowString(4,y,lcddev.width-8,16,16,(u8*)s);
}
static void home(void){
 LCD_Clear(WHITE);POINT_COLOR=BLUE;
 line(4,peer?"MODE: BOARD":"MODE: PC ECHO");line(24,baud==9600?"9600 8N1":"115200 8N1");
 line(44,"K0:SEND K1:BAUD K2:MODE");
}
static void received(uint8_t *p,unsigned n){
 unsigned i,j,w=(lcddev.width-8)/8,y=106;char row[61];
 if(w>60)w=60;
 for(i=0;i<n && y+LCD_TOP_OFFSET+16<=lcddev.height;){for(j=0;j<w && i<n;j++,i++)row[j]=(p[i]>=32 && p[i]<=126)?p[i]:'.';
 row[j]=0;line(y,row);y+=16;}
}
static void keys_init(void){GPIO_InitTypeDef g={0};__HAL_RCC_GPIOE_CLK_ENABLE();g.Pin=GPIO_PIN_2|GPIO_PIN_3|GPIO_PIN_4;g.Mode=GPIO_MODE_INPUT;g.Pull=GPIO_PULLUP;HAL_GPIO_Init(GPIOE,&g);}
static unsigned key(void){
 static unsigned prev,stable;static uint32_t changed;
 unsigned v=(~GPIOE->IDR>>2)&7u;
 if(v!=prev){prev=v;changed=HAL_GetTick();}
 if(v!=stable && HAL_GetTick()-changed>=20){stable=v;return v;}
 return 0;
}
int main(void){
 unsigned k,n,ack,i;uint8_t *p;char msg[48];
 HAL_Init();Stm32_Clock_Init(336,8,2,7);delay_init(168);
 LED_Init();keys_init();uart_init(baud);LCD_Init();
 CoreDebug->DEMCR|=CoreDebug_DEMCR_TRCENA_Msk;DWT->CYCCNT=0;DWT->CTRL|=1;
 Lab_Reset();home();
 while(1){
  k=key();
  if(k==2 || k==1){
   if(k==2){baud=baud==9600?115200:9600;uart_init(baud);}else peer=!peer;
   Lab_Reset();home();
  }
  if(k==4 && peer){
   sequence++;sprintf((char*)sample,"FRAME %lu ",(unsigned long)sequence);
   for(i=strlen((char*)sample);i<120;i++)sample[i]='A'+i%26;
   Lab_Send(sample,120);Lab_Send((uint8_t*)"\r\n",2);
  }
  if(!Lab_Ready())continue;
  if(Lab_Ready()==2){Lab_Send((uint8_t*)"ERR\r\n",5);Lab_Reset();line(66,"RX ERROR / TOO LONG");continue;}
  n=Lab_Count();p=Lab_Data();ack=peer && n>=3 && !memcmp(p,"ok+",3);
  if(!ack){Lab_Send((uint8_t*)"ok+",3);Lab_Send(p,n);Lab_Send((uint8_t*)"\r\n",2);}
  last_us=Lab_Elapsed();last_len=n;Lab_LedOff();
  LCD_Fill(0,64+LCD_TOP_OFFSET,lcddev.width-1,lcddev.height-1,WHITE);
  sprintf(msg,"RX:%u bytes",n);line(66,msg);
  sprintf(msg,"Time:%lu ms",(unsigned long)(last_us/1000));line(86,msg);
  received(p,n);Lab_Reset();
 }
}
