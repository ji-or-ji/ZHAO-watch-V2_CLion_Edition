/* 定义STM32F103C8T6的内存布局：64KB Flash, 20KB RAM */
MEMORY
{
    RAM (xrw)    : ORIGIN = 0x20000000, LENGTH = 20K
    FLASH (rx)   : ORIGIN = 0x8000000,  LENGTH = 64K
}

/* 入口点为 Reset_Handler */
ENTRY(Reset_Handler)

SECTIONS
{
    /* 中断向量表 */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

    /* 程序代码 */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP (*(.init))
        KEEP (*(.fini))
        . = ALIGN(4);
        _etext = .;
    } >FLASH

    /* 只读数据 */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH

    /* 初始化数据（在Flash中，上电拷贝到RAM） */
    .data : AT (_etext)
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >RAM

    /* 未初始化数据（BSS段，上电清零） */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } >RAM

    /* 用户堆栈设置 */
    _stack_top = ORIGIN(RAM) + LENGTH(RAM);
}