/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2015/1/23, v1.0 create this file.
 *******************************************************************************/
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"

#include "user_interface.h"
#include "espconn.h"

#include "user_sntp.h"

struct struct_time time;
unsigned int current_stamp;

LOCAL os_timer_t timer_sntp;

void ICACHE_FLASH_ATTR user_sntp_timer_func(void *arg) {
	unsigned int current_stamp_temp;

	if ((current_stamp == 0 || time.second == 59) && wifi_station_get_connect_status() == STATION_GOT_IP) {
		current_stamp_temp = sntp_get_current_timestamp();
		if (current_stamp_temp > 0) {
			current_stamp = current_stamp_temp;
			os_printf("SNTP : %d \n", current_stamp);
		}else{
			os_printf("SNTP : fail \n");
			current_stamp++;
		}
	} else if (current_stamp > 0) {
		current_stamp++;
	}

	if (current_stamp > 0) {
		time_strtohex((char*) (sntp_get_real_time(current_stamp)));
		os_printf("20%02d/%02d/%02d ��%d %02d:%02d:%02d\n", time.year, time.month, time.day, time.week, time.hour,
				time.minute, time.second);
		user_tm1628_time_refresh();
	}

}

void ICACHE_FLASH_ATTR
user_sntp_init(void) {
	sntp_set_timezone(8);	//ʱ��:+8
	ip_addr_t *addr = (ip_addr_t *) os_zalloc(sizeof(ip_addr_t));
	ipaddr_aton("210.72.145.44", addr);
	sntp_setserver(0, addr); // set server 0 by IP address
	sntp_setservername(1, "us.pool.ntp.org"); // set server 1 by domain name
	sntp_setservername(2, "ntp.sjtu.edu.cn"); // set server 2 by domain name

	sntp_init();
	os_free(addr);
	current_stamp = 0;

	os_timer_disarm(&timer_sntp);
	os_timer_setfn(&timer_sntp, (os_timer_func_t *) user_sntp_timer_func, NULL);
	os_timer_arm(&timer_sntp, 1000, 1);	//1s
}

// ��sntp_get_real_time��ȡ������ʵʱ���ַ���,ת��Ϊ����time
void ICACHE_FLASH_ATTR time_strtohex(char* sntp_time) {
	//��ȡ����
	//��������ΪӢ��Mon,Tues,Wed,Thur,Fri,Sat,Sun �Ƚϵ�2����ĸ(�ܶ���������ͬ�Ƚϵ�һ��)
	switch (sntp_time[1]) {
	case 'o':
		time.week = Monday;
		break;	//��һ
	case 'e':
		time.week = Wednesday;
		break;	//����
	case 'h':
		time.week = Thursday;
		break;	//����
	case 'r':
		time.week = Friday;
		break;	//����
	case 'a':
		time.week = Saturday;
		break;	//����
	case 'u':
		if (sntp_time[0] == 'S')
			time.week = Sunday;		//����
		else if (sntp_time[0] == 'T')
			time.week = Tuesday;		//�ܶ�
		break;
	}

	//��ȡӢ��
	//�Ƚϵ�3����ĸ
	sntp_time = (char *) os_strstr(sntp_time, " ");
	sntp_time++;
	switch (*(sntp_time + 2)) {
	case 'n':
		if (*(sntp_time + 1) == 'a')
			time.month = January;			//һ��
		else if (*(sntp_time + 1) == 'u')
			time.month = June;			//����
		break;
	case 'b':
		time.month = February;
		break;	//����
	case 'r':
		if (*(sntp_time + 1) == 'a')
			time.month = March;			//����
		else if (*(sntp_time + 1) == 'p')
			time.month = April;			//����
		break;
	case 'y':
		time.month = May;
		break;		//����

	case 'l':
		time.month = July;
		break;		//����
	case 'g':
		time.month = August;
		break;	//����
	case 'p':
		time.month = September;
		break;	//����
	case 't':
		time.month = October;
		break;	//ʮ��
	case 'v':
		time.month = November;
		break;	//ʮһ��
	case 'c':
		time.month = December;
		break;	//ʮ����
	}

	//��ȡ��
	sntp_time = (char *) os_strstr(sntp_time, " ");
	sntp_time++;
	time.day = (*sntp_time - 0x30) * 10 + *(sntp_time + 1) - 0x30;
	//��ȡʱ
	sntp_time = (char *) os_strstr(sntp_time, " ");
	sntp_time++;
	time.hour = (*sntp_time - 0x30) * 10 + *(sntp_time + 1) - 0x30;
	//��ȡ��
	sntp_time = (char *) os_strstr(sntp_time, ":");
	sntp_time++;
	time.minute = (*sntp_time - 0x30) * 10 + *(sntp_time + 1) - 0x30;
	//��ȡ��
	sntp_time = (char *) os_strstr(sntp_time, ":");
	sntp_time++;
	time.second = (*sntp_time - 0x30) * 10 + *(sntp_time + 1) - 0x30;
	//��ȡ��
	sntp_time = (char *) os_strstr(sntp_time, " ");
	sntp_time++;
	time.year = (*(sntp_time + 2) - 0x30) * 10 + *(sntp_time + 3) - 0x30;

}
