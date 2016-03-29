// Tested agains SilverLining Version 4.040 January 12, 2016

uniform int ViewOptions_EO;
uniform int ViewOptions_IR;

// Allow overriding of the final sky fragment color
void overrideSkyFragColor(inout vec4 finalColor)
{
	if ((ViewOptions_EO == 1) || (ViewOptions_IR == 1))
	{
		//uses NTSC conversion weights
		float gray = dot(finalColor.rgb, vec3(0.299, 0.587, 0.114));
		finalColor = vec4(gray, gray, gray, finalColor.a);
	}	
}

// Allows overriding of the fog color, fog blend factor, underlying cloud color, and alpha blending of the cloud.
void overrideStratusLighting(in vec3 fogColor, in float fogFactor, in vec3 cloudColor, in float cloudFade, inout vec4 finalColor)
{
}

// Overrides fragment colors in stratocumulus clouds. The pre-lit color (which incorporates light scattering within the cloud) is
// given as well as the light color. These are multiplied together to provide the default finalColor.
// finalColor = color * lightColor;
void overrideStratocumulus(in vec4 color, in vec4 lightColor, inout vec4 finalColor)
{

}


// Overrides fragment colors of billboards (cloud puffs, sun, moon.)
void overrideBillboardFragment(in vec4 texColor, in vec4 lightColor, inout vec4 finalColor)
{

}

//Overrides the final color of the Cirrus Clouds
// original finalColor is texel * gl_Color
void overrideCirrusColor(in vec4 texel, in vec4 litColor, inout vec4 finalColor)
{

}

// Overrides the particle color used to light rain, snow, and sleet.
void overrideParticleColor(in vec4 textureColor, in vec4 lightColor, inout vec4 particleColor)
{

}

// Write the final fragment color. Implement this if you need to write to multiple render targets, for example.
void writeFragmentData(in vec4 finalColor)
{
    gl_FragColor = finalColor;
}

