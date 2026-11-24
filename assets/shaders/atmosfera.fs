#version 330
// Pós-processamento atmosférico (estilo Bloodborne): vinheta, sombras frias,
// luzes quentes e grão de filme por pixel virtual (não por pixel da janela).
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float tempo;
uniform vec2 resolucao;
out vec4 finalColor;

void main() {
    vec4 c = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    vec2 uv = fragTexCoord;

    // Vinheta oval.
    float d = distance(uv * vec2(1.0, 0.85), vec2(0.5, 0.425));
    c.rgb *= mix(0.55, 1.0, smoothstep(0.75, 0.30, d));

    // Split-toning: sombras puxam para o azul, luzes para o âmbar.
    float l = dot(c.rgb, vec3(0.299, 0.587, 0.114));
    c.rgb = mix(c.rgb, c.rgb * vec3(0.88, 0.94, 1.15), (1.0 - l) * 0.35);
    c.rgb = mix(c.rgb, c.rgb * vec3(1.08, 1.0, 0.9), l * 0.25);

    // Grão alinhado à grade de pixels da tela virtual.
    vec2 px = floor(uv * resolucao);
    float n = fract(sin(dot(px + floor(tempo * 18.0), vec2(12.9898, 78.233))) * 43758.5453);
    c.rgb += (n - 0.5) * 0.03;

    finalColor = vec4(c.rgb, 1.0);
}
