/********
Vcftool is a program which will use our base utility functions from vcutil.c to manipulate Vcard files to our choosing using the 
command line in the classic UNIX "filter" style. 

Marc Bodmer
0657005
mbodmer@uoguelph.ca
********/
//#include <Python.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <regex.h>
#include "vcftool.h"

#define DEFAULT_STR_SIZE 200

//Global Arrays for Address
char convertcountrylist[20][30] = {"CA", "JP", "CN", "US", "GB", "ZA", "SG", "NZ", "MX", "IT", "HK", "FR", "DE", "BR", "AU"};
char countrylist[20][30] = {"Canada", "Japan", "China", "United States", "United Kingdom", "South Africa", "Singapore", "New Zealand", "Mexico", "Italy", "Hong Kong", "France", "Germany", "Brazil", "Australia"};
char convertprovincelist[20][20] = {"AB", "BC", "MB", "NB", "NL", "NT", "NS", "NU", "ON", "PE", "QC", "SK", "YT"};
char provincelist[20][30] = {"Alberta", "British Columbia", "Manitoba", "New Brunswick", "Newfoundland and Labrador", "Northwest Territories", "Nova Scotia", "Nunavut", "Ontario", "Prince Edward Island", "Quebec", "Saskatchewan", "Yukon" };

/**************
Function to check if the cards are already sorted
***************/
int check_if_sorted(const VcFile *filep){
  int i = 0, j = 0, firstcheck = 0, onenamefound = 0;
  char nameone[DEFAULT_STR_SIZE];
  char nametwo[DEFAULT_STR_SIZE];
  nameone[0] = '\0';
  nametwo[0] = '\0';
  
  /*strcmp one name to the next, and keep going. If you get to the end, the card is already sorted*/
  for (i = 0; i < filep->ncards ; i++){
    if (filep->cardp[i] != NULL){
      for (j = 0; j < filep->cardp[i]->nprops;j++){
	if (filep->cardp[i]->prop[j].name == VCP_N){
	  if (onenamefound == 1){
	    strcpy(nametwo, filep->cardp[i]->prop[j].value);
	    onenamefound = 0;
	    break;
	  }
	  else{
	    strcpy(nameone, filep->cardp[i]->prop[j].value);
	    onenamefound = 1;
	    break;
	  }
	}
      }
      if (nameone != NULL && strcmp(nametwo, "") != 0){
	if (firstcheck == 0){
	  if (strcmp(nameone, nametwo) == 1){
	    return (0);
	  }
	  firstcheck = 1;
	}
	else if (firstcheck == 1){
	  if (strcmp(nametwo, nameone) == 1){
	    return (0);
	  }
	  firstcheck = 0;
	}	
      }
    }
  }
  return (1);
}

/**************
Precondition: outfile is already open for writing, and *filep was successfully constructed by readVcFile.

Postcondition: Required info has been printed on outfile , or errors have been printed on stderr .

Run through the cards and keep track of whether they are in ascending alphabetical order (case insensitive) by family name and given name. 
Keep count of the cards having PHOTO, URL, and GEO properties, and those having UIDs indicating canonical status for the card.
***************/
int vcfInfo( FILE *const outfile, const VcFile *filep ){
  
  int i = 0, j = 0;
  int photocount = 0, urlcount = 0, geocount = 0, uidcount = 0;
  int pcheck = 0, gcheck = 0, ucheck = 0;
  
  /*Run through the cards checking for the specified properties*/
  for (i = 0; i < filep->ncards ; i++){
    if (filep->cardp[i] != NULL){
      for (j = 0; j < filep->cardp[i]->nprops;j++){
	if(filep->cardp[i]->prop[j].name == VCP_PHOTO){
	  if (pcheck == 0){
	    photocount++;
	    pcheck = 1;
	  }
	}
	if(filep->cardp[i]->prop[j].name == VCP_GEO){
	  if (gcheck == 0){
	    geocount++;
	    gcheck = 1;
	  }
	}
	if(filep->cardp[i]->prop[j].name == VCP_URL){
	  if (ucheck == 0){	    
	    urlcount++;
	    ucheck = 1;
	  }
	}
	if(filep->cardp[i]->prop[j].name == VCP_UID){
	    if (strlen(filep->cardp[i]->prop[j].value) > 6 && filep->cardp[i]->prop[j].value[0] == '@' && filep->cardp[i]->prop[j].value[5] == '@' 
	      && filep->cardp[i]->prop[j].value[1] != '*' && filep->cardp[i]->prop[j].value[2] != '*' && filep->cardp[i]->prop[j].value[3] != '*' 
	      && filep->cardp[i]->prop[j].value[4] != '*'){
	      uidcount++;
	    }
	}
      }
      pcheck = 0, gcheck = 0, ucheck = 0;
    }
  }
  fprintf(outfile, "%d cards ", filep->ncards);
  int check = check_if_sorted(filep);
  if (check == 1){
    fprintf(outfile, "(sorted)\n");
  }
  else{
    fprintf(outfile, "(not sorted)\n");
  }
  fprintf(outfile, "%d with photos\n", photocount);
  fprintf(outfile, "%d with URLs\n", urlcount);
  fprintf(outfile, "%d with geographic coordinates\n", geocount);
  fprintf(outfile, "%d in canonical form\n", uidcount);
  
  //length greater than 6 !=* and the ones that are @ are equal to @
  return EXIT_SUCCESS;
}


/**************
Function to compare the input value of a GEO and tries to match it to a regular expression
***************/
int match_GEO(char * value ){
  regex_t regEX;
  char * regexpattern2 = ("^( *\\-?)([1-9]?[0-9]?\\.[0-9]{6}|(90\\.000000))( *(,|;) *)\\-?([1]?[0-9]?[0-9]?\\.[0-9]{6}|(180\\.000000)) *");
  
  if(regcomp(&regEX, regexpattern2, REG_EXTENDED) != 0){
    fprintf(stderr, "Regcomp() failed!!");
    exit(0);
  }
  
  if (regexec(&regEX, value, 0, NULL, 0) == 0){                
    //Did match
    return (4);
  }
  //To fix Geo space, split into 2 parts and cat them to each other with a ; in between
  
  //To fix Geo decimal, split into 2 parts and cat extra Zero
   
  int i = 0;
  for (i = 0; i < strlen(value); i++){
    if (isalpha(value[i])){
      return 3;
    }
  }
  
  //Split into 2 parts by running a for loop and the ncheck for a SPACE OR , OR ;
  int j = 0;
  char firstpart[30], secondpart[30], temp[20];
  for(j = 0; j < strlen(value) ; j++){
    temp[j] = value[j];
    //Check for a space seperator
    if (isspace(value[j])){
      value[j] = ';';
    }
}
  int splitflag = 0, k =0;
  for(j = 0; j < strlen(value) ; j++){
    if (splitflag == 0){
       firstpart[j] = value[j];
    }
    else{
	secondpart[k] = value[j];
	k++;
    }
    if (value[j] == ',' || value[j] == ';'){
      splitflag = 1;
      firstpart[j] = '\0';
    }    
  }    

  secondpart[k] = '\0';

  int lengthflag = 0;
  if (strlen(firstpart) < 9){
    lengthflag++;
    for(i = 0; i < (10 - strlen(firstpart)); i++){
      strcat(firstpart, "0");
    }
    if (strlen(firstpart) < 9){
       strcat(firstpart, "0");
    }
    strcat(firstpart, ";");
  }
  if (strlen(secondpart) < 9){
    lengthflag++;
    for(i = 0; i < (10 - strlen(secondpart)); i++){
      strcat(secondpart, "0");
    }
    if (strlen(secondpart) < 10){
      strcat(secondpart, "0");
    }
  }
  
  if (lengthflag == 2){
    strcat(firstpart, secondpart);
    strcpy(value, firstpart);
  }
  
 
  if (regexec(&regEX, value, 0, NULL, 0) != 0){                
    //Did not match
    return (0);
  }
  else{
   return (1);
  }
  
  return EXIT_SUCCESS; 
}

/**************
Function to compare the input value to a NAME and tries to match it to a regular expression
***************/
int match_NAME(char * value ){
  
  int changedsomethingflag = 0;
  regex_t regEX;
  //If the first letter is capitalized, and then for any number of letters before a semicolon
  //char * regexpattern2 = ("[A-Z](([^;])*|(['][A-Z])|([ ][A-Z]))*[;]?[A-Z](([^;])*|(['][A-Z])|([ ][A-Z]))*[;]?[A-Z](([^;])*|(['][A-Z])|([ ][A-Z]))*[;]?[A-Z](([^;])*|(['][A-Z])|([ ][A-Z]))*[;]?");
  char * regexpattern2 = ("[A-Z](([^;])*|(['][A-Z])|([ ][A-Z]))*[;]?[A-Z](([^;])*|(['][A-Z])|([ ][A-Z]))*([^;]*;)?([^;]*;)?([^;]*;)?");
  
  if(regcomp(&regEX, regexpattern2, REG_EXTENDED) != 0){
    fprintf(stderr, "Regcomp() failed!!");
    exit(0);
  }
  
  int i = 0;
  //Check for lower case letters
  if (islower(value[0])){
    changedsomethingflag = 1;
    value[0] = toupper(value[0]);
  }
  
  for (i = 0; i < strlen(value); i++){
    if (value[i] == ';'){
      if (islower(value[i + 1])){
	changedsomethingflag = 1;
	value[i + 1] = toupper(value[i + 1]);
      }
    }
  }
  
  int count = 0;
  //Check for Empty Name, strtok for ; and if it returns null make it (none)
  for (i = 0; i < strlen(value); i++){
    if (value[i] == ';'){
      count++;
    }
  }
  
  if (count == 0){
    return 3;
  }
  
  if (strcmp(value, ";") == 0){
    changedsomethingflag = 1;
    strcpy(value, "(none);(none)");
  }

  if (changedsomethingflag == 1){
      return (1);
  }
  else{
      return(4);
  }
  /*else if (regexec(&regEX, value, 0, NULL, 0) != 0){                
    //Did not match
    return (0);
  }
  else{
    //Matched
   return (1);
  }*/
  
}

/**************
Function to compare the input value to a NAME and tries to match it to a regular expression
***************/
int match_ADR(char * value){
  
  //regex_t regEX;
  int changedsomethingflag = 0;
  //If the first letter is capitalized, and then for any number of letters before a semicolon
  //char * regexpattern2 = ("([^;])*[;]([^;])*[;]([^;])*[;]([^;])*[;]([^;])*[;]([^;])*[;]([^;])*");
  
  /*if(regcomp(&regEX, regexpattern2, REG_EXTENDED) != 0){
    fprintf(stderr, "Regcomp() failed!!");
    exit(0);
  }*/
  int count = 0, i = 0, j = 0, h = 0, p = 0;
  
  //Split into two strings
  char firsthalf[20], secondhalf[20];
  int semicount = 0;
  for (i = 0; i < strlen(value); i++){
    if (semicount <= 4){
      firsthalf[i] = value[i];
    }
    else{
     secondhalf[p] = value[i];
     p++;
    }
    if (value[i] == ';'){
      semicount++;
    }
  }
   
  char temp[40], temp2[40];
  strcpy(temp, value);
  //Check for INvalid string
  for (i = 0; i < strlen(value); i++){
    if (value[i] == ';'){
      count++;
    }
 
    if (count == 6){
      for (j = i + 1 ; j < strlen(value); j++){
	temp[h] = value[j];
	h++;
      }
      count = 1;
    }
  }
  temp[h] = '\0';
  //fprintf (stderr, "THIS: %s\n",temp);  
  //Check countrylist for any matching countrys
  for(i = 0; i < 20; i++){
    for(j = 0; j < 30; j++){
      if (strcmp(temp, countrylist[i]) == 0){
	//fprintf (stderr, "CouNTRY: %s\n",countrylist[i]);
	changedsomethingflag = 1;
	strcpy(temp2, convertcountrylist[i]);
	char * csearch;
	csearch = strstr (value,temp);
	strncpy(csearch, temp2, 2);
	csearch[2] = '\0';
	//fprintf (stderr, "FINAL: %s\n",temp2);
	break;
      }
    }
  }
  
  
  if (count == 0){
    return 3;
  }
   
   int pcount = 0;
   char ptemp[40] = {};
   char * psearch;

   for(i = 0; i < 20; i++){
     if (pcount == 1){
       break;
     }
     psearch = strstr (firsthalf,provincelist[i]);
     if (psearch != NULL){
       //changedsomethingflag = 1;
       strncpy(psearch, convertprovincelist[i], 2);
       psearch[2] = '\0';       
       strcpy(ptemp, firsthalf);
       firsthalf[0] = '\0';
       pcount = 1;
     }
     if (pcount == 1){
       break;
     }
   }

  if (strlen(ptemp) > 4){
    strcat(ptemp, ";");
    strcat(ptemp, secondhalf);
    strcpy(value, ptemp);
  }

  if (changedsomethingflag == 1){
      //canonized succeded
    return (1);
  }
  else{
    //it was in form already
    return(4);
  }
  /*if (regexec(&regEX, value, 0, NULL, 0) != 0){                
    //Did not match
    return (0);
  }
  else{
   return (1);
  }*/
  
}

/**************
Function to compare the input value to a NAME and tries to match it to a regular expression
***************/
int match_TEL(char * value ){
  
  int changedsomethingflag = 0;
  regex_t regEX;
  char * regexpattern2 = ("[+][0-9][0-9]?[ ][(][0-9][0-9][0-9]?[)][ ][0-9]{3}[-][0-9]{4}");
  
  if(regcomp(&regEX, regexpattern2, REG_EXTENDED) != 0){
    fprintf(stderr, "Regcomp() failed!!");
    exit(0);
  }
  
  
  int count = 0, i = 0;
  //Check for INvalid string
  for (i = 0; i < strlen(value); i++){
    if (isalpha(value[i])){
      count++;
    }
  }
  if (count > 7){
    return 3;
  }
  
  //Check for +1
  char temp[40] = {};
  strcpy(temp, "+1 ");
  strcat(temp, value);
  if (value[0] == '('){
    strcpy(value, temp);
  }
  
  //Simple conversion case
  //
  char temp2[30], temp4[30];
  int x = 0;
  for (i = 0; i < strlen(value); i++){
      if (isdigit(value[i])){
	temp4[x] = value[i];
	x++;
      }
  }
  
  if (value[0] != '+'){
    changedsomethingflag = 1;
    temp2[0] = '+';
    temp2[1] = '1';
    temp2[2] = ' ';
    temp2[3] = '(';
    temp2[4] = temp4[0];
    temp2[5] = temp4[1];
    temp2[6] = temp4[2];
    temp2[7] = ')';
    temp2[8] = ' ';   
    temp2[9] = temp4[3]; 
    temp2[10] = temp4[4]; 
    temp2[11] = temp4[5]; 
    temp2[12] = '-'; 
    temp2[13] = temp4[6]; 
    temp2[14] = temp4[7]; 
    temp2[15] = temp4[8]; 
    temp2[16] = temp4[9];
    temp2[17] = '\0';
    strcpy(value, temp2);
  }
  /*char temp2[30], temp3[30], temp4[30];
  strcpy(temp4, value);
  if (value[0] != '+'){
    changedsomethingflag = 1;
    temp2[0] = '(';
    temp2[1] = value[0];
    temp2[2] = value[1];
    temp2[3] = value[2];
    temp2[4] = ')';
    temp2[5] = ' ';
    strcpy(temp3, "+1 ");
    strcat(temp3, temp2);
    char * pch;
    pch = strtok (temp4,"-");
    pch = strtok (NULL, " ,.-");
    strcat(temp3, pch);
    pch = strtok (NULL, " ,.-");
    strcat(temp3, "-");
    strcat(temp3, pch);
    strcpy(value, temp3);
  }*/
  
  //Extension Case
  char valuetemp[30] = "";
  strcpy(valuetemp, value);
  char * pch = NULL;
  pch = strstr (valuetemp," Ext. ");
  char numtemp[10] = "";
  int h = 0;

  if (pch != NULL){
    for (i = 0; i < strlen(pch); i++){
      if(isdigit(pch[i])){
	numtemp[h] = pch[i];
	h++;
      }
    }
    changedsomethingflag = 1;
    numtemp[strlen(numtemp)] = '\0';
    char * pch2;
    pch2 = strstr (valuetemp," Ext. ");
    strcpy(pch2, "|");
    strcat(valuetemp,numtemp);
    strcpy(value,valuetemp);
  }

  if (changedsomethingflag == 1){
      //canonized succeded
    return (1);
  }
  else if (regexec(&regEX, value, 0, NULL, 0) != 0){                
    //Did not match
    return (0);
  }
  else{
   return (4);
  }
  
  return EXIT_SUCCESS; 
}

/**************
Precondition: *propp  was constructed by parseVcProp.

Postcondition: The return value gives the status of the operation: 0, canonicalization does not apply to this type of property; 1, it was already in canonical form; 
2, canonicalization succeeded; 3, canonicalization failed because the data could not be recognized. *propp has been modified if 2 is returned.

Check the card properties to see if it can be Canonicalized and if they need to be call the required functions in order to do so.
***************/
int vcfCanProp( VcProp *const propp ){
  
  /*Geo has been found*/
  if (propp->name == VCP_GEO){
    int check = match_GEO(propp->value);
    //Regex did not match
    if (check == 3){
      return(3);
    }
    //Regex did match
    else if(check == 1){
      return(2);
    }
    else if (check == 4){
      return (1);
    }
  }
  /*Name has been found*/
  else if(propp->name == VCP_N){
    int check = match_NAME(propp->value);
    if (check == 3){
      return(3);
    }
    else if(check == 1){
      return(2);
    }
    else if (check == 4){
      return (1);
    }
  }
  /*ADR has been found*/
  else if(propp->name == VCP_ADR){
    int check = match_ADR(propp->value);
    if (check == 3){
      return(3);
    }
    else if(check == 1){
      return(2);
    }
    else if (check == 4){
      return (1);
    }
  }
  /*TEL has been found*/
  else if(propp->name == VCP_TEL){
    int check = match_TEL(propp->value);
    if (check == 3){
      return(3);
    }
    else if(check == 1){
      return(2);
    }
    else if (check == 4){
      return (1);
    }
  }
  else{     
    return (0);
  }
  
  return EXIT_SUCCESS;
}

/**************
Precondition: *filep was successfully constructed by readVcFile.

Postcondition: *filep may have been modified.

For each card, send every N, ADR, TEL, and GEO property to vcfCanProp. 
Depending on the results, construct a new UID property with the flags set appropriately, and append it to the card's prop array.
***************/
int vcfCanon( VcFile *const filep ){
  VcProp propp = {};
  int foundgeo = 0, foundname = 0, foundadr = 0, foundtel = 0, canoncheck = 0, canoncheck2 = 0, canoncheck3 = 0, canoncheck4 = 0;
  char uidstring[6];
  uidstring[0] = '@';
  uidstring[5] = '@';
  uidstring[1] = '-';
  uidstring[2] = '-';
  uidstring[3] = '-';
  uidstring[4] = '-';
  uidstring[6] = '\0';
  int i = 0, j = 0;
  for (i = 0; i < filep->ncards ; i++){
    if (filep->cardp[i] != NULL){
      for (j = 0; j < filep->cardp[i]->nprops;j++){
	if(filep->cardp[i]->prop[j].name == VCP_GEO){
	  //Found GEO so send to CanProp
	  foundgeo = 1;
	  canoncheck = vcfCanProp(&filep->cardp[i]->prop[j]);
	}
	if(filep->cardp[i]->prop[j].name == VCP_N){
	  //Found Name so send to CanProp
	  foundname = 1;
	  canoncheck2 = vcfCanProp(&filep->cardp[i]->prop[j]);
	}
	if(filep->cardp[i]->prop[j].name == VCP_ADR){
	  //Found ADR so send to CanProp
	  foundadr = 1;
	  canoncheck3 = vcfCanProp(&filep->cardp[i]->prop[j]);
	}
	if(filep->cardp[i]->prop[j].name == VCP_TEL){
	  //Found TEL so send to CanProp
	  foundtel = 1;
	  canoncheck4 = vcfCanProp(&filep->cardp[i]->prop[j]);
	}
      }

      if (canoncheck == 1 && foundgeo == 1){
	//append UID G
	uidstring[4] = 'G';
      }
      else if (canoncheck == 3 && foundgeo == 0){
	//append *
	uidstring[4] = '*';
      }
      if ((canoncheck2 == 1 || canoncheck2 == 2) && foundname == 1){
	//append UID N
	uidstring[1] = 'N';
      }
      else if (canoncheck2 == 3 && foundname == 0){
	//append *
	uidstring[1] = '*';
      }
      if (canoncheck3 == 1 && foundadr == 1){
	//append UID A
	uidstring[2] = 'A';
      }
      else if (canoncheck3 == 3 && foundadr == 0){
	//append *
	uidstring[2] = '*';
      }
      if (canoncheck4 == 1 && foundtel == 1){
	//append UID T
	uidstring[3] = 'T';
      }
      else if (canoncheck4 == 3 && foundtel == 0){
	//append *
	uidstring[3] = '*';
      }
      //Append UID
      filep->cardp[i]->nprops++;
      filep->cardp[i] = realloc (filep->cardp[i], (sizeof(Vcard)) + (sizeof(VcProp) * filep->cardp[i]->nprops));
      assert (filep->cardp[i]!=NULL);
      propp.name = VCP_UID;
      //strcat(fullstring, uidstring);
      propp.value = malloc(sizeof(char) * (strlen(uidstring) + 1));
      strcpy(propp.value, uidstring);
      filep->cardp[i]->prop[filep->cardp[i]->nprops-1] = propp;
    }
  }
  
  return EXIT_SUCCESS;
}

/**************
Precondition: *filep was successfully constructed by readVcFile. which is a string that indicates the property(ies) to be selected for output.

Postcondition: *filep may have been modified, even having 0 cards, and the storage for any non-selected components has been freed.

vcfSelect is responsible for confirming whether selecting *which components would result in zero cards, and printing a message on stderr if this is so.
***************/
int vcfSelect( VcFile *const filep, const char *which ){
  
  int checkforphoto = 0, checkforurl = 0, checkforgeo = 0;
  int photofound = 0, urlfound = 0, geofound = 0;
  static int found_cards = 0;
  
  /*check which arguments have been entered
  run through the files and check for the properties and flip on another flag that it has been found
  if the argument is on but the flag has not been found when you are going through free that file p 
    */

  int i = 0, j = 0, h = 0;
  /*Check which arguments the user wishes to find*/
  for (i = 0; i < strlen(which); i++){
    if (which[i] == 'p'){
      checkforphoto = 1;
    }
    else if (which[i] == 'u'){
      checkforurl = 1;
    }
    else if (which[i] == 'g'){
      checkforgeo = 1;
    }
  }
  
  /*Scan through the cards looking for the properties, and free cards if they do not contain the properties the user is looking for*/
  for (i = 0; i < filep->ncards ; i++){
    if (filep->cardp[i] != NULL){
      for (j = 0; j < filep->cardp[i]->nprops;j++){
	if(filep->cardp[i]->prop[j].name == VCP_PHOTO){
	  photofound = 1;
	}  
	if(filep->cardp[i]->prop[j].name == VCP_GEO){
	  geofound = 1;
	}
	if(filep->cardp[i]->prop[j].name == VCP_URL){
	  urlfound = 1;
	}  
      }   
      if ((checkforphoto == 1 && photofound == 0) || (checkforurl == 1 && urlfound == 0) || (checkforgeo == 1 && geofound == 0)){
	  found_cards = 1;
	  //free (filep->cardp[i]);
	  for (h = 0; h < filep->ncards ; h++){
	    filep->cardp[h] = filep->cardp[h + 1];
	  }
	  filep->ncards = filep->ncards - 1;
	  filep->cardp = realloc (filep->cardp, sizeof(Vcard *) * filep->ncards);
	  assert (filep->cardp != NULL);
	  i--;

      }
    }
    photofound = 0;
    geofound = 0;
    urlfound = 0;
  }
  
  if (filep->ncards <= 0){
      fprintf(stderr, "No cards selected.\n");
      filep->ncards = 0;
  }
  
  return EXIT_SUCCESS; 
}

/**************
Function called from qsort to compare the names in a Vcard and sort them accordingly
***************/
int comparator ( const void * elem1, const void * elem2 ){
  
  const Vcard *card1;
  const Vcard *card2;
  int i = 0;
  int firstcard = 0, secondcard = 0;
  
  /*Find the place of the 1st name and the second name, and then use those in the strcasecmp value that you return*/
  card1 = *(Vcard *const *)elem1;
  card2 = *(Vcard *const *)elem2;
  
  for (i = 0; i < card1->nprops;i++){
    if(card1->prop[i].name == VCP_N){
      firstcard = i;
      break;
    }
  }
  
  for (i = 0; i < card2->nprops;i++){
    if(card2->prop[i].name == VCP_N){
      secondcard = i;
      break;
    }
  }
  
  /*if (firstcard == 0 && secondcard == 0){
    return (strcasecmp("", ""));
  }*/
  /*else if (firstcard != 0 && secondcard == 0){
    return (strcasecmp(card1->prop[firstcard].value, ""));
  }
  else if(firstcard ==0 && secondcard != 0){
    return (strcasecmp("", card2->prop[secondcard].value));
  }
  else{*/
    return (strcasecmp(card1->prop[firstcard].value, card2->prop[secondcard].value));
  //}
}

/**************
Precondition: *filep was successfully constructed by readVcFile.

Postcondition: *filep may have been modified, and its cards are in ascending alphabetical order (case insensitive) by family name and given name.

Use the qsort and the created comparator function to put the cards in alphabetical order
***************/
int vcfSort( VcFile *const filep ){
  qsort (filep->cardp, filep->ncards, sizeof(char *), (void *)comparator);
  return EXIT_SUCCESS;
}

/**************
Main for vcftool to handle command line arguments and return an error if arguments are invalid
***************/
int main(int argc, char *argv[]){
  
  VcFile filep;
  VcStatus statuscheck;
  
  /*Check for more than 1 arg value*/
  if (argc > 3){
    printf("NO");
    return EXIT_FAILURE;
  }
  
  /*Call appropriate module and check for any errors*/
  if (strcmp(argv[1], "-info") == 0)
  {
    statuscheck = readVcFile(stdin, &filep);
    vcfInfo(stdout, &filep);
    if (statuscheck.code != OK){
      return EXIT_FAILURE;
    }
  }
  else if(strcmp(argv[1], "-canon") == 0){
      statuscheck = readVcFile(stdin, &filep);
      vcfCanon(&filep);
      writeVcFile(stdout,&filep); 

    /*else{
      fprintf(stderr, "%d" ,statuscheck.code); 
      return EXIT_FAILURE;
    }*/
  }
  else if(strcmp(argv[1], "-select") == 0){
    if (argc < 3){
      fprintf(stderr, "No cards selected.\n");
      return EXIT_FAILURE;
    }
    if (argv[2] == NULL){
      return EXIT_FAILURE;
    }
    statuscheck = readVcFile(stdin, &filep);
    int check = vcfSelect(&filep, argv[2]);
    
    if (check == EXIT_FAILURE){
      fprintf(stderr, "No cards selected.");
    }
    
    writeVcFile(stdout,&filep);
    /*
    else{
      fprintf(stderr, "%d", statuscheck.code);
      return EXIT_FAILURE;
    }*/
  }
  else if(strcmp(argv[1], "-sort") == 0){
    statuscheck = readVcFile(stdin, &filep);
      vcfSort(&filep);
      writeVcFile(stdout,&filep);
      /*
    else{
      fprintf(stderr, "%d" ,statuscheck.code);
      return EXIT_FAILURE;
    }*/
  }
  else if(strcmp(argv[1], "-write") == 0){
    statuscheck = readVcFile(stdin, &filep);
      writeVcFile(stdout,&filep);

   /*else{
      fprintf(stderr, "%d" ,statuscheck.code);
      return EXIT_FAILURE;
    }*/
  }
  else{
    fprintf(stderr, "Syntax Wrong, Expected something like: ");
    fprintf(stderr, "./vcftool -info < samples-10.vcf\n");
    fprintf(stderr, "Other commands: -sort -select -canon\n");
    //fprintf(stderr, "%d", EXIT_FAILURE);
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}