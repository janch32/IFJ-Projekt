#include "expressions.h"


eRelTerm exprConvTypeToTerm(tType tokenType){
	switch(tokenType){
		case T_MUL:
		case T_DIV:
			return E_MULDIV;
		case T_ADD:
		case T_SUB:
			return E_ADDSUB;
		case T_NOT:
			return E_NOT;
		case T_AND:
		case T_OR:
			return E_ANDOR;
		case T_LT:
		case T_GT:
		case T_LTE:
		case T_GTE:
			return E_LTGT;
		case T_EQL:
		case T_NEQ:
			return E_EQL;
		case T_LBRCKT:
			return E_LB;
		case T_RBRCKT:
			return E_RB;
		case T_INTEGER:
		case T_FLOAT:
		case T_STRING:
		case T_ID:
		case T_TRUE:
		case T_FALSE:
		case T_NIL:
			return E_VAL;
		default:
			return E_$;
	}
}


eRelation exprGetRelation(eRelTerm currTerm, eRelTerm newTerm){
	switch(currTerm){
		case E_MULDIV:
			if(newTerm == E_NOT)
				return E_EMPTY;
			else if(newTerm == E_LB || newTerm == E_VAL)
				return E_OPEN;
			return E_CLOSE;
		case E_ADDSUB:
			if(newTerm == E_NOT)
				return E_EMPTY;
			else if(newTerm == E_LB || newTerm == E_VAL || newTerm == E_MULDIV)
				return E_OPEN;
			return E_CLOSE;
		case E_NOT:
			if(newTerm == E_RB || newTerm == E_$ || newTerm == E_ANDOR)
				return E_CLOSE;
			return E_OPEN;
		case E_ANDOR:
			if(newTerm == E_RB || newTerm == E_$)
				return E_CLOSE;
			return E_OPEN;
		case E_LTGT:
			if(newTerm == E_EQL || newTerm == E_RB || newTerm == E_$ || newTerm == E_ANDOR) 
				return E_CLOSE;
			else if(newTerm == E_LTGT) return E_EMPTY;
			return E_OPEN;
		case E_EQL:
			if(newTerm == E_RB || newTerm == E_$ || newTerm == E_ANDOR)
				return E_CLOSE;
			else if(newTerm == E_EQL) return E_EMPTY;
			return E_OPEN;
		case E_LB:
			if(newTerm == E_RB) return E_EQUAL;
			else if(newTerm == E_$) return E_EMPTY;
			return E_OPEN;
		case E_VAL:
		case E_RB:
			if(newTerm == E_LB || newTerm == E_VAL || newTerm == E_NOT)
				return E_EMPTY;
			return E_CLOSE;
		case E_$:
			if(newTerm == E_RB || newTerm == E_$)
				return E_EMPTY;
			return E_OPEN;
	}

	return E_EMPTY;
}

int exprParse(pToken *token, psTree idTable){
	peStack stack;
	exprStackInit(&stack);

	int retCode = -1;

	while(retCode < 0){
		eRelTerm stackT, newT;

		int termPos = exprStackFindTerm(stack);
		if(termPos < 0) stackT = E_$;
		else stackT = exprConvTypeToTerm(stack->s[termPos]->val.term->type);
		newT = exprConvTypeToTerm((*token)->type);
		
		peItem item;
		switch(exprGetRelation(stackT, newT)){
			case E_OPEN:
				exprStackInsertOpen(stack, termPos + 1);
				item = malloc(sizeof(struct eItem));
				item->type = IT_TERM;
				item->val.term = *token;

				exprStackPush(stack, item);
				*token = (*token)->nextToken;
				break;
			case E_CLOSE:
				{
					int stackret = exprStackParse(stack, idTable);
					if(stackret > 0){
						fprintf(stderr, "Nesouhlasí gramatika expr (errid: %d)\n", stackret);
						retCode = stackret;
					}
				}
				break;
			case E_EQUAL:
				item = malloc(sizeof(struct eItem));
				item->type = IT_TERM;
				item->val.term = *token;

				exprStackPush(stack, item);
				*token = (*token)->nextToken;
				break;
			case E_EMPTY:
				if(stackT == newT && stackT == E_$){
					if(stack->top < 0){
						fprintf(stderr, "Expr nemuze byt prazdnej pyco\n");
						retCode = 2;
					}else {
						retCode = 0;
					}
					
				}else{
					fprintf(stderr, "[SYNTAX] Error on line %d:%d - ", (*token)->linePos, (*token)->colPos);
					
					if(termPos < 0)
						fprintf(stderr, "Expression cannot start with %s\n", scannerTypeToString((*token)->type));
					else 
						fprintf(stderr, "%s in expression cannot be followed with %s\n",
							scannerTypeToString(stack->s[termPos]->val.term->type),
							scannerTypeToString((*token)->type)
						);
					retCode = 2;
				}
		}
	}

	exprStackDispose(&stack);
	return retCode;
}

void exprStackInit(peStack *stack){
	if(stack == NULL) return;
	*stack = malloc(sizeof(struct eStack));
	(*stack)->size = EXPR_STACK_CHUNK_SIZE;
	(*stack)->s = malloc(sizeof(peItem) * EXPR_STACK_CHUNK_SIZE);
	(*stack)->top = -1;
}

void exprStackDispose(peStack *stack){
	if(stack == NULL || *stack == NULL) return;

	for(int i = (*stack)->top; i >= 0; i--){
		free((*stack)->s[i]);
	}

	free((*stack)->s);
	free(*stack);
	*stack = NULL;
}

void exprStackPush(peStack stack, peItem item){
	stack->top++;
	
	if(stack->top >= stack->size){
		stack->size += EXPR_STACK_CHUNK_SIZE;
		stack->s = realloc(stack->s, sizeof(peItem) * stack->size);
	}

	stack->s[stack->top] = item;
}

peItem exprStackPop(peStack stack){
	if(stack->top < 0) return NULL;

	stack->top--;
	return stack->s[stack->top + 1];
}

int	exprStackFindTerm(peStack stack){
	int pos = stack->top;
	for(; pos >= 0; pos--)
		if(stack->s[pos]->type == IT_TERM) break;

	return pos;
}

void exprStackInsertOpen(peStack stack, int pos){
	exprStackPush(stack, NULL);
	for(int i = stack->top - 1; i >= pos; i--){
		stack->s[i + 1] = stack->s[i];
	}

	peItem item = malloc(sizeof(struct eItem));
	item->type = IT_OPEN;
	stack->s[pos] = item;
}

int exprStackParse(peStack stack, psTree idTable){
	peItem item = exprStackPop(stack);
	
	if(item->type == IT_TERM){
		if(item->val.term->type == T_RBRCKT){
			// Pravidlo <expr> => ( <expr> )
			free(item);
			item = exprStackPop(stack);
			free(exprStackPop(stack));
		}else{
			// Pravidlo <expr> => <val>
			eTermType ttype = E_UNKNOWN;
			char *out;
			switch(item->val.term->type){
				case T_INTEGER: 
					out = intToInterpret(item->val.term->data);
					ttype = E_INT;
					break;
				case T_FLOAT: 
					out = floatToInterpret(item->val.term->data);
					ttype = E_FLOAT; 
					break;
				case T_STRING: 
					out = stringToInterpret(item->val.term->data);
					ttype = E_STRING;
					break;
				case T_NIL: 
					out = nilToInterpret();
					ttype = E_NIL;
					break;
				case T_TRUE:
					out = trueToInterpret();
					ttype = E_BOOL; 
					break;
				case T_FALSE:
					out = falseToInterpret();
					ttype = E_BOOL; 
					break;
				case T_ID:
					if(symTabSearch(&idTable, item->val.term->data) == NULL){
						return 3; // Chyba
					}
					out = varToInterpret(item->val.term->data);
					break;
				default: break;
			}
			printf("PUSHS %s\n", out);
			free(out);
			item->type = IT_NONTERM;
			item->val.type = ttype;
		}

		free(exprStackPop(stack));
		exprStackPush(stack, item);
		return 0;
	}

	peItem rItem = item;
	item = exprStackPop(stack);
	peItem lItem = exprStackPop(stack);

	eTermType lType = lItem->val.type;
	eTermType rType = rItem->val.type;
	eTermType type = rType;
	bool isSingle = lItem->type == IT_OPEN;
	bool hasUnknown = rType == E_UNKNOWN;
	bool isSame = false;
	
	// Sémantikcá část
	if(!isSingle){
		hasUnknown = lType == E_UNKNOWN || rType == E_UNKNOWN;
		
		if(lType == rType){
			isSame = true;
		}else if(lType == E_INT && rType == E_FLOAT){
			printf("POPS GF@$tmp\n");
			printf("INT2FLOATS\n");
			printf("PUSHS GF@$tmp\n");
			isSame = true;
		}else if(lType == E_FLOAT && rType == E_INT){
			printf("INT2FLOATS\n");
			type = E_FLOAT;
			isSame = true;
		}else if(rType == E_UNKNOWN){
			type = lType;
			isSame = true;
		}else if(lType == E_UNKNOWN){
			isSame = true;
		}
	}

	switch(item->val.term->type){
		case T_ADD:
			if(hasUnknown) printf("CALL $checkIfAdd\n");
			if(!isSame || (type != E_INT && type != E_FLOAT && type != E_STRING && type != E_UNKNOWN)){
				return 4; // Error
			}
			break;
		case T_SUB:
		case T_MUL:
			if(hasUnknown) printf("CALL $checkIfNum\n");
			if(!isSame || (type != E_INT && type != E_FLOAT && type != E_UNKNOWN)){
				return 4; // Error
			}
			break;
		case T_LT:
		case T_LTE:
		case T_GT:
		case T_GTE:
			if(hasUnknown) printf("CALL $checkIfLtGt\n");
			if(!isSame || (type != E_FLOAT && type != E_INT && type != E_UNKNOWN && type != E_STRING)){
				return 4; // Error
			}
			break;
		case T_EQL:
		case T_NEQ:
			if(isSingle){
				return 4; // Error
			}else if(hasUnknown) printf("CALL $checkIfEql\n");
			else if(!isSame) printf("POPS GF@$tmp\nPOPS GF@$tmp\nPUSHS bool@false\n");
			break;
		case T_NOT:
			if(hasUnknown){
				printf("PUSHS bool@false\n");
				printf("CALL $checkIfBool\n");
				printf("POPS GF@$tmp\n");
			}
			if(!isSingle || (type != E_BOOL && type != E_UNKNOWN)){
				return 4; // Error
			}
			break;
		case T_AND:
		case T_OR:
			if(hasUnknown) printf("CALL $checkIfBool\n");
			if(!isSame || (type != E_BOOL && type != E_UNKNOWN)){
				return 4; // Error
			}
			break;
		default:
			return 99; // Return
			break;
	}

	// Převod na kód
	switch(item->val.term->type){
		case T_ADD:
			if(!hasUnknown){
				if(type == E_STRING)
					printf("POPS GF@$tmp2\nPOPS GF@$tmp\nCONCAT GF@$tmp GF@$tmp GF@$tmp2\nPUSHS GF@$tmp\n");
				else
					printf("ADDS\n");
			}
			break;
		case T_SUB:
			printf("SUBS\n");
			break;
		case T_MUL:
			printf("MULS\n");
			break;
		case T_DIV:
			if(hasUnknown && (type == E_FLOAT || type == E_INT || type == E_UNKNOWN)){
				printf("CALL $checkIfNum\n");
				printf("CALL $decideDivOp\n");
			}else if(isSame && type == E_FLOAT){
				printf("DIVS\n");
			}else if(isSame && type == E_INT){
				printf("IDIVS\n");
			}else{
				return 4; // Error
			}
			break;
		case T_GTE:
			printf("LTS\nNOTS\n");
			type = E_BOOL;
			break;
		case T_LT:
			printf("LTS\n");
			type = E_BOOL;
			break;
		case T_LTE:
			printf("GTS\nNOTS\n");
			type = E_BOOL;
			break;
		case T_GT:
			printf("GTS\n");
			type = E_BOOL;
			break;
		case T_EQL:
			if(isSame) printf("EQS\n");
			type = E_BOOL;
			break;
		case T_NEQ:
			if(isSame) printf("EQS\n");
			printf("NOTS\n");
			type = E_BOOL;
			break;
		case T_NOT:
			printf("NOTS\n");
			type = E_BOOL;
			break;
		case T_AND:
			printf("ANDS\n");
			type = E_BOOL;
			break;
		case T_OR:
			printf("ORS\n");
			type = E_BOOL;
			break;
		default:
			return 99; // Return
			break;
	}

	free(lItem);
	free(rItem);
	item->type = IT_NONTERM;
	item->val.type = type;
	if(!isSingle) free(exprStackPop(stack));
	exprStackPush(stack, item);

	return 0;
}
