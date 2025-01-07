/*
* Copyright (C) 2024 ShanGu - All Rights Reserved
* 191954202@qq.com
*/

#pragma once

#include <cassert>
#include <utility>
#include <type_traits>

namespace valley {
    namespace lang {

        template< class... >
        class Delegate;

        template< class R, class... Args >
        class Delegate<R(Args...)>
        {
        public:
            constexpr Delegate() noexcept = default;
            ~Delegate();

            template<typename Return, typename ... XArgs>
            Delegate(Return(*function)(XArgs ...));

            template<typename Functor>
            Delegate(Functor& functor);

            template<typename Functor>
            Delegate(const Functor& functor);

            template<typename T, typename Return, typename ... XArgs>
            Delegate(T* object, Return(T::* mem_fn)(XArgs ...));

            template<typename T, typename Return, typename ... XArgs>
            Delegate(const T* object, Return(T::* mem_fn)(XArgs ...) const);

            constexpr Delegate(const Delegate<R(Args...)>&rhs) noexcept = default;
            Delegate<R(Args...)>& operator=(const Delegate<R(Args...)>&rhs) noexcept = default;

            template<typename Return, typename ... XArgs>
            void bind(Return(*function)(XArgs ...));

            template<typename Functor>
            void bind(Functor& functor);

            template<typename Functor>
            void bind(const Functor& functor);

            template<typename T, typename Return, typename ... XArgs>
            void bind(T* object, Return(T::* mem_fn)(XArgs ...));

            template<typename T, typename Return, typename ... XArgs>
            void bind(const T* object, Return(T::* mem_fn)(XArgs ...) const);

            operator bool() const;
            R operator()(Args ... args) const;

            void clear();
            
        private:
            template <typename FR>
            static constexpr bool is_compatible()
            {
                return std::is_void<R>::value || std::is_convertible<FR, R>::value;
            }

        private:
            struct Storage;

            using Dummy_fn      = R(*)(Args ...);
            using Dummy_mem_fn  = R(Delegate::*)(Args ...);
            using Apply         = R(*)(const Storage&, Args...);

            struct Storage
            {
                const void* object{ nullptr };
                union
                {
                    Dummy_fn        dummy_fn{ nullptr };
                    Dummy_mem_fn    dummy_mem_fn;
                };
            };

        private:
            Storage  storage_;
            Apply    apply_{ nullptr };
        };

        // alias
        template <typename R, typename... Args>
        using Slot = Delegate<R, Args...>;

        ////////////////////////////////////////////////////////////////////////////////////////
        // inline

        template<typename R, typename ... Args>
        inline Delegate<R(Args...)>::~Delegate()
        {
            apply_ = nullptr;
        }

        template<typename R, typename ... Args>
        template<typename Return, typename ... XArgs>
        inline Delegate<R(Args...)>::Delegate(Return(*function)(XArgs ...))
        {
            static_assert(is_compatible<Return>(), "is not compatible");
            bind(function);
        }

        template<typename R, typename ... Args>
        template<typename Functor>
        inline Delegate<R(Args...)>::Delegate(Functor& functor)
        {
            static_assert(is_compatible<typename std::result_of<Functor(Args ...)>::type>(), "is not compatible");
            bind(functor);
        }

        template<typename R, typename ... Args>
        template<typename Functor>
        inline Delegate<R(Args...)>::Delegate(const Functor& functor)
        {
            static_assert(is_compatible<typename std::result_of<Functor(Args ...)>::type>(), "is not compatible");
            bind(functor);
        }

        template<typename R, typename ...Args>
        template<typename T, typename Return, typename ... XArgs>
        inline Delegate<R(Args...)>::Delegate(T* object, Return(T::* mem_fn)(XArgs ...))
        {
            static_assert(is_compatible<Return>(), "is not compatible");
            bind(object, mem_fn);
        }

        template<typename R, typename ...Args>
        template<typename T, typename Return, typename ... XArgs>
        inline Delegate<R(Args...)>::Delegate(const T* object, Return(T::* mem_fn)(XArgs ...) const)
        {
            static_assert(is_compatible<Return>(), "is not compatible");
            bind(object, mem_fn);
        }

        template<typename R, typename ...Args>
        template<typename Return, typename ... XArgs>
        inline void Delegate<R(Args...)>::bind(Return(*function)(XArgs ...))
        {
            static_assert(is_compatible<Return>(), "is not compatible");

            using Fn = Return(*)(XArgs ...);

            storage_.dummy_fn = reinterpret_cast<Dummy_fn>(function);

            apply_ = [](const Storage& storage, Args ... args)->R
            {
                auto function = *reinterpret_cast<Fn>(storage.dummy_fn);
                return static_cast<R>(function(std::forward<Args>(args) ...));
            };
        }

        template<typename R, typename ...Args>
        template<typename Functor>
        inline void Delegate<R(Args...)>::bind(Functor & functor)
        {
            static_assert(is_compatible<typename  std::result_of<Functor(Args ...)>::type>(), "is not compatible");

            storage_.object = reinterpret_cast<void*>(&functor);

            apply_ = [](const Storage& storage, Args ... args)->R
            {
                auto& functor = *reinterpret_cast<Functor*>(const_cast<void*>(storage.object));
                return static_cast<R>(functor(std::forward<Args>(args) ...));
            };
        }

        template<typename R, typename ...Args>
        template<typename Functor>
        inline void Delegate<R(Args...)>::bind(const Functor & functor)
        {
            static_assert(is_compatible<typename  std::result_of<Functor(Args ...)>::type>(), "is not compatible");

            storage_.object = reinterpret_cast<const void*>(&functor);

            apply_ = [](const Storage& storage, Args ... args)->R
            {
                auto& functor = *reinterpret_cast<const Functor*>(storage.object);
                return static_cast<R>(functor(std::forward<Args>(args) ...));
            };
        }

        template<typename R, typename ...Args>
        template<typename T, typename Return, typename ... XArgs>
        inline void Delegate<R(Args...)>::bind(T * object, Return(T:: * mem_fn)(XArgs ...))
        {
            static_assert(is_compatible<Return>(), "is not compatible");

            using Mem_fn = Return(T::*)(XArgs ...);

            storage_.object = reinterpret_cast<void*>(object);
            storage_.dummy_mem_fn = reinterpret_cast<Dummy_mem_fn>(mem_fn);

            apply_ = [](const Storage& storage, Args ... args)->R
            {
                auto* object = reinterpret_cast<T*>(const_cast<void*>(storage.object));
                auto mem_fn = reinterpret_cast<Mem_fn>(storage.dummy_mem_fn);

                return static_cast<R>((object->*mem_fn)(std::forward<Args>(args) ...));
            };
        }

        template<typename R, typename ...Args>
        template<typename T, typename Return, typename ... XArgs>
        inline void Delegate<R(Args...)>::bind(const T * object, Return(T:: * mem_fn)(XArgs ...) const)
        {
            static_assert(is_compatible<Return>(), "is not compatible");

            using Mem_fn = Return(T::*)(XArgs ...) const;
            storage_.object = reinterpret_cast<const void*>(object);
            storage_.dummy_mem_fn = reinterpret_cast<Dummy_mem_fn>(mem_fn);

            apply_ = [](const Storage& storage, Args ... args)->R
            {
                auto* object = reinterpret_cast<const T*>(storage.object);
                auto mem_fn = reinterpret_cast<Mem_fn>(storage.dummy_mem_fn);

                return static_cast<R>((object->*mem_fn)(std::forward<Args>(args) ...));
            };
        }

        template<typename R, typename ...Args>
        inline Delegate<R(Args...)>::operator bool() const
        {
            return apply_ != nullptr;
        }

        template<typename R, typename ...Args>
        inline R Delegate<R(Args...)>::operator()(Args ... args) const
        {
            assert(apply_ != nullptr);
            return (*apply_)(storage_, std::forward<Args>(args)...);
        }

        template<typename R, typename ...Args>
        inline void Delegate<R(Args...)>::clear()
        {
            apply_ = nullptr;
        }
    }
}
