#pragma once
#include <FastSIMD/ToolSet.h>
#include <FastSIMD/DispatchClass.h>

#include <new>

namespace FastSIMD
{    
    template<typename T, FastSIMD::FeatureSet SIMD>
    class DispatchClass;

    template<FastSIMD::FeatureSet SIMD>
    struct DispatchClassFactory 
    {
        template<typename T>
        FS_NEVERINLINE static T* New( FastSIMD::MemoryAllocator allocator );
    };

    // Make sure we only instantiate DispatchClass<T, SIMD> for the current feature set
    template<>
    template<typename T>
    FS_NEVERINLINE T* DispatchClassFactory<FeatureSetDefault()>::New( FastSIMD::MemoryAllocator allocator )
    {
        constexpr auto SIMD = FeatureSetDefault();

        if( allocator )
        {
            void* alloc = allocator( sizeof( DispatchClass<T, SIMD> ), alignof( DispatchClass<T, SIMD> ) );

            return new( alloc ) DispatchClass<T, SIMD>;
        }

        return new DispatchClass<T, SIMD>;
    }


    template<typename T, FastSIMD::FeatureSet SIMD = FeatureSetDefault()>
    class RegisterDispatchClass
    {
        static_assert( SIMD == FeatureSetDefault() );

        // Never called, used to instantiate DispatchClassFactory<SIMD>::New<T>()
        static auto Instantiate()
        {
            return &FastSIMD::DispatchClassFactory<SIMD>::template New<T>;
        }
    };

    // Compile FastSIMD::NewDispatchClass<T> in minimum feature set compilation unit to avoid illegal instructions
    template<typename T>
    class RegisterDispatchClass<T, FastSIMD::FASTSIMD_LIBRARY_NAME::CompiledFeatureSets::Minimum>
    {
        // Never called, used to instantiate NewDispatchClass<T>()
        static auto Instantiate()
        {
            return &FastSIMD::NewDispatchClass<T>;
        }
    };


    template<typename T, FastSIMD::FeatureSet SIMD>
    FS_FORCEINLINE static T* DispatchClassFactoryIterator( FastSIMD::FeatureSet maxFeatureSet, MemoryAllocator allocator )
    {
        if( maxFeatureSet < SIMD )
        {
            return nullptr;
        }

        constexpr auto NextCompiled = FastSIMD::FASTSIMD_LIBRARY_NAME::CompiledFeatureSets::NextAfter<SIMD>;

        if constexpr( NextCompiled != FastSIMD::FeatureSet::Max )
        {
            if( maxFeatureSet >= NextCompiled )
            {
                return DispatchClassFactoryIterator<T, NextCompiled>( maxFeatureSet, allocator );
            }
        }
        
        return DispatchClassFactory<SIMD>::template New<T>( allocator );
    }

    template<typename T>
    FASTSIMD_API T* NewDispatchClass( FastSIMD::FeatureSet maxFeatureSet, MemoryAllocator allocator )
    {
        if( maxFeatureSet == FeatureSet::Max )
        {
            maxFeatureSet = DetectCpuMaxFeatureSet();
        }

        return DispatchClassFactoryIterator<T, FastSIMD::FASTSIMD_LIBRARY_NAME::CompiledFeatureSets::Minimum>( maxFeatureSet, allocator );
    }


} // namespace FastSIMD
