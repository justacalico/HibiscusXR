#include "shaders.h"

#include "../common/log.h"

const char* const kSceneVS = R"(
attribute vec3 aPos;
attribute vec3 aCol;
uniform mat4 uMVP;
varying vec3 vCol;
void main() { vCol = aCol; gl_Position = uMVP * vec4(aPos, 1.0); }
)";

const char* const kSceneFS = R"(
precision mediump float;
varying vec3 vCol;
void main() { gl_FragColor = vec4(vCol, 1.0); }
)";

const char* const kWarpVS = R"(
attribute vec2 aPos;
varying vec2 vUV;
void main() { vUV = aPos * 0.5 + 0.5; gl_Position = vec4(aPos, 0.0, 1.0); }
)";

const char* const kFloatVS = R"(
attribute vec3 aPos;
attribute vec2 aUV;
uniform mat4 uMVP;
varying vec2 vUV;
void main() { vUV = aUV; gl_Position = uMVP * vec4(aPos, 1.0); }
)";

// app panel: samples an external OES texture fed by the virtual display,
// corners rounded off in panel space
const char* const kFloatFS = R"(
#extension GL_OES_EGL_image_external : require
precision mediump float;
varying vec2 vUV;
uniform samplerExternalOES uTex;
uniform mat4 uST;
uniform vec2 uHalf;
uniform float uRadius;
void main() {
    vec2 uv = (uST * vec4(vUV, 0.0, 1.0)).xy;
    vec4 c = texture2D(uTex, uv);
    vec2 p = (vUV - 0.5) * 2.0 * uHalf;
    vec2 q = abs(p) - uHalf + vec2(uRadius);
    float d = min(max(q.x, q.y), 0.0) + length(max(q, vec2(0.0))) - uRadius;
    float a = 1.0 - smoothstep(-0.0015, 0.0015, d);
    gl_FragColor = vec4(c.rgb, c.a * a);
}
)";

// solid rounded shapes - window chrome, shadows, rings. aUV runs -1..1 across
// the quad; uQuad is the quad's physical half size, uBox the shape's, so a
// soft edge can spread past the box into the quad's padding
const char* const kShapeVS = R"(
attribute vec3 aPos;
attribute vec2 aUV;
uniform mat4 uMVP;
varying vec2 vUV;
void main() { vUV = aUV; gl_Position = uMVP * vec4(aPos, 1.0); }
)";

const char* const kShapeFS = R"(
precision mediump float;
varying vec2 vUV;
uniform vec4 uColor;
uniform vec2 uQuad;
uniform vec2 uBox;
uniform float uRadius;   // top corners
uniform float uRadiusB;  // bottom corners - 0 leaves them square
uniform float uBorder;   // >0 ring half-thickness, 0 solid, <0 outward fade
uniform float uSoft;
void main() {
    vec2 p = vUV * uQuad;
    float r = p.y > 0.0 ? uRadius : uRadiusB;
    vec2 q = abs(p) - uBox + vec2(r);
    float d = min(max(q.x, q.y), 0.0) + length(max(q, vec2(0.0))) - r;
    float a;
    if (uBorder > 0.0)
        a = 1.0 - smoothstep(uBorder - uSoft, uBorder + uSoft, abs(d));
    else if (uBorder < 0.0)
        a = 1.0 - smoothstep(0.0, uSoft, d);
    else
        a = 1.0 - smoothstep(-uSoft, uSoft, d);
    if (a < 0.01) discard;
    gl_FragColor = vec4(uColor.rgb, uColor.a * a);
}
)";

// Stock Pico lens field: scale = K0 + K2 r^2 + K4 r^4 + K6 r^6 in tan-angle
// radius, plus per-channel chromatic scales. r comes out of the 90-degree
// projection as ndc = 2p and tan(theta) = (aspect*ndc.x, ndc.y). Off-texture
// samples go black rather than smearing the edge texel.
const char* const kWarpFS = R"(
precision mediump float;
varying vec2 vUV;
uniform sampler2D uTex;
uniform vec2 uLensCenter;
uniform float uAspect;
uniform float uK0;
uniform float uK2;
uniform float uK4;
uniform float uK6;
uniform vec2 uChroma;
void main() {
    vec2 p = vUV - uLensCenter;
    vec2 q = vec2(p.x * uAspect, p.y) * 2.0;
    float r2 = dot(q, q);
    float scale = uK0 + r2 * (uK2 + r2 * (uK4 + r2 * uK6));
    vec2 uv = uLensCenter + p * scale;
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        gl_FragColor = vec4(0.0);
    } else {
        vec2 d = uv - uLensCenter;
        gl_FragColor = vec4(
            texture2D(uTex, uLensCenter + d * uChroma.x).r,
            texture2D(uTex, uv).g,
            texture2D(uTex, uLensCenter + d * uChroma.y).b,
            texture2D(uTex, uv).a);
    }
}
)";

// dock icon: plain 2D texture (an app-icon bitmap upload), corners rounded
// off in quad space like the panel shader but with no ST transform and a
// uniform alpha so minimized items can dim
const char* const kIconFS = R"(
precision mediump float;
varying vec2 vUV;
uniform sampler2D uTex;
uniform vec2 uHalf;
uniform float uRadius;
uniform float uAlpha;
void main() {
    vec4 c = texture2D(uTex, vUV);
    vec2 p = (vUV - 0.5) * 2.0 * uHalf;
    vec2 q = abs(p) - uHalf + vec2(uRadius);
    float d = min(max(q.x, q.y), 0.0) + length(max(q, vec2(0.0))) - uRadius;
    float a = 1.0 - smoothstep(-0.0015, 0.0015, d);
    gl_FragColor = vec4(c.rgb, c.a * a * uAlpha);
}
)";

// hold-progress ring: a thin circle whose arc lights up clockwise from the
// top as uProg runs 0..1. aUV is -1..1 across the quad so angles and radii
// stay in circle space
const char* const kHoldFS = R"(
precision mediump float;
varying vec2 vUV;
uniform vec4 uColor;
uniform float uProg;
void main() {
    float r = length(vUV);
    float ring = 1.0 - smoothstep(0.10, 0.14, abs(r - 0.82));
    // angle from the top of the circle, clockwise, 0..1
    float frac = atan(vUV.x, vUV.y) / 6.2831853;
    if (frac < 0.0) frac += 1.0;
    float a = ring * (frac <= uProg ? 1.0 : 0.25);
    if (a < 0.01) discard;
    gl_FragColor = vec4(uColor.rgb, uColor.a * a);
}
)";

const char* const kTextVS = R"(
attribute vec3 aPos;
attribute vec2 aUV;
uniform mat4 uMVP;
varying vec2 vUV;
void main() { vUV = aUV; gl_Position = uMVP * vec4(aPos, 1.0); }
)";

const char* const kTextFS = R"(
precision mediump float;
varying vec2 vUV;
uniform sampler2D uFont;
uniform vec3 uColor;
void main() {
    float a = texture2D(uFont, vUV).a;
    if (a < 0.05) discard;
    gl_FragColor = vec4(uColor, a);
}
)";

GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char l[512]; glGetShaderInfoLog(s, sizeof(l), nullptr, l);
        LOGE("shader: %s", l); glDeleteShader(s); return 0; }
    return s;
}

GLuint linkProg(const char* vs, const char* fs) {
    GLuint v = compileShader(GL_VERTEX_SHADER, vs),
           f = compileShader(GL_FRAGMENT_SHADER, fs);
    if (!v || !f) return 0;
    GLuint p = glCreateProgram();
    glAttachShader(p, v); glAttachShader(p, f); glLinkProgram(p);
    GLint ok = 0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if (!ok) { char l[512]; glGetProgramInfoLog(p, sizeof(l), nullptr, l);
        LOGE("link: %s", l); glDeleteProgram(p); return 0; }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}
