#include "decompile.h"

void import_process(void) {
    int i, nimport = 0;
    u32 pos, rva, itable, import, symbol;
    char *dllname, *c;
    u16 hint;
    label_t *l;

    /* process each import segment (can there be more than one)? */
    for(i = 0; segment[i].name != NULL; i++) {
        if(segment[i].type != seg_IMPORT) continue;

        printf("import: processing segment '%s'\n", segment[i].name);

        pos = segment[i].coff;
        while(pos < segment[i].coff + segment[i].size) {
            rva = * (u32 *) (core + pos); pos += 12;
            dllname = (char *) core + * (u32 *) (core + pos); pos += 4;
            itable = * (u32 *) (core + pos); pos += 4;

            if(rva == 0)
                break;

            if(verbose)
                printf("  dll: %s\n", dllname);

            import = 0;
            while(* (u32 *) (core + rva + import) != 0x0) {
                hint = * (u16 *) (core + rva + import);
                symbol = * (u32 *) (core + rva + import) + 2;

                l = label_insert(itable + import, label_IMPORT, image_seg_find(itable + import));
                l->import.dllname = dllname;

                c = strchr(dllname, '.');
                if(c != NULL)
                    *c = '\0';

                if(image_seg_find(symbol) == NULL) {
                    if(verbose)
                        printf("    (hint %d)\n", hint);
                    l->import.hint = hint;
                    l->import.symbol = NULL;
                } else {
                    if(verbose)
                        printf("    %s\n", (char *) core + symbol);
                    l->import.hint = -1;
                    l->import.symbol = (char *) core + symbol;
                }

                import += 4;

                nimport++;
            }

            if(verbose)
                printf("\n");
        }
    }

    label_print_count("import");
}

void import_output(FILE *f) {
    int i;

    fprintf(f,
            "BITS 32\n"
            "\n"
            "GLOBAL ENTRY\n"
            "\n");

    for(i = 0; i < nlabel; i++) {
        if(label[i].type != label_IMPORT) continue;

        if(label[i].import.symbol != NULL)
            fprintf(f, "EXTERN %s\n", label[i].import.symbol);
        else
            fprintf(f, "EXTERN %s_%d\n", label[i].import.dllname, label[i].import.hint);
    }

    fprintf(f, "\nSECTION .data\n\n");

    for(i = 0; i < nlabel; i++) {
        if(label[i].type != label_IMPORT) continue;

        if(label[i].import.symbol != NULL)
            fprintf(f, "IMPORT_%s:\n    dd %s\n", label[i].import.symbol, label[i].import.symbol);
        else
            fprintf(f, "IMPORT_%s_%d:\n    dd %s_%d\n", label[i].import.dllname, label[i].import.hint, label[i].import.dllname, label[i].import.hint);
    }
}

void import_stub(FILE *f) {
    int i;

    printf("import: writing C stubs\n");

    fprintf(f,
            "#include <stdio.h>\n"
            "\n"
            "extern void ENTRY();\n"
            "\n"
            "int main(int argc, char **argv) {\n"
            "    ENTRY();\n"
            "    return 0;\n"
            "}\n");

    for(i = 0; i < nlabel; i++) {
        if(label[i].type != label_IMPORT) continue;

        if(label[i].import.symbol != NULL) {
            fprintf(f, "\n/* dll = %s, symbol = %s */\n", label[i].import.dllname, label[i].import.symbol);
            fprintf(f, "void %s(void) {\n", label[i].import.symbol);
            fprintf(f, "    printf(\"STUB: %s\\n\");\n}\n", label[i].import.symbol);
        }

        else {
            fprintf(f, "\n/* dll = %s, hint = %d */\n", label[i].import.dllname, label[i].import.hint);
            fprintf(f, "void %s_%d(void) {\n", label[i].import.dllname, label[i].import.hint);
            fprintf(f, "    printf(\"STUB: %s_%d\\n\");\n}\n", label[i].import.dllname, label[i].import.hint);
        }
    }
}