/**
 * SECTION:element-viperfx
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch audiotestsrc ! audioconverter ! viperfx ! ! audioconverter ! autoaudiosink
 * ]|
 * </refsect2>
 */

#define PACKAGE "viperfx-plugin"
#define VERSION "1.0.0"

#include <string.h>
#include <gst/gst.h>
#include <gst/base/base.h>
#include <gst/base/gstbasetransform.h>
#include <gst/audio/audio.h>
#include <gst/audio/gstaudiofilter.h>
#include <gst/controller/controller.h>

#include "gstviperfx.h"

GST_DEBUG_CATEGORY_STATIC (gst_viperfx_debug);
#define GST_CAT_DEFAULT gst_viperfx_debug

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,

  /* global enable */
  PROP_FX_ENABLE,
  /* convolver */
  PROP_CONV_ENABLE,
  PROP_CONV_IR_PATH,
  PROP_CONV_CC_LEVEL,
  /* vhe */
  PROP_VHE_ENABLE,
  PROP_VHE_LEVEL,
  /* vse */
  PROP_VSE_ENABLE,
  PROP_VSE_REF_BARK,
  PROP_VSE_BARK_CONS,
  /* equalizer */
  PROP_EQ_ENABLE,
  PROP_EQ_BAND1,
  PROP_EQ_BAND2,
  PROP_EQ_BAND3,
  PROP_EQ_BAND4,
  PROP_EQ_BAND5,
  PROP_EQ_BAND6,
  PROP_EQ_BAND7,
  PROP_EQ_BAND8,
  PROP_EQ_BAND9,
  PROP_EQ_BAND10,
  /* colorful music */
  PROP_COLM_ENABLE,
  PROP_COLM_WIDENING,
  PROP_COLM_MIDIMAGE,
  PROP_COLM_DEPTH,
  /* diff sur */
  PROP_DS_ENABLE,
  PROP_DS_LEVEL,
  /* reverb */
  PROP_REVERB_ENABLE,
  PROP_REVERB_ROOMSIZE,
  PROP_REVERB_WIDTH,
  PROP_REVERB_DAMP,
  PROP_REVERB_WET,
  PROP_REVERB_DRY,
  /* agc */
  PROP_AGC_ENABLE,
  PROP_AGC_RATIO,
  PROP_AGC_VOLUME,
  PROP_AGC_MAXGAIN,
  /* viper bass */
  PROP_VB_ENABLE,
  PROP_VB_MODE,
  PROP_VB_FREQ,
  PROP_VB_GAIN,
  /* viper clarity */
  PROP_VC_ENABLE,
  PROP_VC_MODE,
  PROP_VC_LEVEL,
  /* cure */
  PROP_CURE_ENABLE,
  PROP_CURE_LEVEL,
  /* tube */
  PROP_TUBE_ENABLE,
  /* analog-x */
  PROP_AX_ENABLE,
  PROP_AX_MODE,
  /* fet compressor */
  PROP_FETCOMP_ENABLE,
  PROP_FETCOMP_THRESHOLD,
  PROP_FETCOMP_RATIO,
  PROP_FETCOMP_KNEEWIDTH,
  PROP_FETCOMP_AUTOKNEE,
  PROP_FETCOMP_GAIN,
  PROP_FETCOMP_AUTOGAIN,
  PROP_FETCOMP_ATTACK,
  PROP_FETCOMP_AUTOATTACK,
  PROP_FETCOMP_RELEASE,
  PROP_FETCOMP_AUTORELEASE,
  PROP_FETCOMP_META_KNEEMULTI,
  PROP_FETCOMP_META_MAXATTACK,
  PROP_FETCOMP_META_MAXRELEASE,
  PROP_FETCOMP_META_CREST,
  PROP_FETCOMP_META_ADAPT,
  PROP_FETCOMP_NOCLIP,
  /* output volume */
  PROP_OUT_VOLUME,
  /* output pan */
  PROP_OUT_PAN,
  /* limiter */
  PROP_LIM_THRESHOLD
};

#define ALLOWED_CAPS \
  "audio/x-raw,"                            \
  " format=(string){"GST_AUDIO_NE(S16)"},"  \
  " rate=(int)[44100,MAX],"                 \
  " channels=(int)2,"                       \
  " layout=(string)interleaved"

#define gst_viperfx_parent_class parent_class
G_DEFINE_TYPE (Gstviperfx, gst_viperfx, GST_TYPE_AUDIO_FILTER);

static void gst_viperfx_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_viperfx_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_viperfx_finalize (GObject * object);

static gboolean gst_viperfx_setup (GstAudioFilter * self,
    const GstAudioInfo * info);
static gboolean gst_viperfx_stop (GstBaseTransform * base);
static GstFlowReturn gst_viperfx_transform_ip (GstBaseTransform * base,
    GstBuffer * outbuf);

/* GObject vmethod implementations */

/* initialize the viperfx's class */
static void
gst_viperfx_class_init (GstviperfxClass * klass)
{
  GObjectClass *gobject_class = (GObjectClass *) klass;
  GstElementClass *gstelement_class = (GstElementClass *) klass;
  GstBaseTransformClass *basetransform_class = (GstBaseTransformClass *) klass;
  GstAudioFilterClass *audioself_class = (GstAudioFilterClass *) klass;
  GstCaps *caps;

  /* debug category for fltering log messages
   */
  GST_DEBUG_CATEGORY_INIT (gst_viperfx_debug, "viperfx", 0, "viperfx element");

  gobject_class->set_property = gst_viperfx_set_property;
  gobject_class->get_property = gst_viperfx_get_property;
  gobject_class->finalize = gst_viperfx_finalize;

  /* global switch */
  g_object_class_install_property (gobject_class, PROP_FX_ENABLE,
      g_param_spec_boolean ("fx_enable", "FXEnabled", "Enable viperfx processing",
          FALSE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  /* convolver */
  g_object_class_install_property (gobject_class, PROP_CONV_ENABLE,
      g_param_spec_boolean ("conv_enable", "ConvEnabled", "Enable convolver",
          FALSE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_CONV_IR_PATH,
      g_param_spec_string ("conv_ir_path", "ConvIRPath", "Impulse response file path",
          "", G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_CONV_CC_LEVEL,
      g_param_spec_int ("conv_cc_level", "ConvEnabled", "Cross-channel level (percent)",
          0, 100, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  /* vhe */
  g_object_class_install_property (gobject_class, PROP_VHE_ENABLE,
      g_param_spec_boolean ("vhe_enable", "VHEEnabled", "Enable viper headphone engine",
          FALSE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_VHE_LEVEL,
      g_param_spec_int ("vhe_level", "VHELevel", "VHE level",
          0, 4, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  /* vse */
  g_object_class_install_property (gobject_class, PROP_VSE_ENABLE,
      g_param_spec_boolean ("vse_enable", "VSEEnabled", "Enable viper spectrum extend",
          FALSE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_VSE_REF_BARK,
      g_param_spec_int ("vse_ref_bark", "VSERefBark", "VSE reference bark frequency",
          800, 20000, 7600, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_VSE_BARK_CONS,
      g_param_spec_int ("vse_bark_cons", "VSEBarkCons", "VSE bark reconstruct level",
          10, 100, 10, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  /* equalizer */
  g_object_class_install_property (gobject_class, PROP_EQ_ENABLE,
      g_param_spec_boolean ("eq_enable", "EQEnable", "Enable FIR linear equalizer",
          FALSE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_EQ_BAND1,
      g_param_spec_int ("eq_band1", "EQBand1", "Gain of eq band 1",
          -1200, 1200, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_EQ_BAND2,
      g_param_spec_int ("eq_band2", "EQBand2", "Gain of eq band 2",
          -1200, 1200, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_EQ_BAND3,
      g_param_spec_int ("eq_band3", "EQBand3", "Gain of eq band 3",
          -1200, 1200, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_EQ_BAND4,
      g_param_spec_int ("eq_band4", "EQBand4", "Gain of eq band 4",
          -1200, 1200, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_EQ_BAND5,
      g_param_spec_int ("eq_band5", "EQBand5", "Gain of eq band 5",
          -1200, 1200, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_EQ_BAND6,
      g_param_spec_int ("eq_band6", "EQBand6", "Gain of eq band 6",
          -1200, 1200, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_EQ_BAND7,
      g_param_spec_int ("eq_band7", "EQBand7", "Gain of eq band 7",
          -1200, 1200, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_EQ_BAND8,
      g_param_spec_int ("eq_band8", "EQBand8", "Gain of eq band 8",
          -1200, 1200, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_EQ_BAND9,
      g_param_spec_int ("eq_band9", "EQBand9", "Gain of eq band 9",
          -1200, 1200, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_EQ_BAND10,
      g_param_spec_int ("eq_band10", "EQBand10", "Gain of eq band 10",
          -1200, 1200, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  /* colorful music */
  g_object_class_install_property (gobject_class, PROP_COLM_ENABLE,
      g_param_spec_boolean ("colm_enable", "COLMEnabled", "Enable colorful music",
          FALSE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_COLM_WIDENING,
      g_param_spec_int ("colm_widening", "COLMWidening", "Widening of colorful music",
          0, 800, 100, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_COLM_MIDIMAGE,
      g_param_spec_int ("colm_midimage", "COLMMidImage", "Mid-image of colorful music",
          0, 800, 100, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_COLM_DEPTH,
      g_param_spec_int ("colm_depth", "COLMDepth", "Depth of colorful music",
          0, 32767, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  /* diff sur */
  g_object_class_install_property (gobject_class, PROP_DS_ENABLE,
      g_param_spec_boolean ("ds_enable", "DSEnabled", "Enable diff-surround",
          FALSE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_DS_LEVEL,
      g_param_spec_int ("ds_level", "DSLevel", "Diff-surround level (percent)",
          0, 100, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  /* reverb */
  g_object_class_install_property (gobject_class, PROP_REVERB_ENABLE,
      g_param_spec_boolean ("reverb_enable", "ReverbEnabled", "Enable reverb",
          FALSE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_REVERB_ROOMSIZE,
      g_param_spec_int ("reverb_roomsize", "ReverbRoomSize", "Reverb room size (percent)",
          1, 100, 30, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_REVERB_WIDTH,
      g_param_spec_int ("reverb_width", "ReverbWidth", "Reveb room width (percent)",
          1, 100, 40, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_REVERB_DAMP,
      g_param_spec_int ("reverb_damp", "ReverbDamp", "Reverb room damp (percent)",
          1, 100, 10, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_REVERB_WET,
      g_param_spec_int ("reverb_wet", "ReverbWet", "Reverb wet signal (percent)",
          1, 100, 20, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_REVERB_DRY,
      g_param_spec_int ("reverb_dry", "ReverbDry", "Reverb dry signal (percent)",
          1, 100, 80, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  /* agc */
  g_object_class_install_property (gobject_class, PROP_AGC_ENABLE,
      g_param_spec_boolean ("agc_enable", "AGCEnabled", "Enable auto gain control",
          FALSE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_AGC_RATIO,
      g_param_spec_int ("agc_ratio", "AGCRatio", "Working ratio of agc",
          50, 500, 100, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_AGC_VOLUME,
      g_param_spec_int ("agc_volume", "AGCVolume", "Max volume of agc",
          0, 200, 100, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_AGC_MAXGAIN,
      g_param_spec_int ("agc_maxgain", "AGCMaxGain", "Max gain of agc",
          100, 800, 100, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  /* viper bass */
  g_object_class_install_property (gobject_class, PROP_VB_ENABLE,
      g_param_spec_boolean ("vb_enable", "VBEnabled", "Enable viper bass",
          FALSE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_VB_MODE,
      g_param_spec_int ("vb_mode", "VBMode", "ViPER bass mode",
          0, 2, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_VB_FREQ,
      g_param_spec_int ("vb_freq", "VBFreq", "ViPER bass frequency",
          50, 160, 76, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_VB_GAIN,
      g_param_spec_int ("vb_gain", "VBGain", "ViPER bass gain",
          0, 800, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  /* viper clarity */
  g_object_class_install_property (gobject_class, PROP_VC_ENABLE,
      g_param_spec_boolean ("vc_enable", "VCEnabled", "Enable viper clarity",
          FALSE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_VC_MODE,
      g_param_spec_int ("vc_mode", "VCMode", "ViPER clarity mode",
          0, 2, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_VC_LEVEL,
      g_param_spec_int ("vc_level", "VCLevel", "ViPER clarity level",
          0, 800, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  /* cure */
  g_object_class_install_property (gobject_class, PROP_CURE_ENABLE,
      g_param_spec_boolean ("cure_enable", "CureEnabled", "Enable viper cure+",
          FALSE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_CURE_LEVEL,
      g_param_spec_int ("cure_level", "CureLevel", "ViPER cure+ level",
          0, 2, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  /* tube */
  g_object_class_install_property (gobject_class, PROP_TUBE_ENABLE,
      g_param_spec_boolean ("tube_enable", "TubeEnabled", "Enable tube simiulator",
          FALSE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  /* analog-x */
  g_object_class_install_property (gobject_class, PROP_AX_ENABLE,
      g_param_spec_boolean ("ax_enable", "AXEnabled", "Enable viper analog-x",
          FALSE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_AX_MODE,
      g_param_spec_int ("ax_mode", "AXMode", "ViPER analog-x mode",
          0, 2, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  /* fet compressor */
  g_object_class_install_property (gobject_class, PROP_FETCOMP_ENABLE,
      g_param_spec_boolean ("fetcomp_enable", "FETCompEnabled", "Enable fet compressor",
          FALSE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_FETCOMP_THRESHOLD,
      g_param_spec_int ("fetcomp_threshold", "FETCompThreshold", "Compressor threshold (percent)",
          0, 100, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_FETCOMP_RATIO,
      g_param_spec_int ("fetcomp_ratio", "FETCompRatio", "Compressor ratio (percent)",
          0, 100, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_FETCOMP_KNEEWIDTH,
      g_param_spec_int ("fetcomp_kneewidth", "FETCompKneeWidth", "Compressor knee width (percent)",
          0, 100, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_FETCOMP_AUTOKNEE,
      g_param_spec_boolean ("fetcomp_autoknee", "FETCompAutoKnee", "Compressor auto knee control",
          TRUE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_FETCOMP_GAIN,
      g_param_spec_int ("fetcomp_gain", "FETCompGain", "Compressor makeup gain (percent)",
          0, 100, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_FETCOMP_AUTOGAIN,
      g_param_spec_boolean ("fetcomp_autogain", "FETCompAutoGain", "Compressor auto gain control",
          TRUE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_FETCOMP_ATTACK,
      g_param_spec_int ("fetcomp_attack", "FETCompAttack", "Compressor attack time (percent)",
          0, 100, 51, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_FETCOMP_AUTOATTACK,
      g_param_spec_boolean ("fetcomp_autoattack", "FETCompAutoAttack", "Compressor auto attack control",
          TRUE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_FETCOMP_RELEASE,
      g_param_spec_int ("fetcomp_release", "FETCompRelease", "Compressor release time (percent)",
          0, 100, 38, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_FETCOMP_AUTORELEASE,
      g_param_spec_boolean ("fetcomp_autorelease", "FETCompAutoRelease", "Compressor auto release control",
          TRUE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_FETCOMP_META_KNEEMULTI,
      g_param_spec_int ("fetcomp_meta_kneemulti", "FETCompKneeMulti", "Compressor knee width multi in auto mode (percent)",
          0, 100, 50, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_FETCOMP_META_MAXATTACK,
      g_param_spec_int ("fetcomp_meta_maxattack", "FETCompMaxAttack", "Compressor max attack in auto mode (percent)",
          0, 100, 88, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_FETCOMP_META_MAXRELEASE,
      g_param_spec_int ("fetcomp_meta_maxrelease", "FETCompMaxRelease", "Compressor max release in auto mode (percent)",
          0, 100, 88, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_FETCOMP_META_CREST,
      g_param_spec_int ("fetcomp_meta_crest", "FETCompCrest", "Compressor crest in auto mode (percent)",
          0, 100, 61, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_FETCOMP_META_ADAPT,
      g_param_spec_int ("fetcomp_meta_adapt", "FETCompAdapt", "Compressor adapt in auto mode (percent)",
          0, 100, 66, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));
  g_object_class_install_property (gobject_class, PROP_FETCOMP_NOCLIP,
      g_param_spec_boolean ("fetcomp_noclip", "FETCompNoClip", "Compressor prevent clipping",
          TRUE, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  /* output volume */
  g_object_class_install_property (gobject_class, PROP_OUT_VOLUME,
      g_param_spec_int ("out_volume", "OutVolume", "Master output volume (percent)",
          0, 100, 100, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  /* output pan */
  g_object_class_install_property (gobject_class, PROP_OUT_PAN,
      g_param_spec_int ("out_pan", "OutPan", "Master output pan",
          -100, 100, 0, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  /* limiter */
  g_object_class_install_property (gobject_class, PROP_LIM_THRESHOLD,
      g_param_spec_int ("lim_threshold", "LimThreshold", "Master limiter threshold (percent)",
          1, 100, 100, G_PARAM_WRITABLE | GST_PARAM_CONTROLLABLE));

  gst_element_class_set_static_metadata (gstelement_class,
    "viperfx",
    "Filter/Effect/Audio",
    "ViPER-FX Core wrapper for GStreamer1",
    "Jason <jason@vipersaudio.com>");

  caps = gst_caps_from_string (ALLOWED_CAPS);
  gst_audio_filter_class_add_pad_templates (GST_VIPERFX_CLASS (klass), caps);
  gst_caps_unref (caps);

  audioself_class->setup = GST_DEBUG_FUNCPTR (gst_viperfx_setup);
  basetransform_class->transform_ip =
    GST_DEBUG_FUNCPTR (gst_viperfx_transform_ip);
  basetransform_class->transform_ip_on_passthrough = FALSE;
  basetransform_class->stop = GST_DEBUG_FUNCPTR (gst_viperfx_stop);
}

/* sync all parameters to fx core
*/
static void sync_all_parameters (Gstviperfx *self)
{
  int32_t idx;

  // convolver
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_CONV_CROSSCHANNEL, self->conv_cc_level);
  viperfx_command_set_ir_path (self->vfx, self->conv_ir_path);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_CONV_PROCESS_ENABLED, self->conv_enabled);

  // vhe
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_VHE_EFFECT_LEVEL, self->vhe_level);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_VHE_PROCESS_ENABLED, self->vhe_enabled);

  // vse
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_VSE_REFERENCE_BARK, self->vse_ref_bark);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_VSE_BARK_RECONSTRUCT, self->vse_bark_cons);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_VSE_PROCESS_ENABLED, self->vse_enabled);

  // equalizer
  for (idx = 0; idx < 10; idx++) {
    viperfx_command_set_px4_vx4x2 (self->vfx,
        PARAM_HPFX_FIREQ_BANDLEVEL, idx, self->eq_band_level[idx]);
  }
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_FIREQ_PROCESS_ENABLED, self->eq_enabled);

  // colorful music
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_COLM_WIDENING, self->colm_widening);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_COLM_MIDIMAGE, self->colm_midimage);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_COLM_DEPTH, self->colm_depth);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_COLM_PROCESS_ENABLED, self->colm_enabled);

  // diff surr
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_DIFFSURR_DELAYTIME, self->ds_level);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_DIFFSURR_PROCESS_ENABLED, self->ds_enabled);

  // reverb
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_REVB_ROOMSIZE, self->reverb_roomsize);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_REVB_WIDTH, self->reverb_width);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_REVB_DAMP, self->reverb_damp);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_REVB_WET, self->reverb_wet);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_REVB_DRY, self->reverb_dry);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_REVB_PROCESS_ENABLED, self->reverb_enabled);

  // agc
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_AGC_RATIO, self->agc_ratio);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_AGC_VOLUME, self->agc_volume);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_AGC_MAXSCALER, self->agc_maxgain);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_AGC_PROCESS_ENABLED, self->agc_enabled);

  // viper bass
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_VIPERBASS_MODE, self->vb_mode);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_VIPERBASS_SPEAKER, self->vb_freq);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_VIPERBASS_BASSGAIN, self->vb_gain);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_VIPERBASS_PROCESS_ENABLED, self->vb_enabled);

  // viper clarity
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_VIPERCLARITY_MODE, self->vc_mode);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_VIPERCLARITY_CLARITY, self->vc_level);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_VIPERCLARITY_PROCESS_ENABLED, self->vc_enabled);

  // cure
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_CURE_CROSSFEED, self->cure_level);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_CURE_PROCESS_ENABLED, self->cure_enabled);

  // tube
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_TUBE_PROCESS_ENABLED, self->tube_enabled);

  // analog-x
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_ANALOGX_MODE, self->ax_mode);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_ANALOGX_PROCESS_ENABLED, self->ax_enabled);

  // fet compressor
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_FETCOMP_THRESHOLD, self->fetcomp_threshold);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_FETCOMP_RATIO, self->fetcomp_ratio);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_FETCOMP_KNEEWIDTH, self->fetcomp_kneewidth);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_FETCOMP_AUTOKNEE_ENABLED, self->fetcomp_autoknee);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_FETCOMP_GAIN, self->fetcomp_gain);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_FETCOMP_AUTOGAIN_ENABLED, self->fetcomp_autogain);
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_FETCOMP_ATTACK, self->fetcomp_attack);
  viperfx_command_set_px4_vx4x1 (self->vfx,
     PARAM_HPFX_FETCOMP_AUTOATTACK_ENABLED, self->fetcomp_autoattack);
  viperfx_command_set_px4_vx4x1 (self->vfx,
     PARAM_HPFX_FETCOMP_RELEASE, self->fetcomp_release);
  viperfx_command_set_px4_vx4x1 (self->vfx,
     PARAM_HPFX_FETCOMP_AUTORELEASE_ENABLED, self->fetcomp_autorelease);
  viperfx_command_set_px4_vx4x1 (self->vfx,
     PARAM_HPFX_FETCOMP_META_KNEEMULTI, self->fetcomp_meta_kneemulti);
  viperfx_command_set_px4_vx4x1 (self->vfx,
     PARAM_HPFX_FETCOMP_META_MAXATTACK, self->fetcomp_meta_maxattack);
  viperfx_command_set_px4_vx4x1 (self->vfx,
     PARAM_HPFX_FETCOMP_META_MAXRELEASE, self->fetcomp_meta_maxrelease);
  viperfx_command_set_px4_vx4x1 (self->vfx,
     PARAM_HPFX_FETCOMP_META_CREST, self->fetcomp_meta_crest);
  viperfx_command_set_px4_vx4x1 (self->vfx,
     PARAM_HPFX_FETCOMP_META_ADAPT, self->fetcomp_meta_adapt);
  viperfx_command_set_px4_vx4x1 (self->vfx,
     PARAM_HPFX_FETCOMP_META_NOCLIP_ENABLED, self->fetcomp_noclip);
  viperfx_command_set_px4_vx4x1 (self->vfx,
     PARAM_HPFX_FETCOMP_PROCESS_ENABLED, self->fetcomp_enabled);

  // output volume
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_OUTPUT_VOLUME, self->out_volume);

  // output pan
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_OUTPUT_PAN, self->out_pan);

  // limiter
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_HPFX_LIMITER_THRESHOLD, self->lim_threshold);

  // global enable
  viperfx_command_set_px4_vx4x1 (self->vfx,
      PARAM_SET_DOPROCESS_STATUS, self->fx_enabled);

  // reset
  self->vfx->reset (self->vfx);
}

/* initialize the new element
 * allocate private resources
 */
static void
gst_viperfx_init (Gstviperfx *self)
{
  gint32 idx;

  gst_base_transform_set_in_place (GST_BASE_TRANSFORM (self), TRUE);
  gst_base_transform_set_gap_aware (GST_BASE_TRANSFORM (self), TRUE);

  /* initialize properties */
  self->fx_enabled = FALSE;
  // convolver
  self->conv_enabled = FALSE;
  memset (self->conv_ir_path, 0,
      sizeof(self->conv_ir_path));
  self->conv_cc_level = 0;
  // vhe
  self->vhe_enabled = FALSE;
  self->vhe_level = 0;
  // vse
  self->vse_enabled = FALSE;
  self->vse_ref_bark = 7600;
  self->vse_bark_cons = 10;
  // equalizer
  self->eq_enabled = FALSE;
  for (idx = 0; idx < 10; idx++)
    self->eq_band_level[idx] = 0;
  // colorful music
  self->colm_enabled = FALSE;
  self->colm_widening = 100;
  self->colm_midimage = 100;
  self->colm_depth = 200;
  // diff surr
  self->ds_enabled = FALSE;
  self->ds_level = 0;
  // reverb
  self->reverb_enabled = FALSE;
  self->reverb_roomsize = 30;
  self->reverb_width = 40;
  self->reverb_damp = 10;
  self->reverb_wet = 20;
  self->reverb_dry = 80;
  // agc
  self->agc_enabled = FALSE;
  self->agc_ratio = 100;
  self->agc_volume = 100;
  self->agc_maxgain = 100;
  // viper bass
  self->vb_enabled = FALSE;
  self->vb_mode = 0;
  self->vb_freq = 76;
  self->vb_gain = 0;
  // viper clarity
  self->vc_enabled = FALSE;
  self->vc_mode = 0;
  self->vc_level = 0;
  // cure
  self->cure_enabled = FALSE;
  self->cure_level = 0;
  // tube
  self->tube_enabled = FALSE;
  // analog-x
  self->ax_enabled = FALSE;
  self->ax_mode = 0;
  // fet compressor
  self->fetcomp_enabled = FALSE;
  self->fetcomp_threshold = 0;
  self->fetcomp_ratio = 0;
  self->fetcomp_kneewidth = 0;
  self->fetcomp_autoknee = TRUE;
  self->fetcomp_gain = 0;
  self->fetcomp_autogain = TRUE;
  self->fetcomp_attack = 51;
  self->fetcomp_autoattack = TRUE;
  self->fetcomp_release = 38;
  self->fetcomp_autorelease = TRUE;
  self->fetcomp_meta_kneemulti = 50;
  self->fetcomp_meta_maxattack = 88;
  self->fetcomp_meta_maxrelease = 88;
  self->fetcomp_meta_crest = 61;
  self->fetcomp_meta_adapt = 66;
  self->fetcomp_noclip = TRUE;
  // output volume
  self->out_volume = 100;
  // output pan
  self->out_pan = 0;
  // limiter
  self->lim_threshold = 100;

  /* initialize private resources */
  self->vfx = NULL;
  self->so_handle = viperfx_load_library (NULL);
  if (self->so_handle == NULL) {
    self->so_entrypoint = NULL;
  } else {
    self->so_entrypoint = query_viperfx_entrypoint (
        self->so_handle);
    if (self->so_entrypoint == NULL) {
      viperfx_unload_library (
          self->so_handle);
      self->so_handle = NULL;
    } else {
      self->vfx = self->so_entrypoint ();
      if (self->vfx == NULL) {
        viperfx_unload_library (
            self->so_handle);
        self->so_handle = NULL;
        self->so_entrypoint = NULL;
      }
    }
  }

  if (self->vfx != NULL)
    sync_all_parameters (self);

  g_mutex_init (&self->lock);
}

/* free private resources
*/
static void
gst_viperfx_finalize (GObject * object)
{
  Gstviperfx *self = GST_VIPERFX (object);

  if (self->vfx != NULL) {
    self->vfx->release (self->vfx);
    self->vfx = NULL;
  }
  if (self->so_handle != NULL) {
    viperfx_unload_library (
        self->so_handle);
    self->so_handle = NULL;
  }
  self->so_entrypoint = NULL;

  g_mutex_clear (&self->lock);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_viperfx_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gstviperfx *self = GST_VIPERFX (object);

  switch (prop_id) {
    case PROP_FX_ENABLE:
    {
      g_mutex_lock (&self->lock);
      self->fx_enabled = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_SET_DOPROCESS_STATUS, self->fx_enabled);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_CONV_ENABLE:
    {
      g_mutex_lock (&self->lock);
      self->conv_enabled = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_CONV_PROCESS_ENABLED, self->conv_enabled);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_CONV_IR_PATH:
    {
      g_mutex_lock (&self->lock);
      if (strlen (g_value_get_string (value)) < 256) {
          memset (self->conv_ir_path, 0,
              sizeof(self->conv_ir_path));
          strcpy(self->conv_ir_path,
              g_value_get_string (value));
          viperfx_command_set_ir_path (self->vfx, self->conv_ir_path);
      }
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_CONV_CC_LEVEL:
    {
      g_mutex_lock (&self->lock);
      self->conv_cc_level = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_CONV_CROSSCHANNEL, self->conv_cc_level);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_VHE_ENABLE:
    {
      g_mutex_lock (&self->lock);
      self->vhe_enabled = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_VHE_PROCESS_ENABLED, self->vhe_enabled);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_VHE_LEVEL:
    {
      g_mutex_lock (&self->lock);
      self->vhe_level = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_VHE_EFFECT_LEVEL, self->vhe_level);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_VSE_ENABLE:
    {
      g_mutex_lock (&self->lock);
      self->vse_enabled = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_VSE_PROCESS_ENABLED, self->vse_enabled);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_VSE_REF_BARK:
    {
      g_mutex_lock (&self->lock);
      self->vse_ref_bark = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_VSE_REFERENCE_BARK, self->vse_ref_bark);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_VSE_BARK_CONS:
    {
      g_mutex_lock (&self->lock);
      self->vse_bark_cons = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_VSE_BARK_RECONSTRUCT, self->vse_bark_cons);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_EQ_ENABLE:
    {
      g_mutex_lock (&self->lock);
      self->eq_enabled = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FIREQ_PROCESS_ENABLED, self->eq_enabled);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_EQ_BAND1:
    {
      g_mutex_lock (&self->lock);
      self->eq_band_level[0] = g_value_get_int (value);
      viperfx_command_set_px4_vx4x2 (self->vfx,
          PARAM_HPFX_FIREQ_BANDLEVEL, 0, self->eq_band_level[0]);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_EQ_BAND2:
    {
      g_mutex_lock (&self->lock);
      self->eq_band_level[1] = g_value_get_int (value);
      viperfx_command_set_px4_vx4x2 (self->vfx,
          PARAM_HPFX_FIREQ_BANDLEVEL, 1, self->eq_band_level[1]);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_EQ_BAND3:
    {
      g_mutex_lock (&self->lock);
      self->eq_band_level[2] = g_value_get_int (value);
      viperfx_command_set_px4_vx4x2 (self->vfx,
          PARAM_HPFX_FIREQ_BANDLEVEL, 2, self->eq_band_level[2]);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_EQ_BAND4:
    {
      g_mutex_lock (&self->lock);
      self->eq_band_level[3] = g_value_get_int (value);
      viperfx_command_set_px4_vx4x2 (self->vfx,
          PARAM_HPFX_FIREQ_BANDLEVEL, 3, self->eq_band_level[3]);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_EQ_BAND5:
    {
      g_mutex_lock (&self->lock);
      self->eq_band_level[4] = g_value_get_int (value);
      viperfx_command_set_px4_vx4x2 (self->vfx,
          PARAM_HPFX_FIREQ_BANDLEVEL, 4, self->eq_band_level[4]);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_EQ_BAND6:
    {
      g_mutex_lock (&self->lock);
      self->eq_band_level[5] = g_value_get_int (value);
      viperfx_command_set_px4_vx4x2 (self->vfx,
          PARAM_HPFX_FIREQ_BANDLEVEL, 5, self->eq_band_level[5]);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_EQ_BAND7:
    {
      g_mutex_lock (&self->lock);
      self->eq_band_level[6] = g_value_get_int (value);
      viperfx_command_set_px4_vx4x2 (self->vfx,
          PARAM_HPFX_FIREQ_BANDLEVEL, 6, self->eq_band_level[6]);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_EQ_BAND8:
    {
      g_mutex_lock (&self->lock);
      self->eq_band_level[7] = g_value_get_int (value);
      viperfx_command_set_px4_vx4x2 (self->vfx,
          PARAM_HPFX_FIREQ_BANDLEVEL, 7, self->eq_band_level[7]);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_EQ_BAND9:
    {
      g_mutex_lock (&self->lock);
      self->eq_band_level[8] = g_value_get_int (value);
      viperfx_command_set_px4_vx4x2 (self->vfx,
          PARAM_HPFX_FIREQ_BANDLEVEL, 8, self->eq_band_level[8]);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_EQ_BAND10:
    {
      g_mutex_lock (&self->lock);
      self->eq_band_level[9] = g_value_get_int (value);
      viperfx_command_set_px4_vx4x2 (self->vfx,
          PARAM_HPFX_FIREQ_BANDLEVEL, 9, self->eq_band_level[9]);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_COLM_ENABLE:
    {
      g_mutex_lock (&self->lock);
      self->colm_enabled = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_COLM_PROCESS_ENABLED, self->colm_enabled);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_COLM_WIDENING:
    {
      g_mutex_lock (&self->lock);
      self->colm_widening = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_COLM_WIDENING, self->colm_widening);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_COLM_MIDIMAGE:
    {
      g_mutex_lock (&self->lock);
      self->colm_midimage = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_COLM_MIDIMAGE, self->colm_midimage);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_COLM_DEPTH:
    {
      float f_depth_val = (float) g_value_get_int (value);
      gint s_val = 0;
      f_depth_val = (f_depth_val / 32767.00f) * 600.00f;
      s_val = (gint) (f_depth_val + 200.0f);
      if (s_val < 200) s_val = 200;
      if (s_val > 800) s_val = 800;

      g_mutex_lock (&self->lock);
      self->colm_depth = s_val;
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_COLM_DEPTH, self->colm_depth);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_DS_ENABLE:
    {
      g_mutex_lock (&self->lock);
      self->ds_enabled = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_DIFFSURR_PROCESS_ENABLED, self->ds_enabled);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_DS_LEVEL:
    {
      g_mutex_lock (&self->lock);
      self->ds_level = g_value_get_int (value) * 20;
      if (self->ds_level < 0) self->ds_level = 0;
      if (self->ds_level > 2000) self->ds_level = 2000;
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_DIFFSURR_DELAYTIME, self->ds_level);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_REVERB_ENABLE:
    {
      g_mutex_lock (&self->lock);
      self->reverb_enabled = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_REVB_PROCESS_ENABLED, self->reverb_enabled);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_REVERB_ROOMSIZE:
    {
      g_mutex_lock (&self->lock);
      self->reverb_roomsize = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_REVB_ROOMSIZE, self->reverb_roomsize);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_REVERB_WIDTH:
    {
      g_mutex_lock (&self->lock);
      self->reverb_width = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_REVB_WIDTH, self->reverb_width);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_REVERB_DAMP:
    {
      g_mutex_lock (&self->lock);
      self->reverb_damp = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_REVB_DAMP, self->reverb_damp);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_REVERB_WET:
    {
      g_mutex_lock (&self->lock);
      self->reverb_wet = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_REVB_WET, self->reverb_wet);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_REVERB_DRY:
    {
      g_mutex_lock (&self->lock);
      self->reverb_dry = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_REVB_DRY, self->reverb_dry);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_AGC_ENABLE:
    {
      g_mutex_lock (&self->lock);
      self->agc_enabled = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_AGC_PROCESS_ENABLED, self->agc_enabled);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_AGC_RATIO:
    {
      g_mutex_lock (&self->lock);
      self->agc_ratio = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_AGC_RATIO, self->agc_ratio);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_AGC_VOLUME:
    {
      g_mutex_lock (&self->lock);
      self->agc_volume = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_AGC_VOLUME, self->agc_volume);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_AGC_MAXGAIN:
    {
      g_mutex_lock (&self->lock);
      self->agc_maxgain = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_AGC_MAXSCALER, self->agc_maxgain);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_VB_ENABLE:
    {
      g_mutex_lock (&self->lock);
      self->vb_enabled = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_VIPERBASS_PROCESS_ENABLED, self->vb_enabled);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_VB_MODE:
    {
      g_mutex_lock (&self->lock);
      self->vb_mode = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_VIPERBASS_MODE, self->vb_mode);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_VB_FREQ:
    {
      g_mutex_lock (&self->lock);
      self->vb_freq = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_VIPERBASS_SPEAKER, self->vb_freq);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_VB_GAIN:
    {
      g_mutex_lock (&self->lock);
      self->vb_gain = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_VIPERBASS_BASSGAIN, self->vb_gain);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_VC_ENABLE:
    {
      g_mutex_lock (&self->lock);
      self->vc_enabled = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_VIPERCLARITY_PROCESS_ENABLED, self->vc_enabled);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_VC_MODE:
    {
      g_mutex_lock (&self->lock);
      self->vc_mode = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_VIPERCLARITY_MODE, self->vc_mode);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_VC_LEVEL:
    {
      g_mutex_lock (&self->lock);
      self->vc_level = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_VIPERCLARITY_CLARITY, self->vc_level);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_CURE_ENABLE:
    {
      g_mutex_lock (&self->lock);
      self->cure_enabled = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_CURE_PROCESS_ENABLED, self->cure_enabled);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_CURE_LEVEL:
    {
      g_mutex_lock (&self->lock);
      self->cure_level = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_CURE_CROSSFEED, self->cure_level);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_TUBE_ENABLE:
    {
      g_mutex_lock (&self->lock);
      self->tube_enabled = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_TUBE_PROCESS_ENABLED, self->tube_enabled);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_AX_ENABLE:
    {
      g_mutex_lock (&self->lock);
      self->ax_enabled = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_ANALOGX_PROCESS_ENABLED, self->ax_enabled);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_AX_MODE:
    {
      g_mutex_lock (&self->lock);
      self->ax_mode = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_ANALOGX_MODE, self->ax_mode);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_FETCOMP_ENABLE:
    {
      g_mutex_lock (&self->lock);
      self->fetcomp_enabled = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FETCOMP_PROCESS_ENABLED, self->fetcomp_enabled);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_FETCOMP_THRESHOLD:
    {
      g_mutex_lock (&self->lock);
      self->fetcomp_threshold = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FETCOMP_THRESHOLD, self->fetcomp_threshold);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_FETCOMP_RATIO:
    {
      g_mutex_lock (&self->lock);
      self->fetcomp_ratio = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FETCOMP_RATIO, self->fetcomp_ratio);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_FETCOMP_KNEEWIDTH:
    {
      g_mutex_lock (&self->lock);
      self->fetcomp_kneewidth = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FETCOMP_KNEEWIDTH, self->fetcomp_kneewidth);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_FETCOMP_AUTOKNEE:
    {
      g_mutex_lock (&self->lock);
      self->fetcomp_autoknee = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FETCOMP_AUTOKNEE_ENABLED, self->fetcomp_autoknee);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_FETCOMP_GAIN:
    {
      g_mutex_lock (&self->lock);
      self->fetcomp_gain = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FETCOMP_GAIN, self->fetcomp_gain);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_FETCOMP_AUTOGAIN:
    {
      g_mutex_lock (&self->lock);
      self->fetcomp_autogain = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FETCOMP_AUTOGAIN_ENABLED, self->fetcomp_autogain);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_FETCOMP_ATTACK:
    {
      g_mutex_lock (&self->lock);
      self->fetcomp_attack = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FETCOMP_ATTACK, self->fetcomp_attack);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_FETCOMP_AUTOATTACK:
    {
      g_mutex_lock (&self->lock);
      self->fetcomp_autoattack = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FETCOMP_AUTOATTACK_ENABLED, self->fetcomp_autoattack);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_FETCOMP_RELEASE:
    {
      g_mutex_lock (&self->lock);
      self->fetcomp_release = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FETCOMP_RELEASE, self->fetcomp_release);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_FETCOMP_AUTORELEASE:
    {
      g_mutex_lock (&self->lock);
      self->fetcomp_autorelease = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FETCOMP_AUTORELEASE_ENABLED, self->fetcomp_autorelease);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_FETCOMP_META_KNEEMULTI:
    {
      g_mutex_lock (&self->lock);
      self->fetcomp_meta_kneemulti = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FETCOMP_META_KNEEMULTI, self->fetcomp_meta_kneemulti);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_FETCOMP_META_MAXATTACK:
    {
      g_mutex_lock (&self->lock);
      self->fetcomp_meta_maxattack = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FETCOMP_META_MAXATTACK, self->fetcomp_meta_maxattack);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_FETCOMP_META_MAXRELEASE:
    {
      g_mutex_lock (&self->lock);
      self->fetcomp_meta_maxrelease = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FETCOMP_META_MAXRELEASE, self->fetcomp_meta_maxrelease);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_FETCOMP_META_CREST:
    {
      g_mutex_lock (&self->lock);
      self->fetcomp_meta_crest = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FETCOMP_META_CREST, self->fetcomp_meta_crest);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_FETCOMP_META_ADAPT:
    {
      g_mutex_lock (&self->lock);
      self->fetcomp_meta_adapt = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FETCOMP_META_ADAPT, self->fetcomp_meta_adapt);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_FETCOMP_NOCLIP:
    {
      g_mutex_lock (&self->lock);
      self->fetcomp_noclip = g_value_get_boolean (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_FETCOMP_META_NOCLIP_ENABLED, self->fetcomp_noclip);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_OUT_VOLUME:
    {
      g_mutex_lock (&self->lock);
      self->out_volume = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_OUTPUT_VOLUME, self->out_volume);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_OUT_PAN:
    {
      g_mutex_lock (&self->lock);
      self->out_pan = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_OUTPUT_PAN, self->out_pan);
      g_mutex_unlock (&self->lock);
    }
    break;

    case PROP_LIM_THRESHOLD:
    {
      g_mutex_lock (&self->lock);
      self->lim_threshold = g_value_get_int (value);
      viperfx_command_set_px4_vx4x1 (self->vfx,
          PARAM_HPFX_LIMITER_THRESHOLD, self->lim_threshold);
      g_mutex_unlock (&self->lock);
    }
    break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_viperfx_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gstviperfx *self = GST_VIPERFX (object);

  switch (prop_id) {

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstBaseTransform vmethod implementations */

static gboolean
gst_viperfx_setup (GstAudioFilter * base, const GstAudioInfo * info)
{
  Gstviperfx *self = GST_VIPERFX (base);
  gint sample_rate = 0;

  if (self->vfx == NULL)
    return FALSE;

  if (info) {
    sample_rate = GST_AUDIO_INFO_RATE (info);
  } else {
    sample_rate = GST_AUDIO_FILTER_RATE (self);
  }
  if (sample_rate <= 0)
    return FALSE;

  GST_DEBUG_OBJECT (self, "current sample_rate = %d", sample_rate);

  g_mutex_lock (&self->lock);
  if (!self->vfx->set_samplerate (self->vfx, sample_rate)) {
    g_mutex_unlock (&self->lock);
    return FALSE;
  }
  if (!self->vfx->set_channels (self->vfx, 2)) {
    g_mutex_unlock (&self->lock);
    return FALSE;
  }
  self->vfx->reset (self->vfx);
  g_mutex_unlock (&self->lock);

  return TRUE;
}

static gboolean
gst_viperfx_stop (GstBaseTransform * base)
{
  Gstviperfx *self = GST_VIPERFX (base);

  if (self->vfx == NULL)
    return TRUE;
  g_mutex_lock (&self->lock);
  self->vfx->reset (self->vfx);
  g_mutex_unlock (&self->lock);

  return TRUE;
}

/* this function does the actual processing
 */
static GstFlowReturn
gst_viperfx_transform_ip (GstBaseTransform * base, GstBuffer * buf)
{
  Gstviperfx *filter = GST_VIPERFX (base);
  guint idx, num_samples;
  short *pcm_data;
  GstClockTime timestamp, stream_time;
  GstMapInfo map;

  timestamp = GST_BUFFER_TIMESTAMP (buf);
  stream_time =
      gst_segment_to_stream_time (&base->segment, GST_FORMAT_TIME, timestamp);

  if (GST_CLOCK_TIME_IS_VALID (stream_time))
    gst_object_sync_values (GST_OBJECT (filter), stream_time);

  if (G_UNLIKELY (GST_BUFFER_FLAG_IS_SET (buf, GST_BUFFER_FLAG_GAP)))
    return GST_FLOW_OK;

  gst_buffer_map (buf, &map, GST_MAP_READWRITE);
  num_samples = map.size / GST_AUDIO_FILTER_BPS (filter) / 2;
  pcm_data = (short *)(map.data);
  for (idx = 0; idx < num_samples * 2; idx++)
    pcm_data[idx] >>= 1;
  if (filter->vfx != NULL) {
    g_mutex_lock (&filter->lock);
    filter->vfx->process (filter->vfx,
        pcm_data, (int)num_samples);
    g_mutex_unlock (&filter->lock);
  }
  gst_buffer_unmap (buf, &map);

  return GST_FLOW_OK;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
viperfx_init (GstPlugin * viperfx)
{
  return gst_element_register (viperfx, "viperfx", GST_RANK_NONE,
      GST_TYPE_VIPERFX);
}

/* gstreamer looks for this structure to register viperfxs
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    viperfx,
    "viperfx element",
    viperfx_init,
    VERSION,
    "NonGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
