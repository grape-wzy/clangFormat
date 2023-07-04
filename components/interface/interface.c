// TODO: 对接外设接口：I2C, SPI, UART, CAN, etc...
/*
 * 向上实现通用的open, close, init, read, write, config函数接口层，向下通过register函数实现具体接口的注册，
 * 向下对接xxx_interface，向上的函数接口讲最终根据interface对象调用到每个接口的具体驱动函数。
 * xxx_interface接口将再次向下对接真正的外设驱动层，并调用HAL库实现对硬件接口的读写能力。
 * 所有外设的接收功能尽量都以中断形式实现，在xxx_interface驱动接口中实现数据的接收和存储，并可通过用户配置的notify函数实现对用户层的数据到来的通知功能。
 */