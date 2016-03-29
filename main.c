#include "EM78P301N.h"

#define DISI()                _asm{disi}
#define WDTC()                _asm{wdtc}
#define NOP()                 _asm{nop}
#define ENI()                 _asm{eni}
#define SLEP()                _asm{slep}
//R4/(R3+R4) = 10/(10+10) = 0.5
#define LOW_BAT_VOLT_TH       0x955                                 //(3.5V/2)/3V*4096=2389
#define MID_BAT_VOLT_TH       0xA22                                 //(3.8V/2)/3V*4096=2594
#define LOW_BAT_VOLT          0x888                                 //(3.2V/2)/3V*4096=2184
#define LOW_LOAD_VOLT         0x888                                 //(3.2V/2)/3V*4096=2184
#define SHORT_LOAD_VOLT       0x400                                 //(1.5V/2)/3V*4096=1024
#define CHARGE_BAT_VOLT_TH    0xB11                                 //(4.15V/2)/3V*4096=2833
#define WAKEUP_LOAD_VOLT      0x155                                 //(0.5V/2)/3V*4096=341
#define MOS_ON                1                                     //��MOS
#define MOS_OFF               156                                   //�ر�MOS
#define VBAT37                0x9DD                                 //(3.7V/2)/3V*4096=2525  --100
#define VBAT38                0xA22                                 //(3.8V/2)/3V*4096=2594  --94
#define VBAT39                0xA66                                 //(3.9V/2)/3V*4096=2662  --90
#define VBAT40                0xAAB                                 //(4.0V/2)/3V*4096=2731  --85
#define VBAT41                0xAEF                                 //(4.1V/2)/3V*4096=2799  --81
#define VBAT42                0xB33                                 //(4.2V/2)/3V*4096=2867  --77
#define MOS_37_38             5                                     //3%*156 = 5            
#define MOS_38_39             13                                    //8%*156 = 13  
#define MOS_39_40             19                                    //12%*156 = 19  
#define MOS_40_41             27                                    //17%*156 = 27 
#define MOS_41_42             33                                    //21%*156 = 33

#define uchar unsigned char 
#define ushort unsigned short 
#define uint unsigned int 

uchar g_fault_state              @0X20:bank 0;  //����״̬��
ushort g_load_volt               @0X21:bank 0;
uchar g_load_volt_l              @0X21:bank 0;
uchar g_load_volt_h              @0X22:bank 0;
ushort g_battery_volt            @0X23:bank 0;
uchar g_battery_volt_l           @0X23:bank 0;
uchar g_battery_volt_h           @0X24:bank 0;
uchar g_keyval                   @0X25:bank 0;
uchar g_time1ms_cnt              @0X26:bank 0;
uchar g_time50ms_cnt             @0X27:bank 0;
uchar g_time50ms_flag            @0X28:bank 0;
uchar g_time200ms_cnt            @0X29:bank 0;
uchar g_time200ms_flag           @0X2A:bank 0;
uchar g_time2s_flag              @0X2B:bank 0;
bit g_led_r                      @0X2C @0:bank 0;
bit g_led_g                      @0X2C @1:bank 0;
bit g_led_b                      @0X2C @2:bank 0;
bit g_led_onoff_status           @0X2C @3:bank 0;
bit g_led_occupied               @0X2C @7:bank 0;
    
uchar g_led_light_times          @0X2D:bank 0;
uchar g_keypress_times           @0X2E:bank 0;
uchar g_keypress_maxtime         @0X2F:bank 0;
uchar g_led_onoff                @0X30:bank 0;

uchar g_cur_state                @0X31:bank 0;
uchar g_next_state               @0X32:bank 0;

uchar g_lock_flag                @0X33:bank 0;
uchar g_adc_flag                 @0X34:bank 0;  
uchar g_time2s_start             @0X35:bank 0;

extern int IntVecIdx; //occupied 0x10:rpage 0

void delay_us(uchar count)
{
    uchar i,j; 
    
    for (i = 0; i < count; i++)
    {
        NOP(); 
        NOP(); 
        NOP(); 
        NOP();           
    }
}

//��RAM��ͨ�üĴ�������
void clr_ram(void)        
{
    RSR = 0x10;               //BANK 0 ��0x10�Ĵ���
    do
    {  
        R0 = 0;         
        RSR++;
        if (RSR == 0x40)
        {    
           RSR = 0x60;
        }
     }while(RSR < 0x80);

}

void battery_volt_sample(void)     //��ص�Դ����  
{      
    P7CR = 0x01; 
    
    ADE5 = 1;           //P71/ADC5������ΪADC5�����   
    
    ADIS2 = 1;          //ѡ��ADC5�����
    ADIS1 = 0;
    ADIS0 = 1;

    ADPD = 1;          //��ADC��Դ
        
    ADRUN = 1;  
    while(ADRUN == 1);  
    g_battery_volt_h = ADDATA1H;  
    g_battery_volt_l = ADDATA1L;  
    
    P7CR = 0x00; 
    ADE5 = 0; 
}  


void load_volt_sample(void)   //���غ͵�ص�ѹ����
{
    //�����ɼ����β��ܲ�׼��ѹ
    ADE6 = 1;           //P55/ADC6������ΪADC6�����
        
    ADIS2 = 1;          //ѡ��ADC6�����
    ADIS1 = 1;
    ADIS0 = 0; 

    ADPD = 1;          //��ADC��Դ

    ADRUN = 1;
    while(ADRUN == 1);  
      
    delay_us(3);
    
    ADRUN = 1;
    while(ADRUN == 1);  

    g_load_volt_h = ADDATA1H;
    g_load_volt_l = ADDATA1L;    
}

void _intcall ALLInt(void) @ int 
{     
    switch(IntVecIdx)
    {
        case 0x16:
            if (PWM2IF == 1)
            {
                PWM2IF = 0;                 //��PWM2�жϱ�־λ
                g_time1ms_cnt++;
                if(g_time1ms_cnt == 50)
                {
                    g_time1ms_cnt = 0;
                    g_time50ms_flag = 1;
                    g_time50ms_cnt++;
                    if(g_time50ms_cnt == 4)
                    {
                        g_time50ms_cnt = 0;
                        g_time200ms_flag = 1;

                        if(g_time2s_start==1)
                        {
                            g_time2s_start = 0;
                            g_time200ms_cnt = 0;
                            g_time2s_flag = 0;
                        }
                        else
                        {
                            g_time200ms_cnt++;
                        }
                        
                        if(g_time200ms_cnt == 10)
                        {
                            g_time200ms_cnt = 0;
                            g_time2s_flag = 1;
                        } 
                    }
                }
            }
        break;

        case 0x19:
            if (DT1IF == 1)
            {
                delay_us(10);
                if(g_keypress_maxtime < 200)
                {
                    load_volt_sample();
                }
                
                DT1IF = 0;                         //��PWM1�жϱ�־λ
                
                g_adc_flag = 1;
            }
        break;
    }     
}

void _intcall PWM2P_l(void) @0x15:low_int 6
{
    _asm{MOV A,0x2};
}

void _intcall PWM1D_l(void) @0x18:low_int 7
{
    _asm{MOV A,0x2};
}

void led_disp(void)    //LED����
{
    if(g_led_light_times >= 1) 
    {
        if(g_led_onoff_status)   
        {
            if(g_led_light_times != 0xff)
            {
                g_led_onoff_status = 0;
            }
            
            g_led_onoff = 0;          //����
        }
        else
        {
            g_led_onoff_status = 1;
            g_led_onoff = 1;         //����            
            g_led_light_times--;
        }
    }
    else
    {
        g_led_onoff = 1;             //����
    }        
        
    if(g_led_r)
    {
        P70 = g_led_onoff;           //���
    }
    else
    {
        P70 = 1;                    //û�����ƾ����
    }
            
    if(g_led_b)
    {
        P71 = g_led_onoff;          //����
    }
    else
    {
        P71 = 1;                     //û�����ƾ����    
    }
            
    if(g_led_g)
    {
        P51 = ~g_led_onoff;         //β��
    }
    else
    {
        P51 = 0;                     //û�����ƾ����
    }
}

void led_ctrl_by_voltage(ushort volt_sample)        //��ͬ��ѹ���������
{
    if(volt_sample < LOW_BAT_VOLT_TH)      
    {
        g_led_r = 1;                  //���       
        g_led_b = 0;                  //����
        g_led_g = 1;                  //β��        
    }
    else if(volt_sample < MID_BAT_VOLT_TH)    
    {
        g_led_r = 1;                  //���
        g_led_b = 1;                  //����
        g_led_g = 1;                  //β��
    }
    else
    {
        g_led_r = 0;                  //���
        g_led_b = 1;                  //���� 
        g_led_g = 1;                  //β��
    }        
    
    g_led_onoff_status = 1;           //����
    g_led_light_times = 0xff;   
}

void pwm_timer_init(void)
{
    P6CR = 0;           //P67��Ϊ���
    TMRCON = 0X27;      //PWM1Ԥ��Ƶ����Ϊ1��256,PWM2Ԥ��Ƶ����Ϊ1��16
    PWMCON = 0X01;      //ʹ��PWM1�����
    PRD1 = 155;         //����=1/4*(155+1)*256=10ms
    PRD2 = 249;         //����=1/4*(249+1)*16=1ms
    IMR = 0X30;         //ʹ��PWM1ռ�ձȣ�PWM2�����ж�
    T2EN = 1;           //PWM2��ʱ��ʼ
    ENI();  
}

void pwm_set(uchar duty)
{
    DT1 = duty;
    T1EN = 1;
}

void led_blink(uchar times)
{
    if(g_led_occupied == 0)
    {
        g_led_onoff_status = 1;
        g_led_occupied = 1;
        g_led_light_times = times;
    } 
    
    do
    {
        if(g_time200ms_flag == 1)
        {
            g_time200ms_flag = 0;
            led_disp();
        }  
    }while(g_led_light_times != 0);
    
    if(g_led_light_times == 0)
    {
        g_led_occupied = 0;
    }
}

void gpio_init()
{
    P5CR = 0X21;        //P50,P55��Ϊ���� ,P51��Ϊ���
    P5PHCR = 0XFC;      //P50,P51����
    P51 = 0;            //Ĭ�����ƹ�
    P7CR = 0;
}

void adc_init()
{   
    ADOC = 0x4;         //�ڲ��ο���ѹ3V
}

void led_status(uchar red, uchar green, uchar blue)
{
    g_led_r = red;
    g_led_g = green;
    g_led_b = blue;
}

void mcu_init(void)   //MCU��ʼ��
{

    WDTC();
    DISI();
    SCR = 0X7F;       //ѡ��4MHz,TIMER ѡ����Ƶ
    
    clr_ram();
    gpio_init();
    adc_init();

    pwm_timer_init();  
    pwm_set(MOS_OFF);
    
    g_led_occupied = 0;
    led_status(1,1,1);
    led_blink(3);

    g_keyval = P50;
    
    if(g_keyval == 0)
    {
        g_next_state = 0x01; 
    }
    else
    {
        g_next_state = 0x08;
    }
    
    g_fault_state = 0x00;
}

void main(void)
{
    uchar temp_keyval;
  
    mcu_init();
    g_time2s_flag=1;
    temp_keyval = 1;
    g_lock_flag = 0;
  
    while(1)
    {
        g_cur_state = g_next_state;

        switch(g_cur_state)
        {
            case 0x01:                                     //����ģʽ           
                if((g_keypress_maxtime > 0)&&(g_lock_flag == 0x00))
                {
                    if(g_keypress_maxtime >= 200)          //�ж����̳���10s���
                    {
                        if (g_keypress_maxtime == 200)
                        {
                            pwm_set(MOS_OFF);                           
                            led_blink(9);       //��˸8�Σ������ʼ״̬����ʱ��һ��״̬
                        }
                        
                        break;                                                   
                    } 
                    
                    g_adc_flag = 0;
                    while(g_adc_flag == 0);
                    
                    if(g_load_volt < SHORT_LOAD_VOLT)         //���������·����
                    {
                        pwm_set(MOS_OFF);
                        g_fault_state = 0x08;
                        g_next_state = 0x02;
                    }
                    else
                    {   
                        if(g_battery_volt <= VBAT37)
                        {   
                            pwm_set(MOS_ON);
                        } 
                        else if (g_battery_volt <= VBAT38)
                        {
                            pwm_set(MOS_37_38);
                        }
                        else if(g_battery_volt <= VBAT39)
                        {
                            pwm_set(MOS_38_39);
                        }
                        else if(g_battery_volt <= VBAT40)
                        {
                            pwm_set(MOS_39_40);
                        }
                        else if(g_battery_volt <= VBAT41)
                        {
                            pwm_set(MOS_40_41);
                        } 
                        else
                        {
                            pwm_set(MOS_41_42);
                        }
                    }              
                }
                else
                {                        
                    if(g_time2s_flag == 1)
                    {
                        g_next_state = 0x08;                        
                    }
                    else
                    {                  
                        g_next_state = 0x01;
                    }
                    
                    //�ͷŰ������
                    g_led_r = 0;
                    g_led_g = 0;
                    g_led_b = 0;
                }
                     
            break;
                
            case 0x02:                          //����ģʽ
                if(g_fault_state == 0x02)       //���䱣��
                {
                    led_status(0,0,1);
                    led_blink(20);
                    g_next_state = 0x8;
                }
                else if(g_fault_state == 0x04)  //��ѹ����
                {
                    led_status(1,0,0);
                    led_blink(10);
                    g_next_state = 0x8;     
                }
                else if(g_fault_state==0x08)    //����˿��·����
                {
                    led_status(1,1,1);
                    led_blink(3);
                    g_next_state = 0x8;
                }     
                else if(g_fault_state == 0x10)  //�������·����
                {
                    led_status(1,1,1);
                    led_blink(6);
                    g_next_state = 0x8;     
                }    
                else if(g_fault_state==0x20)    //���ɽ�����״̬        
                {
                    led_status(1,0,1);
                    led_blink(3);
                    g_next_state = 0x4;
                }
                else
                {
                    g_next_state = 0x01;
                } 
           
            break;
    
            case 0x04:                                 //���ģʽ 
                g_adc_flag = 0;
                while(g_adc_flag == 0);  
                if(g_load_volt < SHORT_LOAD_VOLT)   
                {
                    pwm_set(MOS_OFF);
                    g_fault_state = 0x10;
                    g_next_state = 0x02;
                }
                else if(g_load_volt > CHARGE_BAT_VOLT_TH)   
                {
                    pwm_set(MOS_OFF);
                    g_fault_state = 0x02;
                    g_next_state = 0x02;
                }
                else
                {
                    pwm_set(MOS_ON);
                    //���ʱû�нӸ��أ�ͨ����⸺�ص�ѹԼ���ڵ�ص�ѹ
                    led_ctrl_by_voltage(g_load_volt);
                }
            break;
    
            case 0x08:                                  //˯��ģʽ
                pwm_set(MOS_OFF);
                g_led_r = 0;
                g_led_g = 0;
                g_led_b = 0;
                P70 = 1;                                //����
                P71 = 1;                                //���̵�
                P51 = 0;                                //������
                ISR1 = 0X02;                            //ʹ��PORT5״̬�ı份�ѹ���
                PORT5 = PORT5;                          //��ȡPORT5״̬
                IDLE = 0;
                delay_us(2);
                SLEP();                                 //����˯��
                delay_us(20);
                
                g_time1ms_cnt = 0;
                g_time50ms_cnt = 0;
                g_time200ms_cnt = 0;
                g_led_light_times = 0;
                g_keypress_maxtime = 0;   
                
                g_adc_flag = 0;
                while(g_adc_flag == 0);
                
                if(g_load_volt < WAKEUP_LOAD_VOLT)       //�ɰ������ѣ���������״̬
                {
                    g_next_state = 0x01;
                }
                else                                             //��c_sens���ѣ����������
                {
                    g_fault_state = 0x20;
                    g_next_state = 0x02;
                    g_lock_flag = 0x0;
                }             
              break;
    
            default:
                g_next_state = 0x08;
            break;                
        }
            
        if((g_time50ms_flag == 1)||(g_cur_state == 0x08))           //key����
        {
            g_time50ms_flag = 0;
            g_keyval = P50;
            
            if(((temp_keyval == g_keyval)&&(g_keyval == 0))||(g_cur_state == 0x08))
            {
                if(g_keypress_maxtime == 255)
                {
                    g_keypress_maxtime = 255;
                }
                else
                {
                    //��������ʱ����¼����ʱ�䣬ͨ��ʱ���ж�10s����
                    g_keypress_maxtime++;
                }
                
                //�ɼ���һ�ΰ�������ʱ�ĵ�ص�ѹ������ֻ�е�ǰΪ����״̬����״̬�Ųɼ�
                if ((g_lock_flag == 0x00) && (g_keypress_maxtime == 1))
                {
                    if ((g_cur_state == 0x08)||(g_cur_state == 0x01))
                    {
                        pwm_set(MOS_ON);

                        battery_volt_sample();

                        if(g_battery_volt < LOW_BAT_VOLT)        //����ص�ѹ���� 
                        {
                            pwm_set(MOS_OFF);
                            g_fault_state = 0x04;
                            g_next_state = 0x02;
                        } 
                        else
                        {
                            led_ctrl_by_voltage(g_battery_volt);
                        }
                    }
                }
                
                //�������¾Ϳ�ʼ����2s��ʱ
                if(g_keypress_times == 0)
                {
                    g_time2s_start = 1;
                }
            }
            else if((temp_keyval == 0)&&(g_keyval == 1))
            {     
                if( g_keypress_maxtime < 40)
                {
                    g_keypress_times++;
                }
                
                //��������ʱ���ص�MOS��
                pwm_set(MOS_OFF);
                
                g_keypress_maxtime = 0;
            }
            
            temp_keyval = g_keyval;
        }
        
        if(g_time200ms_flag == 1)           //ָʾ�ƴ���
        {
            g_time200ms_flag = 0;
            led_disp();
        }   

        if(g_time2s_flag == 1)
        {
            g_time2s_flag = 0;
            if(g_keypress_times >= 5)
            {                 
                pwm_set(MOS_OFF);
                  
                if(g_lock_flag == 0x00)
                {
                    led_status(1,1,1);
                    led_blink(3);
                    g_lock_flag = 0x01;
                }
                else
                {
                    led_status(1,1,1);
                    led_blink(5);
                    g_lock_flag = 0x00;
                }
            }
            g_keypress_times  =  0;
        }  
    }
}