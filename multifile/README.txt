A multiple source files example.  To heck that all files have been compiled and linked use nm command to verify module elf symbol tables.

$ nm multifile.ko 
00000000 T cleanup_module
00000000 t _exit
00000000 T foo
00000000 t _init
00000000 T init_module
         U mcount
00000040 r __module_depends
         U printk
00000000 D __this_module
00000000 r __UNIQUE_ID_author1
0000000d r __UNIQUE_ID_license0
0000001c r __UNIQUE_ID_srcversion1
00000049 r __UNIQUE_ID_vermagic0
00000000 r ____versions
