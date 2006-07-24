#include <stdlib.h>
#include <check.h>
#include "check-peptide.h"

//must set bash export CK_FORK=no

int main(void){
  int nf;
  //Suite* suite_peak = peak_suite();
  Suite* suite_peptide = peptide_suite(); 
  SRunner *sr = srunner_create(NULL);
  //srunner_add_suite(sr,suite_peak);
  srunner_add_suite(sr,suite_peptide);
  srunner_run_all(sr, CK_NORMAL);
  nf = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
