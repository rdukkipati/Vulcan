#include <SDL3/SDL.h>

#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

#define true 1
#define false 0

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef i32      b32;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float    f32;
typedef double   f64;

#define PI32 SDL_PI_F
#define PI64 SDL_PI_D

// IMPORTANT: This assert is assuming I'm using clang debugging
#if DEBUG
#define Assert(Expression) if(!(Expression)) { __builtin_trap(); }
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value)  ((u64)(Value) * 1024)
#define Megabytes(Value)  (Kilobytes(Value) * 1024)
#define Gigabytes(Value)  (Megabytes(Value) * 1024)
#define Terabytes(Value)  (Gigabytes(Value) * 1024)

#define ArrayCount(Array) ((i32)(sizeof(Array) / sizeof((Array)[0])))

typedef struct rng
{
    u64 State;
    u64 SequenceConstant;
} rng;

internal u32
Random_u32(rng *RNG)
{
    u64 OldState = RNG->State;
    RNG->State     = OldState * 6364136223846793005ULL + RNG->SequenceConstant;
    u32 Xorshifted = (u32)(((OldState >> 18u) ^ OldState) >> 27u);
    u32 Rot        = OldState >> 59u;
    return (Xorshifted >> Rot) | (Xorshifted << ((-Rot) & 31));
}

// NOTE: Max is exclusive
internal u32
Random_u32_InRange(u32 Min, u32 Max, rng *RNG)
{
    Assert(Min < Max);
    
    u32 Range = Max - Min;
    u32 Threshold = -Range % Range;
    
    while(true)
    {
        u32 Random = Random_u32(RNG);
        if(Random >= Threshold)
        {
            return Min + (Random % Range);
        }
    }
}

internal void
InitializeRNG(rng *RNG, u64 InitialState, u64 StreamID)
{
    RNG->State            = 0U;
    RNG->SequenceConstant = (StreamID << 1u) | 1u;
    Random_u32(RNG);
    RNG->State += InitialState;
    Random_u32(RNG);
}

internal f32
Random_f32(rng *RNG)
{
    u32 Value  = Random_u32(RNG);
    u32 Bits   = (Value >> 9) | 0x3f800000;
    f32 Result = *(f32 *)&Bits;
    return Result - 1.0f;
}

// NOTE: Max is exclusive
internal f32
Random_f32_InRange(f32 Min, f32 Max, rng *RNG)
{
    return Min + (Max - Min) * Random_f32(RNG);
}
