/*--------------------------------------------------------------------*/
/* predict.c                                                          */
/* Author: Rongxin Fang                                               */
/* E-mail: r3fang@ucsd.edu                                            */
/* Predict Gene Fusion by given fastq files.                          */
/*--------------------------------------------------------------------*/
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h> 
#include <zlib.h>  
#include <assert.h>
#include <math.h>
#include "kseq.h"
#include "common.h"
#include "uthash.h"
#include "kmer_uthash.h"
#include "BAG_uthash.h"
#include "fasta_uthash.h"
#include "utils.h"

/* error code */
#define PR_ERR_NONE		     	 0 // no error
#define PR_ERR_PARAM			-1 // bad paraemter
#define PR_ERR_MALLOC			-2 // fail to malloc memory uthash
#define PR_ERR_POSPARSE			-3
#define PR_ERR_FINDHASH			-4
#define PR_ERR_UNDEFINED		-100 // fail to malloc memory uthash
#define MAX_READ_LEN			500
/*--------------------------------------------------------------------*/
/*Global paramters.*/
static const char *pcPgmName			="predict.c";
static struct kmer_uthash *KMER_HT  	= NULL;
static struct fasta_uthash *FASTA_HT 	= NULL;
static struct BAG_uthash *BAG_HT 		= NULL;

/*--------------------------------------------------------------------*/

/* Find next Maximal Extended Kmer Match (MEKM) on _read at pos_read. */
int
find_next_MEKM(char **exon, char *_read, int pos_read, int k, int min_match){
	/*------------------------------------------------------------*/
	// parameters decleration
	typedef struct freq{
		int LEN;
		size_t SIZE;
	    UT_hash_handle hh;         /* makes this structure hashable */
	} freq; 
	freq* match_lens = NULL;
	char* buff = NULL;	
	char **matches = NULL;
	int i = 0;
	int num = 0;
	char *exon_tmp = NULL;
	char *exon_max = NULL;
	int max_len = -10;
	freq *s_freq = NULL;
	freq *tmp = NULL;
	int error;
	/*------------------------------------------------------------*/
	/* check parameters */
	//if(_read == NULL || *exon != NULL) die("find_next_MEKM: parameter error\n");
	if(_read == NULL) die("find_next_MEKM: parameter error\n");
	*exon = NULL;
	/*------------------------------------------------------------*/
	/* copy a kmer of string */
	if((buff = malloc((k+1) * sizeof(char)))==NULL) die("find_next_MEKM: malloc fails\n");
	strncpy(buff, _read + pos_read, k);
	buff[k] = '\0';	
	if(buff == NULL || strlen(buff) != k) die("find_next_MEKM: buff strncpy fails\n");
	/*------------------------------------------------------------*/
	struct kmer_uthash *s_kmer;
	if((error = find_kmer(KMER_HT, buff, &s_kmer)) != PR_ERR_NONE) die("find_next_MEKM: find_kmer fails\n");
	if(s_kmer == NULL) goto SUCCESS;
	/*------------------------------------------------------------*/
	if((matches = malloc(s_kmer->count * sizeof(char*)))==NULL) goto FAIL_MALLOC;	
	/*------------------------------------------------------------*/
	/*
	 * iterate all kmer matches and extend to max length
	 */
	for(i=0; i<s_kmer->count; i++){		
		int _pos_exon; // position on exon
		exon_tmp = pos_parser(s_kmer->pos[i], &_pos_exon);
		/* error report*/
		if(exon_tmp == NULL || _pos_exon < 0) goto FAIL_POSPARSE;
		
		struct fasta_uthash *s_fasta = NULL;
		if((error = find_fasta(FASTA_HT, exon_tmp, &s_fasta)) != PR_ERR_NONE) die("find_next_MEKM: find_fasta fails\n");
		if(s_fasta == NULL) continue;
		int m = 0;
		/* extending kmer to find MPM */
		while(*(s_fasta->seq + m + _pos_exon) == *(_read+ pos_read + m)){
			m ++;
		}
		if(m >= min_match){
			if(m>max_len){
				max_len=m;
				exon_max = strdup(exon_tmp);
			}
			HASH_FIND_INT(match_lens, &m, s_freq);	
			if(s_freq==NULL){
				if((s_freq=(freq*)malloc(sizeof(freq))) == NULL) goto FAIL_MALLOC;
				s_freq->LEN  = m;
				s_freq->SIZE = 1;				
				HASH_ADD_INT(match_lens, LEN, s_freq);
			}else{
				s_freq->SIZE++;
			}
		}
	}
	/*------------------------------------------------------------*/	
	HASH_FIND_INT(match_lens, &max_len, s_freq);	
	if(s_freq == NULL) goto NO_MATCH; 
	if(s_freq->SIZE==1){
		*exon = strdup(exon_max);
	}
	goto SUCCESS;
	
	FAIL_PARAM:
    	die("find_next_MEKM: invalid NULL parameter");
	FAIL_MALLOC:
		die("find_next_MEKM: fail to malloc memory");
	FAIL_POSPARSE:
		die("find_next_MEKM: fail to parse kmer match postion");
	FAIL_HASH_FIND:
		die("find_next_MEKM: fail to find element in uthahs");	
	FAIL_OTHER:
		die("find_next_MEKM: undefined error");	
	NO_MATCH:
		error = PR_ERR_NONE;
		goto EXIT;
		
	SUCCESS:
		error = PR_ERR_NONE;
		goto EXIT;

	EXIT:		
		if(buff) 		free(buff);
		if(matches) 	free(matches);		
		if(exon_max) 	free(exon_max);	
		if(exon_tmp) 	free(exon_tmp);	
		HASH_ITER(hh, match_lens, s_freq, tmp) {
			HASH_DEL(match_lens, s_freq);
      		free(s_freq);
		}
		return error;
}
/*--------------------------------------------------------------------*/

/* Find all Maximal Extended Kmer Matchs (MEKMs) on _read.            */
int
find_all_MEKMs(char **hits, int *num, char* _read, int _k, int min_match){
/*--------------------------------------------------------------------*/
	/* declare vaiables */
	int error;
	int _read_pos = 0;
	*num = 0; 
	char* _exon = NULL;
/*--------------------------------------------------------------------*/
	/* check parameters */
	if(_read == NULL || hits == NULL || _k < 0 || min_match < 0) goto FAIL_PARAM;
/*--------------------------------------------------------------------*/
	while(_read_pos<(strlen(_read)-_k-1)){
		if((error=find_next_MEKM(&_exon, _read, _read_pos++, _k, min_match)) < 0){
		    fprintf(stderr, "find_all_MEKMs: error=%d\n", error);  
			exit(-1);
		}
		if (_exon != NULL){
			hits[*num] = strdup(_exon);
			(*num)++;
		}
	}
	goto SUCCESS;
/*--------------------------------------------------------------------*/	
	FAIL_PARAM:
		die("find_all_MEKMs: invalid NULL parameter");
		error = PR_ERR_PARAM;
		goto EXIT;

	SUCCESS:
		error = PR_ERR_NONE;
		goto EXIT;

	EXIT:
		if(_exon) 	free(_exon);	
		return error;
}

/* Construct breakend associated graph by given fq files.             */
int
construct_BAG(char *fq_file1, char *fq_file2, int _k, int min_match, struct BAG_uthash **graph_ht){
	if(fq_file1 == NULL || fq_file2 == NULL || *graph_ht != NULL) die("construct_BAG: parameter error\n");
	int error;
	gzFile fp1, fp2;
	kseq_t *seq1, *seq2;
	int l1, l2;
	char** hits_uniq1, **hits_uniq2, **hits1, **hits2, **parts1, **parts2;
	char *_read1, *_read2, *edge_name, *gene1, *gene2;
	if((hits1 = malloc(MAX_READ_LEN * sizeof(char*))) == NULL) die("construct_BAG: malloc error\n");
	if((hits2 = malloc(MAX_READ_LEN * sizeof(char*))) == NULL) die("construct_BAG: malloc error\n");
	if((hits_uniq1 = malloc(MAX_READ_LEN * sizeof(char*)))==NULL) die("construct_BAG: malloc error\n");
	if((hits_uniq2 = malloc(MAX_READ_LEN * sizeof(char*)))==NULL) die("construct_BAG: malloc error\n");
	if((parts1 = calloc(3, sizeof(char *))) == NULL) die("construct_BAG: malloc error\n");
	if((parts2 = calloc(3, sizeof(char *))) == NULL) die("construct_BAG: malloc error\n");				
	
	fp1 = gzopen(fq_file1, "r");
	fp2 = gzopen(fq_file2, "r");
	seq1 = kseq_init(fp1);
	seq2 = kseq_init(fp2);
	
	if(fp1 == NULL || fp2 == NULL || seq1 == NULL || seq2 == NULL) die("construct_BAG: fail to read fastq files\n");
	
	while ((l1 = kseq_read(seq1)) >= 0 && (l2 = kseq_read(seq2)) >= 0 ) {
		_read1 = rev_com(seq1->seq.s);
		_read2 = seq2->seq.s;
		if(_read1 == NULL || _read2 == NULL) die("construct_BAG: fail to get _read1 and _read2\n");
		if(strcmp(seq1->name.s, seq2->name.s) != 0) die("construct_BAG: read pair not matched\n");
				
		if(strlen(_read1) < _k || strlen(_read2) < _k){
			continue;
		}
		int num1=0;
		int num2=0;
		if((error=find_all_MEKMs(hits1, &num1, _read1, _k, min_match)) != PR_ERR_NONE) die("construct_BAG: find_all_MEKMs fails\n");
		if((error=find_all_MEKMs(hits2, &num2, _read2, _k, min_match)) != PR_ERR_NONE) die("construct_BAG: find_all_MEKMs fails\n");
		printf("%d\t%d\n", num1, num2);
		size_t size1 = set_str_arr(hits1, hits_uniq1, num1);
		size_t size2 = set_str_arr(hits2, hits_uniq2, num2);
		int i, j;
		for(i=0; i<size1; i++){
			for(j=0; j<size2; j++){
				size_t len1 = strsplit_size(hits_uniq1[i], ".");
				size_t len2 = strsplit_size(hits_uniq2[j], ".");
				strsplit(hits_uniq1[i], len1, parts1, ".");  
				strsplit(hits_uniq2[j], len2, parts2, ".");  
				if(parts1[0] == NULL || parts2[0] == NULL){
					continue;
				}
				gene1 = strdup(parts1[0]);
				gene2 = strdup(parts2[0]);
				//printf("%s\t%s\n", gene1, gene2);
				int rc = strcmp(gene1, gene2);
				if(rc<0)
					edge_name = concat(concat(gene1, "_"), gene2);
				if(rc>0)
					edge_name = concat(concat(gene2, "_"), gene1);
				if(rc==0)
					edge_name = NULL;
				if(edge_name!=NULL){
					//printf("%s\t%s\n", edge_name, concat(concat(_read1, "_"), _read2));
					//if((error = BAG_uthash_add(&graph_ht, edge_name, concat(concat(_read1, "_"), _read2))) != PR_ERR_NONE)
					//	die("BAG_uthash_add fails\n");					
				}
			}
		}
	}
	if(hits1) 		free(hits1);
	if(hits2) 		free(hits2);
	if(hits_uniq1)  free(hits_uniq1);
	if(hits_uniq2)  free(hits_uniq2);
	if(parts1)		free(parts1);
	if(parts2)		free(parts2);
	if(gene1)		free(gene1);
	if(gene2)		free(gene2);
	if(edge_name)	free(edge_name);
	kseq_destroy(seq1);
	kseq_destroy(seq2);	
	gzclose(fp1);
	gzclose(fp2);
	return PR_ERR_NONE;
}

/*--------------------------------------------------------------------*/

/* main function. */
int main(int argc, char *argv[]) { 
	if (argc != 5) {  
	        fprintf(stderr, "Usage: %s <in.fa> <read_R1.fq> <read_R2.fq> <int k>\n", argv[0]);  
	        return 1;  
	 }
	char *fasta_file = argv[1];
	char *fq_file1 = argv[2];
	char *fq_file2 = argv[3];
	int min_mtch;		
	if (sscanf (argv[4], "%d", &min_mtch)!=1) die("Input error: wrong type for k\n");
	/* load kmer hash table in the memory */
	
	int error;
	///* load kmer_uthash table */
	char *index_file = concat(fasta_file, ".index");
	if(index_file == NULL) die("Fail to concate index_file\n");
	
	int k;
	if((kmer_uthash_load(index_file, &k, &KMER_HT)) != PR_ERR_NONE) die("main: kmer_uthash_load fails\n");	
	if(KMER_HT == NULL) die("Fail to load the index\n");
	if(k > MAX_K) die("input k(%d) greater than allowed lenght - 100\n", k);	
	/* load fasta_uthash table */
	if((error=fasta_uthash_load(fasta_file, &FASTA_HT)) != PR_ERR_NONE) 			   die("main: fasta_uthash_load fails\n");	
	if((error=construct_BAG(fq_file1, fq_file2, k, min_mtch, &BAG_HT)) != PR_ERR_NONE) die("main: construct_BAG fails\n");	
	if((error=kmer_uthash_destroy(&KMER_HT))   != PR_ERR_NONE) 						   die("main: kmer_uthash_destroy\n");	
	//if((error=BAG_uthash_destroy(&BAG_HT))     != PR_ERR_NONE) 						   die("main: BAG_uthash_destroy\n");	
	if((error=fasta_uthash_destroy(&FASTA_HT)) != PR_ERR_NONE) 						   die("main: fasta_uthash_destroy fails\n");		
	//if((error=fasta_uthash_display(FASTA_HT)) != PR_ERR_NONE) 			die("main: fasta_uthash_display fails\n");	
	//if((error=BAG_uthash_display(BAG_HT)) != PR_ERR_NONE) 				die("main: BAG_uthash_display fails\n");		
	return 0;
}

