#include "main.h"

int main(int argc, char const *argv[])
{

	if(argc > 1){
		if(strcmp(argv[1], "j32") == 0) return janchDebug();

		if(strcmp(argv[1], "yell") == 0) return yellDebug();
	}
	
	//return yellDebug();

	return 0;
}

int yellDebug(){
	FILE *source1 = fopen("tests/test-code.1", "r");
	FILE *source2 = fopen("tests/test-code.2", "r");
	FILE *source3 = fopen("tests/test-code.3", "r");
	
	pToken token1 = NULL;
	pToken token2 = NULL;
	pToken token3 = NULL;

	scannerGetTokenList(&token1, source1);
	parser(&token1);
	fclose(source1);
	scannerFreeTokenList(&token1);
	_scannerFSM(NULL, NULL);

	scannerGetTokenList(&token2, source2);
	parser(&token2);
	fclose(source2);
	scannerFreeTokenList(&token2);
	_scannerFSM(NULL, NULL);

	scannerGetTokenList(&token3, source3);
	parser(&token3);
	fclose(source3);
	scannerFreeTokenList(&token3);

	return 0;
}
 
int janchDebug(){
	FILE *source = fopen("tests/test-expr", "r");
	
	pToken token = NULL;
	scannerGetTokenList(&token, source);
	printf(".IFJcode18\nDEFVAR GF@$tmp\nCLEARS\n\n");
	exprParse(&token, NULL);
	printf("\nPOPS GF@$tmp\nWRITE GF@$tmp\n");
	
	fclose(source);
	scannerFreeTokenList(&token);
	return 0;
}