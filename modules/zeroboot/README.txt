		Zeroboot Memory Layout

		Mun-Sik, Park <boggle70@falinux.com>

This document describes the zeroboot memory layout.
Bootloader and other system memroy do not use this area.

base	:	Physical memory or kernel PAGE_OFFSET

Start				End					Size		Use
----------------------------------------------------------------
base + 0x00080000	base + 0x00087FFF	0x00080000	ZBI header


base + 0x00088000	base + 0x0008FFFF	0x00008000	NALCODE


base + 0x00090000	base + 0x0095FFFF	0x00006000	PreLoader


base + 0x00096000	base + 0x0097FFFF	0x00002000	Reserved BBT


See carefully Bootlaod linker script(stack, heap, code, text).
See kernel memory reserved area



BBT policy
1. block array is on memory base + 0x0008E000, size 8192(2 PAGE_SIZE).
2. all array is 8bit unit, so max bad block could count up to 255.
3. block number is accumulated by past block.
4. bad block is build at storage init.
5. bootloader must setup bbt table every booting.
6. nalcode and preloader do not modify bbt table.


