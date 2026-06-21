#include "iff_parser.h"
#include <fstream>
#include <iostream>
#include <cstring>

static uint32_t readBE32(std::ifstream& f) {
    uint8_t b[4];
    f.read((char*)b, 4);
    return (b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

static uint32_t readLE32(std::ifstream& f) {
    uint32_t v;
    f.read((char*)&v, 4);
    return v;
}

static float readLEFloat(std::ifstream& f) {
    float v;
    f.read((char*)&v, 4);
    return v;
}

static std::string readTag(std::ifstream& f) {
    char b[5] = {0};
    f.read(b, 4);
    return std::string(b);
}

IffFile IFF_Parse(const std::string& filepath, IffLogger logger) {
    IffFile out;
    std::ifstream f(filepath, std::ios::binary);
    if(!f) {
        if(logger) logger(0, "Failed to open IFF file: " + filepath);
        return out;
    }

    if(logger) logger(2, "Opened IFF file: " + filepath);

    int32_t bone_count = 0;
    IffClip* current_clip = nullptr;

    while(f && f.peek() != EOF) {
        std::string tag = readTag(f);
        if(f.eof()) break;
        uint32_t size = readBE32(f);
        if(logger) logger(3, "Parsed chunk TAG: [" + tag + "] SIZE: " + std::to_string(size));

        // Sanity check size to prevent out of bounds memory allocation crashes
        if(size > 100000000) {
            if(logger) logger(0, "Chunk size suspiciously large! Aborting to prevent crash.");
            break;
        }

        std::streampos next_pos = f.tellg() + (std::streamoff)size;
        if(size % 2 != 0) next_pos += 1;

        if (tag == "FORM") {
            std::string ftype = readTag(f);
            if(logger) logger(3, "FORM Type: " + ftype);
            if (ftype == "BOBJ" || ftype == "BOAL") {
                // don't skip over children, size is broken
                continue;
            } else if (ftype == "BOBH") {
                continue;
            } else if (ftype == "BOAN") {
                out.clips.push_back(IffClip());
                current_clip = &out.clips.back();
                continue;
            } else {
                f.seekg(next_pos); // skip unknown form
            }
        } else if (tag == "BOSH") {
            out.skeleton.object_id = readLE32(f);
            out.skeleton.bone_count = readLE32(f);
            bone_count = out.skeleton.bone_count;
            if(logger) logger(2, "BOSH: Parsed bone count: " + std::to_string(bone_count));
            f.seekg(next_pos);
        } else if (tag == "PLST") {
            if(logger) logger(3, "PLST: Reading " + std::to_string(bone_count) + " parents");
            for(int i=0; i<bone_count; i++) {
                if(f.eof()) break;
                out.skeleton.parents.push_back(readLE32(f));
            }
            f.seekg(next_pos);
        } else if (tag == "TLST") {
            if(logger) logger(3, "TLST: Reading " + std::to_string(bone_count) + " translations");
            for(int i=0; i<bone_count; i++) {
                if(f.eof()) break;
                out.skeleton.translations.push_back(readLEFloat(f));
                out.skeleton.translations.push_back(readLEFloat(f));
                out.skeleton.translations.push_back(readLEFloat(f));
            }
            f.seekg(next_pos);
        } else if (tag == "BALH") {
            uint32_t anim_count = readLE32(f);
            out.animation_capacity = readLE32(f);
            if(logger) logger(2, "BALH: Capacity: " + std::to_string(out.animation_capacity));
            f.seekg(next_pos);
        } else if (tag == "BOAH" && current_clip) {
            current_clip->duration = readLEFloat(f);
            current_clip->flags = readLE32(f);
            current_clip->animation_id = readLE32(f);
            if(logger) logger(2, "BOAH: Clip Duration: " + std::to_string(current_clip->duration));
            f.seekg(next_pos);
        } else if (tag == "BOTH" && current_clip) {
            uint32_t count = readLE32(f);
            if(count > 10000) { if(logger) logger(0, "BOTH count suspiciously large."); break; }
            current_clip->root_translations.resize(count);
            f.seekg(next_pos);
        } else if (tag == "BOTD" && current_clip) {
            for(auto& k : current_clip->root_translations) {
                if(f.eof()) break;
                k.pos[0] = readLEFloat(f); k.pos[1] = readLEFloat(f); k.pos[2] = readLEFloat(f);
                k.time = readLEFloat(f);
                k.tangent_in[0] = readLEFloat(f); k.tangent_in[1] = readLEFloat(f); k.tangent_in[2] = readLEFloat(f);
                k.tangent_out[0] = readLEFloat(f); k.tangent_out[1] = readLEFloat(f); k.tangent_out[2] = readLEFloat(f);
            }
            f.seekg(next_pos);
        } else if (tag == "BORH" && current_clip) {
            uint32_t count = readLE32(f);
            if(count > 10000) { if(logger) logger(0, "BORH count suspiciously large."); break; }
            current_clip->bone_rotations.push_back(std::vector<IffRotationKey>(count));
            f.seekg(next_pos);
        } else if (tag == "BORD" && current_clip) {
            auto& keys = current_clip->bone_rotations.back();
            for(auto& k : keys) {
                if(f.eof()) break;
                for(int i=0; i<4; i++) k.rot[i] = readLEFloat(f);
                k.time = readLEFloat(f);
                for(int i=0; i<4; i++) k.rot_b[i] = readLEFloat(f);
                for(int i=0; i<4; i++) k.rot_c[i] = readLEFloat(f);
            }
            f.seekg(next_pos);
        } else if (tag == "BOEH" && current_clip) {
            uint32_t count = readLE32(f);
            if(count > 10000) { if(logger) logger(0, "BOEH count suspiciously large."); break; }
            current_clip->events.resize(count);
            f.seekg(next_pos);
        } else if (tag == "BOED" && current_clip) {
            for(auto& e : current_clip->events) {
                if(f.eof()) break;
                e.event_id = readLE32(f);
                e.time = readLEFloat(f);
                e.param = readLEFloat(f);
                e.pos[0] = readLEFloat(f); e.pos[1] = readLEFloat(f); e.pos[2] = readLEFloat(f);
            }
            f.seekg(next_pos);
        } else {
            if(logger) logger(3, "Skipping unknown tag: " + tag);
            f.seekg(next_pos);
        }
    }
    out.valid = true;
    if(logger) logger(1, "Successfully parsed IFF file.");
    return out;
}

void IFF_Dump(const std::string& filepath, const std::string& outfile, IffLogger logger) {
    IffFile iff = IFF_Parse(filepath, logger);
    if(!iff.valid) {
        std::cerr << "Failed to parse IFF\n";
        return;
    }
    std::ofstream out(outfile);
    out << "IFF File Dump: " << filepath << "\n";
    out << "Object ID: " << iff.skeleton.object_id << "\n";
    out << "Bone Count: " << iff.skeleton.bone_count << "\n";
    out << "Animation Capacity: " << iff.animation_capacity << "\n";
    out << "Clips: " << iff.clips.size() << "\n\n";
    
    for(size_t i=0; i<iff.clips.size(); i++) {
        const auto& c = iff.clips[i];
        out << "Clip [" << i << "] ID: " << c.animation_id << ", Duration: " << c.duration 
            << ", Flags: " << c.flags << "\n";
        out << "  Events: " << c.events.size() << "\n";
        for(const auto& e : c.events) {
            out << "    Event " << e.event_id << " at time " << e.time << "\n";
        }
    }
}
