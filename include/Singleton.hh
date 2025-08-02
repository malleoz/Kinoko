#pragma once

#include "Logger.hh"

#include <algorithm>
#include <list>
#include <type_traits>


/// @brief A wrapper class used to preserve all singleton instances when changing heap contexts.
/// @details This base class exists because we want to maintain a static vector of all existing
/// singletons. Since the template type will vary, template specializations will each have their own
/// static member. Storing these pointers in the base class means that we can collect all types.
class SingletonBase {
protected:
    SingletonBase() = default;
    virtual ~SingletonBase() = 0;

    static std::list<void *> m_singletons;
};

template <typename T>
    requires(!std::is_pointer_v<T> && !std::is_reference_v<T>)
class Singleton : public SingletonBase {
public:
    Singleton() = default;
    ~Singleton() override = default;

    explicit operator bool() const {
        return s_instance;
    }

    static T *CreateInstance() {
        ASSERT(!s_instance);

        s_instance = T::CreateInstance();
        m_singletons.push_back(s_instance);

        return s_instance;
    }

    static void DestroyInstance() {
        ASSERT(s_instance);

        m_singletons.erase(std::remove(m_singletons.begin(), m_singletons.end(), s_instance),
                m_singletons.end());

        s_instance->DestroyInstance();
        s_instance = nullptr;
    }

    [[nodiscard]] static T *Instance() {
        ASSERT(s_instance);
        return s_instance;
    }

private:
    static T *s_instance;
};

template <typename T>
    requires(!std::is_pointer_v<T> && !std::is_reference_v<T>)
T *Singleton<T>::s_instance = nullptr;
