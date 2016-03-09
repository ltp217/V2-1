#include "EM78P301N.h"

#define DISI()                _asm{disi}
#define WDTC()                _asm{wdtc}
#define NOP()                 _asm{nop}
#define ENI()                 _asm{eni}
#define SLEP()                _asm{slep}
#define LOW_BAT_VOLT_TH       0x955                                 //(3.5V/2)/3V*4096=2389
#define MID_BAT_VOLT_TH       0xA22                                 //(3.8V/2)/3V*4096=2594
#define LOW_BAT_VOLT          0x888                                 //(3.2V/2)/3V*4096=2184
#define LOW_LOAD_VOLT         0x888                                 //(3.2V/2)/3V*4096=2184
#define SHORT_LOAD_VOLT       0x400                                 //(1.5V/2)/3V*4096=1024
#define CHARGE_BAT_VOLT_TH    0xB11                                 //(4.15V/2)/3V*4096=2833
#define WAKEUP_LOAD_VOLT      0x155                                 //(0.5V/2)/3V*4096=341
#define MOS_ON                0                                     //��MOS
#define MOS_OFF               100                                   //�ر�MOS
#define VBAT37                0x9DD                                 //(3.7V/2)/3V*4096=2525  --100
#define VBAT38                0xA22                                 //(3.8V/2)/3V*4096=2594  --94
#define VBAT39                0xA66                                 //(3.9V/2)/3V*4096=2662  --90
#define VBAT40                0xAAB                                 //(4.0V/2)/3V*4096=2731  --85
#define VBAT41                0xAEF                                 //(4.1V/2)/3V*4096=2799  --81
#define VBAT42                0xB33                                 //(4.2V/2)/3V*4096=2867  --77
#define MOS_37_38             97                           
#define MOS_38_39             92                            
#define MOS_39_40             88                            
#define MOS_40_41             83                            
#define MOS_41_42             79                            

#define uchar unsigned char 
#define ushort unsigned short 
#define uint unsigned int 

uchar g_fault_state              @0X20:bank 0;  //����״̬��
ushort g_battery_load_volt       @0X21:bank 0;
uchar g_battery_load_volt_l      @0X21:bank 0;
uchar g_battery_load_volt_h      @0X22:bank 0;
uchar g_keyval                   @0X25:bank 0;
uchar g_time1ms_cnt              @0X26:bank 0;
uchar g_time50ms_cnt             @0X27:bank 0;
uchar g_time50ms_flag            @0X28:bank 0;
uchar g_time1s_cnt               @0X29:bank 0;
uchar g_time1s_flag              @0X2A:bank 0;
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

uchar g_cur_state          @0X31:bank 0;
uchar g_next_state         @0X32:bank 0;

extern int IntVecIdx; //occupied 0x10:rpage 0

//��RAM��ͨ�üĴ�������
void Clr_Ram(void)        
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

void _intcall ALLInt(void) @ int 
{     
    switch(IntVecIdx)
    {
        case 0x16:
        if(PWM2IF == 1)
        {
            PWM2IF = 0;    //��PWM2�жϱ�־λ
            g_time1ms_cnt++;
            if(g_time1ms_cnt == 50)
            {
                g_time1ms_cnt = 0;
                g_time50ms_flag = 1;
                g_time50ms_cnt++;
                if(g_time50ms_cnt == 20)
                {
                    g_time50ms_cnt = 0;
                    g_time1s_flag = 1;

                    if(g_time2s_flag==0)
                    {
                        g_time1s_cnt++;
                    }

                    if(g_time1s_cnt == 2)
                    {
                        g_time1s_cnt = 0;
                        g_time2s_flag = 1;
                    } 
                }
            }
        }
        break;
    }     
}

void _intcall PWM2P_l(void) @0x15:low_int 6
{
    _asm{MOV A,0x2};
}

void battery_load_volt_sample(void)   //���غ͵�ص�ѹ����
{
    P5CR = 0x21;
    ADE6 = 1;           //P55/ADC6������ΪADC6����� 

    ADIS2 = 1;          //ѡ��ADC6�����
    ADIS1 = 1;
    ADIS0 = 0;    
    
    ADPD = 1;          //��ADC��Դ
   
    ADRUN = 1;
    while(ADRUN == 1);   
    g_battery_load_volt_h = ADDATA1H;
    g_battery_load_volt_l = ADDATA1L;
    NOP();
    NOP();
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
        P51 = ~g_led_onoff;         //�̵�
    }
    else
    {
        P51 = 0;                     //û�����ƾ����
    }
}

void led_ctrl_by_voltage(void)        //��ͬ��ѹ���������
{
     if(g_battery_load_volt < LOW_BAT_VOLT_TH)      
    {
        g_led_r = 1;                  //���   
        g_led_g = 0;                  //�̵�    
        g_led_b = 0;                  //����        
    }
    else if(g_battery_load_volt < MID_BAT_VOLT_TH)    
    {
        g_led_r = 1;                  //���
        g_led_g = 0;                  //�̵�
        g_led_b = 1;                  //����
    }
    else
    {
        g_led_r = 0;                  //��� 
        g_led_g = 0;                  //�̵�
        g_led_b = 1;                  //����
    }        
    
    g_led_onoff_status = 1;           //����
    g_led_light_times = 0xff;   
}

void pwm_set(uchar duty)
{
    P6CR = 0;           //P67��Ϊ���
    PWMCON = 0X01;      //ʹ��PWM1�����
    PRD1 = 99;          //����=1/4*(99+1)*4=100us
    DT1 = duty; 
    T1EN = 1;
}

void pwm_timer_init(void)
{
    TMRCON = 0X22;      //PWM1Ԥ��Ƶ����Ϊ1��4,PWM2Ԥ��Ƶ����Ϊ1��16
    PRD2 = 249;         //����=1/4*(249+1)*16=1ms
    IMR = 0X10;         //ʹ��PWM2�ж�
    T2EN = 1;           //PWM2��ʱ��ʼ
    ENI();  
}

void led_init()
{
    P7CR = 0x00;
    AISR = 0x00;
    g_led_r = 1;                          //���
    g_led_g = 1;                          //���
    g_led_b = 1;                          //����
    g_led_onoff_status = 1;
    g_led_occupied = 0;
    g_led_light_times = 0x3;
    
    do
    {
        if(g_time1s_flag == 1)
        {
            g_time1s_flag = 0;
            led_disp();
        }  
    }while(g_led_light_times != 0);
}

void gpio_init()
{
    P5CR = 0X21;        //P50,P55��Ϊ���� ,P51��Ϊ���
    P5PHCR = 0XFC;      //P50,P51����
    P51 = 0;            //Ĭ�����ƹ�
}

void adc_init()
{
    ADOC = 0x4;         //�ڲ��ο���ѹ3V
}

void fault_detect_protect(uchar red, uchar green, uchar blue, uchar times, uchar next_status)
{
    if(g_led_occupied == 0)
    {
        g_led_r = red;
        g_led_g = green;
        g_led_b = blue;
        g_led_onoff_status = 1;
        g_led_occupied = 1;
        g_led_light_times = times;
    } 
    
    if(g_led_light_times == 0)
    {
        g_led_occupied = 0;
        g_next_state = next_status;
    }
}

void mcu_init(void)   //MCU��ʼ��
{

    WDTC();
    DISI();
    SCR = 0X7F;         //ѡ��4MHz,TIMER ѡ����Ƶ
    
    Clr_Ram();
    pwm_timer_init();  
    pwm_set(MOS_OFF);
    gpio_init();
    adc_init();
    led_init();

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
  
    while(1)
    {
        g_cur_state = g_next_state;

        switch(g_cur_state)
        {
            case 0x01:                                 //����ģʽ
                if(g_keypress_maxtime > 0)
                {
                    if(g_keypress_maxtime >= 200)       //�ж����̳���10s���
                    {
                        pwm_set(MOS_OFF);
                        if(g_led_occupied == 0)        //�ж�LED�Ƿ�ռ��
                        {
                            g_led_occupied = 1;        //ռ�õ�
                            g_led_light_times = 0x8;   //��˸8��
                        }  
                       
                        if(g_led_light_times == 0)     //���ռ��
                        {
                            g_led_occupied = 0;
                        }
                    
                        break;   
                    } 
                    
                    pwm_set(MOS_ON);
                    
                    battery_load_volt_sample();

                    if(g_battery_load_volt < SHORT_LOAD_VOLT)         //���������·����
                    {
                        pwm_set(MOS_OFF);
                        g_led_occupied = 0;
                        g_fault_state = 0x08;
                        g_next_state = 0x02;
                    }
                    else if(g_battery_load_volt < LOW_BAT_VOLT)        //����ص�ѹ���� 
                    {
                        pwm_set(MOS_OFF);
                        g_led_occupied = 0;
                        g_fault_state = 0x04;
                        g_next_state = 0x02;
                    } 
                    else
                    {
                        if (g_led_light_times == 0)
                        {
                            led_ctrl_by_voltage();
                        }
                        
                        if(g_battery_load_volt <= VBAT37)
                        {
                            pwm_set(MOS_ON);
                        } 
                        else if (g_battery_load_volt <= VBAT38)
                        {
                            pwm_set(MOS_37_38);
                        }
                        else if(g_battery_load_volt <= VBAT39)
                        {
                            pwm_set(MOS_38_39);
                        }
                        else if(g_battery_load_volt <= VBAT40)
                        {
                            pwm_set(MOS_39_40);
                        }
                        else if(g_battery_load_volt <= VBAT41)
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
                    g_led_occupied = 0;
                    g_next_state = 0x08;
                }
                     
            break;
                
            case 0x02:                        //����ģʽ
                if(g_fault_state == 0x02)       //���䱣��
                {
                    fault_detect_protect(0,0,1,0x14,0x8);
                }
                else if(g_fault_state == 0x04)  //��ѹ����
                {
                    fault_detect_protect(1,0,0,0xa,0x8);     
                }
                else if(g_fault_state==0x08)    //����˿��·����
                {
                    fault_detect_protect(1,1,1,0x3,0x8);
                }     
                else if(g_fault_state == 0x10)    //�������·����
                {
                    fault_detect_protect(1,1,1,0x6,0x8);     
                }    
                else if(g_fault_state==0x20)    //���ɽ�����״̬        
                {
                    fault_detect_protect(1,0,1,0x3,0x4);
                }
                else
                {
                    g_led_occupied = 0;
                    g_next_state = 0x01;
                } 
           
            break;
    
            case 0x04:                                 //���ģʽ   
                if(g_battery_load_volt < SHORT_LOAD_VOLT)   
                {
                    pwm_set(MOS_OFF);
                    g_fault_state = 0x10;
                    g_next_state = 0x02;
                }
                else if(g_battery_load_volt > CHARGE_BAT_VOLT_TH)   
                {
                    pwm_set(MOS_OFF);
                    g_fault_state = 0x02;
                    g_next_state = 0x02;
                }
                else
                {
                    pwm_set(MOS_ON);
                    battery_load_volt_sample();
                    led_ctrl_by_voltage();
                }
            break;
    
            case 0x08:                                  //˯��ģʽ
                pwm_set(MOS_OFF);
                P5CR = 0X01;                            //P50��Ϊ���� ,P51,P55��Ϊ���
                AISR = 0X00;                            //P55��ΪIO�˿�
                P5PDCR = 0xDF;                          //P55����
                P55 = 0;                                //P55����
                P70 = 1;                                //����
                P71 = 1;                                //���̵�
                P51 = 0;                                //������
                ISR1 = 0X02;                            //ʹ��PORT5״̬�ı份�ѹ���
                PORT5 = PORT5;                          //��ȡPORT5״̬
                IDLE = 0;
                NOP();
                NOP();
                SLEP();                                 //����˯��
                NOP();
                NOP();

                g_keypress_maxtime = 0;
                
                battery_load_volt_sample();             
                
                if(g_battery_load_volt < WAKEUP_LOAD_VOLT)       //�ɰ������ѣ���������״̬
                {
                    g_next_state = 0x01;
                }
                else                                   //��c_sens���ѣ����������
                {
                    g_fault_state = 0x20;
                    g_next_state = 0x02;
                }
              
              break;
    
            default:
                g_next_state = 0x00;
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
                        g_keypress_maxtime++;
                    }
            }
            else if((temp_keyval == 0)&&(g_keyval == 1))
            {
                if(g_keypress_times == 0)
                {
                    g_time2s_flag = 0;
                }
                  
                if( g_keypress_maxtime < 40)
                {
                    g_keypress_times++;
                }
                  
                g_keypress_maxtime = 0;
  
            }
            
            temp_keyval = g_keyval;
        }
        
        if(g_time1s_flag == 1)           //ָʾ�ƴ���
        {
            g_time1s_flag = 0;
            led_disp();
        }   

        if(g_time2s_flag == 1)
        {
            if(g_keypress_times >= 5)
            { 
                g_keypress_times = 0;
                 
                pwm_set(MOS_OFF);
                  
                if(g_cur_state == 0x00)
                {
                    g_next_state = 0x01;
                }
                else
                {
                    g_next_state = 0x00;
                }
            }
            g_keypress_times  =  0;
        }  
    }
}