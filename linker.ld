/* We want the symbol kmain to be our entry point */
ENTRY(_start)

PHDRS
{
    headers PT_PHDR PHDRS;
    load_headers PT_LOAD FILEHDR PHDRS;
    limine_requests PT_LOAD;
    text PT_LOAD;
    rodata PT_LOAD;
    data PT_LOAD;
    dynamic PT_DYNAMIC;
}

SECTIONS
{
    . = SIZEOF_HEADERS;

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .limine_requests : {
        KEEP(*(.limine_requests.start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests.end))
    } :limine_requests

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .text : {
        *(.text .text.*)
    } :text

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .data : {
        *(.data .data.*)
    } :data

    .dynamic : {
        *(.dynamic)
    } :data :dynamic

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data
}
