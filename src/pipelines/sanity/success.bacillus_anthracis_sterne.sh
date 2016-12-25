#!/bin/sh

name=$1

#  https://www.ncbi.nlm.nih.gov/bioproject/?term=10878
#  https://www.ncbi.nlm.nih.gov/nuccore/AE017225.1

dnadiff -p $name.dnadiff \
  /data/references/bacillus_anthracis_sterne_AE017225.1.fasta \
  $name.contigs.fasta

mummerplot --fat -t png -p $name.dnadiff $name.dnadiff.delta

rm -f $name.dnadiff.1coords
rm -f $name.dnadiff.1delta
rm -f $name.dnadiff.mcoords
rm -f $name.dnadiff.mdelta
rm -f $name.dnadiff.qdiff
rm -f $name.dnadiff.rdiff

rm -f $name.dnadiff.filter
rm -f $name.dnadiff.rplot
rm -f $name.dnadiff.fplot
rm -f $name.dnadiff.gp