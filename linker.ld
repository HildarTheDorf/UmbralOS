/* Tell the linker that we want an x86_64 ELF64 output file */
OUTPUT_FORMAT(elf64-x86-64)

/* We want the symbol kmain to be our entry point */
ENTRY(_start)

/* Define the program headers we want so the bootloader gives us the right */
/* MMU permissions; this also allows us to exert more control over the linking */
/* process. */
PHDRS
{
    headers PT_PHDR PHDRS;
    text PT_LOAD FILEHDR PHDRS;
    limine_requests PT_LOAD;
    rodata PT_LOAD;
    data PT_LOAD;
    dynamic PT_DYNAMIC;
}

SECTIONS
{
    /* We want to be placed in the topmost 2GiB of the address space, for optimisations */
    /* and because that is what the Limine spec mandates. */
    /* Any address in this region will do, but often 0xffffffff80000000 is chosen as */
    /* that is the beginning of the region. */
    /* Additionally, leave space for the ELF headers by adding SIZEOF_HEADERS to the */
    /* base load address. */
    . = SIZEOF_HEADERS;

    .text : {
        *(.text .text.*)
    } :text

    /* Move to the next memory page for .limine_requests */
    . = ALIGN(CONSTANT(MAXPAGESIZE));

    /* Define a section to contain the Limine requests and assign it to its own PHDR */
    .limine_requests : {
        KEEP(*(.limine_requests.start))
        KEEP(*(.limine_requests))
        KEEP(*(.limine_requests.end))
    } :limine_requests

    /* Move to the next memory page for .rodata */
    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .rodata : {
        *(.rodata .rodata.*)
    } :rodata

    /* Move to the next memory page for .data */
    . = ALIGN(CONSTANT(MAXPAGESIZE));

    .data : {
        *(.data .data.*)
    } :data

    /* Dynamic section for relocations, both in its own PHDR and inside data PHDR. */
    .dynamic : {
        *(.dynamic)
    } :data :dynamic

    /* NOTE: .bss needs to be the last thing mapped to :data, otherwise lots of */
    /* unnecessary zeros will be written to the binary. */
    /* If you need, for example, .init_array and .fini_array, those should be placed */
    /* above this. */
    .bss : {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    /* Discard .note.* and .eh_frame* since they may cause issues on some hosts. */
    /* Also discard the program interpreter section since we do not need one. This is */
    /* more or less equivalent to the --no-dynamic-linker linker flag, except that it */
    /* works with ld.gold. */
    /DISCARD/ : {
        *(.eh_frame*)
        *(.note .note.*)
        *(.interp)
    }
}
