#pragma once
#include "../Core.h"

namespace RatUI
{
    enum class ETextureFormat : u8
    {
        Unknown = 0,

        R8,
        RG8,
        RGB8,
        RGBA8,
    };

    /**
     * @brief An opaque identifier for a texture resource.
     * @example OpenGL: GLuint; SDL2: SDL_Texture*; etc.
     */
    struct TextureID 
    { 
        union
        {
            void* Ptr;
            uptr  ID{ 0 };
        };

        constexpr bool operator==( const TextureID& a_Other ) const { return ID == a_Other.ID; } 
        constexpr bool IsValid() const { return *this != Null(); }
        static constexpr TextureID Null() { return TextureID{}; }
    };

    //struct TextureData
    //{
    //    TextureID      ID{ TextureID::Null() };
    //    ETextureFormat Format{ ETextureFormat::RGBA8 };
//
    //    u32         Width{ 0 }, Height{ 0 };
    //    const void* Pixels{ nullptr };
//
    //};
//
    //struct TextureUpdate
    //{
    //    TextureID   ID{ TextureID::Null() };
    //    u32         MipLevel{ 0 };
    //    Rectu       Region{ Rectu::Infinite() };
    //    const void* Data{ nullptr };
    //    size        DataSizeBytes{ 0 };
    //};

} // namespace RatUI