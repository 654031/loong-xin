/*
    1�����Ź�
    ��д�˿��Ź���غ����������˳�ʼ�����ʱ��Ż��Զ���λ��

    2������
    ��1������1��ӡ���С������˴���1�����жϡ�
    ��2����������ʱ�ų��� \r\n ��
    ��3����д����1���ͺ��� USART_SendData() ��
    ����
    ��1����Ϊ�ڲ� 8M ʱ�Ӳ���ȷ������ͨ�ŵĲ�����������3%�ᵼ�����룬����ֻ��ʹ���ⲿ 8M ʱ�ӽ��д���ͨ�š�
    ��2������1�жϿ��У�����ÿ���ж����ֻ�ܽ���16���ֽڡ�����16���ֽں���յ������ݻ������ֽ����ܶ�ᵼ��ֻ���յ���һ���ֽڡ�

    3�� I2C ���� OLED
    ��1��һ��Ҫ����Ӳ�� I2C �� GPIO ���ţ�����д CR_SR �Ĵ������ٶ�ȡ CR_SR �Ĵ�����ס������
*/
* @Author: xuyang
 * @Date: 2024-05-26 22:24:39
 * @LastEditors: yueguangyunhai
 * @LastEditTime: 2024-05-31 23:56:39
 * @FilePath: \8266_task_xuy_kenbio\mqtt_8266_big_task_esp32c3\mqtt_8266_big_task_esp32c3.ino
 * @Description:
 *
 * Copyright (c) 2024 by xuyang, All Rights Reserved
 */
// TODO һ�ݺõĴ���ע�ͣ�����������д�ܶ���ĵ�����Ϊ���뱾�������õ��ĵ�
// TODO �иж������õĴ����������ص㣺1�������ӳٵͿ�bug�٣��������������
// TODO  2���ɶ��Ը߼�����ˣ�����OOP��� 3���ֹ���ȷ��û����Ч���壬��������֪�⣬�����������

#include "Xu_schedule.h"
#include "Dai_tone.h"

#include "SignalProcessing.h"

/* -------------------------------------------------------------------------- */
/*                                   ��ƿƼ�����                                   */
/* -------------------------------------------------------------------------- */
#define BLINKER_PRINT Serial
#define BLINKER_WIFI

#include <stdio.h>
#include <stdbool.h>

#include "bsp.h"
#include "console.h"
#include "ls1c102.h"

#include "src/GPIO/user_gpio.h"
#include "src/WDG/ls1x_wdg.h"
#include "src/I2C/ls1c102_i2c.h"
#include "src/OLED/oled96.h"


char auth[] = "92ee9b0c302e";
char ssid[] = "kenbio";
char pswd[] = "123456xuy";

extern HW_PMU_t *g_pmu;

BlinkerButton Button1("btn-abc");
BlinkerNumber Number1("num-abc");

/* -------------------------------------------------------------------------- */
/*                                   �������������                                  */
/* -------------------------------------------------------------------------- */
Scheduler scheduler;
uint8_t mode_flag = 0;
uint8_t tone_flag = 0;
uint8_t blinker_flag = 0;
/* -------------------------------------------------------------------------- */
/*                                   �û�д�ĺ���                                   */
/* -------------------------------------------------------------------------- */
// ���� RGB LED ����
const int redPin = 33;
const int greenPin = 25;
const int bluePin = 26;

enum
{
    SHANG_CHUN_SHAN = 0,
    DA_YU,
    YUAN_YU_CHOU,
    TONE_STOP
};

// ��ɫ�仯����
void setColor(int redValue, int greenValue, int blueValue)
{
    analogWrite(redPin, redValue);
    analogWrite(greenPin, greenValue);
    analogWrite(bluePin, blueValue);
}

// �Ų�Ч������
void rainbowEffect()
{
    static uint8_t hue = 0;
    hue++;
    uint8_t redValue = sin(hue * 0.1) * 127 + 128;
    uint8_t greenValue = sin(hue * 0.1 + 2) * 127 + 128;
    uint8_t blueValue = sin(hue * 0.1 + 4) * 127 + 128;
    setColor(redValue, greenValue, blueValue);
}

void rgb_light_task()
{
    if (mode_flag == FREE)
    {
        switch (tone_flag)
        {
        case SHANG_CHUN_SHAN:
            setColor(255, 0, 0); // ��ɫ
            break;
        case DA_YU:
            setColor(0, 200, 0); // ��ɫ
            break;
        case YUAN_YU_CHOU:
            setColor(0, 0, 100); // ��ɫ
            break;
        case TONE_STOP:
            setColor(0, 0, 0); // �ر�
            break;
        }
    }
    else if (mode_flag != FREE)
    {
        rainbowEffect(); // �Ų�Ч��
    }
}

void microsound_func()
{
    int micValue = 0;
    switch (mode_flag)
    {
    case MICROSOUND:
        micValue = analogRead(MIC_PIN); // ��ȡ��˷��ģ��ֵ
        processSignal(micValue);        // �����źŲ���ʾ
        break;
    case FREE:
        break;
    }
}
/* -------------------------------- �����������Ĳ��� ------------------------------- */
void button1_short_press_func()
{

    if (tone_flag == SHANG_CHUN_SHAN)
    {
        tone_flag = DA_YU;
    }
    else if (tone_flag == DA_YU)
    {
        tone_flag = YUAN_YU_CHOU;
    }
    else if (tone_flag == YUAN_YU_CHOU)
    {
        tone_flag = SHANG_CHUN_SHAN;
    }
}

void button2_short_press_func()
{

    static uint8_t flag = 0;
    if (tone_flag != TONE_STOP)
    {
        flag = tone_flag;
        tone_flag = TONE_STOP;
    }
    else if (tone_flag == TONE_STOP)
    {
        tone_flag = flag;
    }
}
void button1_long_press_func()
{

    mode_flag++;
    mode_flag = mode_flag % 2;
}

void button2_long_press_func()
{

    blinker_flag++;
    blinker_flag = blinker_flag % 2;
}

/* ---------------------- led_blink1 �� led_blink2 �������֣�������� ---------------------- */
static inline void led_blink1()
{
    digitalWrite(pin_led_01, !digitalRead(pin_led_01));
}

static inline void led_blink2()
{
    digitalWrite(pin_led_02, !digitalRead(pin_led_02));
}

/* -------------------------------------------------------------------------- */
/*                             MQTT����ƿƼ��İ����ص���������                             */
/* -------------------------------------------------------------------------- */
void button1_callback(const String &state)
{
    BLINKER_LOG("get button state: ", state);
    if (tone_flag == SHANG_CHUN_SHAN)
    {
        tone_flag = DA_YU;
    }
    else if (tone_flag == DA_YU)
    {
        tone_flag = YUAN_YU_CHOU;
    }
    else if (tone_flag == YUAN_YU_CHOU)
    {
        tone_flag = SHANG_CHUN_SHAN;
    }
}

// ���δ�󶨵���������������ִ����������
void dataRead(const String &data)
{
    static int counter = 0;
    BLINKER_LOG("Blinker readString: ", data);
    counter++;
    Number1.print(counter);
}


void tone_task()
{
    switch (tone_flag)
    {
    case SHANG_CHUN_SHAN:
        tone_shang_chun_shan();
        break;
    case DA_YU:
        tone_da_yu();
        break;
    case YUAN_YU_CHOU:
        tone_yuan_yu_chou();
        break;
    case TONE_STOP:
        noTone(tonepin);
        break;
    }
}


/* -------------------------------------------------------------------------- */
/*                               null��main��������                               */
/* -------------------------------------------------------------------------- */
void null()
{
    /* ----------------------------------- ��ʼ�� ---------------------------------- */
    xu_signal_init();
    Dai_tone_init();
    scheduler.init();
    /* -------------------------------- ��Ӱ����Ļص����� ------------------------------- */
    scheduler.add_key_event_handler(0, button1_short_press_func);
    scheduler.add_key_event_long_press_handler(0, button1_long_press_func);
    scheduler.add_key_event_handler(1, button2_short_press_func);
    scheduler.add_key_event_long_press_handler(1, button2_long_press_func);

    // ����û�������ͳ�ʼ��

    mode_flag = FREE;
    tone_flag = DA_YU;

    scheduler.add_task(led_blink1, 200);
    scheduler.add_task(tone_task, 10);
    scheduler.add_task(microsound_func, 1);
    scheduler.add_task(uart_task, 100);
    scheduler.add_task(led_blink2, 100);
    scheduler.add_task(rgb_light_task, 10);


    //     // ���� PWM Ƶ�ʺ�ͨ��
    // ledcSetup(0, 5000, 8); // ͨ�� 0, 5kHz, 8-bit �ֱ���
    // ledcSetup(1, 5000, 8);
    // ledcSetup(2, 5000, 8);

    //   // ��ʼ����ʱ���ж�
    //     hw_timer_t *timer = timerBegin(0, 80, true); // 80 ��Ƶ��APB ʱ��Ϊ 80MHz
    //     timerAttachInterrupt(timer, &taskScheduler, true);
    //     timerAlarmWrite(timer, taskInterval * 1000, true); // ÿ taskInterval ���봥��һ���ж�
    //     timerAlarmEnable(timer);

    // scheduler.add_task([](){ Blinker.run(); }, 700);
}


int main(void)
{
    WdgInit();
    printk("main() function\r\n");
    gpio_init(20, 1);// GPIO20 ʹ�����
    gpio_init(13, 1);
    gpio_write(13, 1);
    delay_ms(100);
    gpio_write(13, 0);
    delay_ms(100);

    gpio_iosel(4, 1);
    gpio_iosel(5, 1);
    // ���븴��Ӳ�� I2C �� GPIO ���ţ�����д CR_SR �Ĵ������ٶ�ȡ CR_SR �Ĵ����Ῠס
    
    I2C_InitTypeDef I2C_InitStruct0;
    I2C_StructInit(&I2C_InitStruct0);
    
    I2C_Init(&I2C_InitStruct0);
    delay_ms(100);
    OLED_Init();// ��ʼ�� OLED ģ��
    
    OLED_Clear();// OLED ���������ǽ�������Ļ����ɫ��
    //OLED_Full();
    //OLED_Clear();
    
    
    scheduler.run();

    // switch (mode_flag)
    // {
    // case 0:
    //     tone_shang_chun_shan();
    //     break;
    // case 1:
    //     tone_da_yu();
    //     break;
    // case 2:
    //     tone_yuan_yu_chou();
    //     break;
    // case 3:
    //     int micValue = analogRead(MIC_PIN); // ��ȡ��˷��ģ��ֵ
    //     processSignal(micValue);            // �����źŲ���ʾ
    //     break;
    
    
    
    /* OLED_ShowChar() test */
    //OLED_ShowChar(0, 0, 'a');
    //OLED_ShowChar(8, 0, 'b');
    //OLED_ShowChar(0, 2, 'c');
    //OLED_ShowChar(0, 4, 'd');
    //OLED_ShowChar(12, 0, 'e');
    //OLED_Clear();
    //OLED_ShowChar(121, 0, 'f');
    //OLED_ShowChar(8, 2, 'g');
    //OLED_ShowChar(16, 0, 'h');
    //OLED_ShowChar(24, 2, 'i');
    /* OLED_ShowChar() test */
    
    
    /* OLED_ShowNum() test */
    //OLED_ShowNum(0, 0, 123, 3, 16);
    //OLED_ShowNum(0, 2, 123456, 16, 16);
    //OLED_ShowNum(0, 4, 123456, 20, 16);
    /* OLED_ShowNum() test */
    
    
    
    /* OLED_ShowString() test */
    //OLED_ShowString(0, 0, "ab");
    //char *str0 = "12";
    //char str1[] = "34";
    //OLED_ShowString(0, 2, str0);
    //OLED_ShowString(16, 2, str0);
    //OLED_ShowString(104, 4, "YunXiao");
    /* OLED_ShowString() test */
    
    

    /* OLED_ShowCHinese() test */
    //OLED_ShowCHinese(0, 4, 0);
    //OLED_ShowCHinese(16, 4, 1);
    //OLED_ShowCHinese(32, 4, 2);
    //OLED_ShowCHinese(48, 4, 3);
    //OLED_Clear();
    /* OLED_ShowCHinese() test */
    
    for (;;)
    {
        //register unsigned int ticks;
        //ticks = get_clock_ticks();
        //printk("tick count = %u\r\n", ticks);
        
        //gpio_write(20, 1);// GPIO20 ����ߵ�ƽ
        //delay_ms(100);
        //gpio_write(20, 0);// GPIO20 ����͵�ƽ
        //delay_ms(100);

        /**/
            //OLED_Full();
            OLED_ShowCHinese(0, 4, 0);
            //delay_ms(100);
            OLED_ShowCHinese(16, 4, 1);
            OLED_ShowCHinese(32, 4, 2);
            OLED_ShowCHinese(48, 4, 3);
            OLED_Clear();
           
            //OLED_ShowCHinese(64, 4, 0);
            //OLED_ShowCHinese(80, 4, 1);
            //OLED_ShowCHinese(96, 4, 2);
            //OLED_ShowCHinese(112, 4, 3);
            //OLED_Clear();
            
            //OLED_ShowCHinese(64, 2, 0);
            //OLED_ShowCHinese(80, 2, 1);
            //OLED_ShowCHinese(96, 2, 2);
            //OLED_ShowCHinese(112, 2, 3);
            //OLED_Clear();
            
            //OLED_ShowCHinese(0, 2, 0);
            //OLED_ShowCHinese(16, 2, 1);
            //OLED_ShowCHinese(32, 2, 2);
            //OLED_ShowCHinese(48, 2, 3);
            //OLED_Clear();
        /*
/*
 

// �½��������



        
        
    }

    /*
     * Never goto here!
     */
    return 0;
}

/*
 * @@ End
 */
