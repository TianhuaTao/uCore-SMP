set confirm off
set architecture riscv:rv64
target remote 127.0.0.1:15234
symbol-file kernel
# set disassemble-next-line auto
display/12i $pc-8
set riscv use-compressed-breakpoints yes
break kerneltrap
watch *0x80201f16
# break *0x80201cfa
# watch *0x80213f50+24
