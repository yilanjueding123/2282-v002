#include "ap_peripheral_handling.h"
#define LED_ON              1
#define LED_OFF             0

#if C_MOTION_DETECTION == CUSTOM_ON
static INT32U md_work_memory_addr;
static INT32S motion_detect_cnt;
#endif
#if C_BATTERY_DETECT == CUSTOM_ON && USE_ADKEY_NO
static INT32U battery_value_sum;
static INT8U battery_lvl;
static INT8U bat_ck_cnt;
#endif

static INT8U led_flash_timerid;
#if USE_ADKEY_NO
//static INT8U  ad_line_select = 0;
//static INT8U ad_detect_timerid;
//static INT8U ad_18_value_cnt;
//static INT16U ad_value;
//static INT16U ad_18_value;
static KEYSTATUS ad_key_map[USE_ADKEY_NO];
//static INT16U ad_key_cnt = 0;

//static INT8U  bat_ck_timerid ;
//static INT8U  ad_value_cnt ;
#endif
static INT32U key_active_cnt;
static INT8U power_off_timerid;
static INT8U usbd_detect_io_timerid;
static INT32U led_mode;
static KEYSTATUS key_map[USE_IOKEY_NO];

static INT8U key_detect_timerid;
static INT8U 	g_led_count;
static INT8U 	g_led_r_state;	//0 = OFF;	1=ON;	2=Flicker
static INT8U	g_led_g_state;
static INT8U	g_led_flicker_state;	//0=ͬʱ��˸	1=������˸

//	prototypes
void ap_peripheral_key_init(void);
void ap_peripheral_rec_key_exe(INT16U *tick_cnt_ptr);
void ap_peripheral_capture_key_exe(INT16U *tick_cnt_ptr);
void ap_peripheral_function_key_exe(INT16U *tick_cnt_ptr);
void ap_peripheral_next_key_exe(INT16U *tick_cnt_ptr);
void ap_peripheral_prev_key_exe(INT16U *tick_cnt_ptr);
void ap_peripheral_ok_key_exe(INT16U *tick_cnt_ptr);
void ap_peripheral_usbd_plug_out_exe(INT16U *tick_cnt_ptr);
void ap_peripheral_video_key_exe(INT16U *tick_cnt_ptr);

void ap_peripheral_null_key_exe(INT16U *tick_cnt_ptr);

#if USE_ADKEY_NO
void ap_peripheral_ad_detect_init(INT8U adc_channel, void (*bat_detect_isr)(INT16U data));
void ap_peripheral_ad_check_isr(INT16U value);
#endif
static INT8U up_firmware_flag = 0;
static INT8U flash_flag = 0;
static INT8U    led_red_flag;
static INT8U    led_green_flag;
extern INT8U ap_state_handling_storage_id_get(void);
extern volatile  INT8U pic_down_flag;
extern volatile  INT8U video_down_flag;

void ap_peripheral_init(void)
{
    power_off_timerid = usbd_detect_io_timerid = led_flash_timerid = 0xFF;
    key_detect_timerid = 0xFF;


    ap_peripheral_key_init();
    LED_pin_init();
    //#if USE_ADKEY_NO
    //	ad_detect_timerid = 0xFF;
    //	ap_peripheral_ad_detect_init(AD_DETECT_PIN, ap_peripheral_ad_check_isr);
    //#else
    //adc_init();
    //#endif

}



void LED_pin_init(void)
{
    INT32U type;
    //led init as ouput pull-low
    gpio_init_io(LED1, GPIO_OUTPUT);
    gpio_init_io(LED2, GPIO_OUTPUT);
    gpio_set_port_attribute(LED1, ATTRIBUTE_HIGH);
    gpio_set_port_attribute(LED2, ATTRIBUTE_HIGH);
    gpio_write_io(LED1, DATA_LOW);
    gpio_write_io(LED2, DATA_LOW);
    led_red_flag = LED_OFF;
    led_green_flag = LED_OFF;
    type = LED_INIT;
    msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &type, sizeof(INT32U), MSG_PRI_NORMAL);
    sys_registe_timer_isr(LED_blanking_isr);	//timer base c to start adc convert
}

extern INT8U card_space_less_flag;
extern volatile INT8U Secrecy_Failure_Flag;

void set_led_mode(LED_MODE_ENUM mode)
{
    INT8U i;
#if 1
    g_led_g_state = 0;
    g_led_r_state = 0;
    g_led_flicker_state = 0;
    led_mode = mode;
    switch((INT32U)mode)
    {
    //		case LED_INIT:
    //			led_red_off();
    //			led_green_on();
    //			DBG_PRINT("led_type = LED_INIT\r\n");
    //			break;
    case LED_UPDATE_PROGRAM:
        led_red_off();
        g_led_g_state = 1;
        DBG_PRINT("led_type = LED_UPDATE_PROGRAM\r\n");
        break;
    case LED_UPDATE_FAIL:
        led_green_off();
        break;
    case LED_UPDATE_FINISH:
        led_red_off();
        led_green_on();
        DBG_PRINT("led_type = LED_UPDATE_FINISH\r\n");
        break;
    //		case LED_RECORD:
    //			 led_green_on();
    //			 led_red_on();
    //			 //g_led_r_state = 2;
    //			 g_led_flicker_state = 0;
    //			DBG_PRINT("led_type = LED_RECORD\r\n");
    //			break;
    //		case LED_SDC_FULL:
    //			sys_release_timer_isr(LED_blanking_isr);
    //			led_green_off();
    //			OSTimeDly(15);
    //		    DBG_PRINT("led_type = LED_SDC_FULL\r\n");
    //			sys_registe_timer_isr(LED_blanking_isr);
    case LED_WAITING_RECORD:
        led_red_on();
        led_green_off();

        DBG_PRINT("led_type = LED_WAITING_RECORD\r\n");
        break;
    case LED_AUDIO_RECORD:

        DBG_PRINT("led_type = LED_AUDIO_RECORD\r\n");
        break;
    case LED_WAITING_AUDIO_RECORD:

        DBG_PRINT("led_type = LED_WAITING_AUDIO_RECORD\r\n");
        break;
    case LED_CAPTURE:
        led_red_on();
        led_green_on();
        OSTimeDly(60);
        led_green_off();
		OSTimeDly(50);
        DBG_PRINT("led_type = LED_CAPTURE\r\n");
        break;
    case LED_CARD_DETE_SUC:
        if(storage_sd_upgrade_file_flag_get() == 2)
            break;
        sys_release_timer_isr(LED_blanking_isr);
        //			  led_green_off();
        //			  OSTimeDly(2);
        //			  led_green_on();
        led_red_on();
        OSTimeDly(15);
        sys_registe_timer_isr(LED_blanking_isr);
        DBG_PRINT("led_type = LED_CARD_DETE_SUC\r\n");
        break;
    case LED_CAPTURE_FAIL:
        for(i = 0; i < 2; i++)
        {
            led_all_off();
            OSTimeDly(30);
            led_red_on();
            OSTimeDly(30);
        }
    case LED_WAITING_CAPTURE:
        led_red_on();
        led_green_off();
        DBG_PRINT("led_type = LED_WAITING_CAPTURE\r\n");
        break;
    case LED_MOTION_DETECTION:

        DBG_PRINT("led_type = LED_MOTION_DETECTION\r\n");
        break;
    case LED_NO_SDC:
		led_all_off();
		g_led_r_state=2;
        DBG_PRINT("led_type = LED_NO_SDC\r\n");
        break;


    case LED_TELL_CARD:
        // sys_release_timer_isr(LED_blanking_isr);
        // led_red_on();
        // OSTimeDly(15);
        //led_red_off();
        //DBG_PRINT("led_type = LED_TELL_CARD\r\n");
        // sys_registe_timer_isr(LED_blanking_isr);
        break;
    case LED_CARD_NO_SPACE:
        //led_all_off();
        // if(storage_sd_upgrade_file_flag_get() == 2)
        // break;
        // g_led_g_state = 3;	//3��D��?������???��D?������?��?��???��?��?��������??
        // g_led_r_state = 3;
        if(storage_sd_upgrade_file_flag_get() == 2)
            break;
//        led_green_on();
//        led_red_off();
        DBG_PRINT("led_type = LED_CARD_NO_SPACE\r\n");
        break;
    case LED_CARD_TELL_FULL:
        led_green_on();
        g_led_r_state = 3;
        DBG_PRINT("led_type = LED_CARD_TELL_FULL\r\n");
        break;


    }
#endif
    //prev_mode=mode;
}
void led_red_on(void)
{
    if(led_red_flag != LED_ON)
    {
        led_red_flag = LED_ON;
        gpio_write_io(LED2, DATA_HIGH);
    }
}

void led_green_on(void)
{
    if(led_green_flag != LED_ON)
    {
        led_green_flag = LED_ON;
        gpio_write_io(LED1, DATA_HIGH);
    }
}
void led_red_off(void)
{
    if(led_red_flag != LED_OFF)
    {
        led_red_flag = LED_OFF;
        gpio_write_io(LED2, DATA_LOW);
    }
}
void led_green_off(void)
{
    if(led_green_flag != LED_OFF)
    {
        led_green_flag = LED_OFF;
        gpio_write_io(LED1, DATA_LOW);
    }
}
void led_all_off(void)
{
    led_red_off();
    led_green_off();
}

#if SECRECY_ENABLE
static INT8U SecrecyErr = 1;
static INT32U SececyTimeCnt = 0;
static INT32U SececyLedCnt = 0;
#endif

void LED_blanking_isr(void)
{
    if(g_led_count++ == 127)
    {
        g_led_count = 0;
        //DBG_PRINT("***\r\n");
    }
#if SECRECY_ENABLE
	if ((Secrecy_Failure_Flag != 0) && (SecrecyErr != 0))
	{
		//256*30 Լ60��
		led_green_off();
		if (SececyTimeCnt == 0) led_red_on();
		else if (SececyTimeCnt == 7) led_red_off();
		if (++SececyTimeCnt >= 255)
		{
			SececyTimeCnt = 0;
			if (++SececyLedCnt > 30)
			{
				SecrecyErr = 0;
				return;
			}
		}
	}
#endif

    if(g_led_g_state == 1)
    {
        if(g_led_count % 8 == 0)
        {
            if(up_firmware_flag == 1)
            {
                led_green_off();
                up_firmware_flag = 0;
            }
            else
            {
                led_green_on();
                up_firmware_flag = 1;
            }
        }
    }
    else if(g_led_g_state == 2)
    {
        if(g_led_count / 64 == g_led_flicker_state)
            led_green_on();
        else
            led_green_off();
    }
    else if(g_led_g_state == 3)
    {
        if(g_led_count % 32 == 0)
        {
            if(flash_flag == 0)
            {
                led_green_on();
                flash_flag = 1;
            }
            else
            {
                led_green_off();
                flash_flag = 0;
            }
        }
    }

    if(g_led_r_state == 2)
    {
        if(g_led_count < 64)
            led_red_on();
        else
            led_red_off();
    }
    else if(g_led_r_state == 3)
    {
        if(g_led_count % 32 == 0)
        {
            if(flash_flag == 0)
            {
                flash_flag = 1;
                led_red_on();
            }
            else
            {
                flash_flag = 0;
                led_red_off();
            }
        }
    }
    else if(g_led_r_state == 4)
    {
        if(g_led_count % 8 == 0)
        {
            if(flash_flag == 0)
            {
                flash_flag = 1;
                led_red_on();
            }
            else
            {
                flash_flag = 0;
                led_red_off();
            }
        }
    }
}

void ap_peripheral_led_flash_power_off_set(INT32U mode)
{
    if (power_off_timerid == 0xFE)
    {
        return;
    }
    if (power_off_timerid != 0xFF)
    {
        sys_kill_timer(power_off_timerid);
        power_off_timerid = 0xFF;
    }
    if (!s_usbd_pin)
    {
        set_led_mode(mode);
        if (power_off_timerid == 0xFF)
        {
            power_off_timerid = POWER_OFF_TIMER_ID;
            sys_set_timer((void *)msgQSend, (void *) ApQ, MSG_APQ_POWER_KEY_ACTIVE, power_off_timerid, (PERI_TIME_INTERVAL_POWER_OFF >> 1));
            power_off_timerid = 0xFE;
        }
    }
}


void ap_peripheral_usbd_detect_init(void)
{

}

INT8U s_usbd_plug_in = 0;
void ap_peripheral_usbd_detect_check(INT8U *debounce_cnt)
{

}

#if USE_ADKEY_NO
void ap_peripheral_ad_detect_init(INT8U adc_channel, void (*ad_detect_isr)(INT16U data))
{

}

void ap_peripheral_ad_check_isr(INT16U value)
{

}

INT32U b1, b2, b3;
void ap_peripheral_ad_key_judge(void)
{


}
#endif

void ap_peripheral_auto_power_off_set(INT8U type)
{
    if (type)
    {
        key_active_cnt = 0;
    }
    else
    {
        key_active_cnt = 0xFFFFFFFF;
    }
}



void ap_peripheral_key_init(void)
{
    INT32U i;

    gp_memset((INT8S *) &key_map, NULL, sizeof(KEYSTATUS));
    ap_peripheral_key_register(GENERAL_KEY);
    for (i = 0 ; i < USE_IOKEY_NO ; i++)
    {
        if (key_map[i].key_io)
        {
            gpio_init_io(key_map[i].key_io, GPIO_INPUT);
            gpio_set_port_attribute(key_map[i].key_io, ATTRIBUTE_LOW);
            gpio_write_io(key_map[i].key_io, (key_map[i].key_active) ^ 1);

            key_map[i].key_press_times = 0;
            key_map[i].key_up_cnt = 0;
            key_map[i].key_cnt = 0;
        }
    }

    if (key_detect_timerid == 0xFF)
    {
        key_detect_timerid = PERIPHERAL_KEY_TIMER_ID;
        sys_set_timer((void *)msgQSend, (void *) PeripheralTaskQ, MSG_PERIPHERAL_TASK_KEY_DETECT, key_detect_timerid, PERI_TIME_INTERVAL_KEY_DETECT);
    }
}

void ap_peripheral_key_register(INT8U type)
{
    INT32U i;

    if (type == GENERAL_KEY)
    {
        key_map[0].key_io = VIDEO_KEY;
        key_map[0].key_function = (KEYFUNC) ap_peripheral_video_key_exe;
        key_map[0].key_active = VIDEO_ACTIVE_KEY;
        key_map[0].key_long = 0;
		
        //key_map[1].key_io = CAPTURE_KEY;
        //key_map[1].key_function = (KEYFUNC) ap_peripheral_capture_key_exe;
        //key_map[1].key_active= CAPTURE_ACTIVE_KEY;
        //key_map[1].key_long =0;
        ad_key_map[0].key_io = FUNCTION_KEY;
        ad_key_map[0].key_function = (KEYFUNC) ap_peripheral_null_key_exe;


    }
    else if (type == USBD_DETECT)
    {
        //		key_map[0].key_io = C_USBDEVICE_PIN;
        //		key_map[0].key_function = (KEYFUNC) ap_peripheral_usbd_plug_out_exe;
#if USE_IOKEY_NO
        for (i = 0 ; i < USE_IOKEY_NO ; i++)
        {
            key_map[i].key_io = NULL;
        }
#endif

#if USE_ADKEY_NO
        for (i = 0 ; i < USE_ADKEY_NO ; i++)
        {
            ad_key_map[i].key_function = ap_peripheral_null_key_exe;
        }
#endif

#if USBD_TYPE_SW_BY_KEY == 1
    }
    else if (type == USBD_DETECT2)
    {

#if USE_IOKEY_NO
        for (i = 0 ; i < USE_IOKEY_NO ; i++)
        {
            if ((key_map[i].key_io != CAPTURE_ACTIVE_KEY)
#ifdef PW_KEY
                    && (key_map[i].key_io != PW_KEY)
#endif
               )
            {
                key_map[i].key_io = NULL;
            }
        }
#endif

#if USE_ADKEY_NO
        for (i = 0 ; i < USE_ADKEY_NO ; i++)
        {
            ad_key_map[i].key_function = ap_peripheral_null_key_exe;
        }
#endif
#endif
    }
    else if (type == DISABLE_KEY)
    {
        for (i = 0 ; i < USE_IOKEY_NO ; i++)
        {
            key_map[i].key_io = NULL;
        }
    }
}

#define SA_TIME	60	//seconds, for screen saver time. Temporary use "define" before set in "STATE_SETTING".
void ap_peripheral_key_judge(void)
{
    INT32U i;
    //INT32U cnt_sec;
    INT16U key_down = 0;

    for (i = 0 ; i < USE_IOKEY_NO ; i++)
    {
        if(key_map[i].key_io)
        {

            if(key_map[i].key_active)
                key_down = gpio_read_io(key_map[i].key_io);
            else
                key_down =  !gpio_read_io(key_map[i].key_io);
            if(key_down)
            {
                //DBG_PRINT("key%d=1\r\n",i);
                if(!key_map[i].key_long)
                {
                    key_map[i].key_cnt += 1;
#if SUPPORT_LONGKEY == CUSTOM_ON
                    if (key_map[i].key_cnt >= Long_Single_width)
                    {
                        key_map[i].key_long = 1;
                        key_map[i].key_function(&(key_map[i].key_cnt));
                    }
#endif
                }
                else
                {
                    key_map[i].key_cnt = 0;
                }
                if (key_map[i].key_cnt == 65535)
                {
                    key_map[i].key_cnt = 16;
                }
            }
            else
            {
                if(key_map[i].key_long)
                {
                    key_map[i].key_long = 0;
                    key_map[i].key_cnt = 0;
                }
                if(key_map[i].key_cnt >= Short_Single_width) //Short_Single_width
                {

                    key_map[i].key_function(&(key_map[i].key_cnt));

                }
                key_map[i].key_cnt = 0;

            }
        }
    }

}

void ap_peripheral_video_key_exe(INT16U *tick_cnt_ptr)
{
    INT32U led_type;

#if SECRECY_ENABLE
	if ((Secrecy_Failure_Flag != 0) && (SecrecyErr != 0)) 
	{
		__msg("Secrecy_Failure_Flag = %d\n", Secrecy_Failure_Flag);
		return;
	}
#endif
	
    if(ap_state_handling_storage_id_get() == NO_STORAGE)
    {

        led_type = LED_TELL_CARD;
        msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
    }

    if ((!s_usbd_pin) && (!pic_down_flag) && (!card_space_less_flag) && (!video_down_flag))
    {
        if(ap_state_handling_storage_id_get() != NO_STORAGE)
        {
            //	#if SUPPORT_LONGKEY == CUSTOM_ON
            if(*tick_cnt_ptr >= Long_Single_width)
            {
                DBG_PRINT("MSG_APQ_VIDEO_RECORD_ACTIVE\n");
                msgQSend(ApQ, MSG_APQ_VIDEO_RECORD_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
            }
            else
                // #endif
            {
                DBG_PRINT("[VIDEO_RECORD_...]\r\n");
                msgQSend(ApQ, MSG_APQ_CAPTUER_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
                // msgQSend(ApQ, MSG_APQ_VIDEO_RECORD_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
            }
        }
    }
    else if(card_space_less_flag)
    {
        //DBG_PRINT("AA\r\n");
        led_type = LED_CARD_TELL_FULL;
        msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);

    }
    *tick_cnt_ptr = 0;
}

void ap_peripheral_capture_key_exe(INT16U *tick_cnt_ptr)
{
    INT32U led_type;
#if SECRECY_ENABLE
	if ((Secrecy_Failure_Flag != 0) && (SecrecyErr != 0)) return;
#endif

    if(ap_state_handling_storage_id_get() == NO_STORAGE)
    {

        led_type = LED_TELL_CARD;
        msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &led_type, sizeof(INT32U), MSG_PRI_NORMAL);
    }

    if (!s_usbd_pin)
    {
#if SUPPORT_LONGKEY == CUSTOM_ON
        if(*tick_cnt_ptr >= Long_Single_width)
        {
        }
        else
#endif
        {
            DBG_PRINT("[CAPTUER_ACTIVE...]\r\n");
            msgQSend(ApQ, MSG_APQ_CAPTUER_ACTIVE, NULL, NULL, MSG_PRI_NORMAL);
        }
    }
    else
    {

    }
    *tick_cnt_ptr = 0;

}

//============================================================================================

void ap_peripheral_rec_key_exe(INT16U *tick_cnt_ptr)
{

    *tick_cnt_ptr = 0;
}


void ap_peripheral_next_key_exe(INT16U *tick_cnt_ptr)
{
    *tick_cnt_ptr = 0;
}

void ap_peripheral_prev_key_exe(INT16U *tick_cnt_ptr)
{
    *tick_cnt_ptr = 0;
}


void ap_peripheral_function_key_exe(INT16U *tick_cnt_ptr)
{

    *tick_cnt_ptr = 0;
}

void ap_peripheral_ok_key_exe(INT16U *tick_cnt_ptr)
{

}

void ap_peripheral_usbd_plug_out_exe(INT16U *tick_cnt_ptr)
{
    *tick_cnt_ptr = 0;
}

void ap_peripheral_menu_key_exe(INT16U *tick_cnt_ptr)
{
    *tick_cnt_ptr = 0;
}

void ap_peripheral_null_key_exe(INT16U *tick_cnt_ptr)
{

}
