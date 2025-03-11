ENTRY(_start)

PHDRS
{
    headers PT_PHDR PHDRS;
    rodata PT_LOAD FILEHDR PHDRS;
    limine_requests PT_LOAD;
    text PT_LOAD;
    datarelro PT_LOAD;
    data PT_LOAD;
    dynamic PT_DYNAMIC;
    relro PT_GNU_RELRO FLAGS(4);
    stack PT_GNU_STACK FLAGS(6);
}

SECTIONS
{
    . = SIZEOF_HEADERS;

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    . = ALIGN(CONSTANT(MAXPAGESIZE)) + (. & (CONSTANT(MAXPAGESIZE) - 1));

    .limine_requests : {
        KEEP(*(.limine_requests.start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests.end))
    } :limine_requests

    . = ALIGN(CONSTANT(MAXPAGESIZE)) + (. & (CONSTANT(MAXPAGESIZE) - 1));

    .text : {
        *(.text .text.*)
    } :text

    . = DATA_SEGMENT_ALIGN(CONSTANT(MAXPAGESIZE), CONSTANT(COMMONPAGESIZE));

    .data.rel.ro : {
        *(.data.rel.ro)
    } :datarelro :relro

    .dynamic : {
        *(.dynamic)
    } :datarelro :dynamic :relro

    . = DATA_SEGMENT_RELRO_END(0, .);

    .data : {
        *(.data .data.*)
    } :data

    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    . = DATA_SEGMENT_END(.);
}
