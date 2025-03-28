#pragma once

#include <math.h>
#include <stdio.h>

#define PI 3.14159265358979323846f

namespace easing
{
    constexpr float c1 = 1.70158f;
    constexpr float c2 = c1 * 1.525f;
    constexpr float c3 = c1 + 1;
    constexpr float c4 = (2 * PI) / 3.f;
    constexpr float c5 = (2 * PI) / 4.5f;

    // Linear easing function
    inline float linear(float max_time, float current_time)
    {
        float t = current_time / max_time;
        return t; // Linear interpolation from 0 to 1
    }

    // Quadratic easing functions
    inline float quad_in(float max_time, float current_time)
    {
        float t = current_time / max_time;
        return t * t;
    }

    inline float quad_out(float max_time, float current_time)
    {
        float t = current_time / max_time;
        return t * (2 - t);
    }

    inline float quad_in_out(float max_time, float current_time)
    {
        float t = current_time / max_time;
        if (t < 0.5f)
        {
            return 2 * t * t;
        }
        return -1 + (4 - 2 * t) * t;
    }

    // Cubic easing functions
    inline float cubic_in(float max_time, float current_time)
    {
        float t = current_time / max_time;
        return t * t * t;
    }

    inline float cubic_out(float max_time, float current_time)
    {
        float t = current_time / max_time - 1;
        return t * t * t + 1;
    }

    inline float cubic_in_out(float max_time, float current_time)
    {
        float t = current_time / max_time;
        if (t < 0.5f)
        {
            return 4 * t * t * t;
        }
        t -= 1;
        return (t * t * t + 1);
    }

    // Quartic easing functions
    inline float quart_in(float max_time, float current_time)
    {
        float t = current_time / max_time;
        return t * t * t * t;
    }

    inline float quart_out(float max_time, float current_time)
    {
        float t = current_time / max_time - 1;
        return -(t * t * t * t - 1);
    }

    inline float quart_in_out(float max_time, float current_time)
    {
        float t = current_time / max_time;
        if (t < 0.5f)
        {
            return 8 * t * t * t * t;
        }
        t -= 1;
        return -(t * t * t * t - 1);
    }

    // Quintic easing functions
    inline float quint_in(float max_time, float current_time)
    {
        float t = current_time / max_time;
        return t * t * t * t * t;
    }

    inline float quint_out(float max_time, float current_time)
    {
        float t = current_time / max_time - 1;
        return (t * t * t * t * t + 1);
    }

    inline float quint_in_out(float max_time, float current_time)
    {
        float t = current_time / max_time;
        if (t < 0.5f)
        {
            return 16 * t * t * t * t * t;
        }
        t -= 1;
        return (t * t * t * t * t + 1);
    }

    // Exponential easing functions
    inline float expo_in(float max_time, float current_time)
    {
        float t = current_time / max_time;
        return t == 0 ? 0 : powf(2, 10 * (t - 1));
    }

    inline float expo_out(float max_time, float current_time)
    {
        float t = current_time / max_time;
        return t == 1 ? 1 : (-powf(2, -10 * t) + 1);
    }

    inline float expo_in_out(float max_time, float current_time)
    {
        float t = current_time / max_time;
        if (t == 0)
            return 0;
        if (t == 1)
            return 1;
        if (t < 0.5f)
            return 0.5f * powf(2, 20 * t - 10);
        return 1 - 0.5f * powf(2, -20 * t + 10);
    }

    // Circular easing functions
    inline float circ_in(float max_time, float current_time)
    {
        float t = current_time / max_time;
        return 1 - sqrt(1 - t * t);
    }

    inline float circ_out(float max_time, float current_time)
    {
        float t = current_time / max_time - 1;
        return sqrt(1 - t * t);
    }

    inline float circ_in_out(float max_time, float current_time)
    {
        float t = current_time / max_time * 2;
        if (t < 1)
        {
            return 0.5f * (1 - sqrt(1 - t * t));
        }
        t -= 2;
        return 0.5f * (sqrt(1 - t * t) + 1);
    }

    // Elastic easing functions
    inline float elastic_in(float max_time, float current_time)
    {
        float t = current_time / max_time;
        return sinf(13 * PI / 2 * t) * powf(2, 10 * (t - 1));
    }

    inline float elastic_out(float max_time, float current_time)
    {
        float t = current_time / max_time;
        return sinf(-13 * PI / 2 * (t + 1)) * powf(2, -10 * t) + 1;
    }

    inline float elastic_in_out(float max_time, float current_time)
    {
        float t = current_time / max_time;
        if (t < 0.5f)
        {
            return 0.5f * (sinf(13 * PI * t) * powf(2, 20 * t - 10));
        }
        return 0.5f * (sinf(-13 * PI * t) * powf(2, -20 * t + 10)) + 1;
    }

    inline float back_in(float max_time, float current_time)
    {
        float x = current_time / max_time;
        return c3 * x * x * x - c1 * x * x;
    }
    
    inline float back_out(float max_time, float current_time)
    {
        float x = current_time / max_time;
        return 1.f + c3 * pow(x - 1, 3.f) + c1 * pow(x - 1, 2.f);
    }
    
    inline float back_in_out(float max_time, float current_time)
    {
        float x = current_time / max_time;
        return x < 0.5f ? (pow(2.f * x, 2.f) * ((c2 + 1.f) * 2.f * x - c2)) / 2.f
                        : (pow(2.f * x - 2.f, 2.f) * ((c2 + 1.f) * (x * 2.f - 2.f) + c2) + 2.f) / 2.f;
    }

    inline float bounce_out(float x)
    {
        const float n1 = 7.5625f;
        const float d1 = 2.75f;
  
        if (x < 1.f / d1)
        {
            return n1 * x * x;
        }
        else if (x < 2.f / d1)
        {
            return n1 * (x -= 1.5f / d1) * x + 0.75f;
        }
        else if (x < 2.5 / d1)
        {
            return n1 * (x -= 2.25f / d1) * x + 0.9375f;
        }
        else
        {
            return n1 * (x -= 2.625f / d1) * x + 0.984375f;
        }
    }

    inline float bounce_out(float max_time, float current_time)
    {
        float x = current_time / max_time;
        return bounce_out(x);
    }

    inline float bounce_in(float max_time, float current_time)
    {
        float x = current_time / max_time;
        return 1.f - bounce_out(1.f - x);
    }

    inline float bounce_in_out(float max_time, float current_time)
    {
        float x = current_time / max_time;
        return x < 0.5f ? (1.f - bounce_out(1.f - 2.f * x)) / 2.f : (1.f + bounce_out(2.f * x - 1.f)) / 2.f;
    }

    inline float sine_in(float max_time, float current_time)
    {
        float x = current_time / max_time;
        return 1.f - cosf((x * PI) / 2.f);
    }

    inline float sine_out(float max_time, float current_time)
    {
        float x = current_time / max_time;
        return sinf((x * PI) / 2.f);
    }

    inline float sine_in_out(float max_time, float current_time)
    {
        float x = current_time / max_time;
        return -(cosf(PI * x) - 1.f) / 2.f;
    }

    using proc = float (*)(float, float);

    inline proc to_easing(std::string_view id)
    {
        static const std::unordered_map<std::string_view, proc> easing_map =
            {{"linear", linear},
             {"ease", cubic_in_out}, // CSS default "ease"
             {"ease-in", quad_in},
             {"ease-out", quad_out},
             {"ease-in-out", quad_in_out},

             // Cubic easing
             {"ease-in-cubic", cubic_in},
             {"ease-out-cubic", cubic_out},
             {"ease-in-out-cubic", cubic_in_out},

             // Quart easing
             {"ease-in-quart", quart_in},
             {"ease-out-quart", quart_out},
             {"ease-in-out-quart", quart_in_out},

             // Quint easing
             {"ease-in-quint", quint_in},
             {"ease-out-quint", quint_out},
             {"ease-in-out-quint", quint_in_out},

             // Sine easing
             {"ease-in-sine", sine_in},
             {"ease-out-sine", sine_out},
             {"ease-in-out-sine", sine_in_out},

             // Exponential easing
             {"ease-in-expo", expo_in},
             {"ease-out-expo", expo_out},
             {"ease-in-out-expo", expo_in_out},

             // Circular easing
             {"ease-in-circ", circ_in},
             {"ease-out-circ", circ_out},
             {"ease-in-out-circ", circ_in_out},

             // Back easing
             {"ease-in-back", back_in},
             {"ease-out-back", back_out},
             {"ease-in-out-back", back_in_out},

             // Elastic easing
             {"ease-in-elastic", elastic_in},
             {"ease-out-elastic", elastic_out},
             {"ease-in-out-elastic", elastic_in_out}};

        auto it = easing_map.find(id);
        return (it != easing_map.end()) ? it->second : linear;
    }

    /* True / False value with easing, 0.0 - 1.0 data representation */
    template <float MaxTime, proc cb = linear>
    struct value
    {
        void update(float dt)
        {
            if (animating)
            {
                data = cb(MaxTime, time);
                if (active)
                    time += dt;
                else
                    time -= dt;

                if (active && time > MaxTime)
                {
                    data         = 1.f;
                    time         = MaxTime;
                    animating    = false;
                }
                else if (!active && time < 0)
                {
                    data         = 0.f;
                    time         = 0;
                    animating    = false;
                }
            }
        }

        void reset()
        {
            time      = {};
            data      = {};
            animating = true;
            active    = true;
        }

        void animate(bool inc)
        {
            if (active == inc)
                return;
            animating = true;
            active    = inc;
        }

        void operator=(bool set)
        {
            animate(set);
        }

        operator bool() const
        {
            return active;
        }

        template <typename V>
        float operator()(const V& v) const
        {
            return data * v;
        }

        float data{};
        float time{};
        bool  animating{};
        bool  active{};
    };

} // namespace easing