#type vertex
#version 100

precision mediump float;
attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec4 vertexColor;
varying vec2 fragTexCoord;
varying vec4 fragColor;
uniform mat4 mvp;
void main()
{
    fragTexCoord = vertexTexCoord; 
    fragColor = vertexColor;
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}


#type fragment
#version 100                       
precision mediump float; 

uniform vec2 pi;
uniform vec2 gamma;

uniform sampler2D colorTexture;
uniform sampler2D lookupTableTexture0;
uniform sampler2D lookupTableTexture1;

uniform vec2 sunPosition;
uniform vec2 enabled;
uniform vec2 texSize; // <-- Passed manually from CPU

void main() {
  vec2 uv = gl_FragCoord.xy / texSize;
  vec4 color = texture2D(colorTexture, uv);

  if (enabled.x != 1.0) {
    gl_FragColor = color;
    return;
  }

  color.rgb = pow(color.rgb, vec3(gamma.y));

  float u = floor(color.b * 15.0) / 15.0 * 240.0;
  u += floor(color.r * 15.0) / 15.0 * 15.0;
  u /= 255.0;

  float v = 1.0 - floor(color.g * 15.0) / 15.0;

  vec3 left0 = texture2D(lookupTableTexture0, vec2(u, v)).rgb;
  vec3 left1 = texture2D(lookupTableTexture1, vec2(u, v)).rgb;

  u = ceil(color.b * 15.0) / 15.0 * 240.0;
  u += ceil(color.r * 15.0) / 15.0 * 15.0;
  u /= 255.0;

  v = 1.0 - ceil(color.g * 15.0) / 15.0;

  vec3 right0 = texture2D(lookupTableTexture0, vec2(u, v)).rgb;
  vec3 right1 = texture2D(lookupTableTexture1, vec2(u, v)).rgb;

  float sunPos = sin(sunPosition.x * pi.y);
  sunPos = 0.5 * (sunPos + 1.0);

  vec3 left = mix(left0, left1, sunPos);
  vec3 right = mix(right0, right1, sunPos);

  color.r = mix(left.r, right.r, fract(color.r * 15.0));
  color.g = mix(left.g, right.g, fract(color.g * 15.0));
  color.b = mix(left.b, right.b, fract(color.b * 15.0));

  color.rgb = pow(color.rgb, vec3(gamma.x));

  gl_FragColor = color;
}
