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
        Repeat,      ///< Restart from the beginning on completion for a specified number of times, then stop.
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
            if ( m_Setter ) m_Setter( Lerp( m_From, m_To, a_Time ) );
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
            AnimationTrack<Unit>,
            AnimationTrack<Pixel>,
            AnimationTrack<FontUnit>,
            AnimationTrack<Radians>,
            AnimationTrack<Degrees>,
            AnimationTrack<Vec2f>,
            AnimationTrack<Color>,
            AnimationTrack<CornerRounding>,
            AnimationTrack<RenderTransform>
        >;

        f32           Duration    { 0.3f };
        f32           Delay       { 0.f  };
        Easing        EasingFn    { EaseOutQuad{} };
        EPlaybackMode PlaybackMode{ EPlaybackMode::Once };

        Callback<> OnStart;
        Callback<> OnComplete;
        Callback<> OnLoop;     ///< Called each time a loop or ping-pong cycle completes.

        /** @brief The normalized progress of the animation, from 0 to 1. */
        f32 GetProgress() const { return m_Progress; }

        /** @brief The total elapsed time of the animation since it started. */
        f32 GetElapsed() const { return m_Elapsed; }

        /** @brief For Repeat mode, returns how many repeats are left before the animation finishes. */
        u32 GetRemainingRepeats() const { return m_RepeatCount; }

        bool IsIdle()     const { return m_State == EAnimationState::Idle;     }
        bool IsPlaying()  const { return m_State == EAnimationState::Playing; }
        bool IsPaused()   const { return m_State == EAnimationState::Paused;  }
        bool IsFinished() const { return m_State == EAnimationState::Finished; }

        /**
         * @brief
         */
        AnimationClip& Play( u32 a_RepeatCount = 1 )
        {
            m_RepeatCount = a_RepeatCount;
            if ( IsPlaying() )
                return *this; // Already playing, do nothing.

            m_State = EAnimationState::Playing;
            m_Elapsed = 0.f;
            m_Progress = 0.f;
            m_Forward = true;

            for ( TrackVariant& track : m_Tracks )
            {
                std::visit( [&]( auto& a_Track ) { a_Track.ApplyFrom(); }, track );
            }

            if ( OnStart ) OnStart();

            return *this;
        }

        void Pause()
        {
            if ( IsPlaying() )
                m_State = EAnimationState::Paused;
        }

        void Resume()
        {
            if ( IsPaused() )
                m_State = EAnimationState::Playing;
        }

        void Stop()
        {
            if ( IsPlaying() || IsPaused() )
            {
                m_State = EAnimationState::Finished;
                m_Elapsed = Duration; // Jump to end
                m_Progress = 1.f;

                for ( TrackVariant& track : m_Tracks )
                {
                    std::visit( [&]( auto& a_Track ) { a_Track.ApplyTo(); }, track );
                }

                if ( OnComplete ) OnComplete();
            }
        }

        /**
         * @brief Updates the animation state.
         * @param a_DeltaSeconds The time elapsed since the last update.
         * @return true while still playing, false if the animation has finished.
         */
        void Tick( f32 a_DeltaSeconds )
        {
            if ( !IsPlaying() )
                return;

            m_Elapsed += a_DeltaSeconds;

            // Calculate normalized time [0, 1]
            const f32 normTime = Math::Clamp( m_Elapsed / Duration, 0.f, 1.f );

            // Handle ping-pong timing
            const f32 time = (PlaybackMode == EPlaybackMode::PingPong && !m_Forward)
                             ? 1.f - normTime
                             : normTime;

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
                    case EPlaybackMode::Once:
                        m_State = EAnimationState::Finished;
                        if ( OnComplete ) OnComplete();
                        break;

                    case EPlaybackMode::Loop:
                        m_Elapsed = 0.f;
                        if ( OnLoop ) OnLoop();
                        break;

                    case EPlaybackMode::PingPong:
                        m_Elapsed = 0.f;
                        m_Forward = !m_Forward;
                        if ( OnLoop ) OnLoop();
                        break;

                    case EPlaybackMode::Repeat:
                        RATUI_ASSERT( m_RepeatCount > 0, "Repeat mode should have a positive repeat count." );
                        m_Elapsed = 0.f;
                        --m_RepeatCount;
                        if ( m_RepeatCount > 0 )
                        {
                            if ( OnLoop ) OnLoop();
                        }
                        else
                        {
                            m_State = EAnimationState::Finished;
                            if ( OnComplete ) OnComplete();
                        }
                        break;
                }
            }
        }

        // ---------------------------------------------------
        // Builder Methods
        // ---------------------------------------------------

        AnimationClip& WithDuration( f32 a_Duration ) { Duration = a_Duration; return *this; }
        AnimationClip& WithDelay( f32 a_Delay ) { Delay = a_Delay; return *this; }
        AnimationClip& WithEasing( Easing a_Easing ) { EasingFn = std::move( a_Easing ); return *this; }
        AnimationClip& WithPlaybackMode( EPlaybackMode a_Mode ) { PlaybackMode = a_Mode; return *this; }
        AnimationClip& WithOnStart( Callback<> a_Callback ) { OnStart = std::move( a_Callback ); return *this; }
        AnimationClip& WithOnComplete( Callback<> a_Callback ) { OnComplete = std::move( a_Callback ); return *this; }
        AnimationClip& WithOnLoop( Callback<> a_Callback ) { OnLoop = std::move( a_Callback ); return *this; }

        template<typename T>
        AnimationClip& WithEasing( T a_EasingFunc ) { EasingFn = Easing{ std::move( a_EasingFunc ) }; return *this; }

        template<typename T>
        AnimationClip& AddTrack( T a_From, T a_To, typename AnimationTrack<T>::SetterType a_Setter )
        {
            RATUI_USER_ASSERT( a_Setter, "AnimationTrack setter cannot be null." );
            EmplaceBack( m_Tracks, AnimationTrack<T>{ std::move(a_From), std::move(a_To), std::move(a_Setter) } );
            return *this;
        }

    protected:
        Array<TrackVariant> m_Tracks;

        f32 m_Elapsed{ 0.f };
        f32 m_Progress{ 0.f };
        u32 m_RepeatCount{ 0 }; ///< Used only for Repeat mode to track remaining repeats.
        bool m_Forward{ true };
        EAnimationState m_State{ EAnimationState::Idle };
    };

    class AnimationPlayer
    {
    public:

        AnimationPlayer& AddClip( StringID a_ID, AnimationClip a_Clip )
        {
            Emplace( m_Clips, a_ID, std::move(a_Clip) );
            return *this;
        }

        AnimationClip* TryGetClip( StringID a_ID )
        {
            auto it = m_Clips.find( a_ID );
            return ( it != m_Clips.end() ) ? &it->second : nullptr;
        }

        void RemoveClip( StringID a_ID )
        {
            m_Clips.erase( a_ID );
        }

        void Play( StringID a_ID, u32 a_RepeatCount = 1 )
        {
            if ( AnimationClip* clip = TryGetClip( a_ID ) )
            {
                clip->Play( a_RepeatCount );
            }
        }

        void Pause( StringID a_ID )
        {
            if ( AnimationClip* clip = TryGetClip( a_ID ) )
            {
                clip->Pause();
            }
        }

        void Stop( StringID a_ID )
        {
            if ( AnimationClip* clip = TryGetClip( a_ID ) )
            {
                clip->Stop();
            }
        }

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
        HashMap<StringID, AnimationClip, StringIDHash, std::equal_to<StringID>> 
        m_Clips;
    };

} // namespace RatUI