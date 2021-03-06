#include <avr/io.h>
#include <avr/interrupt.h>

int Get_ADC(unsigned char ADC_num);
void Uart_Init();
void Uart_Trans(unsigned char data);
void Num_Trans(int numdata);
int Normal_AD(int AD,int AD_Max, int AD_min);

volatile unsigned int i = 0;
volatile unsigned int j = 0;
volatile unsigned int T = 0;
volatile unsigned int A[8]; // 간편 led 확인
volatile unsigned int AD[8]; // 읽은값 --------------------------------------------------
volatile unsigned int NOMALAD[8] = {0,0,0,0,0,0,0,0}; // 정규화값 저장 ----------------------------------------------
volatile unsigned int AD_min[8]={0,0,0,0,0,0,0,0}; //최솟값
volatile unsigned int AD_Max[8]={0,0,0,0,0,0,0,0}; //최대값
volatile int WEIGHT[8]={-4,-2,-1,0,0,1,2,4}; //가중치 ---------------------------------------------------
volatile unsigned int AD_BW[8];//검정 하양
volatile unsigned char Mode = 's';
volatile unsigned int cnt = 0;
volatile unsigned int data =0;
volatile unsigned int numdata = 0;
volatile int weightvalue = 0; //
volatile int last_wv = 0;
volatile int leftspeed = 0;
volatile int rightspeed = 0;
volatile unsigned int crline = 0;
volatile unsigned int flag = 0;
volatile unsigned int re = 0;
volatile unsigned int sum = 0;
volatile unsigned int cnt1 = 0;	// 잠깐 정지를 위한 카운트 변수1
volatile unsigned int cnt2 = 0;	// 잠깐 정지를 위한 카운트 변수2
volatile unsigned int cnt_line=0;


ISR(INT0_vect)  {
	Mode = 'R';
}

ISR(INT1_vect)  {
	Mode = 'D';
}


ISR(TIMER0_OVF_vect)   //추천->10ms
{
	TCNT0 = 131;
	cnt++;
	
	if(cnt >= 5)
	{
		
		for(int i=0;i<8;i++){
			AD[i]=Get_ADC(i);
		}
		
		if( Mode == 'R')
		{
			PORTA = 0b00110000;
			for(int i=0;i<8;i++){
				
				if(AD[i]>=AD_Max[i])
				{
					AD_Max[i]=AD[i];
				}
				if(AD[i]<=AD_min[i])
				{
					AD_min[i]=AD[i];
				}
			}
		}//mode r
		
		if( Mode == 'D')
		{
			
			for(int i=0;i<8;i++){
				NOMALAD[i]=Normal_AD(AD[i],AD_Max[i],AD_min[i]);
			}
			
			for(int i=0;i<8;i++){
				if(NOMALAD[i]<50) AD_BW[i]=1;//검정일땐 1
				else AD_BW[i]=0; //하양일땐 0
			}
			
			//LED
			T=0xff;
			for(int i=0;i<8;i++){
				A[i]=0xff;
			}
			if(AD_BW[0])A[0]=0b11111110;
			if(AD_BW[1])A[1]=0b11111101;
			if(AD_BW[2])A[2]=0b11111011;
			if(AD_BW[3])A[3]=0b11110111;
			if(AD_BW[4])A[4]=0b11101111;
			if(AD_BW[5])A[5]=0b11011111;
			if(AD_BW[6])A[6]=0b10111111;
			if(AD_BW[7])A[7]=0b01111111;
			
			for(int i=0;i<8;i++){
				T=T&A[i];
			}
			PORTA=T;
			//------------------------가중치--------------------------
			
			for(int i=0;i<8;i++)
			{
				weightvalue += (WEIGHT[i] * NOMALAD[i]);
			}
			
			leftspeed = 650 + weightvalue;
			if (leftspeed>799)	leftspeed = 790;
			
			rightspeed = 650 - weightvalue;
			if (rightspeed>799)	rightspeed = 790;
			

			OCR1A = leftspeed;
			OCR1B = rightspeed;
			
			
			//-----------------------------------------------------
			//직각상황
			volatile unsigned int wb = 0;
			
			for(int i=0;i<8;i++){
				wb += AD_BW[i];
				
				if(wb!= 0)
				{
					last_wv = weightvalue;
				}
				
				if(wb == 0){
					if(last_wv < 0){
						
						OCR1A = 0;		//
						OCR1B = 750;		//왼쪽
					}
					if(last_wv > 0){
						
						OCR1A = 750;			//오른쪽
						OCR1B = 0;	//왼쪽
					}
				}
			}
			//---------------------------------------------------------------
			// 교차 선 세기
			
			if (wb ==8){
				
				flag =	1;
			}
			
			if (wb < 4 && flag == 1){
				
				flag =0;
				crline ++;
				
			}
			
			if(crline ==2){
				
				OCR1A = leftspeed+100;
				OCR1B = rightspeed+100;
				
			}

			if(crline == 5)
			{
				// 				if (cnt_line<=100)
				// 				{
				// 					OCR1A = 0;
				// 					OCR1B = 0;
				// 				}
				// 				cnt_line++;
				// 				if (cnt_line>=100) crline += 1;
				
				for (cnt1 = 0; cnt1 < 1000; cnt1++)
				{
					for(int ss = 0; ss < 1000; ss++)  // if(cnt_line>=100)
					{

					}
					
				}
				
				crline += 1;	// 임의로 교차로 1개 더 세어서 정지모드 탈출

			}
			else if (crline == 6){

				OCR1A = leftspeed;
				OCR1B = rightspeed;

			}

			if(crline == 7){

				for (cnt2 = 0; cnt2 < 3500; cnt2++)
				{
					for(int ss = 0; ss < 1000; ss++)
					{
						PORTE= 0b00001001;

						OCR1A = 700;			//오른쪽
						OCR1B = 700;	//왼쪽
					}
				}
				crline += 1;
			}
			else if (crline == 8){
				
				PORTE= 0b00001010;
				OCR1A = leftspeed;
				OCR1B = rightspeed;

			}
			
			if(crline ==11){
				
				OCR1A = 0;			//오른쪽
				OCR1B = 0;	//왼쪽
				
			}
			
			
			//반전	-------------------------------------------------------
			
			sum = NOMALAD[0]+NOMALAD[1]+NOMALAD[2]+NOMALAD[3]+NOMALAD[4]+NOMALAD[5]+NOMALAD[6]+NOMALAD[7];
			
			if (sum < 200){
				
				OCR1A = rightspeed;
				OCR1B = leftspeed;
				
				weightvalue = (weightvalue*-1);
				
				
			}
			if(sum<140){
				
				if(OCR1A < OCR1B){
					
					OCR1A = 0;		//
					OCR1B = 700;		//왼쪽
				}
				if(OCR1A > OCR1B){
					
					OCR1A = 700;			//오른쪽
					OCR1B = 0;	//왼쪽
				}
			}
			
			

			//-------------------------------------------------------------
			//
			//UART로 센서값 확인
			
			
			// 			for(int i=0;i<8;i++)
			// 			{
			// 				Num_Trans(NOMALAD[i]);//ADC값(int)->unsigned char 로 변환->Uart_trans
			// 				Uart_Trans(0x09);//tab
			// 			}

			Num_Trans(sum);//ADC값(int)->unsigned char 로 변환->Uart_trans
			Uart_Trans(0x2C);
			Num_Trans(crline);
			Uart_Trans(0x0d);//tab

			
			

			weightvalue=0;
		}//mode d
		
		
		cnt = 0;
	}//if(cnt == 5)


	
	
}//ISR(TIMER0_OVF_vect)

int main(void)
{
	DDRA = 0xff;
	DDRF = 0x00;
	DDRD = 0b00001000;
	PORTA = 0xff;
	PORTE= 0b00001010;

	DDRB =0xff;
	DDRE=0x0f;
	
	
	ADMUX = 0b10000000;
	ADCSRA = 0b10000111;
	
	EICRA = (1<<ISC01)|(1<<ISC11);
	EIMSK = (1<<INT1)|(1<<INT0);
	

	Uart_Init();
	
	TCCR1A = (1<<COM1A1)|(0<<COM1A0)|(1<<COM1B1)|(0<<COM1B0)|(1<<WGM11);
	TCCR1B = (1<<WGM13)|(1<<WGM12)|(0<<CS02)|(0<<CS01)|(1<<CS00);
	
	TCCR0 = (0<<WGM01) | (0<<WGM00) | (0<<COM01) | (0<<COM00) | (1<<CS02) | (1<<CS01) | (0<<CS00);
	TIMSK = (1<<TOIE0);
	TCNT0 = 131;


	ICR1 =799;
	OCR1A = 0;		//오른쪽
	OCR1B = 0;		//왼쪽
	
	sei();
	
	while (1){
	}

}

void Uart_Init()
{
	UCSR1A=0x00;
	UCSR1B=(1<<RXEN)|(1<<TXEN);
	UCSR1C=(1<<UCSZ11)|(1<<UCSZ10);
	UBRR1H=0;
	UBRR1L=103;
}

int Get_ADC(unsigned char ADC_num)
{
	ADMUX =(ADC_num|0b01000000);
	ADCSRA |=(1<<ADSC);
	while(!(ADCSRA&(1<<ADIF)));
	return ADC;
}

void Uart_Trans(unsigned char data)
{
	while (!( UCSR1A & (1<<UDRE1)));
	UDR1 = data;
}
void Num_Trans(int numdata)
{
	int data;
	
	if(numdata<0)
	{
		Uart_Trans('-');
		numdata=-1*numdata;
	}
	data=numdata/10000;
	Uart_Trans(data+48);
	data=(numdata%10000)/1000;
	Uart_Trans(data+48);
	data=(numdata%1000)/100;
	Uart_Trans(data+48);
	data=(numdata%100)/10;
	Uart_Trans(data+48);
	data=numdata%10;
	Uart_Trans(data+48);
	
}

int Normal_AD(int AD, int AD_Max, int AD_min)
{
	double res=0;
	res=((double)(AD-AD_min)/(AD_Max-AD_min))*100;
	res=(int)res;
	return res;
}