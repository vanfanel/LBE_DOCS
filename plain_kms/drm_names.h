#include <xf86drmMode.h>

#define type_name_fn(res) \
char * res##_str(int type) { \
int i; \
for (i = 0; i < sizeof(res##_names); i++) { \
if (res##_names[i].type == type) \
return res##_names[i].name; \
} \
return "(invalid)"; \
}

struct type_name {
int type;
char *name;
};

struct type_name connector_type_names[] = {
{ DRM_MODE_CONNECTOR_Unknown, "unknown" },
{ DRM_MODE_CONNECTOR_VGA, "VGA" },
{ DRM_MODE_CONNECTOR_DVII, "DVI-I" },
{ DRM_MODE_CONNECTOR_DVID, "DVI-D" },
{ DRM_MODE_CONNECTOR_DVIA, "DVI-A" },
{ DRM_MODE_CONNECTOR_Composite, "composite" },
{ DRM_MODE_CONNECTOR_SVIDEO, "s-video" },
{ DRM_MODE_CONNECTOR_LVDS, "LVDS" },
{ DRM_MODE_CONNECTOR_Component, "component" },
{ DRM_MODE_CONNECTOR_9PinDIN, "9-pin DIN" },
{ DRM_MODE_CONNECTOR_DisplayPort, "displayport" },
{ DRM_MODE_CONNECTOR_HDMIA, "HDMI-A" },
{ DRM_MODE_CONNECTOR_HDMIB, "HDMI-B" },
{ DRM_MODE_CONNECTOR_TV, "TV" },
{ DRM_MODE_CONNECTOR_eDP, "embedded displayport" },
};

type_name_fn(connector_type)

struct type_name encoder_type_names[] = {
{ DRM_MODE_ENCODER_NONE, "none" },
{ DRM_MODE_ENCODER_DAC, "DAC" },
{ DRM_MODE_ENCODER_TMDS, "TMDS" },
{ DRM_MODE_ENCODER_LVDS, "LVDS" },
{ DRM_MODE_ENCODER_TVDAC, "TVDAC" },
};

type_name_fn(encoder_type)


