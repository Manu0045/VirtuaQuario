@echo off
REM === Compile all tile and map files ===
lcc -c -o TILES\menu_ts.o TILES\menu_ts.c
lcc -c -o TILES\bubbles_ts.o TILES\bubbles_ts.c 

lcc -c -o TILES\clownfish_ts.o TILES\clownfish_ts.c
lcc -c -o TILES\triggerfish_ts.o TILES\triggerfish_ts.c
lcc -c -o TILES\surgeonfish_ts.o TILES\surgeonfish_ts.c
lcc -c -o TILES\gammaloreto_ts.o TILES\gammaloreto_ts.c
lcc -c -o TILES\lionfish_ts.o TILES\lionfish_ts.c

lcc -c -o TILES\cursor_ts.o TILES\cursor_ts.c
lcc -c -o TILES\shop_cursor_ts.o TILES\shop_cursor_ts.c
lcc -c -o TILES\shop_ts.o TILES\shop_ts.c
lcc -c -o TILES\aquarium_ts.o TILES\aquarium_ts.c

lcc -c -o MAPS\menu.o MAPS\menu.c
lcc -c -o MAPS\shop.o MAPS\shop.c
lcc -c -o MAPS\aquarium.o MAPS\aquarium.c

REM === Compile main file ===
lcc -c -o main.o main.c

REM === Link everything into the final Game Boy ROM ===
lcc -Wm-yc3 -Wf-ba0 -Wl-yt3 -Wl-yo4 -Wl-ya4 -o main.gb ^
    main.o ^
    TILES\menu_ts.o TILES\bubbles_ts.o TILES\clownfish_ts.o TILES\triggerfish_ts.o TILES\surgeonfish_ts.o TILES\gammaloreto_ts.o TILES\lionfish_ts.o TILES\cursor_ts.o TILES\shop_cursor_ts.o TILES\shop_ts.o TILES\aquarium_ts.o ^
    MAPS\menu.o MAPS\shop.o MAPS\aquarium.o

REM === Clean up intermediate files ===
DEL TILES\*.lst 2>nul
DEL TILES\*.asm 2>nul
DEL TILES\*.o 2>nul
DEL TILES\*.sym 2>nul

DEL MAPS\*.lst 2>nul
DEL MAPS\*.asm 2>nul
DEL MAPS\*.o 2>nul
DEL MAPS\*.sym 2>nul

DEL *.lst 2>nul
DEL *.asm 2>nul
DEL *.ihx 2>nul
DEL *.o 2>nul
DEL *.sym 2>nul

PAUSE
