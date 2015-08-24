##Get Started     
```
$ git clone https://github.com/r3fang/tfc.git
$ cd tfc
$ make
$ ./tfc predict exon.fa.gz A431-1-ABGHI_S1_L001_R1_001.fastq.gz A431-1-ABGHI_S1_L001_R2_001.fastq.gz
```

##Introduction

**TFC** is a *lightweight*, *stand-alone*, *fast*, *C-implemented*, *mapping-free* and *precise* Bioinformatics software desgined for **fusion detection** between targeted genes from RNA-seq data. It consists of two major components:
 
```
$ ./tfc 

Program: tfc (targeted fusion calling)
Version: 08.19-r15
Contact: Rongxin Fang <r3fang@ucsd.edu>

Usage:   tfc <command> [options]

Command: name2fasta     extract DNA sequences
         predict        predict gene fusions
```

- **name2fasta** (extract *exon/transcript/CDS* sequences of targeted genes)
 
```
$./tfc name2fasta

Usage:   tfc name2fasta [options] <gname.txt> <genes.gtf> <in.fa> <out.fa> 

Details: name2fasta is to extract genomic sequence of gene candiates

Options: -g               'exon' or 'transcript' or 'CDS' 

Inputs:  gname.txt        .txt file contains the names of gene candiates
         genes.gtf        .gft file contains gene annotations sorted by 5th column
         in.fa            .fa file contains the whole genome sequence e.g. [hg19.fa]
         out.fa           .fa files contains output sequences
```

- **predict** (predict fusions between targeted genes).

```
$ ./tfc predict

Usage:   tfc predict [options] <exon.fa> <R1.fq.gz> <R2.fq.gz>

Details: predict gene fusion from pair-end RNA-seq data

Options:
   -- Graph:
         -k INT    kmer length for indexing in.fa [15]
         -n INT    min unique kmer matches for a hit between gene and pair [10]
         -w INT    edges in graph of weight smaller than -w will be removed [3]
   -- Alignment:
         -m INT    score for match [2]
         -u INT    penality for mismatch[-2]
         -o INT    penality for gap open [-5]
         -e INT    penality for gap extension [-1]
         -j INT    penality for jump between genes [-10]
         -s INT    penality for jump between exons [-8]
         -a FLOAT  min identity score for alignment [0.80]
   -- Junction:
         -h INT    min hits for a junction [3]
         -l INT    length for junction string [20]
         -x INT    max mismatches allowed for junction string match [2]
   -- Fusion:
         -A INT    weight for junction containing reads [3]
         -p FLOAT  p-value cutoff for fusions [0.05]

Inputs:  exon.fa   .fasta file that contains exon sequences of 
                   targeted genes, which can be generated by: 
                   tfc name2fasta <gname.txt> <genes.gtf> <in.fa> <exon.fa>  
         R1.fq.gz  5'->3' end of pair-end sequencing reads
         R2.fq.gz  the other end of sequencing reads
```
## Workflow

![workflow](https://github.com/r3fang/tfc/blob/master/img/workflow.jpg)

## A Full Example
```
$ sort -k5,5n genes.gtf > genes.sorted.gtf
$ ./tfc name2fasta -g exon genes.txt genes.sorted.gtf hg19.fa.gz exon.fa
$ zcat A431-1-ABGHI_S1_L001_R1_001.fastq.gz | paste - - - - | sort -k1,1 -S 3G | tr '\t' '\n' | gzip > A431-1-ABGHI_S1_L001_R1_001.sorted.fastq.gz
$ zcat A431-1-ABGHI_S1_L001_R2_001.fastq.gz | paste - - - - | sort -k1,1 -S 3G | tr '\t' '\n' | gzip > A431-1-ABGHI_S1_L001_R2_001.sorted.fastq.gz
$ ./tfc predict exon.fa A431-1-ABGHI_S1_L001_R1_001.sorted.fastq.gz A431-1-ABGHI_S1_L001_R2_001.sorted.fastq.gz
```
## FAQ

 1. **How fast is TFC?**     
 On average, **~5min** per million read pairs using a single x86_64 32-bit 2000 MHz GenuineIntel processor.
 TFC is 100% implemented in C. We tested TFC against ~40 real targeted RNA-seq data with various number of reads ranging from ~6000 to 2.4 million against 506 targeted genes (in the below table). On average, TFC has ~5min run per million reads for predicting ~500 targeted genes. However, as you can see from the table, sometime the running time varies largely between 2 samples even with similar number of reads (e.g. UHRR-2-ABGHI_S32_L001 and Undetermined_S0_L001). The reason is that most of running time of TFC is used for aligning reads that support fusion against coreepsonding constructed transcript, therefore, the more fusions in the sample identified, the longer TFC usually lasts. 
<p align="center">
  <img src="https://github.com/r3fang/tfc/blob/master/img/time_sample.jpg" width="400px" height="350px">
</p>
 2. **What's the maximum memory requirement for TFC?**   
 **1GB** would be the up limit for most of the cases.   
 The majority (~90%) of the memory occupied by TFC is used for storing the kmer hash table indexed from reference sequences. Thus, the more genes are being tested, the more memory will probably be needed (also depends on the complexity of the sequences). Based on our simulations, predicting against ~500 genes with k=15 always takes less than **1GB** memory, which means TFC can definately be used on most of today's PCs.

 3. **How precise is TFC?**  
 **~0.85** and **~0.99** for sensitivity and specificity on the simulated data.     
 We randomly constructed 50 fused transcripts and simulated illumina pair-end sequencing reads from constructed transcripts using [art](http://www.niehs.nih.gov/research/resources/software/biostatistics/art/) in paired-end mode with parameters setting as `-p -l 75 -ss HS25 -f 30 -m 200 -s 10` and run TFC against simulated reads, then caculate sensitivity and specificity. Repeat above process for 200 time and get the average sensitivity and specificity.

 4. **How is the likelihood of fusion calculated?**   
 In very brief, likelihood equals the product of alignment score of the reads that support the fusion normalized by the sequencing depth.   
 In detail, let *e_ij* indicates the fusion between *gene_i* and *gene_j* and *s_ij* and *junc_ij* be the real transcript string and junction of *e(i,j)*. Let *f(x, y)* be the alignment between quary string *x* and reference *y*, for any *x* and *y* (*f(x,y)* is always between [0,1]). Let *S(i)* and *S(j)* be the set of read pairs that aligned to *gene(i)* and *gene(j)* respectively. *S1_ij* is the subset of read pairs that support *e(i,j)* and overlapped with *junc(i,j)* and *S2_ij* be the subset of read pairs also also support *e(i,j)* but not overlaped with *junc(i,j)*. Liklihood of *e(i,j)* can be calculated by      
 ![equation](https://github.com/r3fang/tfc/blob/master/img/Tex2Img_1440266851.jpg)    
 in which ![equation](https://github.com/r3fang/tfc/blob/master/img/Tex2Img_1440196064.jpg)

 5. **What's the null model for p-value?**   
 We extracted all transcripts of targeted genes and simulated pair-end reads by art. Then run TFC against the simulated data and calculate the likelihood for every gene pair. Repeat this for 200 times and get the distribution of likelihood of every gene pair as the null model. 

 6. **How does TFC guarantee specificity without comparing sequencing reads against regions outside targeted genes?**   
 we have several strict criteria to filter out reads that are likely to come from regions outside targeted intervals. For instance, both ends of a pair are aligned against the constructed transcript and those pairs of any end not being successfully aligned will be discarded. Also, any pair with too large or too small insertion size will be filtered out. The likelihood of fusion will be normalized by sequencing depth of the two genes before p-value is calculated.

 7. **Does tfc work for single-end reads?**   
 Unfornately, TFC only works for pair-end sequencing data now, but having it run for single-end read is a feature we would love to add in the near future.

 8. **Does TFC support parallel computing?**    
 No. We realize TFC is fast enough, but this is a feature we would love to add in the near future.

 9.  **Is there anything I should be very careful about for `./tfc name2fasta`?**    
 Yes, genes.gtf needs to be sorted by its 5th column, the end position of the feature.

 10. **Is there anything I should be very careful about for `./tfc predict`?**  
 2 things.    

- First, before running `tfc predict [options] <exon.fa> <R1.fq> <R2.fq>`, user has to make sure R1.fq and R2.fq (RNA-seq) are in the right order(R2.fq must be identical to the psoitive strand of reference genome).         
- Second, name of reads has to be paired up in R1.fq and R2.fq, sort them based on read name if necessary.

####Version     
08.19-r15

####Author     
Rongxin Fang    
r3fang@eng.ucsd.edu
