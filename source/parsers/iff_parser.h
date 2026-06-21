#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct IffTranslationKey {
    float pos[3];
    float time;
    float tangent_in[3];
    float tangent_out[3];
};

struct IffRotationKey {
    float rot[4];
    float time;
    float rot_b[4];
    float rot_c[4];
};

struct IffEvent {
    uint32_t event_id;
    float time;
    float param;
    float pos[3];
};

struct IffClip {
    float duration;
    uint32_t flags;
    uint32_t animation_id;
    
    std::vector<IffTranslationKey> root_translations;
    std::vector<std::vector<IffRotationKey>> bone_rotations;
    std::vector<IffEvent> events;
};

struct IffSkeleton {
    int32_t object_id;
    int32_t bone_count;
    std::vector<int32_t> parents;
    std::vector<float> translations; // 3 * f32 per bone
};

struct IffFile {
    bool valid = false;
    IffSkeleton skeleton;
    int32_t animation_capacity;
    std::vector<IffClip> clips;
};

IffFile IFF_Parse(const std::string& filepath);
void IFF_Dump(const std::string& filepath, const std::string& outfile);
