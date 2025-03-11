define(`B2SUM', `syscmd(b2sum $1 | cut -f 1 -d " ")')dnl
timeout: 0

/umbralos
protocol: limine
path: boot():/boot/umbralos.bin`#'B2SUM(`iso_root/boot/umbralos.bin')dnl
