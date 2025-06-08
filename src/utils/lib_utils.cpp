#include "lib_utils.hpp"
#include <dylib.hpp>

namespace fin
{
    inline dylib* lib(void* handle)
    {
        return reinterpret_cast<dylib*>(handle);
    }

    DynamicLibrary::DynamicLibrary(DynamicLibrary&& ot)
    {
        handle = ot.handle;
        ot.handle = nullptr; // Clear the handle of the moved-from object
    }

    DynamicLibrary::~DynamicLibrary()
    {
        Clear();
    }

    DynamicLibrary::operator bool() const
    {
        return handle != nullptr;
    }

    DynamicLibrary& DynamicLibrary::operator=(DynamicLibrary&& ot)
    {
        if (this != &ot) // Check for self-assignment
        {
            std::swap(handle, ot.handle); // Swap the handles
        }
        return *this;
    }

    void DynamicLibrary::Clear()
    {
        if (handle)
        {
            delete lib(handle);
            handle = nullptr;
        }
    }

    bool DynamicLibrary::Load(std::string_view dir, std::string_view libname)
    {
        Clear();
        try 
        {
            // Create a new dylib instance
            dylib* dlib = new dylib(std::string(dir), std::string(libname));
            handle      = dlib;
            return true;
        }
        catch (const std::exception& e)
        {
            // Handle the exception if the library could not be loaded
        }

        return false;
    }

    void* DynamicLibrary::GetSymbol(std::string_view name) const
    {
        if (!handle)
            return nullptr;

        try
        {
            return lib(handle)->get_symbol(std::string(name));
        }
        catch (const std::exception&)
        {
        }
        return nullptr;
    }

    bool DynamicLibrary::HasSymbol(std::string_view name) const
    {
        if (!handle)
            return false;
        return lib(handle)->has_symbol(std::string(name));
    }

} // namespace fin
