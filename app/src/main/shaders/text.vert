#version 450
layout(push_constant) uniform Push {
    vec4 color;
    vec4 screenInfo;
} push;
layout(location = 0) in vec2 inPos;
void main() {
    vec2 ndc = (inPos / push.screenInfo.xy) * 2.0 - 1.0;
    gl_Position = vec4(ndc.x, ndc.y, 0.0, 1.0);
}
