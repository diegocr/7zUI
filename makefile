PROG	= 7zUI
CC	= m68k-amigaos-gcc $(CPU) -noixemul  -msmall-code
LIB	= -Wl,-Map,$@.map,--cref -lmui -lamiga -linox
DEFINES	= -D_7zSDK_VERSION='"4.43"' -D_7zUI_VERSION='"0.1"' -DDEBUG
WARNS	= -Wall -W -Winline
CFLAGS	= -O3 -funroll-loops -fomit-frame-pointer $(WARNS) $(DEFINES)
LDFLAGS	= -s -nostdlib
OBJDIR	= .objs$(SYS)
RM	= rm -frv

OBJS =	\
	$(OBJDIR)/startup.o	\
	$(OBJDIR)/swapstack.o	\
	$(OBJDIR)/7zAlloc.o	\
	$(OBJDIR)/7zBuffer.o	\
	$(OBJDIR)/7zCrc.o	\
	$(OBJDIR)/7zDebug.o	\
	$(OBJDIR)/7zDecode.o	\
	$(OBJDIR)/7zExtract.o	\
	$(OBJDIR)/7zHeader.o	\
	$(OBJDIR)/7zIn.o	\
	$(OBJDIR)/7zItem.o	\
	$(OBJDIR)/7zMain.o	\
	$(OBJDIR)/7zMethodID.o	\
	$(OBJDIR)/7zTime.o	\
	$(OBJDIR)/7zUI.o	\
	$(OBJDIR)/7zUtils.o	\
	$(OBJDIR)/LzmaDecode.o

all:
	make $(PROG)_060 SYS=_060 CPU="-m68060 -m68881"
	make $(PROG)_020 SYS=_020 CPU=-m68020

$(PROG)$(SYS): $(OBJDIR) $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS) $(LIB)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR)/%.o: %.c
	@echo Compiling $@
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(PROG)_0*0 $(OBJDIR)_0*0

