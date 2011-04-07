/**
 * \file OutputFiles.cpp
 * AUTHOR: Barbara Frewen
 * CREATE DATE: Aug 24, 2009
 * PROJECT: crux
 * \brief A class description for handling all the various
 * output files, excluding parameter and log files.
 *
 * The filenames, locations and overwrite status are taken from
 * parameter.c.
 */

#include "OutputFiles.h"
using namespace std;

/**
 * Default constructor for OutputFiles.  Opens all of the needed
 * files, naming them based on the values of the parameters output-dir
 * and fileroot and on the name given (search, percolator, etc.).
 * Requires that the output directory already exist. 
 */
OutputFiles::OutputFiles(COMMAND_T program_name)
: matches_per_spec_(get_int_parameter("top-match")),
  command_(program_name)
{

  delim_file_array_ = NULL;
  xml_file_array_ = NULL;
  sqt_file_array_ = NULL;
  feature_file_ = NULL;

  // parameters for all three file types
  bool overwrite = get_boolean_parameter("overwrite");
  const char* output_directory = get_string_parameter_pointer("output-dir");
  const char* fileroot = get_string_parameter_pointer("fileroot");
  if( strcmp(fileroot, "__NULL_STR") == 0 ){
    fileroot = NULL;
  }

  int num_decoy_files = get_int_parameter("num-decoy-files");
  num_files_ = num_decoy_files + 1; // plus target file

  // TODO (BF oct-21-09): consider moving this logic to parameter.c
  if( command_ != SEARCH_COMMAND && command_ != SEQUEST_COMMAND ){
    num_files_ = 1;
  }

  makeTargetDecoyList();

  carp(CARP_DEBUG, 
       "OutputFiles is opening %d files (%d decoys) in '%s' with root '%s'."
       " Overwrite: %d.", 
       num_files_, num_decoy_files, output_directory, fileroot, overwrite);

  // all operations create tab files
  createFiles(&delim_file_array_, 
              output_directory, 
              fileroot, 
              command_, 
              "txt");

  // almost all operations create xml files
  if( command_ != SPECTRAL_COUNTS_COMMAND ){
    createFiles(&xml_file_array_,
                output_directory,
                fileroot,
                command_,
                "pep.xml",
                overwrite);
  }
  
  // only sequest creates sqt files
  if( command_ == SEQUEST_COMMAND ){
    createFiles(&sqt_file_array_, 
                 output_directory, 
                 fileroot, 
                 command_, 
                 "sqt", 
                 overwrite);
  }

  // only percolator and q-ranker create feature files
  if( (command_ == PERCOLATOR_COMMAND 
       || command_ == QRANKER_COMMAND)
      && get_boolean_parameter("feature-file") ){
    string filename = makeFileName(fileroot, command_, 
                                   NULL, // not target or decoy
                                   "features.txt");
    createFile(&feature_file_, 
               output_directory, 
               filename.c_str(), 
               overwrite);
  }

}

OutputFiles::~OutputFiles(){
  for(int file_idx = 0; file_idx < num_files_; file_idx ++){
    if( delim_file_array_ ){ delete delim_file_array_[file_idx]; }
    if( sqt_file_array_ ){ fclose(sqt_file_array_[file_idx]); }
    if( xml_file_array_ ){ fclose(xml_file_array_[file_idx]); }
  }
  if( feature_file_ ){ fclose(feature_file_); }

  delete [] delim_file_array_;
  delete [] sqt_file_array_;
  delete [] xml_file_array_;
  delete [] target_decoy_list_;
}

/**
 * Creates an array of num_files_ strings with the target or decoy
 * tag that the file in that position should have.  The first string
 * will always be "target", the second will be "decoy" (iff num_files_
 * = 2) or "decoy-1", the third "decoy-2" and so on.
 */
void OutputFiles::makeTargetDecoyList(){
  target_decoy_list_ = new string[num_files_];
  target_decoy_list_[0] = "target";
  if( num_files_ == 2 ){
    target_decoy_list_[1] = "decoy";
  }else{
    for(int file_idx = 1; file_idx < num_files_; file_idx++){
      ostringstream name_builder;
      name_builder << "decoy-" << file_idx;
      target_decoy_list_[file_idx] = name_builder.str();
    }
  }
}

/**
 * \returns A string with all of the parts of the filename
 * concatenated together as
 * directory/fileroot.command-name.[target|decoy]extension.  Assumes
 * that extension includes a ".".  Either fileroot and/or target_decoy
 * may be NULL. Directory argument is optional.
 */
string OutputFiles::makeFileName(const char* fileroot,
                                 COMMAND_T command,
                                 const char* target_decoy,
                                 const char* extension,
                                 const char* directory ){

  // get command name
  const char* basename = command_type_to_file_string_ptr(command);

  ostringstream name_builder;
  if( directory ){
    name_builder << directory;
    if( directory[strlen(directory) - 1] != '/' ){
      name_builder << "/";
    }
  }
  if( fileroot ){
    name_builder << fileroot << ".";
  }
  name_builder << basename << "." ;
  if( target_decoy != NULL && target_decoy[0] != '\0' ){
    name_builder << target_decoy << ".";
  }
  name_builder << extension;
  string filename = name_builder.str();

  return filename;
}

/**
 * A private function for generating target and decoy files named
 * according to the given arguments.
 *
 * New files are returned via the file_array_ptr argument.  When
 * num_files > 1, exactly one target file is created and the remaining
 * are decoys.  Files are named 
 * "output-dir/fileroot.command_name.target|decoy[-n].extension".
 * Requires that the output-dir already exist and have write
 * permissions. 
 * \returns TRUE if num_files new files are created, else FALSE.
 */
bool OutputFiles::createFiles(FILE*** file_array_ptr,
                              const char* output_dir,
                              const char* fileroot,
                              COMMAND_T command,
                              const char* extension,
                              bool overwrite){
  if( num_files_ == 0 ){
    return FALSE;
  }
  
  // allocate array
  *file_array_ptr = new FILE*[num_files_];

  // create each file
  for(int file_idx = 0; file_idx < num_files_; file_idx++ ){
    string filename = makeFileName( fileroot, command,
                                    target_decoy_list_[file_idx].c_str(),
                                    extension);
    createFile(&(*file_array_ptr)[file_idx], 
               output_dir, 
               filename.c_str(), 
               overwrite);

  }// next file
  
  return TRUE;
}

/**
 * A private function for generating target and decoy MatchFileWriters named
 * according to the given arguments.
 *
 * MatchFileWriters are returned via the file_array_ptr argument.  When
 * num_files > 1, exactly one target file is created and the remaining
 * are decoys.  Files are named 
 * "output-dir/fileroot.command_name.target|decoy[-n].extension".
 * Requires that the output-dir already exist and have write
 * permissions. 
 * \returns TRUE if num_files new MatchFileWriters are created, else FALSE.
 */
bool OutputFiles::createFiles(MatchFileWriter*** file_array_ptr,
                              const char* output_dir,
                              const char* fileroot,
                              COMMAND_T command,
                              const char* extension ){
  if( num_files_ == 0 ){
    return FALSE;
  }
  
  // allocate array
  *file_array_ptr = new MatchFileWriter*[num_files_];

  // create each file writer
  for(int file_idx = 0; file_idx < num_files_; file_idx++ ){
    string filename = makeFileName(fileroot, command,
                                   target_decoy_list_[file_idx].c_str(),
                                   extension, output_dir);
    (*file_array_ptr)[file_idx] = new MatchFileWriter(filename.c_str());
  }
  
  return TRUE;
}

/**
 * \brief A private function for opening a file according to the given
 * arguments.
 *
 * New file is returned via the file_ptr argument.  File is named
 * output-dir/fileroot.comand_name[target_decoy].extension.  Requires that the
 * output-dir already exist and have write permissions.
 * \returns TRUE if the file is created, else FALSE.
 */
bool OutputFiles::createFile(FILE** file_ptr,
                             const char* output_dir,
                             const char* filename,
                             bool overwrite){

  // open the file
  *file_ptr = create_file_in_path(filename,
                                  output_dir,
                                  overwrite);

  if( *file_ptr == NULL ){ return FALSE; }

  return TRUE;
}

/**
 * \brief Write header lines to the .txt, .sqt files, and .pep.xml
 * files.  Optional num_proteins argument for .sqt files.  Use this
 * for search commands, not post-search.
 */
void OutputFiles::writeHeaders(int num_proteins){

  const char* tag = "target";

  // write headers one file at a time for tab and sqt
  for(int file_idx = 0; file_idx < num_files_; file_idx++){
    if( delim_file_array_ ){
        delim_file_array_[file_idx]->addColumnNames(command_, (bool)file_idx);
        delim_file_array_[file_idx]->writeHeader();
    }

    if( sqt_file_array_ ){
      print_sqt_header(sqt_file_array_[file_idx],
                       tag,
                       num_proteins, FALSE); // not post search
    }
    
    if ( xml_file_array_){
      print_xml_header(xml_file_array_[file_idx]);
    }

    tag = "decoy";
  }
}

/**
 * \brief Write header lines to the .txt and .pep.xml
 * files.  Use this for post-search commands, not search.
 */
void OutputFiles::writeHeaders(const vector<bool>& add_this_col){

  const char* tag = "target";

  // write headers one file at a time for tab and sqt
  for(int file_idx = 0; file_idx < num_files_; file_idx++){
    if( delim_file_array_ ){
        delim_file_array_[file_idx]->addColumnNames(command_, 
                                                    (bool)file_idx, 
                                                    add_this_col);
        delim_file_array_[file_idx]->writeHeader();
    }

    if ( xml_file_array_){
      print_xml_header(xml_file_array_[file_idx]);
    }

    tag = "decoy";
  }
}

/**
 * \brief Write header lines to the optional feature file.
 */
void OutputFiles::writeFeatureHeader(char** feature_names,
                                     int num_names){
  // write feature file header
  if( feature_names && feature_file_ && num_names ){
    fprintf(feature_file_, "scan\tlabel");
    for(int name_idx = 0; name_idx < num_names; name_idx++){
      fprintf(feature_file_, "\t%s", feature_names[name_idx]);
    }
    fprintf(feature_file_, "\n");
  }
}


/**
 * \brief Write footer lines to xml files
 */
void OutputFiles::writeFooters(){
  if (xml_file_array_){
    for (int file_idx = 0; file_idx < num_files_; file_idx++){
      print_xml_footer(xml_file_array_[file_idx]);
    }
  }

}

/**
 * \brief Write the given matches to appropriate output files.  Limit
 * the number of matches per spectrum based on top-match parameter
 * using the ranks from rank_type.  
 */
void OutputFiles::writeMatches(
  MATCH_COLLECTION_T*  target_matches, ///< from real peptides
  vector<MATCH_COLLECTION_T*>& decoy_matches_array,  
                                ///< array of collections from shuffled peptides
  SCORER_TYPE_T rank_type,      ///< use ranks for this type
  Spectrum* spectrum            ///< given when all matches are to one spec
  ){

  if( target_matches == NULL ){
    return;  // warn?
  }

  // confirm that there are the expected number of decoy collections
  if( (int)decoy_matches_array.size() != num_files_ - 1){
    carp(CARP_FATAL, 
         "WriteMatches was given %d decoy collections but was expecting %d.",
         (int)decoy_matches_array.size(), num_files_ - 1);
  }

  // print to each file type
  printMatchesTab(target_matches, decoy_matches_array, rank_type, spectrum);
  
  printMatchesSqt(target_matches, decoy_matches_array, spectrum);

  printMatchesXml(target_matches, decoy_matches_array, spectrum, rank_type);

}

// already confirmed that num_files_ = num decoy collections + 1
void OutputFiles::printMatchesTab(
  MATCH_COLLECTION_T*  target_matches, ///< from real peptides
  vector<MATCH_COLLECTION_T*>& decoy_matches_array,  
  SCORER_TYPE_T rank_type,
  Spectrum* spectrum
){

  carp(CARP_DETAILED_DEBUG, "Writing tab delimited results.");

  if( delim_file_array_ == NULL ){
    return;
  }

  // if a spectrum is given, use one print function
  if( spectrum ){
    MATCH_COLLECTION_T* cur_matches = target_matches;

    for(int file_idx = 0; file_idx < num_files_; file_idx++){

      print_match_collection_tab_delimited(delim_file_array_[file_idx],
                                           matches_per_spec_,
                                           cur_matches,
                                           spectrum,
                                           rank_type);

      carp(CARP_DETAILED_DEBUG, "done writing file index %d", file_idx);
      if( decoy_matches_array.size() > (size_t)file_idx ){
        cur_matches = decoy_matches_array[file_idx];
      }// else if it is NULL, num_files_ == 1 and loop will exit here
    }

  } else { // use the multi-spectra print function which assumes
           // targets and decoys are merged
    print_matches_multi_spectra(target_matches,
                                delim_file_array_[0],
                                (num_files_ > 1) ? delim_file_array_[1] : NULL);
  }

}

void OutputFiles::printMatchesSqt(
  MATCH_COLLECTION_T*  target_matches, ///< from real peptides
  vector<MATCH_COLLECTION_T*>& decoy_matches_array,  
                                ///< array of collections from shuffled peptides
  Spectrum* spectrum
){

  if( sqt_file_array_ == NULL ){
    return;
  }

  MATCH_COLLECTION_T* cur_matches = target_matches;

  for(int file_idx = 0; file_idx < num_files_; file_idx++){

    print_match_collection_sqt(sqt_file_array_[file_idx],
                               matches_per_spec_,
                               cur_matches,
                               spectrum);

    if( decoy_matches_array.size() > (size_t)file_idx ){
      cur_matches = decoy_matches_array[file_idx];
    } // else if NULL, num_files_==1 and this is last loop
  }

}


void OutputFiles::printMatchesXml(
  MATCH_COLLECTION_T*  target_matches, ///< from real peptides
  vector<MATCH_COLLECTION_T*>& decoy_matches_array,  
                                ///< array of collections from shuffled peptides
  Spectrum* spectrum,
  SCORER_TYPE_T rank_type
  
){
  
  static int index = 1;
  if( xml_file_array_ == NULL ){
    return;
  }

  MATCH_COLLECTION_T* cur_matches = target_matches;

  for(int file_idx = 0; file_idx < num_files_; file_idx++){

    print_match_collection_xml(xml_file_array_[file_idx],
                               matches_per_spec_,
                               cur_matches,
                               spectrum,
                               rank_type,
                               index);

    if( decoy_matches_array.size() > (size_t)file_idx ){
      cur_matches = decoy_matches_array[file_idx];
    } // else if NULL, num_files_==1 and this is last loop
  }
  index++;

}

void OutputFiles::writeMatches(
  MATCH_COLLECTION_T*  matches ///< from multiple spectra
){
  print_matches_multi_spectra(matches, 
                              delim_file_array_[0],
                              NULL);// no decoy file
  print_matches_multi_spectra_xml(matches,
                                  xml_file_array_[0]);
}

/**
 * \brief Print features from one match to file.
 */
void OutputFiles::writeMatchFeatures(
   MATCH_T* match, ///< match to provide scan num, decoy
   double* features,///< features for this match
   int num_features)///< size of features array
{
  if( feature_file_ == NULL ){ return; }

  // write scan number
  fprintf(feature_file_, "%i\t",
          (get_match_spectrum(match))->getFirstScan() );

  // decoy or target peptide
  if (get_match_null_peptide(match) == FALSE){
    fprintf(feature_file_, "1\t");
  } else { 
    fprintf(feature_file_, "-1\t");
  };
  
  // print each feature, end in new-line
  for(int feature_idx = 0; feature_idx < num_features; feature_idx++){
    if (feature_idx < num_features - 1){
      fprintf(feature_file_, "%.4f\t", features[feature_idx]);
    } else {
      fprintf(feature_file_, "%.4f\n", features[feature_idx]);
    }
  }

}

/**
 * Print the given peptides and their scores in sorted order by score.
 */
void OutputFiles::writeRankedPeptides(PeptideToScore& peptideToScore){

  // rearrange pairs to sort by score
  vector<pair<FLOAT_T, PEPTIDE_T*> > scoreToPeptide;
  for(PeptideToScore::iterator it = peptideToScore.begin();
       it != peptideToScore.end(); ++it){
    PEPTIDE_T* peptide = it->first;
    FLOAT_T score = it->second;
    scoreToPeptide.push_back(make_pair(score, peptide));
  }
  
  // sort by score
  sort(scoreToPeptide.begin(), scoreToPeptide.end());
  reverse(scoreToPeptide.begin(), scoreToPeptide.end());

  MatchFileWriter* file = delim_file_array_[0];
  MATCH_COLUMNS_T score_col = SIN_SCORE_COL;
  if( get_measure_type_parameter("measure") == MEASURE_NSAF ){
    score_col = NSAF_SCORE_COL;
  }
  // print each pair
  for(vector<pair<FLOAT_T, PEPTIDE_T*> >::iterator it = scoreToPeptide.begin();
      it != scoreToPeptide.end(); ++it){
    PEPTIDE_T* peptide = it->second;
    FLOAT_T score = it->first;
    char* seq = get_peptide_sequence(peptide);

    file->setColumnCurrentRow(SEQUENCE_COL, seq);
    file->setColumnCurrentRow(score_col, score);
    file->writeRow();
    free(seq);
  }

}


/**
 * Print all of the proteins and their associated scores in sorted
 * order by score. If there is parsimony information, also print the
 * parsimony rank.
 */
void OutputFiles::writeRankedProteins(ProteinToScore& proteinToScore,
                                      MetaToRank& metaToRank,
                                      ProteinToMetaProtein& proteinToMeta){

  bool isParsimony = (proteinToMeta.size() != 0);

  // reorganize the protein,score pairs to sort by score
  vector<pair<FLOAT_T, Protein*> > scoreToProtein;
  for (ProteinToScore::iterator it = proteinToScore.begin(); 
       it != proteinToScore.end(); ++it){
    Protein* protein = it->first;
    FLOAT_T score = it->second;
    scoreToProtein.push_back(make_pair(score, protein));
  }
  
  // sort and reverse the list
  sort(scoreToProtein.begin(), scoreToProtein.end());
  reverse(scoreToProtein.begin(), scoreToProtein.end());

  MatchFileWriter* file = delim_file_array_[0];
  MATCH_COLUMNS_T score_col = SIN_SCORE_COL;
  if( get_measure_type_parameter("measure") == MEASURE_NSAF ){
    score_col = NSAF_SCORE_COL;
  } else if( get_measure_type_parameter("measure") == MEASURE_EMPAI ){
    score_col = EMPAI_SCORE_COL;
  }

  // print each protein
  for(vector<pair<FLOAT_T, Protein*> >::iterator it = scoreToProtein.begin(); 
      it != scoreToProtein.end(); ++it){
    FLOAT_T score = it->first;
    Protein* protein = it->second;

    file->setColumnCurrentRow(PROTEIN_ID_COL, protein->getId());
    file->setColumnCurrentRow(score_col, score);

    if (isParsimony){
      MetaProtein metaProtein = proteinToMeta[protein];
      int rank = -1;
      if (metaToRank.find(metaProtein) != metaToRank.end()){
	rank = metaToRank[metaProtein];
      } 
      file->setColumnCurrentRow(PARSIMONY_RANK_COL, rank);
    }
    file->writeRow();
  }
}








/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 2
 * End:
 */
