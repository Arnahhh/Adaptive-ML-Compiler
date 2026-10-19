#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
gcc -std=c11 -Wall -Wextra -pedantic -O2 ast.c cfg.c codegen.c common_ir.c common_ir_cfg_optimizer.c common_ir_codegen.c common_ir_lower.c common_ir_optimizer.c dataflow.c error.c frontend.c ir.c java_frontend.c language.c lexer.c main.c optimizer.c parser.c python_frontend.c repair.c semantic.c source.c symbol.c ml.c adaptive.c -o amlc -lm
echo "Build successful: ./amlc"
