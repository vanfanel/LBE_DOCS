#include "ReShade.fxh"

// newpixie CRT
// by Mattias Gustavsson
// adapted for slang by hunterk

/*
------------------------------------------------------------------------------
This software is available under 2 licenses - you may choose the one you like.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2016 Mattias Gustavsson
Permission is hereby granted, free of charge, to any person obtaining a copy of 
this software and associated documentation files (the "Software"), to deal in 
the Software without restriction, including without limitation the rights to 
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
of the Software, and to permit persons to whom the Software is furnished to do 
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all 
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this 
software, either in source code form or as a compiled binary, for any purpose, 
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this 
software dedicate any and all copyright interest in the software to the public 
domain. We make this dedication for the benefit of the public at large and to 
the detriment of our heirs and successors. We intend this dedication to be an 
overt act of relinquishment in perpetuity of all present and future rights to 
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN 
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION 
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/

uniform float curvature <
	ui_type = "drag";
	ui_min = 0.0001;
	ui_max = 4.0;
	ui_step = 0.25;
	ui_label = "Curvature [CRT-NewPixie]";
> = 2.0;
//> = 0.0001;

float3 tsample( sampler samp, float2 tc, float offs, float2 resolution )
{
	tc = tc * float2(1.025, 0.92) + float2(-0.0125, 0.04);
	float3 s = pow( abs( tex2D( samp, float2( tc.x, 1.0-tc.y ) ).rgb), float3( 2.2,2.2,2.2 ) );
	return s*float3(1.25,1.25,1.25);
}
		
float3 filmic( float3 LinearColor )
{
    float3 x = max( float3(0.0,0.0,0.0), LinearColor-float3(0.004,0.004,0.004));
    return (x*(6.2*x+0.5))/(x*(6.2*x+1.7)+0.06);
}
		
float2 curve( float2 uv )
{
    uv = (uv - 0.5);// * 2.0;
    uv *= float2(0.925, 1.095);  
    uv *= curvature;
    uv.x *= 1.0 + pow((abs(uv.y) / 4.0), 2.0);
    uv.y *= 1.0 + pow((abs(uv.x) / 3.0), 2.0);
    uv /= curvature;
    uv  += 0.5;
    uv =  uv *0.92 + 0.04;
    return uv;
}
		
float rand(float2 co){ return frac(sin(dot(co.xy ,float2(12.9898,78.233))) * 43758.5453); }
    
#define resolution ReShade::ScreenSize.xy
#define mod(x,y) (x-y*floor(x/y))
#define gl_FragCoord (uv_tx.xy * resolution)

float4 PS_NewPixie_Final(float4 pos: SV_Position, float2 uv_tx : TexCoord) : SV_Target
{
    float2 uv = uv_tx.xy;
    uv.y = 1.0 - uv_tx.y;
    /* Curve */
    float2 curved_uv = lerp( curve( uv ), uv, 0.4 );
    float scale = -0.101;
    float2 scuv = curved_uv*(1.0-scale)+scale/2.0+float2(0.003, -0.001);
	
    uv = scuv;
		
    /* Main color, Bleed */
    float3 col;
	
    float x = 0;
    float o =sin(gl_FragCoord.y*1.5)/resolution.x;
    x+=o*0.25;

    col.r = tsample(ReShade::BackBuffer,float2(x+scuv.x+0.0009,scuv.y+0.0009),resolution.y/800.0, resolution ).x+0.02;
    col.g = tsample(ReShade::BackBuffer,float2(x+scuv.x+0.0000,scuv.y-0.0011),resolution.y/800.0, resolution ).y+0.02;
    col.b = tsample(ReShade::BackBuffer,float2(x+scuv.x-0.0015,scuv.y+0.0000),resolution.y/800.0, resolution ).z+0.02;
   
    /* Level adjustment (curves) */
    col = clamp(col + col*col + col*col*col*col*col,float3(0.0, 0.0, 0.0),float3(10.0, 10.0, 10.0));

    /* Vignette. Modify the 16.0 value to control the burnout effect in the center area. */
    float vig = ((1.0-0.99*0.8) + 1.0*16.0*curved_uv.x*curved_uv.y*(1.0-curved_uv.x)*(1.0-curved_uv.y));
    vig = 1.3*pow(vig,0.5);
    col *= vig;
    /* Compensate the lack of vignette in case you decide to comment it out for performance reasons */
    // col *= 1.3;

    /* Scanlines (Using uv instead of curved_uv makes dithered patterns look homogeneous) */
    //float scans = clamp( 0.35+0.18*sin(curved_uv.y*resolution.y*1.5), 0.0, 1.0);
    float scans = clamp( 0.35+0.18*sin(uv.y*resolution.y*1.5), 0.0, 1.0);
    float s = pow(scans,0.9);
    col = col * float3(s,s,s);
		
    /* Vertical lines (shadow mask) */
    col*=1.0-0.23*(clamp((mod(gl_FragCoord.xy.x, 3.0))/2.0,0.0,1.0));
		
    /* Tone map */
    col = filmic( col );
		
    /* Clamp */
    float alpha = 0.0; // Inicializar alpha
    // Calcular el efecto de oscurecimiento en los bordes
    alpha += smoothstep(0.01, 0.0, curved_uv.x); // Suavizado al salir del límite izquierdo
    alpha += smoothstep(0.99, 1.0, curved_uv.x); // Suavizado al salir del límite derecho
    alpha += smoothstep(0.01, 0.0, curved_uv.y); // Suavizado al salir del límite inferior
    alpha += smoothstep(0.99, 1.0, curved_uv.y); // Suavizado al salir del límite superior
    // Aplicar el efecto de oscurecimiento
    col *= 1.0 - alpha; // Oscurecer el color original dependiendo de alpha

    return float4( col, 1.0 );
}

technique CRTNewPixie
{
	pass PS_CRTNewPixie_Final
	{
		VertexShader = PostProcessVS;
		PixelShader = PS_NewPixie_Final;
	}
}
