function(set_project_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            /W4
            /permissive-
            /w14640  # thread-safe static initialization
            /w14242  # conversion, possible loss of data
            /w14254  # conversion, possible loss of data
            /w14263  # member function does not override base class virtual
            /w14265  # class has virtual functions but destructor is not virtual
            /w14287  # unsigned/negative constant mismatch
            /w14296  # expression is always true/false
            /w14311  # pointer truncation
            /w14545  # expression before comma has no effect
            /w14546  # function call before comma missing argument list
            /w14547  # operator before comma has no effect
            /w14549  # operator before comma has no effect
            /w14555  # expression has no effect
            /w14619  # pragma warning suppressed
            /w14826  # conversion is sign-extended
            /w14905  # wide string literal cast
            /w14906  # string literal cast
            /w14928  # illegal copy-initialization
        )
        
        if(VECTORVAULT_WERROR)
            target_compile_options(${target_name} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target_name} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wconversion
            -Wshadow
            -Wnon-virtual-dtor
            -Wold-style-cast
            -Wcast-align
            -Wunused
            -Woverloaded-virtual
            -Wsign-conversion
            -Wnull-dereference
            -Wdouble-promotion
            -Wformat=2
        )
        
        if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            target_compile_options(${target_name} PRIVATE
                -Wmisleading-indentation
                -Wduplicated-cond
                -Wduplicated-branches
                -Wlogical-op
                -Wuseless-cast
            )
        endif()
        
        if(VECTORVAULT_WERROR)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif()
    endif()
endfunction()
