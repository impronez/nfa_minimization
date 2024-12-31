cmake_minimum_required(VERSION 3.29)
project(nfa_minimization)

set(CMAKE_CXX_STANDARD 20)
if(WIN32)
    set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

add_executable(nfa_minimization main.cpp
        MooreAutomata.h
        Transition.h
        AutomataReader.h
        Group.h
)
