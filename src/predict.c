#include <stdio.h>   /* gets */
#include <stdlib.h>  /* atoi, malloc */
#include <string.h>  /* strcpy */
#include <zlib.h>  
#include <assert.h>
#include "uthash.h"
#include "kseq.h"
#include "common.h"
KSEQ_INIT(gzFile, gzread)  


static const char *pcPgmName="predict.c";
static struct kmer_uthash *KMER_HT = NULL;
static struct fasta_uthash *FASTA_HT = NULL;

/*
 *	the MPM (Maxmum Prefix Match) structure 
 */
struct MPM
{
	char* READ_NAME;
	char* EXON_NAME;	
	int READ_POS;
	int EXON_POS;
	int LENGTH;
};

struct kmer_uthash *find_kmer(char* quary_kmer) {
    struct kmer_uthash *s;
    HASH_FIND_STR(KMER_HT, quary_kmer, s);  /* s: output pointer */
    return s;
}

struct kmer_uthash *load_kmer_htable(char *fname){
	/* load kmer_htable*/
	struct kmer_uthash *htable = NULL;
	gzFile fp;  
	kseq_t *seq;  
	int l;
	fp = gzopen(fname, "r");
	if (fp == NULL) {
	  fprintf(stderr, "Can't open input file %s!\n", fname);
	  exit(1);
	}
	seq = kseq_init(fp); // STEP 3: initialize seq  
	if (seq == NULL){
		return NULL;
	}
	while ((l = kseq_read(seq)) >= 0) { // STEP 4: read sequence 
		/* add kmer */
		char *kmer = seq->name.s;
		int k = strlen(kmer);
		struct kmer_uthash *s;
		s = (struct kmer_uthash*)malloc(sizeof(struct kmer_uthash));
		strncpy(s->kmer, kmer, k);
		s->kmer[k] = '\0'; /* just in case*/
		
		/* add count */
		int count = atoi(seq->comment.s);
		s->count = count;
		
		/* add pos */
		char *pos = seq->seq.s;				
		s->pos = malloc((s->count) * sizeof(char*));
		/* split a string by delim */
		char *token;	    
		/* get the first token */
	    token = strtok(pos, "|");				
		/* walk through other tokens */
		int i = 0;
	    while(token != NULL) 
	    {
			s->pos[i] = malloc((strlen(token)+1) * sizeof(char));
			/*duplicate a string*/
			s->pos[i] = strdup(token);
			token = strtok(NULL, "|");
			i ++;
	    }		
		HASH_ADD_STR(htable, kmer, s);
	}	
	gzclose(fp); // STEP 6: close the file handler  
	kseq_destroy(seq);
	return(htable);
}

struct fasta_uthash *fasta_uthash_load(char *fname){
	gzFile fp;
	kseq_t *seq;
	int l;
	struct fasta_uthash *res = NULL;
	fp = gzopen(fname, "r");
	if(fp == NULL){
		return NULL;
	}
	seq = kseq_init(fp);
	while ((l = kseq_read(seq)) >= 0) {
		struct fasta_uthash *s;
		s = (struct fasta_uthash*)malloc(sizeof(struct fasta_uthash));
		/* if we kseq_destroy(seq);, we need to duplicate the string!!!*/
		s->name = strdup(seq->name.s);
		s->seq = strdup(strToUpper(seq->seq.s));
		if(seq->comment.l) s->comment = seq->comment.s;
		HASH_ADD_STR(res, name, s);
	}	
	kseq_destroy(seq);
	gzclose(fp);
	return res;
}

char* 
find_mpm(char *_read, int *pos_read, int *pos_exon, int *mpm_len, int k){
	/* copy a part of string */
	char buff[k];
	strncpy(buff, _read + *pos_read, k);
	buff[k] = '\0';
	
	//if(buff == NULL || strlen(buff) != k){
	//	*pos_read += 1; // move the pointer one base forward
	//	return NULL;
	//}
	struct kmer_uthash *s_kmer;
	HASH_FIND_STR(KMER_HT, buff, s_kmer);
	if(s_kmer==NULL){ // kmer does not exist in the hash table
		*pos_read += 1;
		return NULL;
	}

	//int _pos_exon; /* tmp */
	//char *max_exon;
	//int *max_len_list = malloc(s_kmer->count * sizeof(int));
	//int max_len = 0;
	//	
	///* discard current kmer if it matches too many MPM */
	//for(int i =0; i < s_kmer->count; i++){
	//	char *tmp = strdup(s_kmer->pos[i]);
	//	char *exon = pos_parser(tmp, &_pos_exon);
	//	/* error report*/
	//	if(exon==NULL || _pos_exon==NULL || _pos_exon<0){
	//		*pos_read += 1;
	//		return NULL;
	//	}
	//	struct fasta_uthash *s_fasta;
	//	HASH_FIND_STR(*fasta_ht, exon, s_fasta);
	//	if(s_fasta == NULL){
	//		* pos_read += 1;
	//		return NULL;
	//	}
	//	char *_seq = strdup(s_fasta->seq);
	//	int m = 0;
	//	while(*(_seq + m + _pos_exon) == *(_read+ *pos_read + m)){
	//		m ++;
	//	}
	//	max_len_list[i] = m;
	//	if(m > max_len){
	//		max_len = m;
	//		max_exon = strdup(exon);
	//		*pos_exon = _pos_exon;
	//	}
	//	free(exon);
	//	free(tmp);				
	//}
	//
	//int max_count = 0; // count how many MPM found
	//for(int i=0; i < s_kmer->count; i++)
	//	if(max_len == max_len_list[i])
	//		max_count ++;
	//if(max_count > 1){
	//	return NULL;
	//}else{
	//	*pos_read += max_len;
	//	*mpm_len = max_len;
	//	return max_exon;		
	//}
	return NULL;
}


int 
find_MPMs_on_read(struct MPM *_qptr, char* _read, char* _read_name, int _k){
	int _read_pos = 0;
	int _exon_pos;
	int _i = 0; 
	int _mpm_len;
	while(_read_pos<(strlen(_read)-_k)){
		char* _exon = find_mpm(_read, &_read_pos, &_exon_pos, &_mpm_len, _k);
		if (_exon!=NULL){
			printf("%s\n", _exon);
		//	_qptr[_i].READ_NAME = strdup(_read_name);
		//	_qptr[_i].READ_POS = _read_pos - _mpm_len;
		//	_qptr[_i].EXON_NAME = strdup(_exon);
		//	_qptr[_i].EXON_POS = _exon_pos;
		//	_qptr[_i].LENGTH = _mpm_len;
		//	_i++;	
		}
		//free(_exon);
	}
	return _i;
}

char* 
construct_BAG(char *_fastq_file, int _k){	
	gzFile fp;
	kseq_t *seq;
	int l;
	fp = gzopen(_fastq_file, "r");
	seq = kseq_init(fp);
	while ((l = kseq_read(seq)) >= 0) {
		char *_read = strdup(seq->seq.s);
		char * _read_name = strdup(seq->name.s);
		if(_read == NULL || _read_name == NULL)
			return NULL; 
		struct MPM *qptr = calloc(100, sizeof(struct MPM));
		if(qptr == NULL)
			return NULL;
		int num = find_MPMs_on_read(qptr, _read, _read_name, _k);
		free(qptr);
	}
	return NULL;
}

int predict_main(char *fasta_file, char *fastq_file, int k){
	/* MAX_K is defined in common.h */
	if(k > MAX_K){
		fprintf(stderr, "input k(%d) greater than allowed lenght - 100\n", k);
		exit(-1);		
	}			
	/* load kmer hash table in the memory */
	assert(fastq_file != NULL);
	assert(fasta_file != NULL);
	
	char *index_file = concat(fasta_file, ".index");
	if(index_file == NULL)
		return -1;
	/* load kmer_uthash table */
	KMER_HT = load_kmer_htable(index_file);	
	if(KMER_HT == NULL){
		fprintf(stderr, "Fail to load kmer_uthash table\n");
		exit(-1);		
	}	
	/* load fasta_uthash table */
	FASTA_HT = fasta_uthash_load(fasta_file);
	if(FASTA_HT == NULL){
		fprintf(stderr, "Fail to load fasta_uthash table\n");
		exit(-1);		
	}
	
	construct_BAG(fastq_file, k);
	
	/* starting parsing and processing fastq file*/
	//gzFile fp;
	//kseq_t *seq;
	//int l;
	//fp = gzopen(fastq_file, "r");
	//seq = kseq_init(fp);
	//while ((l = kseq_read(seq)) >= 0) {
	//	char *_read = strdup(seq->seq.s);
	//	char * _read_name = strdup(seq->name.s);
	//	struct MPM *qptr = calloc(100, sizeof(struct MPM));
	//	int num = find_MPMs_on_read(qptr, _read, _read_name, k, &kmer_ht, &fasta_ht);		
    //
	//	if(num > 0){
	//		printf("num=%d\n", num);
	//		for(int i=0; i < num; i++){
	//			printf("%s\t%d\t%s\t%d\t%d\n", qptr[i].READ_NAME, qptr[i].READ_POS, qptr[i].EXON_NAME, qptr[i].EXON_POS, qptr[i].LENGTH);
	//			for(int j=0; j<qptr[i].LENGTH; j++){
	//				struct fasta_uthash *s;
	//				HASH_FIND_STR(fasta_ht, qptr[i].EXON_NAME, s);
	//				if(s!=NULL){
	//					int m = (qptr[i].LENGTH);
	//					char tmp_fa[m+1];
	//					char tmp_re[m+1];					
	//					memset(tmp_fa, '\0', sizeof(tmp_fa));
	//					memset(tmp_re, '\0', sizeof(tmp_re));												
	//					strncpy(tmp_fa, s->seq+qptr[i].EXON_POS, m);
	//					strncpy(tmp_re, _read+qptr[i].READ_POS, m);
	//					printf("%s\t%s\n", tmp_fa, tmp_re);
	//				}
	//			}
	//		}
	//	}
	//	free(qptr);
	//}	
	//
	//kseq_destroy(seq);
	//gzclose(fp);
	kmer_uthash_destroy(KMER_HT);	
	fasta_uthash_destroy(FASTA_HT);	
	return 0;
}

