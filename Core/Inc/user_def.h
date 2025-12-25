/*
 * user_def.h
 *
 *  Created on: Dec 16, 2025
 *      Author: SIDO
 */

#ifndef INC_USER_DEF_H_
#define INC_USER_DEF_H_

void Out_esp32_rst_pin(uint8_t ctrl);
void usr_MX_SDMMC1_SD_Init(void);
void SD_GPIO_Low_Out(void);
void SD_GPIO_Float(void);
void sd_card_ctl(uint8_t ctl_val);	// 1: stm32, 0: esp32
void sd_scan_dir(void);

void test_gpio(void);
void init_proc(void);
void run_proc(void);


#endif /* INC_USER_DEF_H_ */
