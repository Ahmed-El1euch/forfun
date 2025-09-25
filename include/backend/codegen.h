#ifndef FUNGCC_BACKEND_CODEGEN_H
#define FUNGCC_BACKEND_CODEGEN_H

#include <stdio.h>

#include "frontend/ast.h"

#ifdef __cplusplus
extern "C" {
#endif

int codegen_emit_translation_unit(const AstNode *unit, FILE *out);

#ifdef __cplusplus
}
#endif

#endif /* FUNGCC_BACKEND_CODEGEN_H */
