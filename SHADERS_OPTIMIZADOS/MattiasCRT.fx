#include "ReShade.fxh"

// Loosely based on postprocessing shader by inigo quilez, License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

uniform float  iGlobalTime < source = "timer"; >;

#define mod(x,y) (x-y*floor(x/y))

float2 curve(float2 uv)
{
	uv = (uv - 0.5) * 2.0;
	uv *= 1.1;	
	uv.x *= 1.0 + pow((abs(uv.y) / 5.0), 2.0);
	uv.y *= 1.0 + pow((abs(uv.x) / 4.0), 2.0);
	uv  = (uv / 2.0) + 0.5;
	uv =  uv *0.92 + 0.04;
	return uv;
}

void PS_CRTMattias1(in float4 pos : SV_POSITION, in float2 txcoord : TexCoord, out float4 fragColor : COLOR0)
{
	float2 fragCoord = txcoord * ReShade::ScreenSize;
    float2 q = fragCoord.xy / ReShade::ScreenSize.xy;
    float2 uv = q;
    uv = curve( uv );
    float3 col;

    col.r = tex2D(ReShade::BackBuffer,float2(uv.x+0.001,uv.y+0.001)).x+0.05;
    col.g = tex2D(ReShade::BackBuffer,float2(uv.x+0.000,uv.y-0.002)).y+0.05;
    col.b = tex2D(ReShade::BackBuffer,float2(uv.x-0.002,uv.y+0.000)).z+0.05;

    col = clamp(col*0.6+0.4*col*col*1.0,0.0,1.0);

    /*float vig = (0.0 + 1.0*16.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y));
	col *= float3(pow(vig,0.3),pow(vig,0.3),pow(vig,0.3));
    */
    col *= float3(0.95,1.05,0.95);
	col *= 2.8;

	float scans = clamp( 0.35+0.35*sin(uv.y*ReShade::ScreenSize.y*1.5), 0.0, 1.0);
	
	float s = pow(scans,1.7);
	col = col*float3( 0.4+0.7*s, 0.4+0.7*s, 0.4+0.7*s) ;

	col*=1.0-0.65*float3(clamp((mod(fragCoord.x, 2.0)-1.0)*2.0,0.0,1.0),clamp((mod(fragCoord.x, 2.0)-1.0)*2.0,0.0,1.0),clamp((mod(fragCoord.x, 2.0)-1.0)*2.0,0.0,1.0));

    /* Clamp */
    if (uv.x < 0.0 || uv.x > 1.0)
        col *= 0.0;
    if (uv.y < 0.0 || uv.y > 1.0)
        col *= 0.0;
	
    fragColor = float4(col,1.0);
}

technique CRTMattias {
    pass CRTMattias {
        VertexShader=PostProcessVS;
        PixelShader=PS_CRTMattias1;
    }
}
