#version 330 core
in vec3 FragPos;
in vec2 TexCoords;
in mat3 TBN;

out vec4 FragColor;

uniform sampler2D albedoMap;
uniform sampler2D normalMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D shadowMapFish;
uniform sampler2D shadowMapCam;
uniform sampler2D flowMap;

uniform vec3 camPos;
uniform vec3 lightPositions[4];
uniform vec3 lightDirections[4];
uniform vec3 lightColors[4];
uniform float lightRadii[4];
uniform int numLights;
uniform float headlightCosCutoff;
uniform float headlightSoftCosCutoff;
uniform mat4 fishLightSpaceMatrix;
uniform mat4 camLightSpaceMatrix;
uniform bool useNormalMap;
uniform float materialMetallic;
uniform float materialRoughness;
uniform vec3 materialAlbedoTint;
uniform vec3 materialEmissive;
uniform float underwaterFogDensity;
uniform float time;
uniform bool useSandFlow;
uniform bool shadowsEnabled;
uniform bool u_enableTopMoss;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / max(PI * denom * denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness) *
           GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float ShadowCalculation(sampler2D mapTex, vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0 || projCoords.z < 0.0) return 0.0;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;
    float current = projCoords.z;
    float bias = 0.0025;
    vec2 texel = 1.0 / vec2(textureSize(mapTex, 0));
    float sum = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcf = texture(mapTex, projCoords.xy + vec2(x, y) * texel).r;
            sum += current - bias > pcf ? 1.0 : 0.0;
        }
    }
    return sum / 9.0;
}

void main() {
    vec2 sandUV0 = TexCoords;
    vec2 sandUV1 = TexCoords;
    float sandBlend = 0.0;
    if (useSandFlow) {
        vec2 flow = texture(flowMap, TexCoords).rg * 2.0 - 1.0;
        const float flowSpeed = 0.08;
        float phase0 = fract(time * flowSpeed);
        float phase1 = fract(time * flowSpeed + 0.5);
        sandBlend = abs(phase0 - 0.5) * 2.0;
        sandUV0 = TexCoords + flow * phase0;
        sandUV1 = TexCoords + flow * phase1;
    }

    vec3 albedo;
    if (useSandFlow) {
        albedo = mix(texture(albedoMap, sandUV0).rgb, texture(albedoMap, sandUV1).rgb, sandBlend) * materialAlbedoTint;
    } else {
        albedo = texture(albedoMap, TexCoords).rgb * materialAlbedoTint;
    }
    float metallic = texture(metallicMap, TexCoords).r * materialMetallic;
    float roughness = texture(roughnessMap, TexCoords).r * materialRoughness;

    vec3 N = normalize(TBN[2]);
    if (useNormalMap) {
        vec3 tangentNormal;
        if (useSandFlow) {
            tangentNormal = mix(texture(normalMap, sandUV0).rgb, texture(normalMap, sandUV1).rgb, sandBlend) * 2.0 - 1.0;
        } else {
            tangentNormal = texture(normalMap, TexCoords).rgb * 2.0 - 1.0;
        }
        N = normalize(TBN * tangentNormal);
    }

    if (u_enableTopMoss) {
        float mossFactor = clamp(dot(normalize(N), vec3(0.0, 1.0, 0.0)) * 2.5 - 0.5, 0.0, 1.0);
        vec3 mossColor = vec3(0.06, 0.22, 0.09);
        albedo = mix(albedo, mossColor, mossFactor * 0.9);
        roughness = mix(roughness, 0.95, mossFactor);
        metallic = mix(metallic, 0.0, mossFactor);
    }

    vec3 V = normalize(camPos - FragPos);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);
    float fishShadow = ShadowCalculation(shadowMapFish, fishLightSpaceMatrix * vec4(FragPos, 1.0));
    float camShadow = ShadowCalculation(shadowMapCam, camLightSpaceMatrix * vec4(FragPos, 1.0));

    for (int i = 0; i < numLights; ++i) {
        vec3 toLight = lightPositions[i] - FragPos;
        vec3 L = normalize(toLight);
        vec3 H = normalize(V + L);
        float dist = length(toLight);
        float attenuation = 1.0 / (1.0 + 0.045 * dist + 0.008 * dist * dist);
        attenuation *= 1.0 - smoothstep(lightRadii[i] * 0.65, lightRadii[i], dist);
        if (i < 2) {
            vec3 spotDir = normalize(lightDirections[i]);
            float theta = dot(normalize(FragPos - lightPositions[i]), spotDir);
            float spot = smoothstep(headlightSoftCosCutoff, headlightCosCutoff, theta);
            attenuation *= spot;
        }

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        vec3 numerator = NDF * G * F;
        float denom = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denom;
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
        float NdotL = max(dot(N, L), 0.0);
        float shadowFactor = 1.0;
        if (shadowsEnabled) {
            const float fishShadowStrength = 0.78;
            const float cameraShadowStrength = 0.58;
            if (i == 2) shadowFactor *= (1.0 - fishShadow * fishShadowStrength);
            if (i < 2) shadowFactor *= (1.0 - camShadow * cameraShadowStrength);
        }
        Lo += (kD * albedo / PI + specular) * lightColors[i] * NdotL * attenuation * shadowFactor;
    }

    vec3 ambient = albedo * vec3(0.06, 0.10, 0.14);
    vec3 color = ambient + Lo + materialEmissive;

    float fog = 1.0 - exp(-underwaterFogDensity * 2.5 * length(camPos - FragPos));
    vec3 fogColor = vec3(0.06, 0.21, 0.23);
    color = mix(color, fogColor, clamp(fog, 0.0, 0.98));

    FragColor = vec4(color, 1.0);
}
