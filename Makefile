# make wolf: build/wolf/Wolf3D.bin (default: Activision 1.4)
# make sod: build/sod/SpearOfDestiny.bin (default: Activision 1.4)
# make shareware: build/shareware/Wolf3D-Shareware.bin (shareware 1.4)

SDK ?= $(HOME)/opt/leapsterSDK
CC  = $(SDK)/bin/arc-elf32-gcc
CXX = $(SDK)/bin/arc-elf32-g++

ARCH   = -mno-sdata -fno-branch-count-reg
CFLAGS = $(ARCH) -O2 -ffreestanding -fno-builtin -Wall -Iplatform/include -Iplatform -std=gnu23
LDFLAGS = $(ARCH) -nostdlib -T platform/leapster.ld -Wl,-Map,$@.map,--no-warn-rwx-segments
LIBS = -lgcc

PLATFORM = start l2 libc conl2
PLAT_OBJ = $(PLATFORM:%=build/platform/%.o) build/platform/stack.o

build/platform/%.o: platform/%.c platform/*.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@
build/platform/stack.o: platform/stack.S
	@mkdir -p $(@D)
	$(CC) $(ARCH) -c $< -o $@
build/platform/l2.o: platform/CP437.F16

# wolf3d

WOLF_SRC = dosbox/dbopl id_ca id_in id_pm id_sd id_us_1 id_vh id_vl signon wl_act1 wl_act2 \
           wl_agent wl_atmos wl_cloudsky wl_debug wl_draw wl_floorceiling wl_game wl_inter \
           wl_main wl_menu wl_parallax wl_play wl_state wl_text
WOLF_OBJ = $(WOLF_SRC:%=build/wolf/%.o)
CXXFLAGS = $(ARCH) -O2 -ffreestanding -fno-exceptions -fno-rtti -fno-threadsafe-statics \
           -fsigned-char -fno-strict-aliasing -DLEAPSTER -DUSE_GPL $(TRACEFLAG) -Iplatform/include -Iplatform/sdl -Iplatform -Iwolf \
           -Wall -Wno-unused -Wno-sign-compare -Wno-parentheses -Wno-misleading-indentation -Wno-switch -Wno-unknown-pragmas -Wno-maybe-uninitialized -Wno-narrowing -Wno-char-subscripts -Wno-address
PCFLAGS = $(CFLAGS) -Iplatform/include -Iplatform/sdl -fsigned-char $(TRACEFLAG)
PORT_SRC = sdl_l2 libc_io libm alaw
PORT_OBJ = $(PORT_SRC:%=build/platform/%.o)

build/wolf/%.o: wolf/%.cpp wolf/*.h platform/include/*.h
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@
$(PORT_OBJ): build/platform/%.o: platform/%.c platform/include/*.h platform/l2.h
	@mkdir -p $(@D)
	$(CC) $(PCFLAGS) -c $< -o $@

wolf: build/wolf/SDMenu.bin
build/wolf/wolf.elf: $(WOLF_OBJ) $(PLAT_OBJ) $(PORT_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)
build/wolf/SDMenu.bin: build/wolf/wolf.elf tools/binToRib
	cp tools/SDMenu-template.bin $@
	tools/binToRib $@ $< sd
	cp $@ build/wolf/Wolf3D.bin

# wolf3d shareware

SW_FLAGS = -DVERSIONALREADYCHOSEN -DCARMACIZED -DUPLOAD
SW_OBJ = $(WOLF_SRC:%=build/shareware/%.o)

build/shareware/%.o: wolf/%.cpp wolf/*.h platform/include/*.h
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(SW_FLAGS) -c $< -o $@

shareware: build/shareware/SDMenu.bin
build/shareware/shareware.elf: $(SW_OBJ) $(PLAT_OBJ) $(PORT_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)
build/shareware/SDMenu.bin: build/shareware/shareware.elf tools/binToRib
	cp tools/SDMenu-template.bin $@
	tools/binToRib $@ $< sd
	cp $@ build/shareware/Wolf3D-Shareware.bin
	
# spear of destiny

SOD_FLAGS = -DVERSIONALREADYCHOSEN -DSPEAR -DCARMACIZED -DGOODTIMES
SOD_OBJ = $(WOLF_SRC:%=build/sod/%.o)

build/sod/%.o: wolf/%.cpp wolf/*.h platform/include/*.h
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(SOD_FLAGS) -c $< -o $@

sod: build/sod/SDMenu.bin
build/sod/sod.elf: $(SOD_OBJ) $(PLAT_OBJ) $(PORT_OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)
build/sod/SDMenu.bin: build/sod/sod.elf tools/binToRib
	cp tools/SDMenu-template.bin $@
	tools/binToRib $@ $< sd
	cp $@ build/sod/SpearOfDestiny.bin

clean:
	rm -rf build

.PHONY: wolf sod shareware clean
