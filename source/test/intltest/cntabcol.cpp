/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2001, International Business Machines Corporation and
 * others. All Rights Reserved. 
 ********************************************************************/
#include "cntabcol.h"

U_NAMESPACE_USE

ContractionTableTest::ContractionTableTest() {
  status = U_ZERO_ERROR;
  testMapping = ucmpe32_open(0, 0, 0, &status);
}

ContractionTableTest::~ContractionTableTest() {
  ucmpe32_close(testMapping);
}

void ContractionTableTest::TestGrowTable(/* char* par */) {
  uint32_t i = 0, res = 0;
  testTable = uprv_cnttab_open(testMapping, &status);

  // fill up one contraction so that it has to expand
  for(i = 0; i<65536; i++) {
    uprv_cnttab_addContraction(testTable, 0, (UChar)i, i, &status);
    if(U_FAILURE(status)) {
      errln("Error occurred at position %i, error = %i (%s)\n", i, status, u_errorName(status));
      break;
    }
  }
  // test whether the filled up contraction really contains the data we input
  if(U_SUCCESS(status)) {
    for(i = 0; i<65536; i++) {
      res = uprv_cnttab_getCE(testTable, 0, i, &status);
      if(U_FAILURE(status)) {
        errln("Error occurred at position %i, error = %i (%s)\n", i, status, u_errorName(status));
        break;
      }
      if(res != i) {
        errln("Error: expected %i, got %i\n", i, res);
        break;
      }
    }
  }
  uprv_cnttab_close(testTable);
}

void ContractionTableTest::TestSetContraction(){ 
  testTable = uprv_cnttab_open(testMapping, &status);
  // This should make a new contraction
  uprv_cnttab_setContraction(testTable, 1, 0, 0x41, 0x41, &status);
  if(U_FAILURE(status)) {
    errln("Error setting a non existing contraction error = %i (%s)\n", status, u_errorName(status));
  }
  // if we try to change the non existing offset, we should get an error
  status = U_ZERO_ERROR;
  // currently this tests whether there is enough space, maybe it should test whether the element is actually in 
  // range. Also, maybe a silent growing should take place....
  uprv_cnttab_setContraction(testTable, 1, 0x401, 0x41, 0x41, &status);
  if(status != U_INDEX_OUTOFBOUNDS_ERROR) {
    errln("changing a non-existing offset should have resulted in an error\n");
  }
  status = U_ZERO_ERROR;
  uprv_cnttab_close(testTable);
}

void ContractionTableTest::TestAddATableElement(){
  testTable = uprv_cnttab_open(testMapping, &status);
  uint32_t i = 0, res = 0;

  // fill up one contraction so that it has to expand
  for(i = 0; i<0x1000; i++) {
    uprv_cnttab_addContraction(testTable, i, (UChar)i, i, &status);
    if(U_FAILURE(status)) {
      errln("Error occurred at position %i, error = %i (%s)\n", i, status, u_errorName(status));
      break;
    }
  }
  // test whether the filled up contraction really contains the data we input
  if(U_SUCCESS(status)) {
    for(i = 0; i<0x1000; i++) {
      res = uprv_cnttab_getCE(testTable, i, 0, &status);
      if(U_FAILURE(status)) {
        errln("Error occurred at position %i, error = %i (%s)\n", i, status, u_errorName(status));
        break;
      }
      if(res != i) {
        errln("Error: expected %i, got %i\n", i, res);
        break;
      }
    }
  }
  uprv_cnttab_close(testTable);
}

void ContractionTableTest::TestClone(){
  testTable = uprv_cnttab_open(testMapping, &status);
  int32_t i = 0, res = 0;
  // we must construct table in order to copy codepoints and CEs
  // fill up one contraction so that it has to expand
  for(i = 0; i<0x500; i++) {
    uprv_cnttab_addContraction(testTable, i, (UChar)i, i, &status);
    if(U_FAILURE(status)) {
      errln("Error occurred at position %i, error = %i (%s)\n", i, status, u_errorName(status));
      break;
    }
  }
  uprv_cnttab_constructTable(testTable, 0, &status);
  if(U_FAILURE(status)) {
    errln("Error constructing table error = %i (%s)\n", status, u_errorName(status));
  } else {
    testClone = uprv_cnttab_clone(testTable);
    if(U_SUCCESS(status)) {
      for(i = 0; i<0x500; i++) {
        res = uprv_cnttab_getCE(testTable, i, 0, &status);
        if(U_FAILURE(status)) {
          errln("Error occurred at position %i, error = %i (%s)\n", i, status, u_errorName(status));
          break;
        }
        if(res != i) {
          errln("Error: expected %i, got %i\n", i, res);
          break;
        }
      }
    }
    uprv_cnttab_close(testClone);
  }
  uprv_cnttab_close(testTable);
  testTable = uprv_cnttab_open(testMapping, &status);
  if(U_FAILURE(status)) {
    errln("Error opening table error = %i (%s)\n", status, u_errorName(status));
  }
  uprv_cnttab_close(testTable);
}

void ContractionTableTest::TestChangeContraction(){
  testTable = uprv_cnttab_open(testMapping, &status);
  uint32_t i = 0, res = 0;
  res = uprv_cnttab_changeContraction(testTable, 0, 0x41, 0xAB, &status);
  if(res != 0) {
    errln("found a non existing contraction!\n");
  }

  for(i = 0; i < 0x20; i+=2) {
    uprv_cnttab_addContraction(testTable, 0, (UChar)i, i, &status);
  }

  res = uprv_cnttab_changeContraction(testTable, 0, 0x41, 0xAB, &status);
  if(res != UCOL_NOT_FOUND) {
    errln("managed to change a non existing contraction!\n");
  }

  for(i = 1; i < 0x20; i+=2) {
    res = uprv_cnttab_changeContraction(testTable, 0, (UChar)i, 0xAB, &status);
    if(res != UCOL_NOT_FOUND) {
      errln("managed to change a non existing contraction!\n");
    }
  }
  uprv_cnttab_close(testTable);
}

void ContractionTableTest::TestChangeLastCE(){
  testTable = uprv_cnttab_open(testMapping, &status);
  uint32_t res = uprv_cnttab_changeLastCE(testTable, 1, 0xABCD, &status);
  if(res!=0) {
    errln("managed to change the last CE in an non-existing contraction!\n");
  }
  uprv_cnttab_close(testTable);
}


void ContractionTableTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char* /*par*/ )
{
    if (exec) logln("TestSuite ContractionTableTest: ");
    switch (index) {
        case 0: name = "TestGrowTable";         if (exec)   TestGrowTable(/* par */); break;
        case 1: name = "TestSetContraction";    if (exec)   TestSetContraction(/* par */); break;
        case 2: name = "TestAddATableElement";  if (exec)   TestAddATableElement(/* par */); break;
        case 3: name = "TestClone";             if (exec)   TestClone(/* par */); break;
        case 4: name = "TestChangeContraction"; if (exec)   TestChangeContraction(/* par */); break;
        case 5: name = "TestChangeLastCE";      if (exec)   TestChangeLastCE(/* par */); break;
        default: name = ""; break;
    }
}


