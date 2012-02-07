/********
Utililty functions which will read in a vcf input file into appropriate data structures and process parts of interest from a vCard.
Last updated:  February 14, 2011 10:35 PM 

Marc Bodmer
0657005
mbodmer@uoguelph.ca
********/
#include <python2.6/Python.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include "vcutil.h"
#define DEFAULT_STR_SIZE 200

int folded = 0;

/********
Precondition: vcf has been opened for reading, and is assumed to be positioned at beginning-of-file. filep points to the VcFile structure to be filled in

Postcondition: If status=OK, the file was read to normal EOF and is available in memory for access via the filep pointer. The caller is responsible for eventually freeing the contents of filep with the help of freeVcFile. 
If status is not OK, VcStatus contains both the error code and the line numbers in the file where the error was detected, and the VcFile contents has already been freed.

Fill in a VcFile structure with appropraite values and read each card by calling readVcard. Each time another card is successfully returned, increment ncards and extend the cardp array of card pointers.
Return to caller with OK status when readVcard returns OK and NULL, signifying EOF.
*********/
VcStatus readVcFile( FILE *const vcf, VcFile *const filep){
  
  filep->ncards = 0;
  filep->cardp = NULL;
  VcStatus status = {};
  status.code = 0;
  Vcard *newcard;
  
  if (vcf == NULL){
    return(status);
  }
  
  while (status.code == 0){	
    //newcard = malloc(sizeof(Vcard));
    //newcard->nprops = 0;	
    status = readVcard(vcf, &newcard);
    /*If status.code OK and cardp!=NULL, structure has been filled in*/
    if (status.code == 0 && newcard->nprops > 0){
      filep->ncards++;
      //printf("\nNCARDS:%d\n", filep->ncards);	
      //printf("\nNCARDS:%d\n", newcard->nprops);	
      filep->cardp = realloc (filep->cardp, sizeof(Vcard *) * filep->ncards);
      filep->cardp[filep->ncards - 1] = newcard;
      newcard = NULL;
    }
    else if (status.code == 0 && newcard->nprops == 0){
      //printf("PARTYPE:%s\n", filep->cardp[0]->prop[8].parval);
      //printf("Code-> %d", status.code);
      //printf("\nNCARDS:%d\n", filep->ncards);	
      free(newcard);
      return (status);
    }
  }
  return (status);
}

/********
Precondition: vcf is open for reading. cardp points to a variable where the address of a newly allocated Vcard will be returned.

Postcondition: If status=OK, and * cardp is non-NULL, the Vcard structure has been filled in. OK status and NULL * cardp means normal EOF. 
If error status, * cardp contents are undefined (but no additional storage remains allocated).

This function repeatedly calls getUnfolded to read the next content line, or set of lines from the file, and unfold them into a single buffer.
*********/
VcStatus readVcard( FILE *const vcf, Vcard **const cardp ){
  
  *cardp = malloc(sizeof(Vcard));
  assert((*cardp)!=NULL);
  (*cardp)->nprops = 0;
  /*Variable declarations*/	
  VcStatus status;
  status.code = 0;
  char *buff = malloc(500 * sizeof(char));
  assert(buff!=NULL);
  VcProp propp;
  int versionflag = 0, nfncheck = 0, versionfound = 0;
  static int beginendcheck = 0;
  
  while (status.code == 0){
    status = getUnfolded(vcf, &buff);					
    /*Check for end of file*/
    if (feof(vcf)){
      return(status);
    }
    if (strcmp(buff, "BEGIN:VCARD") == 0){
      beginendcheck++;
      //nfncheck--;
      status = getUnfolded(vcf, &buff);
    }
    /*Run version checks*/
    if (strcmp(buff, "VERSION:2.1") == 0){
      status.code = BADVER;
      return(status);
    }
    else if (strcmp(buff, "VERSION:3.0") == 0){
      versionflag = 1, versionfound = 1;
      nfncheck--;
      continue;
    }
    else if (strcasestr(buff, "VERSION:") && versionflag == 0){
      versionfound = 1;
      nfncheck--;
      continue;
    }
    /*Check if there is at least 2 FN or N*/
    if(buff[1] == 'N' || buff[0] == 'N'){
      nfncheck++;
    }
    /*Once you reach END:VCARD check if you have gotten any errors*/		
    if (strcmp(buff, "END:VCARD") == 0){
      beginendcheck++;
      /*if (beginendcheck <= 2){		
      printf("TEST11");		
      status.code = BEGEND;
      return(status);	
      }*/
      if (nfncheck < 2){		
	//printf("TEST2");				
	status.code = NOPNFN;
	return(status);
      }
      /*if(versionfound != 1){
      printf("TEST3");			
      status.code = NOPVER;
      return(status);
      }
      if (versionflag == 0){
	printf("TEST4");					
	status.code = BADVER;
	return(status);
      }*/
      nfncheck = 0;
      status = getUnfolded(vcf, &buff);		
      if (strcmp(buff, "BEGIN:VCARD") == 0){
	//beginendcheck++;
	status = getUnfolded(vcf, &buff);
	//nfncheck--;
      }
      beginendcheck = 0;
      status.code = OK;
      return(status);
    }
    
    /*If you are not on a folded line, send buff to parseVcProp*/
    if (folded == 0){
      status.code = parseVcProp(buff, &propp);
      //printf("PROPNAME: %d\n", propp.name);	
      /*if (strcmp(propp.value, "") == 0){
      status.code = SYNTAX;
      return(status);
      }*/
      //printf("-> %s\n", buff);			
      //printf("P: %s\n", propp.value);
      //printf("PTYPE: %s\n", propp.partype);
      /*Set the fields in Vcard*/
      if (status.code == 0){
	(*cardp)->nprops++;
	//printf("HERE:%d\n", (*cardp)->nprops);
	(*cardp) = realloc (*cardp, (sizeof(Vcard)) + (sizeof(VcProp) * (*cardp)->nprops));
	assert ((*cardp)!=NULL);
	//(*cardp)->prop->value = propp->value;
	//(*cardp)->prop[(*cardp)->nprops - 1] = *propp;
	//free(propp.value);
	(*cardp)->prop[(*cardp)->nprops-1] = propp;
	/*printf("VAL0: %s\n", (*cardp)->prop[0].value);
	printf("VAL4: %s\n", (*cardp)->prop[4].value);
	printf("VAL5: %s\n", (*cardp)->prop[5].value);*/		
      }
      else{
	return(status);
      }
    }
  }	
  status.code = OK;
  return (status); 
}

/********
Precondition: vcf is open for reading. buff points to a variable where the address of a newly allocated buffer will be returned.

Postcondition: If status=OK, * buff contains the file's next unfolded content line with trailing EOL characters removed, and the caller is responsible to eventually free * buff . 
* buff contains NULL in case of EOF or if status is not OK. VcStatus indicates the line (or lines) that were included in the the unfolded buffer (undefined for error status).

This function has to read lines ahead to check for folding. It can assume that only one file at a time is being read. Multiple folded lines are reassembled into an allocated buffer.
*********/
VcStatus getUnfolded( FILE *const vcf, char **const buff ){
  static int fileline = 0;
  static int folding = 0;
  fileline++;
  VcStatus status = {};
  *buff = malloc(300 * sizeof(char));
  assert(buff!=NULL);
  
  if (vcf == NULL){
    folded = 0;
    fileline = 0;
    folding = 0;
    status.linefrom = 0;
    status.lineto = 0;
    status.code = OK;
    return(status);
  }
  
  if (feof(vcf)){
    return(status);
  }
  status.linefrom = fileline;
  int c = 0;
  char checkspace;
  if(!feof(vcf)){
    status.lineto = (fileline) + 1;
    char delims[] = "\r\n";
    char *temp = malloc(200 * sizeof(char));
    assert (temp!=NULL);
    char str[DEFAULT_STR_SIZE];
    fgets(str,200, vcf);
    if (str == NULL){
      if (ferror(vcf)){
	status.code = IOERR;
	return(status);
      }
    }
    temp = strtok(str, delims);
    if (temp != NULL){
      strcpy(*buff, temp);
    }
  }
  
  while(1){
    if (!feof(vcf)){
      c = fgetc(vcf);
      checkspace = (char)c;
      if (isspace(checkspace) != 0){
	char str[DEFAULT_STR_SIZE];
	fgets(str,200, vcf);
	status.lineto = (fileline + 1);
	char delims[] = "\r\n";
	char *result = malloc(200 * sizeof(char));
	assert (result!=NULL);
	result = strtok(str, delims);
	/*if (strstr(result,":") != NULL){
	return(status);
      }*/
	if (result != NULL){
	  strcat(*buff, result);	
	}
      }
      else{
	ungetc(c, vcf);
	break;
      }
    }
    else{
      break;
    }
  }
    
  status.code = 0;
  return (status);	
}


/********
Precondition: buff contains the unfolded line to be parsed and propp points to a VcProp structure to be filled in.

Postcondition:  If status=OK, *propp structure has been filled in, otherwise its contents are undefined (but no storage remains allocated).

Initialize all the pointer fields of *propp to NULL. Parse the buffer as a vCard property, dividing the content line according to RFC 2426.
*********/
VcError parseVcProp( const char *buff, VcProp *const propp ){
  
  propp->partype = NULL, propp->parval = NULL, propp->value = NULL, propp->hook = NULL;
  int syntaxerr = 1, parovercheck = 0;
  propp->partype = calloc(30, sizeof(char));
  assert(propp->partype != NULL);
  propp->parval = calloc(30, sizeof(char));
  assert(propp->parval != NULL);
  propp->value = calloc(1000, sizeof(char));
  assert(propp->value!=NULL);
  
  //printf("->%s\n", buff);
  if(strcasestr(buff, "group1.") || strcasestr(buff, "group2.") || strcasestr(buff, "group3.")){
    propp->partype = NULL, propp->parval = NULL;
    char * periodsearch = strcasestr(buff, ".");
    char properbuff[DEFAULT_STR_SIZE];
    strcpy(properbuff,periodsearch + 1);
    buff = properbuff;
  }
  
  if(strstr(buff, "MEDIUM=") || strstr(buff, "SUBJUGATION=")){
    return (PAROVER);
  }
  /*Case checks for Vcard properties*/
  char * startptr = strcasestr(buff, "FN:");
  char * startptrcol = strcasestr(buff, "FN;");
  char * startptr2 = strcasestr(buff, "N:");
  char * startptr2col = strcasestr(buff, "N;");
  char * startptr3 = strcasestr(buff, "NICKNAME:");
  char * startptr3col = strcasestr(buff, "NICKNAME;");
  char * startptr4 = strcasestr(buff, "PHOTO:");
  char * startptr4col = strcasestr(buff, "PHOTO;");
  char * startptr5 = strcasestr(buff, "BDAY:");
  char * startptr5col = strcasestr(buff, "BDAY;");	
  char * startptr6 = strcasestr(buff, "ADR:");
  char * startptr6col = strcasestr(buff, "ADR;");
  char * startptr7 = strcasestr(buff, "LABEL:");
  char * startptr7col = strcasestr(buff, "LABEL;");
  char * startptr8 = strcasestr(buff, "TEL:");
  char * startptr8col = strcasestr(buff, "TEL;");
  char * startptr9 = strcasestr(buff, "EMAIL:");
  char * startptr9col = strcasestr(buff, "EMAIL;");
  char * startptr10 = strcasestr(buff, "GEO:");
  char * startptr11 = strcasestr(buff, "TITLE:");
  char * startptr11col = strcasestr(buff, "TITLE;");
  char * startptr12 = strcasestr(buff, "ORG:");
  char * startptr12col = strcasestr(buff, "ORG;");		
  char * startptr13 = strcasestr(buff, "NOTE:");
  char * startptr13col = strcasestr(buff, "NOTE;");	
  char * startptr14 = strcasestr(buff, "UID:");
  char * startptr14col = strcasestr(buff, "UID;");
  char * startptr15 = strcasestr(buff, "URL:");
  char * startptr15col = strcasestr(buff, "URL;");
  char * startptr16 = strcasestr(buff, "LABEL:");
  char * startptr16col = strcasestr(buff, "LABEL;");
  
  /*if (strcasestr(buff, "ENCODING=") != NULL){
  return(PAROVER);
    }*/
  /*If a colon is found after property name perform basic operations*/		
  if(startptr){
    syntaxerr = 0;
    propp->name = VCP_FN;
    char str[DEFAULT_STR_SIZE];
    strcpy(str, buff + 3);
    //propp->value = malloc(strlen(str) * sizeof(char));
    //assert(propp->value!=NULL);
    strcpy(propp->value, str);
    return (OK);
  }
  /*If a semi colon is found, look for appropriate types or values*/
  if (startptrcol){
    syntaxerr = 0;
    propp->name = VCP_FN;
    /*Run checks for type= string*/
    char * searchptr = strcasestr(buff, "TYPE=");
    if (searchptr){
      parovercheck = 1;
      char * searchptr2 = strcasestr(searchptr, ":");
      char str[DEFAULT_STR_SIZE];
      /*Isolate the type values*/
      int len = (strlen(searchptr) - strlen(searchptr2) - 5);
      strncpy(str,searchptr + 5, len);
      str[len] = '\0';
      /*Run PAROVER checks*/
      if(str != NULL){
	if(strcasestr(str, "TYPE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str == NULL){
	free(str);
	return(SYNTAX);
      }
      strcpy(propp->partype, str);
      //propp->value = realloc(propp->value, strlen(searchptr2 + 1) * sizeof(char));
      //assert(propp->value!=NULL); 
      strcpy(propp->value, searchptr2 + 1);
      return(OK);
    }
    /*Run checks for value= string*/
    char * valsearchptr = strcasestr(buff, "VALUE=");
    if (valsearchptr){			
      parovercheck = 1;
      char * valsearchptr2 = strcasestr(valsearchptr, ":");
      char str2[DEFAULT_STR_SIZE];	
      /*Isolate the type values*/
      int len2 = (strlen(valsearchptr) - strlen(valsearchptr2) - 6);
      strncpy(str2,valsearchptr + 6, len2);
      str2[len2] = '\0';
      /*Run PAROVER checks*/
      if(str2 != NULL){
	if(strcasestr(str2, "VALUE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str2 == NULL){
	return(SYNTAX);
      }
      strcpy(propp->parval, str2);
      //propp->value = malloc(strlen(valsearchptr2 + 1) * sizeof(char));
      //assert(propp->value!=NULL);
      strcpy(propp->value, valsearchptr2 + 1);
      return(OK);
    }
  }
  /*If a colon is found after property name perform basic operations*/		
  if(startptr2 && (buff[0] == 'N') && (buff[1] == ':')){
    syntaxerr = 0;
    propp->name = VCP_N;
    char str[DEFAULT_STR_SIZE];
    strcpy(str, buff + 2);
    //propp->value = malloc(strlen(str) * sizeof(char));
    //assert(propp->value!=NULL);
    strcpy(propp->value, str);
    return (OK);
  }
  /*If a semi colon is found, look for appropriate types or values*/
  if (startptr2col){
    syntaxerr = 0;
    propp->name = VCP_N;
    /*Run checks for type= string*/
    char * searchptr = strcasestr(buff, "TYPE=");
    if (searchptr){
      parovercheck = 1;
      char * searchptr2 = strcasestr(searchptr, ":");
      char str[DEFAULT_STR_SIZE];
      /*Isolate the type values*/
      int len = (strlen(searchptr) - strlen(searchptr2) - 5);
      strncpy(str,searchptr + 5, len);
      str[len] = '\0';
      /*Run PAROVER checks*/
      if(str != NULL){
	if(strcasestr(str, "TYPE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str == NULL){
	return(SYNTAX);
      }		
      strcpy(propp->partype, str);
      //propp->value = malloc(strlen(searchptr2 + 1) * sizeof(char));
      //assert(propp->value!=NULL);
      strcpy(propp->value, searchptr2 + 1);
      return(OK);
    }
    /*Run checks for value= string*/
    char * valsearchptr = strcasestr(buff, "VALUE=");
    if (valsearchptr){
      parovercheck = 1;
      char * valsearchptr2 = strcasestr(valsearchptr, ":");
      char str2[DEFAULT_STR_SIZE];	
      /*Isolate the type values*/
      int len2 = (strlen(valsearchptr) - strlen(valsearchptr2) - 6);
      strncpy(str2,valsearchptr + 6, len2);
      str2[len2] = '\0';
      /*Run PAROVER checks*/
      if(str2 != NULL){
	if(strcasestr(str2, "VALUE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str2 == NULL){
	return(SYNTAX);
      }
      strcpy(propp->parval, str2);
      //propp->value = malloc(strlen(valsearchptr2 + 1) * sizeof(char));
      //assert(propp->value!=NULL);
      strcpy(propp->value, valsearchptr2 + 1);
      return(OK);
    }
  }
  /*If a colon is found after property name perform basic operations*/
  if(startptr3){
    syntaxerr = 0;
    propp->name = VCP_NICKNAME;
    char str[DEFAULT_STR_SIZE];
    strcpy(str, buff + 9);
    //propp->value = malloc(strlen(str) * sizeof(char));
    //assert(propp->value!=NULL);
    strcpy(propp->value, str);
    return (OK);
  }
  /*If a semi colon is found, look for appropriate types or values*/
  if (startptr3col){
    syntaxerr = 0;
    propp->name = VCP_NICKNAME;
    /*Run checks for type= string*/
    char * searchptr = strcasestr(buff, "TYPE=");
    if (searchptr){
      parovercheck = 1;
      char * searchptr2 = strcasestr(searchptr, ":");
      char str[DEFAULT_STR_SIZE];
      /*Isolate the type values*/
      int len = (strlen(searchptr) - strlen(searchptr2) - 5);
      strncpy(str,searchptr + 5, len);
      str[len] = '\0';
      /*Run PAROVER checks*/
      if(str != NULL){
	if(strcasestr(str, "TYPE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str == NULL){
	return(SYNTAX);
      }		
      strcpy(propp->partype, str);
      //propp->value = malloc(strlen(searchptr2 + 1) * sizeof(char));
      //assert(propp->value!=NULL);
      strcpy(propp->value, searchptr2 + 1);
      return(OK);
    }
    /*Run checks for value string*/
    char * valsearchptr = strcasestr(buff, "VALUE=");
    if (valsearchptr){
      parovercheck = 1;
      char * valsearchptr2 = strcasestr(valsearchptr, ":");
      char str2[DEFAULT_STR_SIZE];	
      /*Isolate the type values*/
      int len2 = (strlen(valsearchptr) - strlen(valsearchptr2) - 6);
      strncpy(str2,valsearchptr + 6, len2);
      str2[len2] = '\0';
      /*Run PAROVER checks*/
      if(str2 != NULL){
	if(strcasestr(str2, "VALUE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str2 == NULL){
	return(SYNTAX);
      }
      strcpy(propp->parval, str2);
      //propp->value = malloc(strlen(valsearchptr2 + 1) * sizeof(char));
      strcpy(propp->value, valsearchptr2 + 1);
      return(OK);
    }
  }
  /*If a colon is found after property name perform basic operations*/		
  if(startptr4){
    syntaxerr = 0;
    propp->name = VCP_PHOTO;
    char str[DEFAULT_STR_SIZE];
    strcpy(str, buff + 6);
    //propp->value = malloc(strlen(str) * sizeof(char));
    strcpy(propp->value, str);
    return (OK);
  }
  /*If a semi colon is found, look for appropriate types or values*/
  if (startptr4col){
    syntaxerr = 0;
    propp->name = VCP_PHOTO;
    /*Run checks for type= string*/
    char * searchptr = strcasestr(buff, "TYPE=");
    if (searchptr){
      parovercheck = 1;
      char * searchptr2 = strcasestr(searchptr, ":");
      char str[DEFAULT_STR_SIZE];
      /*Isolate the type values*/
      int len = (strlen(searchptr) - strlen(searchptr2) - 5);
      strncpy(str,searchptr + 5, len);
      str[len] = '\0';
      /*Run PAROVER checks*/
      if(str != NULL){
	if(strcasestr(str, "TYPE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str == NULL){
	return(SYNTAX);
      }		
      strcpy(propp->partype, str);
      //propp->value = malloc(strlen(searchptr2 + 1) * sizeof(char));
      strcpy(propp->value, searchptr2 + 1);
      return(OK);
    }
    /*Run checks for value string*/
    char * valsearchptr = strcasestr(buff, "VALUE=");
    if (valsearchptr){
      parovercheck = 1;
      char * valsearchptr2 = strcasestr(valsearchptr, ":");
      char str2[DEFAULT_STR_SIZE];	
      /*Isolate the type values*/
      int len2 = (strlen(valsearchptr) - strlen(valsearchptr2) - 6);
      strncpy(str2,valsearchptr + 6, len2);
      str2[len2] = '\0';
      /*Run PAROVER checks*/
      if(str2 != NULL){
	if(strcasestr(str2, "VALUE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str2 == NULL){
	return(SYNTAX);
      }
      strcpy(propp->parval, str2);
      //propp->value = malloc(strlen(valsearchptr2 + 1) * sizeof(char));
      strcpy(propp->value, valsearchptr2 + 1);
      return(OK);
    }
  }
  /*If a colon is found after property name perform basic operations*/
  if(startptr5){
    syntaxerr = 0;
    propp->name = VCP_BDAY;
    char str[DEFAULT_STR_SIZE];
    strcpy(str, buff + 5);
    //propp->value = malloc(strlen(str) * sizeof(char));
    strcpy(propp->value, str);
    return (OK);
  }
  /*If a semi colon is found, look for appropriate types or values*/
  if (startptr5col){
    syntaxerr = 0;
    propp->name = VCP_BDAY;
    /*Run checks for type= string*/
    char * searchptr = strcasestr(buff, "TYPE=");
    if (searchptr){
      parovercheck = 1;
      char * searchptr2 = strcasestr(searchptr, ":");
      char str[DEFAULT_STR_SIZE];
      /*Isolate the type values*/
      int len = (strlen(searchptr) - strlen(searchptr2) - 5);
      strncpy(str,searchptr + 5, len);
      str[len] = '\0';
      /*Run PAROVER checks*/
      if(str != NULL){
	if(strcasestr(str, "TYPE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str == NULL){
	return(SYNTAX);
      }		
      strcpy(propp->partype, str);
      //propp->value = malloc(strlen(searchptr2 + 1) * sizeof(char));
      strcpy(propp->value, searchptr2 + 1);
      return(OK);
    }
    /*Run checks for value string*/
    char * valsearchptr = strcasestr(buff, "VALUE=");
    if (valsearchptr){
      parovercheck = 1;
      char * valsearchptr2 = strcasestr(valsearchptr, ":");
      char str2[DEFAULT_STR_SIZE];	
      /*Isolate the type values*/
      int len2 = (strlen(valsearchptr) - strlen(valsearchptr2) - 6);
      strncpy(str2,valsearchptr + 6, len2);
      str2[len2] = '\0';
      /*Run PAROVER checks*/
      if(str2 != NULL){
	if(strcasestr(str2, "VALUE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str2 == NULL){
	return(SYNTAX);
      }
      strcpy(propp->parval, str2);
      //propp->value = malloc(strlen(valsearchptr2 + 1) * sizeof(char));
      strcpy(propp->value, valsearchptr2 + 1);
      return(OK);
    }
  }
  /*If a colon is found after property name perform basic operations*/
  if(startptr6){
    syntaxerr = 0;
    propp->name = VCP_ADR;
    char str[DEFAULT_STR_SIZE];
    strcpy(str, buff + 4);
    //propp->value = malloc(strlen(str) * sizeof(char));
    strcpy(propp->value, str);
    return (OK);
  }
  /*If a semi colon is found, look for appropriate types or values*/
  if (startptr6col){
    syntaxerr = 0;
    propp->name = VCP_ADR;
    /*Run checks for type= string*/
    char * searchptr = strcasestr(buff, "TYPE=");
    if (searchptr){
      parovercheck = 1;
      char * searchptr2 = strcasestr(searchptr, ":");
      char str[DEFAULT_STR_SIZE];
      /*Isolate the type values*/
      int len = (strlen(searchptr) - strlen(searchptr2) - 5);
      strncpy(str,searchptr + 5, len);
      str[len] = '\0';
      /*Run PAROVER checks*/
      if(str != NULL){
	if(strcasestr(str, "TYPE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str == NULL){
	return(SYNTAX);
      }		
      strcpy(propp->partype, str);
      //propp->value = malloc(strlen(searchptr2 + 1) * sizeof(char));
      strcpy(propp->value, searchptr2 + 1);
      if (strcmp(propp->parval, "")==0){
	propp->parval = NULL;
      }
      if (strcmp(propp->partype, "")==0){
	propp->partype = NULL;
      }
      return(OK);
    }
    /*Run checks for value string*/
    char * valsearchptr = strcasestr(buff, "VALUE=");
    if (valsearchptr){
      parovercheck = 1;
      char * valsearchptr2 = strcasestr(valsearchptr, ":");
      char str2[DEFAULT_STR_SIZE];	
      /*Isolate the type values*/
      int len2 = (strlen(valsearchptr) - strlen(valsearchptr2) - 6);
      strncpy(str2,valsearchptr + 6, len2);
      str2[len2] = '\0';
      /*Run PAROVER checks*/
      if(str2 != NULL){
	if(strcasestr(str2, "VALUE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str2 == NULL){
	return(SYNTAX);
      }
      strcpy(propp->parval, str2);
      //propp->value = malloc(strlen(valsearchptr2 + 1) * sizeof(char));
      strcpy(propp->value, valsearchptr2 + 1);
      if (strcmp(propp->parval, "")==0){
	propp->parval = NULL;
      }
      if (strcmp(propp->partype, "")==0){
	propp->partype = NULL;
      }
      return(OK);
    }
  }
  if(startptr7){
    syntaxerr = 0;
    propp->name = VCP_LABEL;
    char str[DEFAULT_STR_SIZE];
    strcpy(str, buff + 6);
    //propp->value = malloc(strlen(str) * sizeof(char));
    strcpy(propp->value, str);
    return (OK);
  }
  if (startptr7col){
    syntaxerr = 0;
    propp->name = VCP_LABEL;
    /*Run checks for encoding string*/
    char * encodingptr = strcasestr(buff, "ENCODING=");
    if (encodingptr){
      char * searchptr3 = strcasestr(encodingptr, ":");
      strcpy(propp->value,searchptr3 + 1);
      if (strcmp(propp->parval, "")==0){
	propp->parval = NULL;
      }
      if (strcmp(propp->partype, "")==0){
	propp->partype = NULL;
      }
      return(OK);
    }
    /*Run checks for type= string*/
    char * searchptr = strcasestr(buff, "TYPE=");
    if (searchptr){
      parovercheck = 1;
      char * searchptr2 = strcasestr(searchptr, ":");
      char str[DEFAULT_STR_SIZE];
      /*Isolate the type values*/
      int len = (strlen(searchptr) - strlen(searchptr2) - 5);
      strncpy(str,searchptr + 5, len);
      str[len] = '\0';
      /*Run PAROVER checks*/
      if(str != NULL){
	if(strcasestr(str, "TYPE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str == NULL){
	return(SYNTAX);
      }		
      strcpy(propp->partype, str);
      //propp->value = malloc(strlen(searchptr2 + 1) * sizeof(char));
      strcpy(propp->value, searchptr2 + 1);
      if (strcmp(propp->parval, "")==0){
	propp->parval = NULL;
      }
      if (strcmp(propp->partype, "")==0){
	propp->partype = NULL;
      }
      return(OK);
    }
    /*Run checks for value string*/
    char * valsearchptr = strcasestr(buff, "VALUE=");
    if (valsearchptr){
      parovercheck = 1;
      char * valsearchptr2 = strcasestr(valsearchptr, ":");
      char str2[DEFAULT_STR_SIZE];	
      /*Isolate the type values*/
      int len2 = (strlen(valsearchptr) - strlen(valsearchptr2) - 6);
      strncpy(str2,valsearchptr + 6, len2);
      str2[len2] = '\0';
      /*Run PAROVER checks*/
      if(str2 != NULL){
	if(strcasestr(str2, "VALUE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str2 == NULL){
	return(SYNTAX);
      }
      strcpy(propp->parval, str2);
      //propp->value = malloc(strlen(valsearchptr2 + 1) * sizeof(char));
      strcpy(propp->value, valsearchptr2 + 1);
      if (strcmp(propp->parval, "")==0){
	propp->parval = NULL;
      }
      if (strcmp(propp->partype, "")==0){
	propp->partype = NULL;
      }
      return(OK);
    }
  }
  if(startptr8){
    syntaxerr = 0;
    propp->name = VCP_TEL;
    char str[DEFAULT_STR_SIZE];
    strcpy(str, buff + 4);
    //propp->value = malloc(strlen(str) * sizeof(char));
    strcpy(propp->value, str);
    return (OK);
  }
  if (startptr8col){
    syntaxerr = 0;
    propp->name = VCP_TEL;
    /*Run checks for type= string*/
    char * searchptr = strcasestr(buff, "TYPE=");
    if (searchptr){
      parovercheck = 1;
      char * searchptr2 = strcasestr(searchptr, ":");
      char str[DEFAULT_STR_SIZE];
      /*Isolate the type values*/
      int len = (strlen(searchptr) - strlen(searchptr2) - 5);
      strncpy(str,searchptr + 5, len);
      str[len] = '\0';
      /*Run PAROVER checks*/
      if(str != NULL){
	if(strcasestr(str, "TYPE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str == NULL){
	return(SYNTAX);
      }		
      strcpy(propp->partype, str);
      //propp->value = malloc(strlen(searchptr2 + 1) * sizeof(char));
      strcpy(propp->value, searchptr2 + 1);
      if (strcmp(propp->parval, "")==0){
	propp->parval = NULL;
      }
      if (strcmp(propp->partype, "")==0){
	propp->partype = NULL;
      }
      return(OK);
    }
    /*Run checks for value string*/
    char * valsearchptr = strcasestr(buff, "VALUE=");
    if (valsearchptr){
      parovercheck = 1;
      char * valsearchptr2 = strcasestr(valsearchptr, ":");
      char str2[DEFAULT_STR_SIZE];	
      /*Isolate the type values*/
      int len2 = (strlen(valsearchptr) - strlen(valsearchptr2) - 6);
      strncpy(str2,valsearchptr + 6, len2);
      str2[len2] = '\0';
      /*Run PAROVER checks*/
      if(str2 != NULL){
	if(strcasestr(str2, "VALUE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str2 == NULL){
	return(SYNTAX);
      }
      strcpy(propp->parval, str2);
      //propp->value = malloc(strlen(valsearchptr2 + 1) * sizeof(char));
      strcpy(propp->value, valsearchptr2 + 1);
      if (strcmp(propp->parval, "")==0){
	propp->parval = NULL;
      }
      if (strcmp(propp->partype, "")==0){
	propp->partype = NULL;
      }
      return(OK);
    }
  }
  if(startptr9){
    syntaxerr = 0;
    propp->name = VCP_EMAIL;
    char str[DEFAULT_STR_SIZE];
    strcpy(str, buff + 6);
    //propp->value = malloc(strlen(str) * sizeof(char));
    strcpy(propp->value, str);
    return (OK);
  }
  if (startptr9col){
    syntaxerr = 0;
    propp->name = VCP_EMAIL;
    /*Run checks for type= string*/
    char * searchptr = strcasestr(buff, "TYPE=");
    if (searchptr){
      parovercheck = 1;
      char * searchptr2 = strcasestr(searchptr, ":");
      char str[DEFAULT_STR_SIZE];
      /*Isolate the type values*/
      int len = (strlen(searchptr) - strlen(searchptr2) - 5);
      strncpy(str,searchptr + 5, len);
      str[len] = '\0';
      /*Run PAROVER checks*/
      /*if(str != NULL){
      printf("STR:%s", str);
      char * extravalueptr = strcasestr(str, "VALUE=");
      char properstr[DEFAULT_STR_SIZE];
      if(strcasestr(str, "TYPE=") && parovercheck == 1){
	//return(PAROVER);
	char *tempptr = strcasestr(str, "TYPE=");
	char properstr2[DEFAULT_STR_SIZE];
	printf("STR:%s\n", str);
	printf("TYPESTR:%s\n", tempptr + 5);
	strcpy(str, tempptr + 5);
      }
      else if(extravalueptr){
	strncpy(properstr,str,(strlen(str) - strlen(extravalueptr)));
	strcpy(str,properstr);
	printf("STR:%s\n", str);
      }
      }*/
      /*Check if parameters are null*/
      if(str == NULL){
	return(SYNTAX);
      }		
      strcpy(propp->partype, str);
      //propp->value = malloc(strlen(searchptr2 + 1) * sizeof(char));
      strcpy(propp->value, searchptr2 + 1);
      return(OK);
    }
    /*Run checks for value= string*/
    char * valsearchptr = strcasestr(buff, "VALUE=");
    if (valsearchptr){
      parovercheck = 1;
      char * valsearchptr2 = strcasestr(valsearchptr, ":");
      char str2[DEFAULT_STR_SIZE];	
      /*Isolate the type values*/
      int len2 = (strlen(valsearchptr) - strlen(valsearchptr2) - 6);
      strncpy(str2,valsearchptr + 6, len2);
      str2[len2] = '\0';
      /*Run PAROVER checks*/
      if(str2 != NULL){
	if(strcasestr(str2, "VALUE=") && parovercheck == 1){
	  /*return(PAROVER);*/
	}
      }
      /*Check if parameters are null*/
      if(str2 == NULL){
	return(SYNTAX);
      }
      strcpy(propp->parval, str2);
      //propp->value = malloc(strlen(valsearchptr2 + 1) * sizeof(char));
      strcpy(propp->value, valsearchptr2 + 1);
      if (strcmp(propp->parval, "")==0){
	propp->parval = NULL;
      }
      if (strcmp(propp->partype, "")==0){
	propp->partype = NULL;
      }
      return(OK);
    }
  }
  if(startptr10){
    syntaxerr = 0;
    propp->name = VCP_GEO;
    char str[DEFAULT_STR_SIZE];
    strcpy(str, buff + 4);
    //propp->value = malloc(strlen(str) * sizeof(char));
    strcpy(propp->value, str);
    return (OK);
  }
  if(startptr11){
    syntaxerr = 0;
    propp->name = VCP_TITLE;
    char str[DEFAULT_STR_SIZE];
    
    strcpy(str, buff + 6);	
    //propp->value = malloc(strlen(str) * sizeof(char));
    strcpy(propp->value, str);
    return (OK);
  }
  if (startptr11col){
    syntaxerr = 0;
    propp->name = VCP_TITLE;
    /*Run checks for type= string*/
    char * searchptr = strcasestr(buff, "TYPE=");
    if (searchptr){
      parovercheck = 1;
      char * searchptr2 = strcasestr(searchptr, ":");
      char str[DEFAULT_STR_SIZE];
      /*Isolate the type values*/
      int len = (strlen(searchptr) - strlen(searchptr2) - 5);
      strncpy(str,searchptr + 5, len);
      str[len] = '\0';
      /*Run PAROVER checks*/
      if(str != NULL){
	if(strcasestr(str, "TYPE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str == NULL){
	return(SYNTAX);
      }		
      strcpy(propp->partype, str);
      //propp->value = malloc(strlen(searchptr2 + 1) * sizeof(char));
      strcpy(propp->value, searchptr2 + 1);
      return(OK);
    }
    /*Run checks for value= string*/
    char * valsearchptr = strcasestr(buff, "VALUE=");
    if (valsearchptr){
      parovercheck = 1;
      char * valsearchptr2 = strcasestr(valsearchptr, ":");
      char str2[DEFAULT_STR_SIZE];	
      /*Isolate the type values*/
      int len2 = (strlen(valsearchptr) - strlen(valsearchptr2) - 6);
      strncpy(str2,valsearchptr + 6, len2);
      str2[len2] = '\0';
      /*Run PAROVER checks*/
      if(str2 != NULL){
	if(strcasestr(str2, "VALUE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str2 == NULL){
	return(SYNTAX);
      }
      strcpy(propp->parval, str2);
      //propp->value = malloc(strlen(valsearchptr2 + 1) * sizeof(char));
      strcpy(propp->value, valsearchptr2 + 1);
      if (strcmp(propp->parval, "")==0){
	propp->parval = NULL;
      }
      if (strcmp(propp->partype, "")==0){
	propp->partype = NULL;
      }
      return(OK);
    }
  }
  if(startptr12){
    syntaxerr = 0;
    propp->name = VCP_ORG;
    char str[DEFAULT_STR_SIZE];
    strcpy(str, buff + 4);
    //propp->value = malloc(strlen(str) * sizeof(char));
    strcpy(propp->value, str);
    return (OK);
  }
  if (startptr12col){
    syntaxerr = 0;
    propp->name = VCP_ORG;
    char * searchptr = strcasestr(buff, "TYPE=");
    if (searchptr){
      parovercheck = 1;
      char * searchptr2 = strcasestr(searchptr, ":");
      char str[DEFAULT_STR_SIZE];
      /*Isolate the type values*/
      int len = (strlen(searchptr) - strlen(searchptr2) - 5);
      strncpy(str,searchptr + 5, len);
      str[len] = '\0';
      /*Run PAROVER checks*/
      if(str != NULL){
	if(strcasestr(str, "TYPE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str == NULL){
	return(SYNTAX);
      }		
      strcpy(propp->partype, str);
      //propp->value = malloc(strlen(searchptr2 + 1) * sizeof(char));
      strcpy(propp->value, searchptr2 + 1);
      if (strcmp(propp->parval, "")==0){
	propp->parval = NULL;
      }
      if (strcmp(propp->partype, "")==0){
	propp->partype = NULL;
      }
      return(OK);
    }
    /*Run checks for value string*/
    char * valsearchptr = strcasestr(buff, "VALUE=");
    if (valsearchptr){
      parovercheck = 1;
      char * valsearchptr2 = strcasestr(valsearchptr, ":");
      char str2[DEFAULT_STR_SIZE];	
      /*Isolate the type values*/
      int len2 = (strlen(valsearchptr) - strlen(valsearchptr2) - 6);
      strncpy(str2,valsearchptr + 6, len2);
      str2[len2] = '\0';
      /*Run PAROVER checks*/
      if(str2 != NULL){
	if(strcasestr(str2, "VALUE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str2 == NULL){
	return(SYNTAX);
      }
      strcpy(propp->parval, str2);
      //propp->value = malloc(strlen(valsearchptr2 + 1) * sizeof(char));
      strcpy(propp->value, valsearchptr2 + 1);
      if (strcmp(propp->parval, "")==0){
	propp->parval = NULL;
      }
      if (strcmp(propp->partype, "")==0){
	propp->partype = NULL;
      }
      return(OK);
    }
  }
  if(startptr13){
    syntaxerr = 0;
    propp->name = VCP_NOTE;
    char str[DEFAULT_STR_SIZE];
    strcpy(str, buff + 5);
    //propp->value = malloc(strlen(str) * sizeof(char));
    strcpy(propp->value, str);
    if (strcmp(propp->parval, "")==0){
      propp->parval = NULL;
    }
    if (strcmp(propp->partype, "")==0){
      propp->partype = NULL;
    }
    return (OK);
  }
  if (startptr13col){
    syntaxerr = 0;
    propp->name = VCP_NOTE;
    char * searchptr = strcasestr(buff, "TYPE=");
    if (searchptr){
      parovercheck = 1;
      char * searchptr2 = strcasestr(searchptr, ":");
      char str[DEFAULT_STR_SIZE];
      /*Isolate the type values*/
      int len = (strlen(searchptr) - strlen(searchptr2) - 5);
      strncpy(str,searchptr + 5, len);
      str[len] = '\0';
      /*Run PAROVER checks*/
      if(str != NULL){
	if(strcasestr(str, "TYPE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str == NULL){
	return(SYNTAX);
      }		
      strcpy(propp->partype, str);
      //propp->value = malloc(strlen(searchptr2 + 1) * sizeof(char));
      strcpy(propp->value, searchptr2 + 1);
      if (strcmp(propp->parval, "")==0){
	propp->parval = NULL;
      }
      if (strcmp(propp->partype, "")==0){
	propp->partype = NULL;
      }
      return(OK);
    }
    /*Run checks for value string*/
    char * valsearchptr = strcasestr(buff, "VALUE=");
    if (valsearchptr){
      parovercheck = 1;
      char * valsearchptr2 = strcasestr(valsearchptr, ":");
      char str2[DEFAULT_STR_SIZE];	
      /*Isolate the type values*/
      int len2 = (strlen(valsearchptr) - strlen(valsearchptr2) - 6);
      strncpy(str2,valsearchptr + 6, len2);
      str2[len2] = '\0';
      /*Run PAROVER checks*/
      if(str2 != NULL){
	if(strcasestr(str2, "VALUE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str2 == NULL){
	return(SYNTAX);
      }
      strcpy(propp->parval, str2);
      //propp->value = malloc(strlen(valsearchptr2 + 1) * sizeof(char));
      strcpy(propp->value, valsearchptr2 + 1);
      if (strcmp(propp->parval, "")==0){
	propp->parval = NULL;
      }
      if (strcmp(propp->partype, "")==0){
	propp->partype = NULL;
      }
      return(OK);
    }
  }
  if(startptr14){
    syntaxerr = 0;
    propp->name = VCP_UID;
    char str[DEFAULT_STR_SIZE];
    strcpy(str, buff + 4);
    //propp->value = malloc(strlen(str) * sizeof(char));
    strcpy(propp->value, str);
    return (OK);
  }
  if(startptr14col){
    syntaxerr = 0;
    propp->name = VCP_UID;
    char * searchptr = strcasestr(buff, "TYPE=");
    if (searchptr){
      parovercheck = 1;
      char * searchptr2 = strcasestr(searchptr, ":");
      char str[DEFAULT_STR_SIZE];
      //Isolate the type values
      int len = (strlen(searchptr) - strlen(searchptr2) - 5);
      strncpy(str,searchptr + 5, len);
      str[len] = '\0';
      //Run PAROVER checks
      if(str != NULL){
	if(strcasestr(str, "TYPE=") && parovercheck == 1){
	  //return(PAROVER);	
	}
      }
      //Check if parameters are null
      if(str == NULL){
	return(SYNTAX);
      }		
      strcpy(propp->partype, str);
      //propp->value = malloc(strlen(searchptr2 + 1) * sizeof(char));
      strcpy(propp->value, searchptr2 + 1);
      return(OK);
    }
    //Run checks for value string
    char * valsearchptr = strcasestr(buff, "VALUE=");
    if (valsearchptr){
      parovercheck = 1;
      char * valsearchptr2 = strcasestr(valsearchptr, ":");
      char str2[DEFAULT_STR_SIZE];	
      //Isolate the type values
      int len2 = (strlen(valsearchptr) - strlen(valsearchptr2) - 6);
      strncpy(str2,valsearchptr + 6, len2);
      str2[len2] = '\0';
      //Run PAROVER checks
      if(str2 != NULL){
	if(strcasestr(str2, "VALUE=") && parovercheck == 1){
	  //return(PAROVER);	
	}
      }
      //Check if parameters are null
      if(str2 == NULL){
	return(SYNTAX);
      }
      strcpy(propp->parval, str2);
      //propp->value = malloc(strlen(valsearchptr2 + 1) * sizeof(char));
      strcpy(propp->value, valsearchptr2 + 1);
      return(OK);
    }
  }
  
  if(startptr15){
    syntaxerr = 0;
    propp->name = VCP_URL;
    char str[DEFAULT_STR_SIZE];
    strcpy(str, buff + 4);
    //propp->value = malloc(strlen(str) * sizeof(char));
    strcpy(propp->value, str);
    return (OK);
  }
  if(startptr15col){
    syntaxerr = 0;
    propp->name = VCP_URL;
    char * searchptr = strcasestr(buff, "TYPE=");
    if (searchptr){
      parovercheck = 1;
      char * searchptr2 = strcasestr(searchptr, ":");
      char str[DEFAULT_STR_SIZE];
      /*Isolate the type values*/
      int len = (strlen(searchptr) - strlen(searchptr2) - 5);
      strncpy(str,searchptr + 5, len);
      str[len] = '\0';
      /*Run PAROVER checks*/
      if(str != NULL){
	if(strcasestr(str, "TYPE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str == NULL){
	return(SYNTAX);
      }		
      strcpy(propp->partype, str);
      //propp->value = malloc(strlen(searchptr2 + 1) * sizeof(char));
      strcpy(propp->value, searchptr2 + 1);
      return(OK);
    }
    /*Run checks for value string*/
    char * valsearchptr = strcasestr(buff, "VALUE=");
    if (valsearchptr){
      parovercheck = 1;
      char * valsearchptr2 = strcasestr(valsearchptr, ":");
      char str2[DEFAULT_STR_SIZE];	
      /*Isolate the type values*/
      int len2 = (strlen(valsearchptr) - strlen(valsearchptr2) - 6);
      strncpy(str2,valsearchptr + 6, len2);
      str2[len2] = '\0';
      /*Run PAROVER checks*/
      if(str2 != NULL){
	if(strcasestr(str2, "VALUE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str2 == NULL){
	return(SYNTAX);
      }
      strcpy(propp->parval, str2);
      //propp->value = malloc(strlen(valsearchptr2 + 1) * sizeof(char));
      strcpy(propp->value, valsearchptr2 + 1);
      return(OK);
    }
    
  }
  if (startptr16){
    syntaxerr = 0;
    propp->name = VCP_LABEL;
    char str[DEFAULT_STR_SIZE];
    strcpy(str, buff + 3);
    //propp->value = malloc(strlen(str) * sizeof(char));
    strcpy(propp->value, str);	
    return (OK);
  }
  if (startptr16col){
    syntaxerr = 0;
    propp->name = VCP_LABEL;
    char * searchptr = strcasestr(buff, "TYPE=");
    if (searchptr){
      parovercheck = 1;
      char * searchptr2 = strcasestr(searchptr, ":");
      char str[DEFAULT_STR_SIZE];
      /*Isolate the type values*/
      int len = (strlen(searchptr) - strlen(searchptr2) - 5);
      strncpy(str,searchptr + 5, len);
      str[len] = '\0';
      /*Run PAROVER checks*/
      if(str != NULL){
	if(strcasestr(str, "TYPE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str == NULL){
	return(SYNTAX);
      }		
      strcpy(propp->partype, str);
      //propp->value = malloc(strlen(searchptr2 + 1) * sizeof(char));
      strcpy(propp->value, searchptr2 + 1);
    }
    /*Run checks for value string*/
    char * valsearchptr = strcasestr(buff, "VALUE=");
    if (valsearchptr){
      parovercheck = 1;
      char * valsearchptr2 = strcasestr(valsearchptr, ":");
      char str2[DEFAULT_STR_SIZE];	
      /*Isolate the type values*/
      int len2 = (strlen(valsearchptr) - strlen(valsearchptr2) - 6);
      strncpy(str2,valsearchptr + 6, len2);
      str2[len2] = '\0';
      /*Run PAROVER checks*/
      if(str2 != NULL){
	if(strcasestr(str2, "VALUE=") && parovercheck == 1){
	  /*return(PAROVER)*/;	
	}
      }
      /*Check if parameters are null*/
      if(str2 == NULL){
	return(SYNTAX);
      }
      strcpy(propp->parval, str2);
      //propp->value = malloc(strlen(valsearchptr2 + 1) * sizeof(char));
      strcpy(propp->value, valsearchptr2 + 1);
      return(OK);
    }
  }
  
  if(strcasestr(buff, ":")==NULL && strcasestr(buff, ";") == NULL){
    return(SYNTAX);
  }
  //if (syntaxerr == 1){
    //return(SYNTAX);
    //}
    //else{
      /*Run checks for begin, version, end and OTHER*/
      if(strcmp(buff, "BEGIN:VCARD")==0){
	propp->name = VCP_BEGIN;
	strcpy(propp->value, "VCARD"); 
	if (strcmp(propp->parval, "")==0){
	  propp->parval = NULL;
	}
	if (strcmp(propp->partype, "")==0){
	  propp->partype = NULL;
	}
	return(OK);
      }
      if(strcmp(buff, "VERSION:3.0")==0){
	propp->name = VCP_VERSION;
	strcpy(propp->value, "3.0"); 
	if (strcmp(propp->parval, "")==0){
	  propp->parval = NULL;
	}
	if (strcmp(propp->partype, "")==0){
	  propp->partype = NULL;
	}
	return(OK);
      }
      if(strcmp(buff, "END:VCARD")==0){
	propp->name = VCP_END;
	strcpy(propp->value, "VCARD"); 
	if (strcmp(propp->parval, "")==0){
	  propp->parval = NULL;
	}
	if (strcmp(propp->partype, "")==0){
	  propp->partype = NULL;
	}
	return(OK);
      }
      propp->name = VCP_OTHER;
      strcpy(propp->value, buff);
      if (strcmp(propp->parval, "")==0){
	propp->parval = NULL;
      }
      if (strcmp(propp->partype, "")==0){
	propp->partype = NULL;
      }
      return (OK);	
      //}
}

/************
Precondition: filep points to a VcFile structure.

Postcondition: All pointers within VcFile have been freed. filep itself is not freed; the caller must do that if the structure is not to be reused.

Traverse the data structure, freeing all of the pointers, including char* arrays and variable length arrays.
*************/
void freeVcFile( VcFile *filep ){
  int i = 0, j = 0;
  for(i = 0; i < filep->ncards ; i++){
    if (filep->cardp[i] != NULL){
      for (j = 0; j < filep->cardp[i]->nprops;j++){
	free(filep->cardp[i]->prop[j].value); 
      }
      free(filep->cardp[i]);
    }
  }
}

/************
Precondition: vcf is already open for writing and is assumed to be positioned at beginning of file. 
*filep contains the contents (at least one card) to be written out on the file in textual form.

Postcondition: The contents have been written on the specified file, and the total number of lines output is returned as the function's value. 
A -1 return indicates failure, in which case the file contents is undefined. In either case, the file is still open. 
Any storage allocated by this function has been freed.

Get the filep contains your vcards, and output them appropriately to a file chosen from stdin.
*************/
int writeVcFile( FILE* const vcf, const VcFile *filep ){
  
  char propertyname [][10] = {"BEGIN","END", "VERSION", "N", "FN", "NICKNAME", "PHOTO", "BDAY", "ADR", "LABEL", "TEL", "EMAIL", "GEO", "TITLE", "ORG", "NOTE", "UID", "URL", "OTHER"};
  int i = 0, j =0, h = 0;
  int linesoutput = 0;
  char finalstring[1000];
  finalstring[0] = '\0';
  
  if (vcf == NULL){
    return (-1);
  }
  
  for (i = 0; i < filep->ncards ; i++){
    if (filep->cardp[i] != NULL){
      fprintf(vcf, "BEGIN:VCARD\n");
      linesoutput++;
      fprintf(vcf, "VERSION:");
      fprintf(vcf, VCARD_VER);
      linesoutput++;
      fprintf(vcf, "\r\n");
      for (j = 0; j < filep->cardp[i]->nprops;j++){
	/*Check all non-NULL values and create a string*/
	strcat(finalstring, propertyname[filep->cardp[i]->prop[j].name]);
	if (filep->cardp[i]->prop[j].partype != NULL && strcmp(filep->cardp[i]->prop[j].partype, "") != 0){
	  strcat(finalstring, ";TYPE=");
	  strcat(finalstring, filep->cardp[i]->prop[j].partype);
	}
	if (filep->cardp[i]->prop[j].parval != NULL && strcmp(filep->cardp[i]->prop[j].parval, "") != 0){
	  strcat(finalstring, ";VALUE=");
	  strcat(finalstring, filep->cardp[i]->prop[j].parval);
	}
	if (filep->cardp[i]->prop[j].value != NULL && strcmp(filep->cardp[i]->prop[j].value, "") != 0){
	  strcat(finalstring, ":");
	  strcat(finalstring, filep->cardp[i]->prop[j].value);
	}
	
	int lengthcheck = 0;
	//print character by character
	for (h = 0; h < strlen(finalstring) ; h++){
	  if (strlen(finalstring) == 75){
	    fprintf(vcf, "%s", finalstring);
	    break;
	  }
	  lengthcheck++;
	  fprintf(vcf, "%c", finalstring[h]);
	  if ((lengthcheck%FOLD_LEN) == 0){
	    linesoutput++;
	    fprintf(vcf, "\r\n ");
	    lengthcheck = 0;
	  }
	}
	linesoutput++;
	fprintf(vcf, "\r\n");
	finalstring[0] = '\0';
      }
      fprintf(vcf, "END:VCARD\n");
      linesoutput++;
    }
  }
  return (linesoutput);
}

/*int main(void){
FILE *vcf = NULL;
vcf = fopen ("test.vcf","r");
	VcFile *filep = (VcFile *) malloc (sizeof(VcFile));
	readVcFile(vcf, filep);
	//writeVcFile(vcf,filep);
	//printf("PARTYPE:%s\n", filep->cardp[0]->prop[0].value);
	//printf("PARTYPE:%s\n", filep->cardp[0]->prop[4].partype);
	//printf("PARTYPE:%s\n", filep->cardp[0]->prop[4].parval);
	//free(filep);
	return 0;
	}*/