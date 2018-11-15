INCLUDE generated/output_format.ld
ENTRY(_start)

__DYNAMIC = 0;

INCLUDE generated/regions.ld

SECTIONS
{
	.fbi :
	{
		. = . + 8; /* Firmware Base Image format (len/crc) data comes first. */
	} > spiflash

	.text :
	{
		_ftext = .;
		*(.text .stub .text.* .gnu.linkonce.t.*)
		_etext = .;
	} > spiflash

	.rodata :
	{
		. = ALIGN(4);
		_frodata = .;
		*(.rodata .rodata.* .gnu.linkonce.r.*)
		*(.rodata1)
		_erodata = .;
	} > spiflash

	.data : 
	{
		. = ALIGN(4);
		_fdata = .;
		*(.data .data.* .gnu.linkonce.d.*)
		*(.data1)
		_gp = ALIGN(16);
		*(.sdata .sdata.* .gnu.linkonce.s.*)
		_edata = ALIGN(16); /* Make sure _edata is >= _gp. */
	} > sram

	.bss :
	{
		. = ALIGN(4);
		_fbss = .;
		*(.dynsbss)
		*(.sbss .sbss.* .gnu.linkonce.sb.*)
		*(.scommon)
		*(.dynbss)
		*(.bss .bss.* .gnu.linkonce.b.*)
		*(COMMON)
		. = ALIGN(4);
		_ebss = .;
		_end = .;
	} > sram
}

PROVIDE(_fstack = ORIGIN(sram) + LENGTH(sram) - 4);
