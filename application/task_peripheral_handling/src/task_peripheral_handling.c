#include "task_peripheral_handling.h"

MSG_Q_ID PeripheralTaskQ;
void *peripheral_task_q_stack[PERIPHERAL_TASK_QUEUE_MAX];
static INT8U peripheral_para[PERIPHERAL_TASK_QUEUE_MAX_MSG_LEN];
APP_STATES app_states_flg = APP_STATES_INIT;
static INT32S  no_storage_states_count = 0;
INT32S capture_continues_shoot_flag = 0;


//	prototypes
void task_peripheral_handling_init(void);

void task_peripheral_handling_init(void)
{
    PeripheralTaskQ = msgQCreate(PERIPHERAL_TASK_QUEUE_MAX, PERIPHERAL_TASK_QUEUE_MAX, PERIPHERAL_TASK_QUEUE_MAX_MSG_LEN);
    ap_peripheral_init();
    msgQSend(ApQ, MSG_APQ_PERIPHERAL_TASK_READY, NULL, NULL, MSG_PRI_NORMAL);
}
INT8U card_space_less_flag = 0;
void task_peripheral_handling_entry(void *para)
{
    INT32U msg_id;

    INT32U type;
    INT8U usbd_debounce_cnt;

    INT32U bat_ck_timerid;
    //	INT8U  bat_check;

    bat_ck_timerid = 0xFF;
    //	bat_check=0;
    usbd_debounce_cnt = 0;
    task_peripheral_handling_init();
    while(1)
    {
        if(msgQReceive(PeripheralTaskQ, &msg_id, peripheral_para, PERIPHERAL_TASK_QUEUE_MAX_MSG_LEN) == STATUS_FAIL)
        {
            continue;
        }
        switch (msg_id)
        {
        case MSG_PERIPHERAL_TASK_KEY_DETECT:
            ap_peripheral_key_judge();
            /* add begin by Zhibo, 2017-05-05 */
            ap_no_storage_states_set_led(app_states_flg);
            /* add end by Zhibo, 2017-05-05 */

            break;
        case MSG_PERIPHERAL_TASK_KEY_REGISTER:
            ap_peripheral_key_register(peripheral_para[0]);
            break;
        case MSG_PERIPHERAL_TASK_USBD_DETECT_INIT:
            ap_peripheral_usbd_detect_init();
            break;
        case MSG_PERIPHERAL_TASK_USBD_DETECT_CHECK:
            //ap_peripheral_usbd_detect_check(&usbd_debounce_cnt);
            break;
        case MSG_PERIPHERAL_TASK_LED_SET:
            __msg(" peripheral_para[0] = %d\n",  peripheral_para[0]);
            /* add begin by Zhibo, 2017-05-05 */
            no_storage_states_count = 0;
            capture_continues_shoot_flag = 0;
            /*if(LED_NO_SDC == peripheral_para[0])
            {
                app_states_flg = APP_STATES_NO_SD;
            }
            else */if(LED_RECORD == peripheral_para[0])
            {
                app_states_flg = APP_STATES_RECORD;
            }
            else if(LED_CAPTURE == peripheral_para[0])
            {
                app_states_flg = APP_STATES_CAPTURE;
            }
            else if(LED_CARD_DETE_SUC == peripheral_para[0])
            {
                app_states_flg = APP_STATES_CARD_DETE_SUC;
            }
            else if(LED_INIT == peripheral_para[0])
            {
                app_states_flg = APP_STATES_INIT_LED;
            }
            else if(LED_CARD_NO_SPACE == peripheral_para[0])
            {
                app_states_flg = APP_STATES_CARD_FULL;
            }
            else
            {
                app_states_flg = APP_STATES_INIT;
            }
            /* add end by Zhibo, 2017-05-05 */

            set_led_mode(peripheral_para[0]);
            if(peripheral_para[0] == LED_CARD_NO_SPACE)
                card_space_less_flag = 1;

            break;
        case MSG_PERIPHERAL_TASK_LED_FLASH_SET:

            ap_peripheral_led_flash_power_off_set(peripheral_para[0]);
            break;
        case MSG_PERIPHERAL_TASK_LED_FLASH:

            break;

        case MSG_PERIPHERAL_TASK_BAT_CHECK:
            //if (bat_ck_timerid == 0xFF) {
            //	bat_ck_timerid = BATTERY_DETECT_TIMER_ID;
            //	sys_set_timer((void*)msgQSend, (void*)PeripheralTaskQ, MSG_PERIPHERAL_TASK_BAT_STS_FULL, bat_ck_timerid, PERI_TIME_INTERVAL_BAT_CHECK);
            //}
            break;
        case MSG_PERIPHERAL_TASK_BAT_STS_FULL:
            //sys_kill_timer(bat_ck_timerid);
            //bat_ck_timerid = 0xFF;

            type = LED_WAITING_RECORD;
            msgQSend(PeripheralTaskQ, MSG_PERIPHERAL_TASK_LED_SET, &type, sizeof(INT32U), MSG_PRI_NORMAL);

            break;

#if USE_ADKEY_NO
        case MSG_PERIPHERAL_TASK_AD_DETECT_CHECK:
            ap_peripheral_ad_key_judge();
            ap_peripheral_usbd_detect_check(&usbd_debounce_cnt);
#if C_BATTERY_DETECT == CUSTOM_ON
            ap_peripheral_battery_check_calculate();
#endif
            break;
#endif
#if C_BATTERY_DETECT == CUSTOM_ON && USE_ADKEY_NO
        case MSG_PERIPHERAL_TASK_BAT_STS_REQ:
            ap_peripheral_battery_sts_send();
            break;
#endif
#if C_MOTION_DETECTION == CUSTOM_ON
        case MSG_PERIPHERAL_TASK_MOTION_DETECT_JUDGE:
            ap_peripheral_motion_detect_judge();
            break;
        case MSG_PERIPHERAL_TASK_MOTION_DETECT_START:
            ap_peripheral_motion_detect_start();
            break;
        case MSG_PERIPHERAL_TASK_MOTION_DETECT_STOP:
            ap_peripheral_motion_detect_stop();
            break;
#endif
        case MSG_PERIPHERAL_TASK_AUTO_POWER_OFF_SET:
            ap_peripheral_auto_power_off_set(peripheral_para[0]);
            break;

        default:
            break;
        }

    }
}


/*****************************************************************************
 * 函 数 名  : ap_no_storage_states_set_led
 * 负 责 人  : Zhibo
 * 创建日期  : 2017年5月5日
 * 函数功能  : 设置led的状态闪烁
 * 输入参数  : void  NULL
 * 输出参数  : 无
 * 返 回 值  :
 * 调用关系  :
 * 其    它  :

*****************************************************************************/
extern void led_red_on(void);
extern void led_green_on(void);
extern void led_red_off(void);
extern void led_green_off(void);
void ap_no_storage_states_set_led(APP_STATES states)
{
    if( APP_STATES_NO_SD == states)
    {
        no_storage_states_count++;
        //		__msg("states = %d, no storage states\n", states);
        led_green_off();
        if(no_storage_states_count < 12)
        {
            led_red_on();
        }
        else if((no_storage_states_count >= 12 ) && (no_storage_states_count < 24))
        {
            led_red_off();
        }
        if(no_storage_states_count >= 24)
        {
            no_storage_states_count = 0;
        }
    }
    else if(APP_STATES_RECORD == states)
    {
        no_storage_states_count++;
        //		__msg("states = %d, record states\n", states);
        led_red_on();
        if(no_storage_states_count < 80)
        {
            led_green_on();
        }
        else if((no_storage_states_count >= 80 ) && (no_storage_states_count < 160))
        {
            led_green_off();
        }
        if(no_storage_states_count >= 160)
        {
            no_storage_states_count = 0;
        }
    }
    else if((APP_STATES_CARD_DETE_SUC ==  states)/* || (APP_STATES_INIT_LED == states)*/)
    {
        led_green_off();
        led_red_on();
    }
    else if(APP_STATES_CARD_FULL ==  states)
    {
        led_green_on();
        led_red_on();
    }
    //	else if(APP_STATES_CAPTURE == states)
    //	{
    //		no_storage_states_count++;
    //		capture_continues_shoot_flag++;
    //		__msg("states = %d, capture states\n", states);

    //		led_red_on();
    //		if(no_storage_states_count<20)
    //		{
    //			led_green_on();
    //		}
    //		else if((no_storage_states_count >= 20 )&&(no_storage_states_count < 40))
    //		{
    //			led_green_off();
    //		}
    //		if(no_storage_states_count>=40)
    //		{
    //			no_storage_states_count = 0;
    //		}
    //		if(capture_continues_shoot_flag > 3)
    //		{
    //			capture_continues_shoot_flag = 0;
    //			app_states_flg = -1;
    //		}
    //	}
}


