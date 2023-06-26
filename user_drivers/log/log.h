/*******************************************************************************
* file    klog.h
* author  mackgim
* version 1.0.0
* date
* brief   debug
*******************************************************************************/


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __KLOG_H
#define __KLOG_H

#ifdef __cplusplus
extern "C" 
#endif

#include <stdint.h>

	
#ifdef DEBUG
#include <stdio.h>
#define log_init()			klog_init()
#define log_deinit()		klog_deinit()
#define log_flush()			klog_flush()
#define log_get()			klog_get()
#define log_set_mode(m)		klog_set_mode(m)
#define log_register_rx_cb(n)	klog_register_rx_cb(n)
#define log_is_busy()		klog_is_busy()


uint8_t klog_init(void);
uint8_t klog_deinit(void);
uint8_t klog_flush(void);
uint8_t klog_is_busy(void);

uint8_t klog_get(void);
void klog_set_mode(uint8_t mode);
void klog_register_rx_cb(void* cb);

#else

#define log_init()       
#define log_deinit()   
#define log_flush()	
#define log_get()  (0)
#define log_set_mode(m)	
#define log_register_rx_cb()
#define log_is_busy()	(0)
#endif


#endif /* __KLOG_H*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
