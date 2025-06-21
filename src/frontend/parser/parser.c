#include "parser.h"
#include "../../vm.h"
#include "../../runtime/value/value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// All parser functions have been moved to modular components:
// - Core parser state and error handling: frontend/parser/core.c
// - Expression parsing and literals: frontend/parser/expressions.c  
// - Statement parsing: frontend/parser/statements.c
//
// This file is now minimal and will eventually be removed entirely
// as the parser becomes fully modular.