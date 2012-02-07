#include <python2.6/Python.h>
#include "vcutil.h"

//Static function python headers
static PyObject* getFile(PyObject *self, PyObject *args);
PyObject *Vcf_getCard( PyObject *self, PyObject *args );
PyObject *Vcf_freeFile( PyObject *self, PyObject *args );

/*** method list to export to python ***/
static PyMethodDef VcfMethods[] = {
	// {"Python_func_name", c function, Argument style}
	{"getFile", getFile, METH_VARARGS},
	//{"Vcf_getCard", Vcf_getCard, METH_VARARGS},
	{NULL, NULL}, //denotes end of list
};

/*** Initialize the module. Called when import statement in python executes***/
void initVcf( void ) { Py_InitModule("Vcf", VcfMethods); }

static PyObject* getFile(PyObject *self, PyObject *args){
  
  char propertyname [][10] = {"BEGIN","END", "VERSION", "N", "FN", "NICKNAME", "PHOTO", "BDAY", "ADR", "LABEL", "TEL", "EMAIL", "GEO", "TITLE", "ORG", "NOTE", "UID", "URL", "OTHER"};
  char * filename;
  FILE * vcf = NULL;
  PyObject * card;
  card = PyList_New(0);
  PyObject *temp;
  VcFile * filep = (VcFile *) malloc (sizeof(VcFile));;
  //filep->ncards = 0;
  VcStatus status;
  int i = 0, j = 0;
  //Add a new card to the property array when the called array is full
  if (PyArg_ParseTuple (args, "sO", &filename, &card)){  
    
    vcf = fopen (filename,"r");
    status = readVcFile(vcf, filep);
    fclose(vcf);     
    
    //Parse the file pointer and build the card
    for (i = 0; i < filep->ncards ; i++){
      temp = PyList_New(0);
      for (j = 0; j < filep->cardp[i]->nprops;j++){
	//fprintf(stderr, "Partype:%d", filep->cardp[i]->prop[j].partype);
	PyList_Append(temp, Py_BuildValue("s", propertyname[filep->cardp[i]->prop[j].name]));
	if (filep->cardp[i]->prop[j].partype == NULL){
	    filep->cardp[i]->prop[j].partype = "\0";
	}
	PyList_Append(temp, Py_BuildValue("s",filep->cardp[i]->prop[j].partype));
	if (filep->cardp[i]->prop[j].parval == NULL){
	    filep->cardp[i]->prop[j].parval = "\0";
	}
	PyList_Append(temp, Py_BuildValue("s",filep->cardp[i]->prop[j].parval));
	PyList_Append(temp, Py_BuildValue("s",filep->cardp[i]->prop[j].value));
      }
      PyList_Append(card, temp);
    }
   return Py_BuildValue("iO", status.code, card);
    
  }
  
  //get filep down to lowest values
  
  return NULL;
}

/*PyObject* Vcf_getCard(PyObject *self, PyObject *args){  
  
  static int cardnum = 0;
  int name;
  char * partype;
  char * parval;
  char * value; 
  PyObject *cards;
  //card = cardList;
  //Check if your input is equal to 0
  if (== cardnum){
    
}
  
  
  
  if (!PyArg_ParseTuple(args, "O", &cards))
  {
    //couldnt parse the list object out of args
    return NULL;
  }
  
  //temp = Py_BuildValue("s", Cpower(i,2));
  //PyList_Append(myList, temp);
    //cards = PyList_SetItem(cards, cardnum, );
    //return Py_BuildValue("s", "CARD");
}*/

PyObject *Vcf_freeFile( PyObject *self, PyObject *args ){
  
}