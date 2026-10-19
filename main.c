#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "language.h"
#include "source.h"
#include "frontend.h"
#include "semantic.h"
#include "common_ir_lower.h"
#include "common_ir_optimizer.h"
#include "common_ir_codegen.h"
#include "cfg.h"
#include "common_ir_cfg_optimizer.h"
#include "dataflow.h"
#include "ast.h"
#include "error.h"
#include "repair.h"
#include "ml.h"
#include "adaptive.h"

static void printUsage(const char* programName) {
    printf("Usage: %s <source-file>\n", programName);
    printf("Supported file types: .c, .cpp, .cc, .cxx, .py, .java\n");
}

static void printAllSyntaxErrors(void) {
    int count = getErrorCount();

    for (int i = 0; i < count; ++i) {
        const ErrorDiagnostic* d = getErrorDiagnostic(i);
        if (!d) continue;

        printf("\nSyntax Error %d:\n", i + 1);
        printf("  Error Type : %s\n", errorTypeToString(d->type));
        printf("  Line       : %d\n", d->line);
        printf("  Message    : %s\n", d->message);
        printf("  Found      : %s\n", d->found);
        printf("  Expected   : %s\n", d->expected);
    }
}

static void printRepairSuggestion(const SourceFile* input) {
    int count = getErrorCount();
    if (count <= 0) return;

    const ErrorDiagnostic* d = getErrorDiagnostic(0);
    if (!d) return;

    char* repaired = repairProgram(input->code, d->type, d->line);
    if (!repaired) return;

    printf("\n========== PROGRAM REPAIR ==========\n");
    printf("Detected : %s\n", errorTypeToString(d->type));
    printf("Line     : %d\n", d->line);

    if (d->type == ERROR_MISSING_SEMICOLON) {
        printf("Suggestion: Add the missing ';' at the reported line.\n");
        printf("\nRepaired Source:\n");
        printf("------------------------------------\n");
        printf("%s", repaired);
        if (repaired[0] != '\0' && repaired[strlen(repaired) - 1] != '\n') printf("\n");
        printf("------------------------------------\n");
    } else {
        printf("Repair suggestion available for this diagnostic type.\n");
    }

    free(repaired);
}

static void printMLPrediction(const MLFeatures* features, const MLPrediction* prediction) {
    printf("\n========== ML PHASE PREDICTION ==========\n");
    mlPrintFeatures(features);
    printf("  Predicted Phase    : %s\n", prediction->label);
    printf("  Confidence         : %.1f%%\n", prediction->confidence);
    printf("  KNN Neighbors      : %d\n", prediction->neighbors);
    printf("  ML Role            : Recommends the optimization focus for this program.\n");
}

int main(int argc, char* argv[]) {
    SourceFile input;
    ASTNode* root = NULL;

    if (argc != 2) {
        printUsage(argv[0]);
        return 1;
    }

    if (!loadSourceFile(argv[1], &input)) {
        fprintf(stderr, "Error: Could not read source file '%s'.\n", argv[1]);
        return 1;
    }

    printf("\n========================================\n");
    printf("       ADAPTIVE ML COMPILER\n");
    printf("========================================\n");
    printf("File     : %s\n", input.filename);
    printf("Language : %s\n", languageToString(input.language));
    printf("Size     : %zu bytes\n", input.length);

    if (input.language == LANGUAGE_UNKNOWN) {
        printf("\nUnsupported or unknown file extension.\n");
        freeSourceFile(&input);
        return 1;
    }

    clearError();

    {
        int position = 0;
        FrontendStatus status = frontendParse(
            input.language,
            input.code,
            &position,
            &root
        );

        printf("\n========== LEXICAL / SYNTAX ANALYSIS ==========\n");

        if (status == FRONTEND_UNSUPPORTED) {
            printf("Frontend not implemented for %s.\n", languageToString(input.language));
            freeAST(root);
            freeSourceFile(&input);
            return 1;
        }

        int syntaxErrors = getErrorCount();

        if (syntaxErrors > 0) {
            printf("Status : FAILED (%d error(s))\n", syntaxErrors);
            printAllSyntaxErrors();
            printRepairSuggestion(&input);
        } else if (status != FRONTEND_SUCCESS) {
            printf("Status : FAILED\n");
            if (getLastError() != ERROR_NONE) printAllSyntaxErrors();
        } else {
            printf("Status : PASS\n");
        }

        if (root == NULL || status == FRONTEND_UNSUPPORTED) {
            freeAST(root);
            freeSourceFile(&input);
            return 1;
        }
    }

    {
        SemanticResult semanticResult;
        semanticResultInit(&semanticResult);

        int semanticErrors = semanticAnalyze(root, &semanticResult);

        printf("\n========== SEMANTIC ANALYSIS ==========");
        if (semanticErrors > 0) {
            printf("\nStatus : FAILED (%d error(s))\n", semanticErrors);
            semanticResultFree(&semanticResult);
            freeAST(root);
            freeSourceFile(&input);
            return 1;
        }

        printf("\nStatus : PASS (0 error(s))\n");
        semanticResultFree(&semanticResult);
    }

    if (getErrorCount() == 0) {
        CommonIR commonIR;

        if (lowerASTToCommonIR(root, &commonIR)) {
            MLFeatures features;
            MLPrediction prediction;
            memset(&prediction, 0, sizeof(prediction));

            mlExtractFeatures(input.code, input.length, 1, &features);
            if (mlPredict(&features, "ml/model.csv", 5, &prediction)) {
                printMLPrediction(&features, &prediction);
            } else {
                printf("\n========== ML PHASE PREDICTION ==========\n");
                printf("ML model could not be loaded. Check ml/model.csv.\n");
            }

            printf("\n========== ORIGINAL INTERMEDIATE CODE ==========\n");
            commonIRPrint(&commonIR);

            if (prediction.label[0] != '\0') {
                adaptivePrintStrategy(&prediction);
                AdaptiveResult adaptiveResult;
                adaptiveOptimize(&commonIR, &prediction, &adaptiveResult);
                printf("\n========== REDEFINED / OPTIMIZED IR ==========\n");
                printf("  Strategy applied     : %s\n", adaptiveResult.strategy);
                printf("  Transformations      : %d\n", adaptiveResult.transformations);
                commonIRPrint(&commonIR);
            } else {
                printf("\n========== OPTIMIZATION ==========\n");
                int localChanges = commonIROptimize(&commonIR);
                printf("Local optimization transformations: %d\n", localChanges);
            }

            printf("\n========== CFG OPTIMIZATION ==========\n");
            int cfgChanges = commonIRCFGOptimize(&commonIR);
            printf("CFG optimization transformations   : %d\n", cfgChanges);

            ControlFlowGraph cfg;

            if (cfgBuild(&commonIR, &cfg)) {
                printf("\n========== CONTROL FLOW GRAPH ==========\n");
                printf("Basic blocks : %zu\n", cfg.blockCount);
                printf("CFG edges    : %zu\n", cfg.edgeCount);

                mlExtractFeatures(input.code, input.length, cfg.blockCount, &features);
                if (mlPredict(&features, "ml/model.csv", 5, &prediction)) {
                    printf("Refined ML prediction using CFG features: %s (%.1f%%)\n",
                           prediction.label, prediction.confidence);
                }

                DataFlowAnalysis dataFlow;
                dataFlowInit(&dataFlow);
                if (dataFlowAnalyze(&commonIR, &cfg, &dataFlow)) {
                    printf("\n========== DATA-FLOW ANALYSIS ==========\n");
                    printf("Reaching Definitions iterations : %zu\n", dataFlow.reachingIterations);
                    printf("Liveness iterations              : %zu\n", dataFlow.livenessIterations);
                    printf("Tracked variables                : %zu\n", dataFlow.variableCount);
                    printf("Tracked definitions              : %zu\n", dataFlow.definitionCount);
                }
                dataFlowFree(&dataFlow);
                cfgFree(&cfg);
            }

            printf("\n========== CODE GENERATION ==========\n");
            if (generateCommonIRCode(&commonIR, "output.s")) {
                printf("Target code generated: output.s\n");
            } else {
                printf("Target code generation failed.\n");
            }

            commonIRFree(&commonIR);
        } else {
            printf("\nIR generation failed.\n");
        }
    }

    printf("\n========================================\n");
    printf("        COMPILATION COMPLETED\n");
    printf("========================================\n");

    freeAST(root);
    freeSourceFile(&input);
    return 0;
}
