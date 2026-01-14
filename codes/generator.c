#include <stdio.h>
#include <stdlib.h>

int main() {
#ifdef _WIN32
    system("chcp 65001 > nul");
#endif
    FILE *fp = fopen("../codes/huge_code.c", "w");
    if (!fp) {
        printf("无法创建文件\n");
        return 1;
    }

    printf("正在生成 huge_code.c ...\n");

    // 写入头文件
    fprintf(fp, "/**\n * 自动生成的压力测试代码\n * 用于测试UI动画效果\n */\n");
    fprintf(fp, "#include <stdio.h>\n");
    fprintf(fp, "#include <stdlib.h>\n\n");

    // 循环生成 5000 个函数
    // 这会产生大量的 Token，强制让词法分析器和语法分析器工作一段时间
    for (int i = 0; i < 50000; i++) {
        fprintf(fp, "// 函数块 %d\n", i);
        fprintf(fp, "int logic_function_%d(int input) {\n", i);
        fprintf(fp, "    int x = input * %d;\n", i);
        fprintf(fp, "    int y = x + 32;\n");

        // 生成一些嵌套结构来增加语法树(AST)的构建难度
        fprintf(fp, "    if (x > 1000) {\n");
        fprintf(fp, "        return x * x;\n");
        fprintf(fp, "    } else {\n");
        fprintf(fp, "        while (y > 0) {\n");
        fprintf(fp, "            y--;\n");
        fprintf(fp, "            x += (y %% 2);\n");
        fprintf(fp, "        }\n");
        fprintf(fp, "    }\n");
        fprintf(fp, "    return x + y;\n");
        fprintf(fp, "}\n\n");
    }

    // 写入主函数
    fprintf(fp, "int main() {\n");
    fprintf(fp, "    int total = 0;\n");
    fprintf(fp, "    printf(\"Start Processing...\\n\");\n");
    // 调用一部分函数
    for (int i = 0; i < 100; i++) {
        fprintf(fp, "    total += logic_function_%d(i);\n", i);
    }
    fprintf(fp, "    return 0;\n");
    fprintf(fp, "}\n");

    fclose(fp);
    printf("生成完成！请使用 huge_code.c 进行测试。\n");
    return 0;
}
