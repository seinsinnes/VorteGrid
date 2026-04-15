/** \file timer.h

    \brief Timer utility routines

    \author Copyright 2010 MJG; All rights reserved.
*/
#ifndef TIMER_H
#define TIMER_H

#include <time.h>
#include <sys/time.h>

#if defined( _XBOX )
    #include <xtl.h>
#elif defined( WIN32 )
    #include <windows.h>
#elif defined( __MACH__ ) || defined( __linux__ )
    #include <time.h>
#endif


#if defined( __MACH__ ) || defined( __linux__ )

void QueryPerformanceCounter(LARGE_INTEGER *tsl) {
    struct timespec ts;
    
    // Use CLOCK_MONOTONIC instead of CLOCK_REALTIME
    clock_gettime(CLOCK_MONOTONIC, &ts);
    
    // Cast to long long to prevent 32-bit overflow before adding nsec
    *tsl = (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

void QueryPerformanceFrequency(LARGE_INTEGER *freq) {
    *freq = 1000000000LL; 
}

#endif

// Macros ----------------------------------------------------------------------


// Types -----------------------------------------------------------------------

/** Timer block.

    Create an object of this class at the beginning of any code to time, in
    its own scope.  When the scope ends, the block timing ends automatically.

*/
class Timer
{
    public:
        /** Initialize a timer.
        */
        Timer()
        {
            //QueryPerformanceFrequency( & mQwTicksPerSec ) ;
            //mSecondsPerTick = 1.0f / float( mQwTicksPerSec ) ;
            mSecondsPerTick = 1.0f / 1e9;
            StartTimer() ;
        }


        ~Timer() {}


        /** Reset timer, that is, start it at zero.
        */
        void StartTimer()
        {
            QueryPerformanceCounter( & mTimeEnter ) ;
        }


        /** Get duration elapsed since timer started.
        */
        float GetElapsedTimeSeconds()
        {
            LARGE_INTEGER iTimeExit ;
            QueryPerformanceCounter( & iTimeExit ) ;
            LARGE_INTEGER iDuration ;
            iDuration = iTimeExit - mTimeEnter ;
            const float fDuration = mSecondsPerTick * static_cast< float >( iDuration ) ;
            return fDuration ;
        }


        /** Busy-wait for a given amount of time.
        */
        static void BusySleep( float sleepDurationSeconds )
        {
            Timer timer ;
            for(;;)
            {   // Loop unconditionally.
                const float elapsedTimeSeconds = timer.GetElapsedTimeSeconds() ;
                if( elapsedTimeSeconds >= sleepDurationSeconds )
                {   // Sleep time has elapsed.
                    break ;
                }
            }
        }


    private:
        LARGE_INTEGER   mQwTicksPerSec  ;   ///< Performance counter property; number of ticks per second.
        float           mSecondsPerTick ;   ///< Floating-point reciprocal of ticks-per-second
        LARGE_INTEGER   mTimeEnter      ;   ///< Performance counter value when timer started.
} ;

// Private variables -----------------------------------------------------------
// Public variables ------------------------------------------------------------
// Private functions -----------------------------------------------------------
// Public functions ------------------------------------------------------------

#endif
