/*******************************************************************************
* file    log.c
* author  mackgim
* version 1.0.0
* date
* brief   debug
*******************************************************************************/

#include "log.h"
#include "platform.h"
#include "standard_lib.h"


#if defined(__CC_ARM)
#elif defined(__ICCARM__)
#elif defined(__GNUC__)
#include <sys/stat.h>
#endif

#ifdef __cplusplus
extern "C"
#endif

#ifdef DEBUG



#pragma region 函数
uint8_t klog_send_by_poll(uint8_t * pBuffer, int size);
uint8_t klog_send_by_dma(void);

static void klog_mspinit(UART_HandleTypeDef* huart);
static void klog_mspdeinit(UART_HandleTypeDef* huart);
static void klog_uart_tx_cplt_callback(UART_HandleTypeDef* huart);
static void klog_uart_rx_cplt_callback(UART_HandleTypeDef* huart);
static void klog_uart_error_callback(UART_HandleTypeDef* huart);
#pragma endregion


#pragma region 变量
static uint8_t sKlogTxBuff[KLOG_TX_MAX_BUFF];
static volatile uint32_t sKlogTxBuffPtrIn = 0;
static volatile uint32_t sKlogTxBuffPtrOut = 0;
static volatile uint32_t sKlogTxBuffPtrOutTemp = 0; //当前传输的数据大小
static volatile uint8_t sKlogTxCplt = true;

static uint8_t sKlogRxBuffer;

static UART_HandleTypeDef sKlogHandle;
static uint8_t sKlogHwIsInit = false;
static uint8_t sKlogMode = 0; //0-FIFO配合硬件打印, 1-仅仅FIFO打印
typedef void(*LOG_RX_CALLBACK_TYPE) (void);
static LOG_RX_CALLBACK_TYPE rx_callback;
#pragma endregion


#pragma region 基本功能 
uint8_t klog_init(void)
{
	if (sKlogHwIsInit == false)
	{

		sKlogHandle.Instance = KLOG_USARTx;
		sKlogHandle.Init.BaudRate = KLOG_USARTx_BAUD_RATE;
		sKlogHandle.Init.WordLength = UART_WORDLENGTH_8B;
		sKlogHandle.Init.StopBits = UART_STOPBITS_1;
		sKlogHandle.Init.Parity = UART_PARITY_NONE;
		sKlogHandle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
		sKlogHandle.Init.Mode = UART_MODE_TX_RX;
		sKlogHandle.Init.OverSampling = UART_OVERSAMPLING_16;
		sKlogHandle.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
		sKlogHandle.Init.ClockPrescaler = UART_PRESCALER_DIV1;
		sKlogHandle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

		HAL_UART_RegisterCallback(&sKlogHandle, HAL_UART_MSPINIT_CB_ID, klog_mspinit);
		HAL_UART_RegisterCallback(&sKlogHandle, HAL_UART_MSPDEINIT_CB_ID, klog_mspdeinit);


		HAL_UART_Init(&sKlogHandle);

		HAL_UARTEx_SetTxFifoThreshold(&sKlogHandle, UART_TXFIFO_THRESHOLD_1_8);
		HAL_UARTEx_SetRxFifoThreshold(&sKlogHandle, UART_RXFIFO_THRESHOLD_1_8);
		HAL_UARTEx_DisableFifoMode(&sKlogHandle);

		HAL_UART_RegisterCallback(&sKlogHandle, HAL_UART_TX_COMPLETE_CB_ID, klog_uart_tx_cplt_callback);
		HAL_UART_RegisterCallback(&sKlogHandle, HAL_UART_RX_COMPLETE_CB_ID, klog_uart_rx_cplt_callback);
		HAL_UART_RegisterCallback(&sKlogHandle, HAL_UART_ERROR_CB_ID, klog_uart_error_callback);


		HAL_UART_Receive_IT(&sKlogHandle, &sKlogRxBuffer, sizeof(sKlogRxBuffer));
		//while ((KLOG_USARTx->SR & USART_SR_TC) == (uint16_t)RESET);
		while (__HAL_UART_GET_FLAG(&sKlogHandle, UART_FLAG_TC) == RESET);
		sKlogHwIsInit = true;
	}

	return STD_SUCCESS;
}

uint8_t klog_deinit(void)
{
	if (sKlogHwIsInit == true)
	{
		sKlogHwIsInit = false;
		HAL_UART_DeInit(&sKlogHandle);
		sKlogTxCplt = true;
		//sKlogTxBuffPtrIn = sKlogTxBuffPtrOut = sKlogTxBuffPtrOutTemp = 0;

	}

	return STD_SUCCESS;
}

uint8_t klog_flush(void)
{

	ATOMIC_SECTION_BEGIN();
	sKlogMode = 1;
	//正在dma传输中,等待传输完成
	while (!sKlogTxCplt)
	{
		HAL_DMA_IRQHandler(sKlogHandle.hdmatx);
		HAL_UART_IRQHandler(&sKlogHandle);
	}

	while (sKlogTxBuffPtrIn != sKlogTxBuffPtrOut)
	{
		klog_send_by_poll(&sKlogTxBuff[sKlogTxBuffPtrOut], 1);
		sKlogTxBuffPtrOut++;
		if (sKlogTxBuffPtrOut == KLOG_TX_MAX_BUFF)
		{
			sKlogTxBuffPtrOut = 0;
		}
	}
	sKlogMode = 0;
	ATOMIC_SECTION_END();
	return true;
	//while (sKlogTxBuffPtrIn != sKlogTxBuffPtrOut)
	//{
	//	klog_send_by_dma();
	//}

	//while (__HAL_UART_GET_FLAG(&sKlogHandle, UART_FLAG_TC) == RESET);
	//return true;
}

uint8_t klog_is_busy(void)
{
	if (sKlogTxBuffPtrIn != sKlogTxBuffPtrOut)
	{
		return true;
	}

	if (__HAL_UART_GET_FLAG(&sKlogHandle, UART_FLAG_TC) == RESET)
	{
		return true;
	}

	if (sKlogTxCplt == false)
	{
		return true;
	}

	return false;
}

void klog_set_mode(uint8_t mode)
{
	sKlogMode = mode;
}

void klog_register_rx_cb(void* cb)
{
	rx_callback = cb;
}

uint8_t klog_send_by_dma(void)
{
	if (!sKlogHwIsInit)
	{
		return STD_FAILED;
	}
	if (sKlogMode)//1-只保留在缓存中，不输出串口
	{
		return STD_DO_NOTHING;
	}
	if (sKlogTxCplt == false)
	{
		return STD_BUSY;
	}

	uint32_t in = sKlogTxBuffPtrIn;
	uint32_t out = sKlogTxBuffPtrOut;

	if (in != out)
	{
		uint32_t tout = sKlogTxBuffPtrOut;
		uint32_t size = 0;
		if (out < in)
		{
			size = in - out;
			if (size > 0xffff)
			{
				size = 0xffff;
			}

			//out = in;
			out = out + size;
		}
		else
		{
			size = KLOG_TX_MAX_BUFF - out;

			//没有到队尾
			if (size > 0xffff)
			{
				size = 0xffff;
				out = out + size;
			}
			//已经到了队尾
			else
			{
				out = 0;
			}
		}

		uint8_t* addr = &sKlogTxBuff[tout];
		sKlogTxCplt = false;
		ATOMIC_SECTION_BEGIN();
		HAL_StatusTypeDef ret = HAL_UART_Transmit_DMA(&sKlogHandle, addr, (uint16_t)size);
		ATOMIC_SECTION_END();
		if (ret == HAL_OK)
		{
			sKlogTxBuffPtrOutTemp = out;
			return STD_SUCCESS;
		}
		else
		{
			sKlogTxCplt = true;
			return STD_FAILED;
		}
	}
	else
	{
		//sKlogTxCplt = true;
		return STD_DO_NOTHING;
	}
}

uint8_t klog_send_by_poll(uint8_t* pBuffer, int size)
{
	HAL_StatusTypeDef ret = HAL_UART_Transmit(&sKlogHandle, pBuffer, (uint16_t)size, 100);
	if (ret == HAL_OK)
	{
		return STD_SUCCESS;
	}
	else
	{
		return STD_FAILED;
	}
}

void klog_put_fifo(const char* pBuffer, int size)
{

	//osKernelGetState
	static uint32_t running = false;
	static uint32_t count = 0;
	if (running == true) //防止重复载入
	{
		count++;
		if (count > KLOG_TX_MAX_BUFF)
		{
			count = 0;
		}
		return;
	}
	running = true;

	uint32_t in = sKlogTxBuffPtrIn;
	uint32_t out = sKlogTxBuffPtrOut;
	uint32_t c = count;
	uint8_t overflow = false;
	uint32_t ll = size + c;
	if (out > in)
	{
		if (ll >= (out - in))
		{
			overflow = true;
		}
	}
	else if (out < in)
	{
		uint32_t l = KLOG_TX_MAX_BUFF + out - in;
		if (ll >= l)
		{
			overflow = true;
		}
	}
	else
	{
		if (ll >= KLOG_TX_MAX_BUFF)
		{
			overflow = true;
		}
	}
	if (overflow) //添加一个溢出&标志
	{
		uint32_t t = in;
		in++;
		if (in == KLOG_TX_MAX_BUFF)
		{
			in = 0;
		}
		if (in != out) //防止首尾相连，抹除数据
		{
			sKlogTxBuff[t] = '&'; //添加溢出标志输出
			sKlogTxBuffPtrIn = in;
		}
		running = false;
		//sKlogTxCplt = true;
		klog_send_by_dma();
		return;
	}

	for (uint32_t i = 0; i < (uint32_t)size; i++)
	{
		sKlogTxBuff[in] = pBuffer[i];
		in++;
		if (in == KLOG_TX_MAX_BUFF)
		{
			in = 0;
		}
	}
	for (uint32_t i = 0; i < c; i++) //添加丢弃标志输出
	{
		sKlogTxBuff[in] = '$';
		in++;
		if (in == KLOG_TX_MAX_BUFF)
		{
			in = 0;
		}
	}
	sKlogTxBuffPtrIn = in;
	if (count < c)
	{
	}
	else
	{
		count = count - c;
	}
	running = false;
	klog_send_by_dma();
}

void klog_put_fifo2(const uint8_t* pBuffer, int size)
{

	//osKernelGetState
	static uint32_t running = false;
	static uint32_t count = 0;
	if (running == true) //防止重复载入
	{
		count++;
		if (count > KLOG_TX_MAX_BUFF)
		{
			count = 0;
		}
		return;
	}
	running = true;

	uint32_t in = sKlogTxBuffPtrIn;
	uint32_t out = sKlogTxBuffPtrOut;
	uint32_t c = count;
	uint8_t overflow = false;
	uint32_t ll = size + c;
	if (out > in)
	{
		if (ll >= (out - in))
		{
			overflow = true;
		}
	}
	else if (out < in)
	{
		uint32_t l = KLOG_TX_MAX_BUFF + out - in;
		if (ll >= l)
		{
			overflow = true;
		}
	}
	else
	{
		if (ll >= KLOG_TX_MAX_BUFF)
		{
			overflow = true;
		}
	}
	if (overflow) //添加一个溢出&标志
	{
		uint32_t t = in;
		in++;
		if (in == KLOG_TX_MAX_BUFF)
		{
			in = 0;
		}
		if (in != out) //防止首尾相连，抹除数据
		{
			sKlogTxBuff[t] = '&'; //添加溢出标志输出
			sKlogTxBuffPtrIn = in;
		}
		running = false;
		return;
	}

	for (uint32_t i = 0; i < (uint32_t)size; i++)
	{
		sKlogTxBuff[in] = pBuffer[i];
		in++;
		if (in == KLOG_TX_MAX_BUFF)
		{
			in = 0;
		}
	}
	for (uint32_t i = 0; i < c; i++) //添加丢弃标志输出
	{
		sKlogTxBuff[in] = '$';
		in++;
		if (in == KLOG_TX_MAX_BUFF)
		{
			in = 0;
		}
	}
	sKlogTxBuffPtrIn = in;
	if (count < c)
	{
	}
	else
	{
		count = count - c;
	}
	running = false;
}

uint8_t klog_get(void)
{
	uint8_t ret = sKlogRxBuffer;
	sKlogRxBuffer = 0;
	return ret;
}
#pragma endregion


#pragma region 中断

void KLOG_USARTx_IRQHandler(void)
{
	HAL_UART_IRQHandler(&sKlogHandle);
}
void KLOG_USARTx_TX_DMAx_IRQHandler(void)
{
	HAL_DMA_IRQHandler(sKlogHandle.hdmatx);
}

#pragma endregion

#pragma region 回调

static DMA_HandleTypeDef hdma_tx;
void klog_mspinit(UART_HandleTypeDef* huart)
//void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{

	if (huart->Instance == KLOG_USARTx)
	{

		KLOG_USARTx_SET_CLK_SOURCE();

		GPIO_InitTypeDef  GPIO_InitStruct;

		/*##-1- Enable peripherals and GPIO Clocks #################################*/
		/* Enable GPIO TX/RX clock */
		KLOG_USARTx_TX_CLK_ENABLE();
		KLOG_USARTx_RX_CLK_ENABLE();
		/* Enable USART2 clock */
		KLOG_USARTx_CLK_ENABLE();
		KLOG_DMAMUXx_CLK_ENABLE();
		KLOG_DMAx_CLK_ENABLE();
		/*##-2- Configure peripheral GPIO ##########################################*/
		/* UART TX GPIO pin configuration  */
		GPIO_InitStruct.Pin = KLOG_USARTx_TX_PIN;
		GPIO_InitStruct.Mode = KLOG_USARTx_TX_MODE;
		GPIO_InitStruct.Pull = KLOG_USARTx_TX_PULL;
		GPIO_InitStruct.Speed = KLOG_USARTx_TX_SPEED;
		GPIO_InitStruct.Alternate = KLOG_USARTx_TX_ALTERNATE;

		HAL_GPIO_Init(KLOG_USARTx_TX_PORT, &GPIO_InitStruct);


		/* UART RX GPIO pin configuration  */
		GPIO_InitStruct.Pin = KLOG_USARTx_RX_PIN;
		GPIO_InitStruct.Mode = KLOG_USARTx_RX_MODE;
		GPIO_InitStruct.Pull = KLOG_USARTx_RX_PULL;
		GPIO_InitStruct.Speed = KLOG_USARTx_RX_SPEED;
		GPIO_InitStruct.Alternate = KLOG_USARTx_RX_ALTERNATE;

		HAL_GPIO_Init(KLOG_USARTx_RX_PORT, &GPIO_InitStruct);

		/*##-3- Configure the NVIC for UART ########################################*/
		/* NVIC for USART1 */
	


		hdma_tx.Instance = KLOG_USARTx_TX_DMAx_INSTANCE;
		hdma_tx.Init.Request = KLOG_USARTx_TX_DMAx_REQUEST;
		hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
		hdma_tx.Init.PeriphInc = DMA_PINC_DISABLE;
		hdma_tx.Init.MemInc = DMA_MINC_ENABLE;
		hdma_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
		hdma_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
		hdma_tx.Init.Mode = DMA_NORMAL;
		hdma_tx.Init.Priority = DMA_PRIORITY_LOW;

		__HAL_LINKDMA(huart, hdmatx, hdma_tx);

		//HAL_DMA_DeInit(&hdma_tx);
		HAL_DMA_Init(&hdma_tx);

		HAL_NVIC_SetPriority(KLOG_USARTx_IRQn, KLOG_USARTx_NVIC_PreemptionPriority, KLOG_USARTx_NVIC_SubPriority);
		HAL_NVIC_EnableIRQ(KLOG_USARTx_IRQn);

		HAL_NVIC_SetPriority(KLOG_USARTx_TX_DMAx_IRQn, KLOG_TX_DMAx_NVIC_PreemptionPriority, KLOG_TX_DMAx_NVIC_SubPriority);
		HAL_NVIC_EnableIRQ(KLOG_USARTx_TX_DMAx_IRQn);
	}
}

void klog_mspdeinit(UART_HandleTypeDef* huart)

//void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{

	if (huart->Instance == KLOG_USARTx)
	{
		/*##-1- Reset peripherals ##################################################*/
		//KLOG_USARTx_FORCE_RESET();
		//KLOG_USARTx_RELEASE_RESET();
		//HAL_UART_AbortReceive_IT(huart);
		HAL_NVIC_DisableIRQ(KLOG_USARTx_IRQn);
		HAL_NVIC_DisableIRQ(KLOG_USARTx_TX_DMAx_IRQn);

		//__HAL_UART_CLEAR_FLAG(&sKlogHandle, USART_FLAG_PE | USART_FLAG_FE | USART_FLAG_NE | USART_FLAG_ORE | USART_FLAG_IDLE | USART_FLAG_RXNE | USART_FLAG_TC | USART_FLAG_TXE);
		KLOG_USARTx_CLK_DISABLE();
		/*##-2- Disable peripherals and GPIO Clocks #################################*/
		/* Configure UART Tx as alternate function  */
		HAL_GPIO_DeInit(KLOG_USARTx_TX_PORT, KLOG_USARTx_TX_PIN);
		/* Configure UART Rx as alternate function  */
		HAL_GPIO_DeInit(KLOG_USARTx_RX_PORT, KLOG_USARTx_RX_PIN);

		/*##-3- Disable the NVIC for UART ##########################################*/

	}
}

void klog_uart_rx_cplt_callback(UART_HandleTypeDef* huart)
{
	if (huart->Instance == KLOG_USARTx)
	{
		/* Start another reception */
		HAL_UART_Receive_IT(huart, &sKlogRxBuffer, sizeof(sKlogRxBuffer));
		if (rx_callback != NULL)
		{
			rx_callback();
		}
	}
}

void klog_uart_error_callback(UART_HandleTypeDef* huart)
{
	if (huart->Instance == KLOG_USARTx)
	{
		//sKlogTxCplt = true;
		kprint("error\r\n");
	}
}

void klog_uart_tx_cplt_callback(UART_HandleTypeDef* huart)
{
	if (huart->Instance == KLOG_USARTx)
	{
		sKlogTxBuffPtrOut = sKlogTxBuffPtrOutTemp;
		sKlogTxCplt = true;
		klog_send_by_dma();
	}
}
#pragma endregion


#pragma region 接入编译器提供的api

#if defined(__CC_ARM)

// This part is taken from MDK-ARM template file and is required here to prevent
// linker from selecting libraries functions that use semihosting and failing
// because of multiple definitions of fgetc() and fputc().
// Refer to: http://www.keil.com/support/man/docs/gsac/gsac_retargetcortex.htm
// -- BEGIN --
struct __FILE
{
	int handle; /* Add whatever you need here */
};
FILE __stdout;
FILE __stdin;
// --- END ---

int fgetc(FILE* p_file)
{
	uint8_t input;
	while (app_uart_get(&input) == NRF_ERROR_NOT_FOUND)
	{
		// No implementation needed.
	}
	return input;
}

int fputc(int ch, FILE* p_file)
{
	UNUSED_PARAMETER(p_file);

	UNUSED_VARIABLE(app_uart_put((uint8_t)ch));
	return ch;
}

#elif defined(__GNUC__) && defined(__SES_ARM)

int __getchar(FILE* p_file)
{
	uint8_t input;
	while (app_uart_get(&input) == NRF_ERROR_NOT_FOUND)
	{
		// No implementation needed.
	}
	return input;
}

#if defined(__SES_VERSION) && (__SES_VERSION >= 34000)
int __putchar(int ch, __printf_tag_ptr tag_ptr)
{
	UNUSED_PARAMETER(tag_ptr);

	UNUSED_VARIABLE(app_uart_put((uint8_t)ch));
	return ch;
}
#else
int __putchar(int ch, FILE* p_file)
{
	UNUSED_PARAMETER(p_file);

	UNUSED_VARIABLE(app_uart_put((uint8_t)ch));
	return ch;
}
#endif

#elif defined(__GNUC__) && !defined(__SES_ARM)

#include <errno.h>
#include <sys/time.h>
#include <sys/times.h>

extern int errno;

#if 0
register char* stack_ptr asm("sp");

char* __env[1] = { 0 };
char** environ = __env;

caddr_t _sbrk(int incr)
{
	extern char end asm("end");
	static char* heap_end;
	char* prev_heap_end;

	if (heap_end == 0)
		heap_end = &end;

	prev_heap_end = heap_end;
	if (heap_end + incr > stack_ptr)
	{
		errno = ENOMEM;
		return (caddr_t)-1;
	}

	heap_end += incr;

	return (caddr_t)prev_heap_end;
}

#else

static uint8_t* __sbrk_heap_end = NULL;

void* _sbrk(ptrdiff_t incr)
{
	extern uint8_t _end; /* Symbol defined in the linker script */
	extern uint8_t _estack; /* Symbol defined in the linker script */
	extern uint32_t _Min_Stack_Size; /* Symbol defined in the linker script */
	const uint32_t stack_limit = (uint32_t)&_estack - (uint32_t)&_Min_Stack_Size;
	const uint8_t* max_heap = (uint8_t*)stack_limit;
	uint8_t* prev_heap_end;

	/* Initalize heap end at first call */
	if (NULL == __sbrk_heap_end)
	{
		__sbrk_heap_end = &_end;
	}

	/* Protect heap from growing into the reserved MSP stack */
	if (__sbrk_heap_end + incr > max_heap)
	{
		errno = ENOMEM;
		return (void*)-1;
	}

	prev_heap_end = __sbrk_heap_end;
	__sbrk_heap_end += incr;

	return (void*)prev_heap_end;
}
#endif
/* Functions */
void initialise_monitor_handles()
{
}

int _getpid(void)
{
	return 1;
}

int _kill(int pid, int sig)
{
	errno = EINVAL;
	return -1;
}

void _exit(int status)
{
	_kill(status, -1);
	while (1) {}		/* Make sure we hang here */
}

__attribute__((weak)) int _read(int file, char* ptr, int len)
{

	return len;
}

__attribute__((weak)) int _write(int file, char* ptr, int len)
{

	klog_put_fifo((const char*)ptr, len);

	return len;
}

int _close(int file)
{
	return -1;
}


int _fstat(int file, struct stat* st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int file)
{
	return 1;
}

int _lseek(int file, int ptr, int dir)
{
	return 0;
}

int _open(char* path, int flags, ...)
{
	/* Pretend like we always fail */
	return -1;
}

int _wait(int* status)
{
	errno = ECHILD;
	return -1;
}

int _unlink(char* name)
{
	errno = ENOENT;
	return -1;
}

int _times(struct tms* buf)
{
	return -1;
}

int _stat(char* file, struct stat* st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int _link(char* old, char* new)
{
	errno = EMLINK;
	return -1;
}

int _fork(void)
{
	errno = EAGAIN;
	return -1;
}

int _execve(char* name, char** argv, char** env)
{
	errno = ENOMEM;
	return -1;
}

#elif defined(__ICCARM__)

size_t __write(int handle, const unsigned char* buffer, size_t size)
{
	int i;
	UNUSED_PARAMETER(handle);
	for (i = 0; i < size; i++)
	{
		UNUSED_VARIABLE(app_uart_put(*buffer++));
	}
	return size;
}

size_t __read(int handle, unsigned char* buffer, size_t size)
{
	UNUSED_PARAMETER(handle);
	UNUSED_PARAMETER(size);
	while (app_uart_get((uint8_t*)buffer) == NRF_ERROR_NOT_FOUND)
	{
		// No implementation needed.
	}

	return 1;
}

long __lseek(int handle, long offset, int whence)
{
	return -1;
}
int __close(int handle)
{
	return 0;
}
int remove(const char* filename)
{
	return 0;
}

#endif
#pragma endregion

#endif