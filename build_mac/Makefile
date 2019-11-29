include Makefile.env

LIBCUI		:= libcui.a
LIBCPU		:= libcpu.a
LIBDEVICE	:= libdevice.a
LIBBUS		:= libbus.a
LIBMPU		:= libmpu.a
LIBLOADER	:= libloader.a
LIBSTD		:= libstd.a
LIBMAIN		:= libmain.a
LIBMROS		:= libmros.a


AROBJS		:= $(LIBMAIN)
AROBJS		+= $(LIBCUI)
AROBJS		+= $(LIBCPU)
AROBJS		+= $(LIBBUS)
AROBJS		+= $(LIBMPU)
AROBJS		+= $(LIBDEVICE)
AROBJS		+= $(LIBLOADER)
AROBJS		+= $(LIBSTD)
AROBJS		+= $(LIBMROS)

all: $(AROBJS)
	$(RANLIB) $(AROBJS)
	$(GCC) -O3 $(LFLAGS) $(AROBJS) -o $(TARGET)  $(LIBS) -lm
	$(CP) $(TARGET) $(INSTALL_DIR)/
	chmod +x $(INSTALL_DIR)/$(TARGET)

build_all:
	make -f $(CORE_MKDIR)/Makefile.main all
	make -f $(CORE_MKDIR)/Makefile.cui all
	make -f $(TARGET_MKDIR)/Makefile.cpu all
	make -f $(TARGET_MKDIR)/Makefile.device all
	make -f $(CORE_MKDIR)/Makefile.bus all
	make -f $(CORE_MKDIR)/Makefile.loader all
	make -f $(CORE_MKDIR)/Makefile.mpu all
	make -f $(TARGET_MKDIR)/Makefile.std all
	make -f $(CORE_MKDIR)/Makefile.mros all
	$(GCC) -O3 $(LFLAGS) $(AROBJS) -o $(TARGET)  $(LIBS)
	$(CP) $(TARGET) $(INSTALL_DIR)/
	chmod +x $(INSTALL_DIR)/$(TARGET)

$(LIBMAIN):
	make -f $(CORE_MKDIR)/Makefile.main all

$(LIBCUI): 
	make -f $(CORE_MKDIR)/Makefile.cui all

$(LIBCPU): 
	make -f $(TARGET_MKDIR)/Makefile.cpu all

$(LIBDEVICE): 
	make -f $(TARGET_MKDIR)/Makefile.device all

$(LIBBUS): 
	make -f $(CORE_MKDIR)/Makefile.bus all

$(LIBLOADER):
	make -f $(CORE_MKDIR)/Makefile.loader all

$(LIBMPU): 
	make -f $(CORE_MKDIR)/Makefile.mpu all

$(LIBSTD):
	make -f $(TARGET_MKDIR)/Makefile.std all

$(LIBMROS):
	make -f $(CORE_MKDIR)/Makefile.mros all

clean:
	make -f $(TARGET_MKDIR)/Makefile.cpu clean
	make -f $(TARGET_MKDIR)/Makefile.device clean
	make -f $(CORE_MKDIR)/Makefile.bus clean
	make -f $(CORE_MKDIR)/Makefile.cui clean
	make -f $(CORE_MKDIR)/Makefile.loader clean
	make -f $(CORE_MKDIR)/Makefile.mpu clean
	make -f $(TARGET_MKDIR)/Makefile.std clean
	make -f $(CORE_MKDIR)/Makefile.main clean
	make -f $(CORE_MKDIR)/Makefile.mros clean
	$(RM) -f $(TARGET) 
	$(RM) -f $(INSTALL_DIR)/$(TARGET)
		
list:
	@echo $(AROBJS)

