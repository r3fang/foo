##Get Started

```
$ git clone git@github.com:r3fang/tfc.git
$ cd tfc
$ make
$ ./tfc -predict exon.fa reads1.fq.gz reads2.fq.gz > tfc.out
```


##Introduction

TFC is a super lightwieght, stand-alone, ultrafast, C-implemented, mapping-free and highly sensitive software desgined for detection of fusion between candiate genes using Illumina RNA-seq data. It consists of two major components: 
 
```
$./tfc

Program: tfc (targeted gene fusion calling)
Version: 0.8.05-r15
Contact: Rongxin Fang <r3fang@ucsd.edu>

Usage:   tfc <command> [options]

Command: name2fasta     extract exon sequences by gene name
         predict        predict gene fusion
```

 - name2fasta

```
$./tfc name2fasta

Usage:   tfc name2fasta <gname.txt> <genes.gff> <in.fa> <exon.fa>

Details: name2fasta is to extract genomic sequence of gene candiates

Inputs:  gname.txt   plain txt file contains names of genes candiates
         genes.gff   standard gff files contains genes annotation
         in.fa       fasta file contains the entire genome sequence [hg19.fa]
         exon.fa     output fasta files that contains sequences of targeted genes
```

  - predict
	
```
$./tfc predict

Usage:   tfc predict [options] <exon.fa> <R1.fq> <R2.fq>

Details: predict gene fusion from RNA-seq data

Options: -k INT    kmer length [15]
         -n INT    min number kmer matches [10]
         -w INT    min weight for an edge [3]
         -m INT    match score [2]
         -u INT    penality for mismatch [-2]
         -o INT    penality for gap open [-5]
         -e INT    penality for gap extension [-1]
         -j INT    penality for jump between genes [-10]
         -s INT    penality for jump between exons [-8]
         -h INT    min hits for a junction [3]
         -l INT    seed length for junction rediscovery [20]
         -x INT    max mismatches of seed match [2]
         -a FLOAT  min alignment score [0.80]

Inputs:  exon.fa   fasta file that contains exon sequences of targeted 
                   genes, which could be generated by: 
                   tfc name2fasta <genes.txt> <genes.gff> <in.fa> <exon.fa> 
         R1.fq     5'->3' end of pair-end sequencing reads
         R2.fq     the other end of pair-end sequencing reads
```


## Workflow

![workflow](https://github.com/r3fang/tfc/blob/master/workflow.jpg)

#### Version
0.8.05-r15


#### Author
Rongxin Fang (r3fang@eng.ucsd.edu)

