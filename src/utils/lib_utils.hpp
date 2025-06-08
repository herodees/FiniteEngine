#pragma once

#include <string_view>

namespace fin
{
    class DynamicLibrary
    {
    public:
        DynamicLibrary() = default;
        DynamicLibrary(DynamicLibrary&& ot);
        DynamicLibrary(const DynamicLibrary& ot) = delete;
        ~DynamicLibrary();
        DynamicLibrary& operator=(DynamicLibrary&& ot);
        DynamicLibrary& operator=(const DynamicLibrary& ot) = delete;
        explicit        operator bool() const;

        void  Clear();
        bool  Load(std::string_view dir, std::string_view libname);
        void* GetSymbol(std::string_view name) const;
        bool  HasSymbol(std::string_view name) const;
        template <typename T>
        T* GetFunction(std::string_view symbol_name) const;
        template <typename T>
        T* GetVariable(std::string_view symbol_name) const;

    private:
        void* handle = nullptr;
    };


    template <typename T>
    inline T* DynamicLibrary::GetFunction(std::string_view symbol_name) const
    {
#if (defined(__GNUC__) && __GNUC__ >= 8)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif
        return reinterpret_cast<T*>(GetSymbol(symbol_name));
#if (defined(__GNUC__) && __GNUC__ >= 8)
#pragma GCC diagnostic pop
#endif
    }

    template <typename T>
    inline T* DynamicLibrary::GetVariable(std::string_view symbol_name) const
    {
        return reinterpret_cast<T*>(GetSymbol(symbol_name));
    }

} // namespace fin
