########################################################################
# Get Linux Build Enviornment:
include ../../../build.env

LOCAL_32_BIT_ONLY := true
ENABLE_ARM_V5	:= true
ENABLE_ARM_V8	:= false

LIB_INSTALL	:=	../../lib
INC_INSTALL	:= 	../../include

######################################################################
# Build options
#CFLAGS	+= -D_LIB 
CFLAGS	+= -DMULTICORE
CFLAGS	+= -O3
CFLAGS	+= -mfpu=neon
CFLAGS	+= -fPIC

AFLAGS	+= -mfpu=neon

ifeq ($(ENABLE_ARM_V5),true)
CFLAGS += -DARMV5E -DARM_INASM -DARMV5_INASM
endif

ifeq ($(ENABLE_ARM_V8),true)
CFLAGS += -DARMV5E -DARMV7Neon -DARM_INASM -DARMV5_INASM -DARMV6_INASM
endif

LDFLAGS += -Wl,-Bsymbolic

INCLUDE += -I./ -I../../include -I$(KERNDIR)/include
INCLUDE	+= -I./common -I./decoder
INCLUDE	+= -I./common/arm -I./decoder/arm

#LIBRARY += -L../../lib/
#LIBRARY += -lnxvmem

######################################################################
# Target
COBJS  	:=
COBJS  	+= common/ihevc_quant_tables.o
COBJS  	+= common/ihevc_inter_pred_filters.o
COBJS  	+= common/ihevc_weighted_pred.o
COBJS  	+= common/ihevc_padding.o
COBJS  	+= common/ihevc_deblk_edge_filter.o
COBJS  	+= common/ihevc_deblk_tables.o
COBJS  	+= common/ihevc_cabac_tables.o
COBJS  	+= common/ihevc_common_tables.o
COBJS  	+= common/ihevc_intra_pred_filters.o
COBJS  	+= common/ihevc_chroma_intra_pred_filters.o
COBJS  	+= common/ihevc_mem_fns.o
COBJS  	+= common/ihevc_sao.o
COBJS  	+= common/ihevc_trans_tables.o
COBJS  	+= common/ihevc_recon.o
COBJS  	+= common/ihevc_itrans.o
COBJS  	+= common/ihevc_itrans_recon.o
COBJS  	+= common/ihevc_iquant_recon.o
COBJS  	+= common/ihevc_iquant_itrans_recon.o
COBJS  	+= common/ihevc_itrans_recon_32x32.o
COBJS  	+= common/ihevc_itrans_recon_16x16.o
COBJS  	+= common/ihevc_itrans_recon_8x8.o
COBJS  	+= common/ihevc_chroma_itrans_recon.o
COBJS  	+= common/ihevc_chroma_iquant_recon.o
COBJS  	+= common/ihevc_chroma_iquant_itrans_recon.o
COBJS  	+= common/ihevc_chroma_recon.o
COBJS  	+= common/ihevc_chroma_itrans_recon_16x16.o
COBJS  	+= common/ihevc_chroma_itrans_recon_8x8.o
COBJS  	+= common/ihevc_buf_mgr.o
COBJS  	+= common/ihevc_disp_mgr.o
COBJS  	+= common/ihevc_dpb_mgr.o
COBJS  	+= common/ithread.o

COBJS  	+= decoder/ihevcd_version.o
COBJS  	+= decoder/ihevcd_api.o
COBJS  	+= decoder/ihevcd_decode.o
COBJS  	+= decoder/ihevcd_nal.o
COBJS  	+= decoder/ihevcd_bitstream.o
COBJS  	+= decoder/ihevcd_parse_headers.o
COBJS  	+= decoder/ihevcd_parse_slice_header.o
COBJS  	+= decoder/ihevcd_parse_slice.o
COBJS  	+= decoder/ihevcd_parse_residual.o
COBJS  	+= decoder/ihevcd_cabac.o
COBJS  	+= decoder/ihevcd_intra_pred_mode_prediction.o
COBJS  	+= decoder/ihevcd_process_slice.o
COBJS  	+= decoder/ihevcd_utils.o
COBJS  	+= decoder/ihevcd_job_queue.o
COBJS  	+= decoder/ihevcd_ref_list.o
COBJS  	+= decoder/ihevcd_get_mv.o
COBJS  	+= decoder/ihevcd_mv_pred.o
COBJS  	+= decoder/ihevcd_mv_merge.o
COBJS  	+= decoder/ihevcd_iquant_itrans_recon_ctb.o
COBJS  	+= decoder/ihevcd_itrans_recon_dc.o
COBJS  	+= decoder/ihevcd_common_tables.o
COBJS  	+= decoder/ihevcd_boundary_strength.o
COBJS  	+= decoder/ihevcd_deblk.o
COBJS  	+= decoder/ihevcd_inter_pred.o
COBJS  	+= decoder/ihevcd_sao.o
COBJS  	+= decoder/ihevcd_ilf_padding.o
COBJS  	+= decoder/ihevcd_fmt_conv.o

COBJS  	+= decoder/arm/ihevcd_function_selector.o
COBJS  	+= decoder/arm/ihevcd_function_selector_noneon.o

CFLAGS	+= -DDISABLE_NEONINTR  -DARM -DARMGCC

LOCAL_ARM_MODE         := arm

AOBJS	:=

ifeq ($(ENABLE_ARM_V5),true)
COBJS  	+= decoder/arm/ihevcd_function_selector_a9q.o
COBJS  	+= common/arm/ihevc_intra_ref_substitution_a9q.o
COBJS  	+= common/arm/ihevc_intra_pred_filters_neon_intr.o
COBJS  	+= common/arm/ihevc_weighted_pred_neon_intr.o

AOBJS	+= common/arm/ihevc_mem_fns.o
AOBJS	+= common/arm/ihevc_itrans_recon_32x32.o
AOBJS  	+= common/arm/ihevc_weighted_pred_bi_default.o
AOBJS  	+= common/arm/ihevc_weighted_pred_bi.o
AOBJS  	+= common/arm/ihevc_weighted_pred_uni.o
AOBJS  	+= common/arm/ihevc_deblk_luma_horz.o
AOBJS  	+= common/arm/ihevc_deblk_luma_vert.o
AOBJS  	+= common/arm/ihevc_deblk_chroma_vert.o
AOBJS  	+= common/arm/ihevc_deblk_chroma_horz.o
AOBJS  	+= common/arm/ihevc_sao_band_offset_luma.o
AOBJS  	+= common/arm/ihevc_sao_band_offset_chroma.o
AOBJS  	+= common/arm/ihevc_sao_edge_offset_class0.o
AOBJS  	+= common/arm/ihevc_sao_edge_offset_class0_chroma.o
AOBJS  	+= common/arm/ihevc_sao_edge_offset_class1.o
AOBJS  	+= common/arm/ihevc_sao_edge_offset_class1_chroma.o
AOBJS  	+= common/arm/ihevc_sao_edge_offset_class2.o
AOBJS  	+= common/arm/ihevc_sao_edge_offset_class2_chroma.o
AOBJS  	+= common/arm/ihevc_sao_edge_offset_class3.o
AOBJS  	+= common/arm/ihevc_sao_edge_offset_class3_chroma.o
AOBJS  	+= common/arm/ihevc_inter_pred_luma_horz_w16out.o
AOBJS  	+= common/arm/ihevc_inter_pred_filters_luma_horz.o
AOBJS  	+= common/arm/ihevc_inter_pred_filters_luma_vert.o
AOBJS  	+= common/arm/ihevc_inter_pred_chroma_horz.o
AOBJS  	+= common/arm/ihevc_inter_pred_chroma_horz_w16out.o
AOBJS  	+= common/arm/ihevc_inter_pred_chroma_vert.o
AOBJS  	+= common/arm/ihevc_inter_pred_chroma_vert_w16out.o
AOBJS  	+= common/arm/ihevc_inter_pred_chroma_vert_w16inp.o
AOBJS  	+= common/arm/ihevc_inter_pred_chroma_vert_w16inp_w16out.o
AOBJS  	+= common/arm/ihevc_inter_pred_filters_luma_vert_w16inp.o
AOBJS  	+= common/arm/ihevc_inter_pred_luma_vert_w16inp_w16out.o
AOBJS  	+= common/arm/ihevc_inter_pred_luma_copy_w16out.o
AOBJS  	+= common/arm/ihevc_inter_pred_luma_copy.o
AOBJS  	+= common/arm/ihevc_inter_pred_chroma_copy.o
AOBJS  	+= common/arm/ihevc_inter_pred_chroma_copy_w16out.o
AOBJS  	+= common/arm/ihevc_itrans_recon_4x4_ttype1.o
AOBJS  	+= common/arm/ihevc_itrans_recon_4x4.o
AOBJS  	+= common/arm/ihevc_itrans_recon_8x8.o
AOBJS  	+= common/arm/ihevc_itrans_recon_16x16.o
AOBJS  	+= common/arm/ihevc_intra_pred_chroma_planar.o
AOBJS  	+= common/arm/ihevc_intra_pred_chroma_dc.o
AOBJS  	+= common/arm/ihevc_intra_pred_chroma_horz.o
AOBJS  	+= common/arm/ihevc_intra_pred_chroma_ver.o
AOBJS  	+= common/arm/ihevc_intra_pred_chroma_mode2.o
AOBJS  	+= common/arm/ihevc_intra_pred_chroma_mode_18_34.o
AOBJS  	+= common/arm/ihevc_intra_pred_filters_chroma_mode_11_to_17.o
AOBJS  	+= common/arm/ihevc_intra_pred_filters_chroma_mode_19_to_25.o
AOBJS  	+= common/arm/ihevc_intra_pred_chroma_mode_3_to_9.o
AOBJS  	+= common/arm/ihevc_intra_pred_chroma_mode_27_to_33.o
AOBJS  	+= common/arm/ihevc_intra_pred_luma_planar.o
AOBJS  	+= common/arm/ihevc_intra_pred_luma_horz.o
AOBJS  	+= common/arm/ihevc_intra_pred_luma_mode2.o
AOBJS  	+= common/arm/ihevc_intra_pred_luma_mode_27_to_33.o
AOBJS   += common/arm/ihevc_intra_pred_luma_mode_18_34.o
AOBJS   += common/arm/ihevc_intra_pred_luma_vert.o
AOBJS   += common/arm/ihevc_intra_pred_luma_dc.o
AOBJS   += common/arm/ihevc_intra_pred_filters_luma_mode_11_to_17.o
AOBJS   += common/arm/ihevc_intra_pred_filters_luma_mode_19_to_25.o
AOBJS   += common/arm/ihevc_intra_pred_luma_mode_3_to_9.o
AOBJS   += common/arm/ihevc_padding.o

AOBJS   += decoder/arm/ihevcd_itrans_recon_dc_luma.o
AOBJS   += decoder/arm/ihevcd_itrans_recon_dc_chroma.o
AOBJS   += decoder/arm/ihevcd_fmt_conv_420sp_to_420p.o
AOBJS   += decoder/arm/ihevcd_fmt_conv_420sp_to_420sp.o
AOBJS   += decoder/arm/ihevcd_fmt_conv_420sp_to_rgba8888.o
CFLAGS	+= -DDEFAULT_ARCH=D_ARCH_ARM_A9Q
else
CFLAGS	+= -DDISABLE_NEON -DDEFAULT_ARCH=D_ARCH_ARM_NONEON
endif

ifeq ($(ENABLE_ARM_V8),true)
INCLUDE += -I./common/arm64
INCLUDE += -I./decoder/arm64

CFLAGS	+= -DARMV8 -DDISABLE_NEONINTR  -DARM -DARMGCC

OBJS    +=  decoder/arm/ihevcd_function_selector.o
OBJS    +=  decoder/arm/ihevcd_function_selector_noneon.o
OBJS    +=  decoder/arm64/ihevcd_function_selector_av8.o
OBJS    +=  common/arm/ihevc_intra_pred_filters_neon_intr.o
OBJS    +=  common/arm/ihevc_weighted_pred_neon_intr.o

AOBJS    +=  common/arm64/ihevc_mem_fns.o
AOBJS    +=  common/arm64/ihevc_itrans_recon_32x32.o
AOBJS    +=  common/arm64/ihevc_weighted_pred_bi_default.o
AOBJS    +=  common/arm64/ihevc_weighted_pred_bi.o
AOBJS    +=  common/arm64/ihevc_weighted_pred_uni.o
AOBJS    +=  common/arm64/ihevc_deblk_luma_horz.o
AOBJS    +=  common/arm64/ihevc_deblk_luma_vert.o
AOBJS    +=  common/arm64/ihevc_deblk_chroma_vert.o
AOBJS    +=  common/arm64/ihevc_deblk_chroma_horz.o
AOBJS    +=  common/arm64/ihevc_sao_band_offset_luma.o
AOBJS    +=  common/arm64/ihevc_sao_band_offset_chroma.o
AOBJS    +=  common/arm64/ihevc_sao_edge_offset_class0.o
AOBJS    +=  common/arm64/ihevc_sao_edge_offset_class0_chroma.o
AOBJS    +=  common/arm64/ihevc_sao_edge_offset_class1.o
AOBJS    +=  common/arm64/ihevc_sao_edge_offset_class1_chroma.o
AOBJS    +=  common/arm64/ihevc_sao_edge_offset_class2.o
AOBJS    +=  common/arm64/ihevc_sao_edge_offset_class2_chroma.o
AOBJS    +=  common/arm64/ihevc_sao_edge_offset_class3.o
AOBJS    +=  common/arm64/ihevc_sao_edge_offset_class3_chroma.o
AOBJS    +=  common/arm64/ihevc_inter_pred_luma_horz_w16out.o
AOBJS    +=  common/arm64/ihevc_inter_pred_filters_luma_horz.o
AOBJS    +=  common/arm64/ihevc_inter_pred_filters_luma_vert.o
AOBJS    +=  common/arm64/ihevc_inter_pred_chroma_horz.o
AOBJS    +=  common/arm64/ihevc_inter_pred_chroma_horz_w16out.o
AOBJS    +=  common/arm64/ihevc_inter_pred_chroma_vert.o
AOBJS    +=  common/arm64/ihevc_inter_pred_chroma_vert_w16out.o
AOBJS    +=  common/arm64/ihevc_inter_pred_chroma_vert_w16inp.o
AOBJS    +=  common/arm64/ihevc_inter_pred_chroma_vert_w16inp_w16out.o
AOBJS    +=  common/arm64/ihevc_inter_pred_filters_luma_vert_w16inp.o
AOBJS    +=  common/arm64/ihevc_inter_pred_filters_luma_vert_w16out.o
AOBJS    +=  common/arm64/ihevc_inter_pred_luma_vert_w16inp_w16out.o
AOBJS    +=  common/arm64/ihevc_inter_pred_luma_copy_w16out.o
AOBJS    +=  common/arm64/ihevc_inter_pred_luma_copy.o
AOBJS    +=  common/arm64/ihevc_inter_pred_chroma_copy.o
AOBJS    +=  common/arm64/ihevc_inter_pred_chroma_copy_w16out.o
AOBJS    +=  common/arm64/ihevc_itrans_recon_4x4_ttype1.o
AOBJS    +=  common/arm64/ihevc_itrans_recon_4x4.o
AOBJS    +=  common/arm64/ihevc_itrans_recon_8x8.o
AOBJS    +=  common/arm64/ihevc_itrans_recon_16x16.o
AOBJS    +=  common/arm64/ihevc_intra_pred_chroma_planar.o
AOBJS    +=  common/arm64/ihevc_intra_pred_chroma_dc.o
AOBJS    +=  common/arm64/ihevc_intra_pred_chroma_horz.o
AOBJS    +=  common/arm64/ihevc_intra_pred_chroma_ver.o
AOBJS    +=  common/arm64/ihevc_intra_pred_chroma_mode2.o
AOBJS    +=  common/arm64/ihevc_intra_pred_chroma_mode_18_34.o
AOBJS    +=  common/arm64/ihevc_intra_pred_filters_chroma_mode_11_to_17.o
AOBJS    +=  common/arm64/ihevc_intra_pred_filters_chroma_mode_19_to_25.o
AOBJS    +=  common/arm64/ihevc_intra_pred_chroma_mode_3_to_9.o
AOBJS    +=  common/arm64/ihevc_intra_pred_chroma_mode_27_to_33.o
AOBJS    +=  common/arm64/ihevc_intra_pred_luma_planar.o
AOBJS    +=  common/arm64/ihevc_intra_pred_luma_horz.o
AOBJS    +=  common/arm64/ihevc_intra_pred_luma_mode2.o
AOBJS    +=  common/arm64/ihevc_intra_pred_luma_mode_27_to_33.o
AOBJS    +=  common/arm64/ihevc_intra_pred_luma_mode_18_34.o
AOBJS    +=  common/arm64/ihevc_intra_pred_luma_vert.o
AOBJS    +=  common/arm64/ihevc_intra_pred_luma_dc.o
AOBJS    +=  common/arm64/ihevc_intra_pred_filters_luma_mode_11_to_17.o
AOBJS    +=  common/arm64/ihevc_intra_pred_filters_luma_mode_19_to_25.o
AOBJS    +=  common/arm64/ihevc_intra_pred_luma_mode_3_to_9.o
AOBJS    +=  common/arm64/ihevc_padding.o
AOBJS    +=  decoder/arm64/ihevcd_itrans_recon_dc_luma.o
AOBJS    +=  decoder/arm64/ihevcd_itrans_recon_dc_chroma.o
AOBJS    +=  decoder/arm64/ihevcd_fmt_conv_420sp_to_420p.o
AOBJS    +=  decoder/arm64/ihevcd_fmt_conv_420sp_to_420sp.o
AOBJS    +=  decoder/arm64/ihevcd_fmt_conv_420sp_to_rgba8888.o

CFLAGS	+= -DDEFAULT_ARCH=D_ARCH_ARMV8_GENERIC
endif

LIBNAME	:= libhevcdec
TARGET  := $(LIBNAME).so

######################################################################
# Build
OBJS	:= $(COBJS) $(CPPOBJS) $(AOBJS)

all: $(TARGET)

$(TARGET):	depend $(OBJS)
	$(AR) $(ARFLAGS) $(LIBNAME).a $(OBJS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$(TARGET) -o $@ $(OBJS) $(LIBRARY)

install: 
	install -m 755 -d $(LIB_INSTALL)
	install -m 644 $(LIBNAME).a $(LIB_INSTALL)
	install -m 644 $(TARGET) $(LIB_INSTALL)

clean:
	@find $(OBJTREE) -type f \
		\( -name '*.o'  -o -name $(TARGET) -o -name '*.a' -o -name '*.so*' -o \
		-name '*.bin' -o -name '*.map' -o -name 'core' -o -name '*~' -o -name \
		'.depend' \) -print | xargs rm -f

distclean: clean
	rm -f $(LIB_INSTALL)/$(TARGET)
	rm -f $(LIB_INSTALL)/$(LIBNAME).a


#########################################################################
# Dependency
ifeq (.depend,$(wildcard .depend))
include .depend
endif

SRCS := $(AOBJS:.o=.s) $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
INCS := $(INCLUDE)
depend dep:
	$(CC) -M $(CFLAGS) $(INCS) $(SRCS) > .depend  
