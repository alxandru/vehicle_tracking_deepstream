CUDA_VER?=
ifeq ($(CUDA_VER),)
  $(error "CUDA_VER is not set")
endif

BIN=./bin/
SOURCE=./src/

APP:= vehicle-tracking-deepstream

TARGET_DEVICE = $(shell gcc -dumpmachine | cut -f1 -d -)

NVDS_VERSION:=6.0

LIB_INSTALL_DIR?=/opt/nvidia/deepstream/deepstream-$(NVDS_VERSION)/lib/

ifeq ($(TARGET_DEVICE),aarch64)
  CFLAGS:= -DPLATFORM_TEGRA
endif

SRCS:= $(wildcard $(SOURCE)*.cpp)

INCS:= $(wildcard *.h)

PKGS:= gstreamer-1.0

OBJS:= $(SRCS:.cpp=.o)

CFLAGS+= -I/opt/nvidia/deepstream/deepstream-$(NVDS_VERSION)/sources/includes \
		-I /usr/local/cuda-$(CUDA_VER)/include

CFLAGS+= $(shell pkg-config --cflags $(PKGS))

LIBS:= $(shell pkg-config --libs $(PKGS))

LIBS+= -L/usr/local/cuda-$(CUDA_VER)/lib64/ -lcudart \
		-L$(LIB_INSTALL_DIR) -lnvdsgst_meta -lnvds_meta \
		-Wl,-rpath,$(LIB_INSTALL_DIR)

all: $(BIN)$(APP)

%.o: %.cpp $(INCS) Makefile
	$(CXX) -c -o $@ $(CFLAGS) $<

$(BIN)$(APP): $(OBJS) Makefile
	$(CXX) -o $@ $(OBJS) $(LIBS)

clean:
	rm -rf $(OBJS) $(BIN)$(APP)
	$(MAKE) -C 3pp/DeepStream-Yolo/nvdsinfer_custom_impl_Yolo clean

subsystem:
	$(MAKE) -C 3pp/DeepStream-Yolo/nvdsinfer_custom_impl_Yolo

