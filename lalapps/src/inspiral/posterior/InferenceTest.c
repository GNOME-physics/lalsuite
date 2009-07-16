#include <stdio.h>
#include "LALInference.h"


LALVariables variables;

LALVariables param;

REAL4 number,five;
ProcessParamsTable *ppt, *ptr;
int i;


int main(int argc, char *argv[]){
  /* test "LALVariables" stuff: */
  LALIFOData *IFOdata=NULL;
  number = 10.0;
  five=5.0;
  variables.head=NULL;
  variables.dimension=0;
  addVariable(&variables,"number",&number,REAL4_t);
  number=*(REAL4 *)getVariable(&variables,"number");
  fprintf(stdout,"Got %lf\n",number);
  setVariable(&variables,"number",&five);
  number=*(REAL4 *)getVariable(&variables,"number");
  fprintf(stdout,"Got %lf\n",number);
  fprintf(stdout,"Checkvariable?: %i\n",checkVariable(&variables,"number"));	
  removeVariable(&variables,"number");
  fprintf(stdout,"Removed, Checkvariable?: %i\n",checkVariable(&variables,"number"));
  destroyVariables(&variables);
  
  /* test "parseCommandLine()" function: */
  ppt = (ProcessParamsTable*) parseCommandLine(argc,argv);
  printf("parsed command line arguments:\n");
  ptr = ppt;
  i=1;
  while (ptr != NULL){
    printf(" (%d)  %s  %s  %s  \"%s\"\n", i, ptr->program, ptr->param, ptr->type, ptr->value);
    ptr = ptr->next;
    ++i;
  }
	
  /* Test the data setup */
//  IFOdata=ReadData(ppt);
//  if(IFOdata) fprintf(stdout,"Successfully read in the data!\n");
	
 // IFOdata->modelParams=calloc(1, sizeof(LALVariables));
	
 // IFOdata->modelParams->head=NULL;
 // IFOdata->modelParams->dimension=0;
  REAL4 m1 = 1.4;
 // addVariable(IFOdata->modelParams,"m1",&m1,REAL4_t);
  REAL4 m2 = 1.4;
 // addVariable(IFOdata->modelParams,"m2",&m2,REAL4_t);
  REAL4 inc = 0.0;
 // addVariable(IFOdata->modelParams,"inc",&inc,REAL4_t);
  REAL4 phii = 0.0;
 // addVariable(IFOdata->modelParams,"phii",&phii,REAL4_t);

	
  //REAL8TimeSeries timeModelhPlus;
  //REAL8TimeSeries timeModelhCross;
  
	  LALIFOData *ifo=NULL;
	
  LALTemplateWrapper(ifo);
  return 0;
}
