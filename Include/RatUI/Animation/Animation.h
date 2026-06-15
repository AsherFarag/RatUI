#pragma once
#include "../Core.h"
#include "Easing.h"
#include "Interpolate.h"

namespace RatUI
{
    /**
     * @brief Defines how an animation should behave when it reaches its end.
     */
    enum class EPlaybackMode : u8
    {
        Once,        ///< Play once and stop, holding the final value.
        Loop,        ///< Restart from the beginning on completion.
        PingPong,    ///< Reverse direction on each completion.
    };

    /**
     * @brief Represents the current state of an animation clip.
     */
    enum class EAnimationState : u8
    {
        Idle,   
        Playing,
        Paused, 
        Finished,
    };

    /**
     * @brief
     */
    template<typename T>
    class AnimationTrack
    {
    public:
        using ValueType  = T;
        using SetterType = Callback<const T&>;

        AnimationTrack( T a_From, T a_To, SetterType a_Setter )
            : m_From  ( std::move(a_From)   )
            , m_To    ( std::move(a_To)     )
            , m_Setter( std::move(a_Setter) )
        {}

        void Apply( f32 a_Time )
        {
            if ( m_Setter ) m_Setter( Interpolate( m_From, m_To, a_Time ) );
        }

        void ApplyFrom() { if ( m_Setter ) m_Setter( m_From ); }
        void ApplyTo()   { if ( m_Setter ) m_Setter( m_To );   }

    private:
        T          m_From;
        T          m_To;
        SetterType m_Setter;
    };

    /**
     * @brief
     */
    class AnimationClip
    {
    public:
        using TrackVariant = Variant<
            AnimationTrack<f32>,
            AnimationTrack<Vec2f>,
            AnimationTrack<Color>,
            AnimationTrack<CornerRounding>
        >;

        f32           Duration    { 0.3f };
        f32           Delay       { 0.f  };
        Easing        EasingFn    { EaseOutQuad{} };
        EPlaybackMode PlaybackMode{ EPlaybackMode::Once };

        Callback<> OnStart;
        Callback<> OnComplete;
        Callback<> OnLoop;     ///< Called each time a loop or ping-pong cycle completes.

        template<typename T>
        AnimationClip& AddTrack( T a_From, T a_To, typename AnimationTrack<T>::SetterType a_Setter )
        {
            EmplaceBack( m_Tracks, AnimationTrack<T>{ std::move(a_From), std::move(a_To), std::move(a_Setter) } );
            return *this;
        }

        /**
         * @brief
         * @return true while still playing, false if the animation has finished.
         */
        bool Tick( f32 a_DeltaSeconds )
        {
            if ( m_State != EAnimationState::Playing )
                return m_State != EAnimationState::Finished;

            m_Elapsed += a_DeltaSeconds;

            const f32 rawTime = Math::Clamp( m_Elapsed / Duration, 0.f, 1.f );

            const f32 time = (PlaybackMode == EPlaybackMode::PingPong && !m_Forward)
                             ? 1.f - rawTime
                             : rawTime;

            const f32 easedTime = EasingFn( time );
            for ( TrackVariant& track : m_Tracks )
            {
                std::visit( [&]( auto& a_Track ) { a_Track.Apply( easedTime ); }, track );
            }

            m_Progress = time;

            // Handle completion and looping logic
            if ( m_Elapsed >= Duration )
            {
                switch ( PlaybackMode )
                {
                }
            }

            return true;
        }

    protected:
        Array<TrackVariant> m_Tracks;

        f32 m_Elapsed{ 0.f };
        f32 m_Progress{ 0.f };
        bool m_Forward{ true };
        EAnimationState m_State{ EAnimationState::Idle };
    };

    using AnimationID = String;

    class AnimationPlayer
    {
    public:

        /**
         * @brief
         */
        void Tick(f32 a_DeltaSeconds)
        {
            for ( auto& [_, clip] : m_Clips )
            {
                clip.Tick( a_DeltaSeconds );
            }
        }

    protected:
        HashMap<AnimationID, AnimationClip> m_Clips;
    };

} // namespace RatUI