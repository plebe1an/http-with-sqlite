
cmake_minimum_required (VERSION 3.8)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("Net")

add_executable (Net "src/Service.cpp" "main.cpp")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Net PROPERTY CXX_STANDARD 17)
endif()

set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_DEBUG_LIBS ON)

set(BOOST_ROOT "D:/boost_1_86_0")
set(BOOST_INCLUDEDIR "${BOOST_ROOT}/include")
set(BOOST_LIBRARYDIR "${BOOST_ROOT}/stage/lib")

include(D:/vcpkg/scripts/buildsystems/vcpkg.cmake)

find_package(Boost 1.86.0 REQUIRED COMPONENTS system filesystem)
find_package(SQLiteCpp REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
target_link_directories(Net PRIVATE ${BOOST_LIBRARYDIR})
target_link_libraries(Net PRIVATE Boost::system Boost::filesystem)
target_link_libraries(Net PRIVATE SQLiteCpp)

add_custom_command(
    TARGET Net POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "D:/vcpkg/installed/x64-windows/bin/SQLiteCpp.dll" $<TARGET_FILE_DIR:Net>
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "D:/vcpkg/installed/x64-windows/bin/sqlite3.dll" $<TARGET_FILE_DIR:Net>
)

set(DB_FILE_PATH "${CMAKE_SOURCE_DIR}/database.db")
add_custom_command(
    TARGET Net POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${DB_FILE_PATH} $<TARGET_FILE_DIR:Net>
    COMMENT "Copying database.db to output directory"
)