#version 120

attribute vec4 aPos; //base shape vertex data
attribute vec3 aNor;
attribute vec2 aTex;

attribute vec4 delta_xa;
attribute vec3 delta_na;

attribute vec4 delta_xb;
attribute vec3 delta_nb;

attribute vec4 delta_xc;
attribute vec3 delta_nc;

uniform mat4 P;
uniform mat4 MV;

uniform float a_t;
uniform float b_t;
uniform float c_t;

varying vec3 vPos;
varying vec3 vNor;
varying vec2 vTex;

void main()
{
	//newPos = aPos;
	//newNor = aNor;
	vec4 newPos = aPos + (a_t * delta_xa) + (b_t * delta_xb) + (c_t * delta_xc);
	vec3 newNor = normalize(aNor + (a_t * delta_na) + (b_t * delta_nb) + (c_t * delta_nc)); //this needs to be normalized? -- yes
	newPos.w = 1.0;

	vec4 posCam = MV * newPos;
	vec3 norCam = (MV * vec4(newNor, 0.0)).xyz;
	gl_Position = P * posCam;
	vPos = posCam.xyz;
	vNor = norCam;
	vTex = aTex;
}
