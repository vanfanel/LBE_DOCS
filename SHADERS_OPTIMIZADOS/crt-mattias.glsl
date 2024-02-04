// CRT Emulation
// by Mattias
// https://www.shadertoy.com/view/lsB3DV

#pragma parameter CURVATURE "Curvature" 0.5 0.0 1.0 0.05
#pragma parameter SCANSPEED "Scanline Crawl Speed" 1.0 0.0 10.0 0.5

#if defined(VERTEX)

#if __VERSION__ >= 130
#define COMPAT_VARYING out
#define COMPAT_ATTRIBUTE in
#define COMPAT_TEXTURE texture
#else
#define COMPAT_VARYING varying 
#define COMPAT_ATTRIBUTE attribute 
#define COMPAT_TEXTURE texture2D
#endif

#ifdef GL_ES
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif

COMPAT_ATTRIBUTE vec4 VertexCoord;
COMPAT_ATTRIBUTE vec4 COLOR;
COMPAT_ATTRIBUTE vec4 TexCoord;
COMPAT_VARYING vec4 COL0;
COMPAT_VARYING vec4 TEX0;
// out variables go here as COMPAT_VARYING whatever

vec4 _oPosition1; 
uniform mat4 MVPMatrix;
uniform COMPAT_PRECISION int FrameDirection;
uniform COMPAT_PRECISION int FrameCount;
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;

// compatibility #defines
#define vTexCoord TEX0.xy
#define SourceSize vec4(TextureSize, 1.0 / TextureSize) //either TextureSize or InputSize
#define OutSize vec4(OutputSize, 1.0 / OutputSize)

void main()
{
    gl_Position = MVPMatrix * VertexCoord;
    TEX0.xy = TexCoord.xy;
}

#elif defined(FRAGMENT)

#ifdef GL_ES
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif

#if __VERSION__ >= 130
#define COMPAT_VARYING in
#define COMPAT_TEXTURE texture
out COMPAT_PRECISION vec4 FragColor;
#else
#define COMPAT_VARYING varying
#define FragColor gl_FragColor
#define COMPAT_TEXTURE texture2D
#endif

uniform COMPAT_PRECISION int FrameDirection;
uniform COMPAT_PRECISION int FrameCount;
uniform COMPAT_PRECISION vec2 OutputSize;
uniform COMPAT_PRECISION vec2 TextureSize;
uniform COMPAT_PRECISION vec2 InputSize;
uniform sampler2D Texture;
COMPAT_VARYING vec4 TEX0;

// compatibility #defines
#define Source Texture
#define vTexCoord TEX0.xy

#define SourceSize vec4(TextureSize, 1.0 / TextureSize) //either TextureSize or InputSize
#define OutSize vec4(OutputSize, 1.0 / OutputSize)

#ifdef PARAMETER_UNIFORM
uniform COMPAT_PRECISION float CURVATURE, SCANSPEED;
#else
#define CURVATURE 0.5
#define SCANSPEED 1.0
#endif

#define iChannel0 Texture
#define iResolution OutputSize.xy
#define fragCoord gl_FragCoord.xy

vec3 sample_( sampler2D tex, vec2 tc )
{
	vec3 s = pow(COMPAT_TEXTURE(tex,tc).rgb, vec3(2.2));
	return s;
}

vec3 blur(sampler2D tex, vec2 tc, float offs)
{
	vec4 xoffs = offs * vec4(-2.0, -1.0, 1.0, 2.0) / (iResolution.x * TextureSize.x / InputSize.x);
	vec4 yoffs = offs * vec4(-2.0, -1.0, 1.0, 2.0) / (iResolution.y * TextureSize.y / InputSize.y);
   tc = tc * InputSize / TextureSize;
	
	vec3 color = vec3(0.0, 0.0, 0.0);
	color += sample_(tex,tc + vec2(xoffs.x, yoffs.x)) * 0.00366;
	color += sample_(tex,tc + vec2(xoffs.y, yoffs.x)) * 0.01465;
	color += sample_(tex,tc + vec2(    0.0, yoffs.x)) * 0.02564;
	color += sample_(tex,tc + vec2(xoffs.z, yoffs.x)) * 0.01465;
	color += sample_(tex,tc + vec2(xoffs.w, yoffs.x)) * 0.00366;
	
	color += sample_(tex,tc + vec2(xoffs.x, yoffs.y)) * 0.01465;
	color += sample_(tex,tc + vec2(xoffs.y, yoffs.y)) * 0.05861;
	color += sample_(tex,tc + vec2(    0.0, yoffs.y)) * 0.09524;
	color += sample_(tex,tc + vec2(xoffs.z, yoffs.y)) * 0.05861;
	color += sample_(tex,tc + vec2(xoffs.w, yoffs.y)) * 0.01465;
	
	color += sample_(tex,tc + vec2(xoffs.x, 0.0)) * 0.02564;
	color += sample_(tex,tc + vec2(xoffs.y, 0.0)) * 0.09524;
	color += sample_(tex,tc + vec2(    0.0, 0.0)) * 0.15018;
	color += sample_(tex,tc + vec2(xoffs.z, 0.0)) * 0.09524;
	color += sample_(tex,tc + vec2(xoffs.w, 0.0)) * 0.02564;
	
	color += sample_(tex,tc + vec2(xoffs.x, yoffs.z)) * 0.01465;
	color += sample_(tex,tc + vec2(xoffs.y, yoffs.z)) * 0.05861;
	color += sample_(tex,tc + vec2(    0.0, yoffs.z)) * 0.09524;
	color += sample_(tex,tc + vec2(xoffs.z, yoffs.z)) * 0.05861;
	color += sample_(tex,tc + vec2(xoffs.w, yoffs.z)) * 0.01465;
	
	color += sample_(tex,tc + vec2(xoffs.x, yoffs.w)) * 0.00366;
	color += sample_(tex,tc + vec2(xoffs.y, yoffs.w)) * 0.01465;
	color += sample_(tex,tc + vec2(    0.0, yoffs.w)) * 0.02564;
	color += sample_(tex,tc + vec2(xoffs.z, yoffs.w)) * 0.01465;
	color += sample_(tex,tc + vec2(xoffs.w, yoffs.w)) * 0.00366;

	return color;
}

//Canonical noise function; replaced to prevent precision errors
//float rand(vec2 co){
//    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
//}

float rand(vec2 co)
{
    float a = 12.9898;
    float b = 78.233;
    float c = 43758.5453;
    float dt= dot(co.xy ,vec2(a,b));
    float sn= mod(dt,3.14);
    return fract(sin(sn) * c);
}

vec2 curve(vec2 uv)
{
	uv = (uv - 0.5) * 2.0;
	uv *= 1.1;	
	uv.x *= 1.0 + pow((abs(uv.y) / 5.0), 2.0);
	uv.y *= 1.0 + pow((abs(uv.x) / 4.0), 2.0);
	uv  = (uv / 2.0) + 0.5;
	uv =  uv *0.92 + 0.04;
	return uv;
}

void main()
{
    vec2 q = (vTexCoord.xy * TextureSize.xy / InputSize.xy);//fragCoord.xy / iResolution.xy;
    vec2 uv = q;
    uv = mix( uv, curve( uv ), CURVATURE ) * InputSize.xy / TextureSize.xy;
    vec3 col;
	float x =  sin(uv.y*21.0)*sin(uv.y*29.0)*sin(uv.y*31.0)*0.0017;
	float o =2.0*mod(fragCoord.y,2.0)/iResolution.x;
	x+=o;
   uv = uv * TextureSize / InputSize;
    col.r = 1.0*blur(iChannel0,vec2(uv.x+0.0009,uv.y+0.0009),1.2).x+0.005;
    col.g = 1.0*blur(iChannel0,vec2(uv.x+0.000,uv.y-0.0015),1.2).y+0.005;
    col.b = 1.0*blur(iChannel0,vec2(uv.x-0.0015,uv.y+0.000),1.2).z+0.005;

    col *= vec3(0.95,1.05,0.95);
	col = mix( col, col * col, 0.3) * 3.8;

	float scans = clamp( 0.35+0.15*sin(uv.y*iResolution.y*1.5), 0.0, 1.0);
	
	float s = pow(scans,0.9);
	col = col*vec3( s) ;

    col *= 1.0+0.0015*sin(300.0);
	
	col*=1.0-0.15*vec3(clamp((mod(fragCoord.x+o, 2.0)-1.0)*2.0,0.0,1.0));
	col = pow(col, vec3(0.45));

	if (uv.x < 0.0 || uv.x > 1.0)
		col *= 0.0;
	if (uv.y < 0.0 || uv.y > 1.0)
		col *= 0.0;
	
    FragColor = vec4(col,1.0);
} 
#endif