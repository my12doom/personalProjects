# A simple test for the minimal standard C++ library
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

FF_COMMON_SRC := \
    libavcodec/8bps.c \
    libavcodec/aac_ac3_parser.c \
    libavcodec/aac_parser.c \
    libavcodec/aacadtsdec.c \
    libavcodec/aacdec.c \
    libavcodec/aacps.c \
    libavcodec/aacsbr.c \
    libavcodec/aactab.c \
    libavcodec/ac3.c \
    libavcodec/ac3_parser.c \
    libavcodec/ac3dec.c \
    libavcodec/ac3dec_data.c \
    libavcodec/ac3dsp.c \
    libavcodec/ac3tab.c \
    libavcodec/acelp_filters.c \
    libavcodec/acelp_vectors.c \
    libavcodec/adpcm.c \
    libavcodec/adpcm_data.c \
    libavcodec/adx.c \
    libavcodec/adxdec.c \
    libavcodec/alac.c \
    libavcodec/allcodecs.c \
    libavcodec/amrnbdec.c \
    libavcodec/amrwbdec.c \
    libavcodec/anm.c \
    libavcodec/ansi.c \
    libavcodec/sbrdsp.c \
    libavcodec/arm/ac3dsp_arm.S \
    libavcodec/arm/ac3dsp_armv6.S \
    libavcodec/arm/ac3dsp_init_arm.c \
    libavcodec/arm/dcadsp_init_arm.c \
    libavcodec/arm/dsputil_arm.S \
    libavcodec/arm/dsputil_armv6.S \
    libavcodec/arm/dsputil_init_arm.c \
    libavcodec/arm/dsputil_init_armv5te.c \
    libavcodec/arm/fmtconvert_vfp.S \
    libavcodec/arm/dsputil_vfp.S \
    libavcodec/arm/dsputil_init_vfp.c \
    libavcodec/arm/fmtconvert_init_arm.c \
    libavcodec/arm/dsputil_init_armv6.c \
    libavcodec/arm/fft_fixed_init_arm.c \
    libavcodec/arm/fft_init_arm.c \
    libavcodec/arm/fmtconvert_init_arm.c \
    libavcodec/arm/h264dsp_init_arm.c \
    libavcodec/arm/h264pred_init_arm.c \
    libavcodec/arm/jrevdct_arm.S \
    libavcodec/arm/mpegvideo_arm.c \
    libavcodec/arm/mpegvideo_armv5te.c \
    libavcodec/arm/mpegvideo_armv5te_s.S \
    libavcodec/arm/simple_idct_arm.S \
    libavcodec/arm/simple_idct_armv5te.S \
    libavcodec/arm/simple_idct_armv6.S \
    libavcodec/arm/vp56dsp_init_arm.c \
    libavcodec/arm/mpegaudiodsp_init_arm.c \
    libavcodec/arm/mpegaudiodsp_fixed_armv6.S \
    libavcodec/arm/sbrdsp_init_arm.c \
    libavcodec/arm/vp8_armv6.S \
    libavcodec/arm/vp8dsp_init_arm.c \
    libavcodec/arm/vp8dsp_armv6.S \
    libavcodec/ass.c \
    libavcodec/ass_split.c \
    libavcodec/assdec.c \
    libavcodec/atrac.c \
    libavcodec/atrac1.c \
    libavcodec/atrac3.c \
    libavcodec/audioconvert.c \
    libavcodec/avpacket.c \
    libavcodec/acelp_pitch_delay.c \
    libavcodec/bitstream.c \
    libavcodec/bitstream_filter.c \
    libavcodec/cabac.c \
    libavcodec/celp_filters.c \
    libavcodec/celp_math.c \
    libavcodec/cga_data.c \
    libavcodec/cook.c \
    libavcodec/libstagefright.cpp \
    libavcodec/dca.c \
    libavcodec/dca_parser.c \
    libavcodec/dcadsp.c \
    libavcodec/dct.c \
    libavcodec/dirac_parser.c \
    libavcodec/dpcm.c \
    libavcodec/dpx.c \
    libavcodec/dsicinav.c \
    libavcodec/dsputil.c \
    libavcodec/dv.c \
    libavcodec/dvbsub_parser.c \
    libavcodec/dvbsubdec.c \
    libavcodec/dvdata.c \
    libavcodec/dvdsub_parser.c \
    libavcodec/dvdsubdec.c \
    libavcodec/eac3dec.c \
    libavcodec/eaidct.c \
    libavcodec/error_resilience.c \
    libavcodec/faanidct.c \
    libavcodec/faxcompr.c \
    libavcodec/fft_fixed.c \
    libavcodec/fft_float.c \
    libavcodec/flashsv.c \
    libavcodec/flvdec.c \
    libavcodec/fmtconvert.c \
    libavcodec/g726.c \
    libavcodec/golomb.c \
    libavcodec/gsmdec.c \
    libavcodec/gsmdec_data.c \
    libavcodec/h263.c \
    libavcodec/h263_parser.c \
    libavcodec/h263dec.c \
    libavcodec/h264.c \
    libavcodec/h264_cabac.c \
    libavcodec/h264_cavlc.c \
    libavcodec/h264_direct.c \
    libavcodec/h264_loopfilter.c \
    libavcodec/h264_parser.c \
    libavcodec/h264_ps.c \
    libavcodec/h264_refs.c \
    libavcodec/h264_sei.c \
    libavcodec/h264dsp.c \
    libavcodec/h264idct.c \
    libavcodec/h264pred.c \
    libavcodec/huffman.c \
    libavcodec/imgconvert.c \
    libavcodec/intrax8.c \
    libavcodec/intrax8dsp.c \
    libavcodec/ituh263dec.c \
    libavcodec/jrevdct.c \
    libavcodec/kbdwin.c \
    libavcodec/latm_parser.c \
    libavcodec/lsp.c \
    libavcodec/mdct_fixed.c \
    libavcodec/mdct_float.c \
    libavcodec/mimic.c \
    libavcodec/mpeg12.c \
    libavcodec/mpeg12data.c \
    libavcodec/mpeg4audio.c \
    libavcodec/mpeg4video.c \
    libavcodec/mpeg4video_parser.c \
    libavcodec/mpeg4videodec.c \
    libavcodec/mpegaudio.c \
    libavcodec/mpegaudio_parser.c \
    libavcodec/mpegaudiodata.c \
    libavcodec/mpegaudiodec.c \
    libavcodec/mpegaudiodec_float.c \
    libavcodec/mpegaudiodecheader.c \
    libavcodec/mpegvideo.c \
    libavcodec/mpegvideo_parser.c \
    libavcodec/msgsmdec.c \
    libavcodec/msmpeg4.c \
    libavcodec/msmpeg4data.c \
    libavcodec/nellymoser.c \
    libavcodec/nellymoserdec.c \
    libavcodec/options.c \
    libavcodec/parser.c \
    libavcodec/pcm-mpeg.c \
    libavcodec/pcm.c \
    libavcodec/pgssubdec.c \
    libavcodec/png.c \
    libavcodec/pngdec.c \
    libavcodec/pnm.c \
    libavcodec/pnm_parser.c \
    libavcodec/pnmdec.c \
    libavcodec/pthread.c \
    libavcodec/qcelpdec.c \
    libavcodec/qdm2.c \
    libavcodec/qdrw.c \
    libavcodec/qpeg.c \
    libavcodec/r210dec.c \
    libavcodec/ra144.c \
    libavcodec/ra144dec.c \
    libavcodec/ra288.c \
    libavcodec/rangecoder.c \
    libavcodec/raw.c \
    libavcodec/rawdec.c \
    libavcodec/rdft.c \
    libavcodec/resample.c \
    libavcodec/resample2.c \
    libavcodec/rpza.c \
    libavcodec/rv10.c \
    libavcodec/rv30.c \
    libavcodec/rv30dsp.c \
    libavcodec/rv34.c \
    libavcodec/rv34dsp.c \
    libavcodec/rv40.c \
    libavcodec/rv40dsp.c \
    libavcodec/svq1.c \
    libavcodec/svq1dec.c \
    libavcodec/svq3.c \
    libavcodec/sgidec.c \
    libavcodec/simple_idct.c \
    libavcodec/sinewin.c \
    libavcodec/sipr.c \
    libavcodec/sipr16k.c \
    libavcodec/sonic.c \
    libavcodec/srtdec.c \
    libavcodec/synth_filter.c \
    libavcodec/tta.c \
    libavcodec/ulti.c \
    libavcodec/utils.c \
    libavcodec/v210dec.c \
    libavcodec/v210x.c \
    libavcodec/vc1.c \
    libavcodec/vc1_parser.c \
    libavcodec/vc1data.c \
    libavcodec/vc1dec.c \
    libavcodec/vc1dsp.c \
    libavcodec/vcr1.c \
    libavcodec/vorbis.c \
    libavcodec/vorbis_data.c \
    libavcodec/vorbisdec.c \
    libavcodec/vp3.c \
    libavcodec/vp3_parser.c \
    libavcodec/vp3dsp.c \
    libavcodec/vp5.c \
    libavcodec/vp56.c \
    libavcodec/vp56data.c \
    libavcodec/vp56dsp.c \
    libavcodec/vp56rac.c \
    libavcodec/vp6.c \
    libavcodec/vp6dsp.c \
    libavcodec/vp8.c \
    libavcodec/vp8_parser.c \
    libavcodec/vp8dsp.c \
    libavcodec/wavpack.c \
    libavcodec/wma.c \
    libavcodec/wma_common.c \
    libavcodec/wmadec.c \
    libavcodec/wmaprodec.c \
    libavcodec/wmavoice.c \
    libavcodec/wmv2.c \
    libavcodec/wmv2dec.c \
    libavcodec/xiph.c \
    libavcodec/xsubdec.c \
    libavcodec/bit_depth_template.c \
    libavcodec/cos_tablegen.c \
    libavcodec/dct32_fixed.c \
    libavcodec/dct32_float.c \
    libavcodec/dsputil_template.c \
    libavcodec/eac3_data.c \
    libavcodec/mpegaudiodsp.c \
    libavcodec/mpegaudiodsp_fixed.c \
    libavcodec/mpegaudiodsp_float.c \
    libavcodec/mqc.c \
    libavcodec/mqcdec.c \
    libavcodec/mqcenc.c \
    libavcodec/rv34_parser.c \
    libavcodec/s302m.c \
    libavcodec/timecode.c \
    libavutil/adler32.c \
    libavutil/aes.c \
    libavutil/audioconvert.c \
    libavutil/avstring.c \
    libavutil/base64.c \
    libavutil/cpu.c \
    libavutil/crc.c \
    libavutil/des.c \
    libavutil/error.c \
    libavutil/eval.c \
    libavutil/fifo.c \
    libavutil/file.c \
    libavutil/imgutils.c \
    libavutil/intfloat_readwrite.c \
    libavutil/inverse.c \
    libavutil/lfg.c \
    libavutil/lls.c \
    libavutil/log.c \
    libavutil/lzo.c \
    libavutil/mathematics.c \
    libavutil/md5.c \
    libavutil/mem.c \
    libavutil/dict.c \
    libavutil/opt.c \
    libavutil/parseutils.c \
    libavutil/pixdesc.c \
    libavutil/random_seed.c \
    libavutil/rational.c \
    libavutil/rc4.c \
    libavutil/samplefmt.c \
    libavutil/sha.c \
    libavutil/tree.c \
    libavutil/utils.c \
    libavutil/timecode.c
 
FF_COMMON_SRC += \
    libavformat/allformats.c \
    libavformat/asf.c \
    libavformat/asfcrypt.c \
    libavformat/asfdec.c \
    libavformat/avidec.c \
    libavformat/avio.c \
    libavformat/aviobuf.c \
    libavformat/avlanguage.c \
    libavformat/concat.c \
    libavformat/crypto.c \
    libavformat/cutils.c \
    libavformat/dv.c \
    libavformat/flvdec.c \
    libavformat/file.c \
    libavformat/http.c \
    libavformat/httpauth.c \
    libavformat/isom.c \
    libavformat/m4vdec.c \
    libavformat/matroska.c \
    libavformat/matroskadec.c \
    libavformat/mov.c \
    libavformat/mov_chan.c \
    libavformat/mpeg.c \
    libavformat/mpegts.c \
    libavformat/mpegvideodec.c \
    libavformat/options.c \
    libavformat/os_support.c \
    libavformat/riff.c \
    libavformat/rm.c \
    libavformat/rmdec.c \
    libavformat/rawdec.c \
    libavformat/id3v1.c \
    libavformat/id3v2.c \
    libavformat/seek.c \
    libavformat/swfdec.c \
    libavformat/tcp.c \
    libavformat/utils.c \
    libavformat/network.c \
    libavformat/metadata.c 


FF_ARM_NEON_SRC := \
    libavcodec/arm/rv34dsp_init_neon.c \
    libavcodec/arm/rv34dsp_neon.S \
    libavcodec/arm/rv40dsp_init_neon.c \
    libavcodec/arm/rv40dsp_neon.S \
    libavcodec/arm/dsputil_init_neon.c \
    libavcodec/arm/dsputil_neon.S \
    libavcodec/arm/fmtconvert_neon.S \
    libavcodec/arm/int_neon.S \
    libavcodec/arm/mpegvideo_neon.S \
    libavcodec/arm/simple_idct_neon.S \
    libavcodec/arm/fft_neon.S \
    libavcodec/arm/fft_fixed_neon.S \
    libavcodec/arm/mdct_neon.S \
    libavcodec/arm/mdct_fixed_neon.S \
    libavcodec/arm/rdft_neon.S \
    libavcodec/arm/h264dsp_neon.S \
    libavcodec/arm/h264idct_neon.S \
    libavcodec/arm/h264pred_neon.S \
    libavcodec/arm/h264cmc_neon.S \
    libavcodec/arm/ac3dsp_neon.S \
    libavcodec/arm/dcadsp_neon.S \
    libavcodec/arm/synth_filter_neon.S \
    libavcodec/arm/vp3dsp_neon.S \
    libavcodec/arm/vp56dsp_neon.S \
    libavcodec/arm/sbrdsp_neon.S \
    libavcodec/arm/vp8dsp_neon.S

LOCAL_LDLIBS := -lz
LOCAL_ARM_MODE := arm
ifeq ($(BUILD_WITH_NEON),1)
LOCAL_ARM_NEON := true
endif
LOCAL_MODULE := avcodec


LOCAL_SRC_FILES :=  $(FF_COMMON_SRC)
#LOCAL_C_INCLUDES +=  $(LOCAL_PATH)/include
LOCAL_CFLAGS += -DHAVE_ARMV6=1
LOCAL_CFLAGS += -DHAVE_AV_CONFIG_H=1

ifeq ($(BUILD_WITH_NEON),1)
LOCAL_CFLAGS += -DHAVE_NEON=1
LOCAL_CFLAGS += -DHAVE_VFPV3=1
LOCAL_SRC_FILES += $(FF_ARM_NEON_SRC)
else
LOCAL_CFLAGS += -DHAVE_NEON=0
LOCAL_CFLAGS += -DHAVE_VFPV3=1
endif

ifeq ($(BUILD_WITH_NEON),1)
LOCAL_STATIC_LIBRARIES += arm_neon
endif

include $(BUILD_STATIC_LIBRARY)
