cmake_minimum_required(VERSION 4.0)
project(CourseDesignTasks C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ========== 公共核心库：AST + 序列化 + 编辑距离 ==========
add_library(core STATIC
        src/ast.c
        src/ast_parser.c
        src/ast_serial.c
        src/edit_distance.c
)

target_include_directories(core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# （可选）警告
if (MSVC)
    target_compile_options(core PRIVATE /W4)
else()
    target_compile_options(core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ========== Tokenizer 库（从上层目录引入） ==========
add_library(tokenizer STATIC
        ./src/tokenizer.c
        ./src/std_token.c
)

# 添加上层目录到 include 路径（让 tokenizer 能找到 std_token.h）
target_include_directories(tokenizer PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/./include
)

# ========== 1) AST 验证程序（入口 tests/test_ast.c） ==========
add_executable(ast_demo
        tests/test_ast.c
)
target_link_libraries(ast_demo PRIVATE core)

# ========== 2) 最终程序（入口 src/main.c） ==========
add_executable(final_app
        src/main.c
)

add_executable(generator
        codes/generator.c
)
# 链接所有需要的库
target_link_libraries(final_app PRIVATE
        core          # AST、序列化、编辑距离
        tokenizer     # 词法分析器
)

# 添加 include 路径，让 main.c 能找到所有头文件
target_include_directories(final_app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include   # ast.h, ast_parser.h 等
        ${CMAKE_CURRENT_SOURCE_DIR}/.        # tokenizer.h, std_token.h
)
