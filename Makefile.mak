
# pluginesque modules for filling in map regions with stuff
REGION_MODS=rmod_forest.obj rmod_bridge.obj rmod_rocks.obj rmod_buildings.obj \
rmod_graveyard.obj rmod_mound.obj rmod_trench.obj

ZIPMODS=zip.obj ioapi.obj iowin32.obj codzip.obj

MODS=codgen.obj mazemaker.obj terrain.obj primitives.obj \
codgen_random.obj region.obj region_maker.obj stat.obj rmods.obj \
mapfile_parser.obj catenary.obj taken_area.obj xml.obj output_terrain.obj \
$(REGION_MODS) $(ZIPMODS)

LIBS=3plibs\gd\gd.lib 3plibs\libpng\libpng.lib 3plibs\zlib\zlib.lib 3plibs\zlib\zlibstat.lib
INC=/I 3plibs\gd /I 3plibs\zlib /I 3plibs\zlib\minizip

codgen.exe : $(MODS)
	cl /MT /Wall $(MODS) -o $@ $(LIBS)

zip.obj: 3plibs\zlib\minizip\zip.c
	cl /D WIN32 /MT /c $(INC) 3plibs\zlib\minizip\zip.c

ioapi.obj: 3plibs\zlib\minizip\zip.c
	cl /D WIN32 /MT /c $(INC) 3plibs\zlib\minizip\ioapi.c

iowin32.obj: 3plibs\zlib\minizip\iowin32.c
	cl /D WIN32 /MT /c $(INC) 3plibs\zlib\minizip\iowin32.c

.c.obj:
	cl /D WIN32 /MT /c $(INC) $?

clean:
	del *.obj
	del codgen.exe

genclean:
	del *.map *.gsc *.arena

