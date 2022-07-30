CC = cl

CFLAGS =
LFLAGS = 

SOURCE = src/*.c
ASMIN = src/asm/utils.asm

BIN_FOLDER = bin/

ASMOUT = $(BIN_FOLDER)asm_utils.obj
X64 = $(BIN_FOLDER)DeathSleep.exe

all: clean $(ASMOUT) $(X64) fix_epilogue clean_obj

$(ASMOUT):
	nasm -f win64 $(ASMIN) -o $@

$(X64):
	$(CC) $(SOURCE) $(ASMOUT) /Fo:$(BIN_FOLDER) /Fe: $@ $(CFLAGS) $(LFLAGS) 

fix_epilogue:
	python3 scripts/fixEpilogueSize.py -f $(X64)

clean:
	del /q bin\*

clean_obj:
	del /q bin\*.obj