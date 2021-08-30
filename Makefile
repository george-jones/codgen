
# pluginesque modules for filling in map regions with stuff
REGION_MODS=rmod_forest.o rmod_bridge.o rmod_rocks.o rmod_buildings.o \
rmod_graveyard.o rmod_mound.o rmod_trench.o

MODS=codgen.o mazemaker.o terrain.o primitives.o codgen_random.o region.o \
region_maker.o stat.o map.o rmods.o mapfile_parser.o catenary.o taken_area.o \
xml.o output_terrain.o \
$(REGION_MODS)

LIBS=-lm -lgd

codgen : $(MODS)
	gcc $(MODS) -o $@ $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $(INC) -g -o $*.o $<

clean:
	rm *.o
	rm codgen

genclean:
	rm *.map *.gsc *.arena

