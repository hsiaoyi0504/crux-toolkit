int search_loop(FILTERED_SPECTRUM_CHARGE_ITERATOR_T* spectrum_iterator,
		BOOLEAN_T combine_target_decoy,
		int num_peptide_mods,
		PEPTIDE_MOD_T** peptide_mods,
		DATABASE_T* database,
		INDEX_T* index,
		int sample_per_pep_mod,
		BOOLEAN_T compute_pvalues,
		FILE** psm_file_array,
		FILE* sqt_file,
		FILE* decoy_sqt_file,
		FILE* tab_file,
		FILE* decoy_tab_file,
		int num_decoys);
